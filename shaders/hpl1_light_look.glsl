// hpl1_light_look.glsl
//
// What a light can look like beyond the original's ramp and picture
// (cLightLook, set for each light with iLowLevelGraphics::SetLightLook).
//
// From hpl_State:
//  hpl_Light0  x: 1 for falloff by distance, y: the distance (share of the
//              reach) at which the light is as bright as its colour,
//              z: how many times brighter it gets at most, w: 1 for a tube
//  hpl_Light1  x: 1 for the beam made here (SPOT), y: hotspot size,
//              z: brightness of the field around it, w: beam size
//  hpl_Light2  xyz: scale for the light's colour (tint and brightness)

// The share of the light that arrives, as the falloff ramps give it (an
// amount on screen), for a squared distance in reaches: one over the
// square of the distance, held below a ceiling near the light, and faded
// out so that it ends where the light's reach does.
float hpl_DistanceFalloff(float dist2)
{
	float even2 = hpl_Light0.y * hpl_Light0.y;
	float light = min((even2 + 0.002) / (dist2 + 0.002), hpl_Light0.z);
	float window = clamp(1.0 - dist2 * dist2, 0.0, 1.0);
	return pow(light * window * window, 1.0 / 2.2);
}

// A torch's beam for a place in the spotlight's picture (0..1 across the
// cone): a bright hotspot, a dimmer field around it that ends in a soft
// edge, and the faint rings a reflector leaves. As the picture it stands
// in for: a screen value, the same in alpha (the specular amount).
vec4 hpl_Beam(vec2 uv)
{
	float r = length(uv - vec2(0.5)) * 2.0 / max(hpl_Light1.w, 0.01);
	float hotspot = exp(-(r * r) / max(hpl_Light1.y * hpl_Light1.y, 0.0001));
	float field = hpl_Light1.z * (1.0 - smoothstep(0.55, 0.95, r));
	float rings = 1.0 + 0.05 * cos(r * 34.0) * smoothstep(0.15, 0.4, r);
	float beam = clamp((field + (1.0 - field) * hotspot) * rings, 0.0, 1.0);
	return vec4(beam);
}

// A tube light's highlight: the direction to the place on the tube that the
// mirrored line of sight passes closest to, which is what a highlight is a
// picture of (Karis 2013, "representative point"). centre: to the tube's
// middle, axis: from there to one end, toEye and normal normalised; all in
// one space.
vec3 hpl_TubeDirection(vec3 centre, vec3 axis, vec3 toEye, vec3 normal)
{
	vec3 mirrored = reflect(-toEye, normal);
	vec3 start = centre - axis;
	vec3 along = axis * 2.0;
	float mirroredAlong = dot(mirrored, along);
	float t = (dot(mirrored, start) * mirroredAlong - dot(start, along)) /
				max(dot(along, along) - mirroredAlong * mirroredAlong, 1.0e-6);
	vec3 place = start + along * clamp(t, 0.0, 1.0);
	return place * inversesqrt(max(dot(place, place), 1.0e-12));
}

// What the highlight of a tube is scaled by: the same light spread over a
// longer picture is dimmer. roughness2: the square of the roughness.
float hpl_TubeSpread(vec3 centre, vec3 axis, float roughness2)
{
	float size = length(axis) / max(length(centre), 0.001);
	return roughness2 / clamp(roughness2 + size, 0.0, 1.0);
}
