// Player perception on screen values, after tone mapping and SMAA, before
// the HUD. The neutral path is exactly the existing scene output. Sampling
// neighbours converts them the same way as the centre; SMAA already has
// screen values and sets tonemap.x to zero.

float PlayerLuma(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float PlayerHash(vec2 cell)
{
    // The output pass's interleaved gradient noise is useful for dithering,
    // but repeats visibly when interpolated into frost or splatter shapes.
    uvec2 p = uvec2(ivec2(floor(cell)));
    uint h = p.x * 0x9e3779b9u ^ p.y * 0x85ebca6bu;
    h = (h ^ (h >> 16u)) * 0x7feb352du;
    h = (h ^ (h >> 15u)) * 0x846ca68bu;
    h ^= h >> 16u;
    return float(h & 0x00ffffffu) / 16777216.0;
}

float PlayerNoise(vec2 p)
{
    vec2 cell = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(PlayerHash(cell), PlayerHash(cell + vec2(1, 0)), f.x),
               mix(PlayerHash(cell + vec2(0, 1)), PlayerHash(cell + vec2(1, 1)), f.x), f.y);
}

vec3 PlayerSample(vec2 uv)
{
    // Clamp to pixel centres so a wide blur cannot wrap at an edge.
    uv = clamp(uv, sceneSize.zw * 0.5, 1.0 - sceneSize.zw * 0.5);
    vec3 color = max(hpl_TextureLod0(uv, 0.0).rgb, 0.0);
    if(tonemap.x > 0.5)
        color = pow(Tonemap(color * tonemap.y), vec3(1.0 / 2.2));
    return color;
}

float PlayerBloodSplatter(vec2 uv)
{
    // Out-of-focus splashes on the edges of the player's vision. A seed
    // changes their position, size and shape once per hit, never per frame.
    float aspect = invOutputSize.y / invOutputSize.x;
    vec2 metric = vec2(aspect / (16.0 / 9.0), 1.0);
    float seed = playerDetails.x;
    vec2 noiseOffset = vec2(seed, seed * 0.13);
    float shape = 10.0;
    for(int i = 0; i < 6; ++i)
    {
        vec2 key = vec2(float(i) * 19.0, seed);
        vec2 random = vec2(PlayerHash(key), PlayerHash(key + vec2(7, 13)));
        int side = i < 4 ? i : int(PlayerHash(key + vec2(41, 5)) * 4.0);
        vec2 centre = vec2(-0.025 + random.x * 0.09, 0.15 + random.y * 0.70);
        if(side == 1) centre = centre.yx;
        if(side == 2) centre.x = 1.0 - centre.x;
        if(side == 3) centre = vec2(centre.y, 1.0 - centre.x);
        vec2 radius = vec2(0.10 + 0.075 * PlayerHash(key + vec2(23, 17)),
                           0.12 + 0.11 * PlayerHash(key + vec2(5, 31)));
        float angle = (PlayerHash(key + vec2(61, 7)) - 0.5) * 1.2;
        vec2 q = (uv - centre) * metric;
        q = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * q;
        shape = min(shape, length(q / radius));
        // A smaller overlapping lobe gives each splash an uneven outline.
        shape = min(shape, length((q - radius * vec2(0.5, 0.55)) / (radius * 0.55)));
    }
    shape += (PlayerNoise(uv * vec2(aspect, 1) * 28.0 + noiseOffset) - 0.5) * 0.42;
    shape += (PlayerNoise(uv * vec2(aspect, 1) * 65.0 + noiseOffset) - 0.5) * 0.06;
    float aa = max(fwidth(shape), 0.20);
    float splash = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, shape);

    vec2 q = uv * vec2(aspect, 1) * 25.0;
    vec2 cell = floor(q) + noiseOffset;
    vec2 centre = 0.23 + 0.54 * vec2(PlayerHash(cell + vec2(7, 11)), PlayerHash(cell + vec2(19, 4)));
    float radius = 0.035 + 0.17 * PlayerHash(cell + vec2(31, 23));
    float distance = length((fract(q) - centre) * vec2(1, 1.5)) - radius;
    aa = max(fwidth(distance), 0.075);
    float drops = (1.0 - smoothstep(-aa, aa, distance)) * step(0.76, PlayerHash(cell + vec2(43, 9)));
    float outer = smoothstep(0.25, 0.85, max(abs(uv.x * 2.0 - 1.0), abs(uv.y * 2.0 - 1.0)));
    return max(splash, drops * outer);
}

vec3 PlayerEffects(vec3 color)
{
    if(dot(playerEffects0, vec4(1)) + dot(playerEffects1, vec4(1)) +
       playerEffects2.x + playerEffects2.z + playerDetails.y <= 0.0) return color;

    vec2 uv = gl_FragCoord.xy * invOutputSize.xy;
    vec2 p = uv * 2.0 - 1.0;
    float edge = smoothstep(0.35, 1.2, length(p * vec2(0.85, 1.0)));
    float panic = playerEffects1.x;
    float pulse = playerEffects2.y;
    float flash = playerEffects1.y;
    float save = playerEffects1.z;
    float underwater = playerEffects2.z;
    float whiteout = max(flash, save);

    // Keep a separated second outline between heartbeats. A gentle pulse
    // varies its separation and opacity without making it disappear.
    // It samples this frame, so healing leaves no history behind.
    float health = playerEffects0.y;
    float vision = playerDetails.y;
    if(vision > 0.0)
    {
        vec2 offset = vec2((0.012 + 0.006 * pulse) * invOutputSize.x / invOutputSize.y,
                           0.0015 * (pulse * 2.0 - 1.0));
        color = mix(color, PlayerSample(uv + offset), vision * (0.32 + edge * 0.06) * (0.85 + 0.15 * pulse));
    }

    // Blur the periphery while leaving the centre readable. Whiteouts
    // also gather nearby highlights, so the light spreads before white.
    float blur = panic * (0.12 + edge * 0.65) * (0.35 + 0.65 * pulse);
    blur = max(blur, playerEffects0.w * edge * 0.15);
    blur = max(blur, max(save * 0.50, underwater * 0.22));
    if(blur > 0.001 || whiteout > 0.001)
    {
        float radius = 0.002 + 0.014 * whiteout + 0.006 * panic;
        vec2 metric = vec2(invOutputSize.x / invOutputSize.y, 1.0);
        vec3 soft = color * 4.0;
        vec3 glow = vec3(0);
        for(int ring = 1; ring <= 2; ++ring)
        {
            for(int i = 0; i < 8; ++i)
            {
                float a = (float(i) + 0.5 * float(ring)) * 0.785398163;
                vec2 offset = vec2(cos(a), sin(a)) * metric * radius * float(ring);
                vec3 sampleColor = PlayerSample(uv + offset);
                soft += sampleColor;
                glow += max(sampleColor - 0.5, 0.0);
            }
        }
        color = mix(color, soft / 20.0, blur);
        color += glow / 16.0 * (whiteout * playerEffects2.w * 2.2);
    }

    float adaptation = playerEffects0.x;
    if(adaptation > 0.0)
    {
        float shadows = 1.0 - smoothstep(0.08, 0.55, PlayerLuma(color));
        // A bounded shadow lift preserves absolute black and leaves lamps
        // and the flashlight alone. This is driven by hiding, not by a
        // global exposure meter that would brighten every dark room.
        vec3 lifted = mix(color, sqrt(max(color, 0.0)), adaptation * shadows * 0.58);
        vec3 rods = vec3(PlayerLuma(lifted)) * vec3(0.60, 0.83, 1.45);
        color = mix(lifted, rods, adaptation * shadows * 0.55);
        color *= 1.0 - edge * adaptation * 0.08;
        color += vec3(0.010, 0.026, 0.075) * edge * adaptation;
    }

    if(panic > 0.0)
    {
        color = mix(color, vec3(PlayerLuma(color)), panic * 0.32);
        color *= 1.0 - edge * panic * (0.75 + 0.25 * pulse) * 0.55;
    }

    if(underwater > 0.0)
        color = mix(color, color * playerWaterColor.rgb, underwater * 0.75);

    float injury = max(health, playerEffects0.z);
    if(injury > 0.0)
    {
        // Hits and the low-health heartbeat share the same red veil.
        // Combining them uses the stronger amount rather than doubling it.
        float injuryEdge = smoothstep(0.28, 1.15, length(p * vec2(0.85, 1.0)));
        float uneven = 0.72 + 0.28 * PlayerNoise(uv * vec2(13, 9));
        float veil = injuryEdge * uneven * injury;
        color *= 1.0 - veil * 0.30;
        color = mix(color, vec3(0.36, 0.012, 0.020), veil * 0.55);
    }

    float blood = playerEffects0.z;
    if(blood > 0.0)
    {
        float splash = PlayerBloodSplatter(uv);
        float wetness = PlayerNoise(uv * vec2(37, 23) + vec2(playerDetails.x, 7));
        vec3 bloodColor = mix(vec3(0.14, 0.004, 0.009), vec3(0.42, 0.016, 0.022), wetness);
        color = mix(color, bloodColor, splash * blood * 0.82);
    }

    if(playerEffects0.w > 0.0)
    {
        vec2 q = uv * vec2(invOutputSize.y / invOutputSize.x, 1.0);
        float coarse = PlayerNoise(q * 24.0);
        float medium = PlayerNoise(q * 75.0);
        float fine = PlayerNoise(q * 240.0);
        float frost = smoothstep(0.30, 1.08, max(abs(p.x), abs(p.y)) +
                                (coarse - 0.5) * 0.38 + (medium - 0.5) * 0.12);
        float crystals = pow(1.0 - abs(medium * 2.0 - 1.0), 12.0);
        float grit = smoothstep(0.68, 0.95, PlayerNoise(q * min(320.0, 0.35 / invOutputSize.y)));
        vec3 iceColor = mix(vec3(0.44, 0.66, 0.84), vec3(0.90, 0.97, 1.0),
                           clamp(fine * 0.55 + crystals * 0.35 + grit * 0.45, 0.0, 1.0));
        color *= mix(vec3(1), vec3(0.88, 0.96, 1.06), playerEffects0.w * 0.25);
        color = mix(color, iceColor, frost * playerEffects0.w * (0.30 + coarse * 0.35 + crystals * 0.25 + grit * 0.20));
    }

    // At full strength flashes are opaque white, independent of scene
    // brightness or bloom settings. Saves begin with a warm haze.
    if(whiteout > 0.0)
    {
        color *= mix(vec3(1), vec3(1.15, 0.98, 0.85), save * 0.35);
        color += vec3(0.14, 0.06, 0.012) * save * (1.0 - save);
        // Light sources burn out first. The veil arrives later, leaving
        // silhouettes visible until the last part of the transition.
        color *= 1.0 + 2.5 * whiteout * whiteout;
        float veil = smoothstep(0.25, 1.0, whiteout);
        color = mix(color, vec3(1), veil * veil);
    }

    float death = playerEffects1.w;
    color = mix(color, vec3(PlayerLuma(color)), death * 0.75);
    color *= (1.0 - edge * death * 0.85) * (1.0 - playerEffects2.x);
    return color;
}
