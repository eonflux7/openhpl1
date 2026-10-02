// hpl1_shadow.glsl
//
// How much of the light that is drawn reaches a point, from the light's
// shadow map (cRenderer3D::RenderShadowMap, cShadowMapParams).
//
// SPOT: the map is 2D, drawn with the spotlight's view and projection, and
// positions are in the light's view. Else it is a cube map around the light
// and positions are relative to the light, along the world's axes.
//
// From hpl_State:
//  hpl_ShadowMap0  x: filter (0 none, 1 hard, 2 blurred, 3 blurred by
//                  distance), y: near, z: far, w: texel size at distance 1
//  hpl_ShadowMap1  x: softness, y: light size, z: widest blur (both blurs
//                  at distance 1), w: 1 to show the result alone
//  hpl_ShadowMap2  x: bias, y: slope bias in texels, z: samples,
//                  w: samples of the search for what casts the shadow
//  hpl_ShadowMap3  xy: texture coordinates per x/z and y/z (SPOT),
//                  z: 1 to turn the samples differently in every pixel

#ifdef SPOT
uniform sampler2DShadow hpl_ShadowMap;
uniform sampler2D hpl_ShadowMapDepth;
#else
uniform samplerCubeShadow hpl_ShadowMap;
uniform samplerCube hpl_ShadowMapDepth;
#endif

// The distance the map stores for a position: along the light's axis, or
// along the axis of the cube face the position is seen in.
float hpl_ShadowDist(vec3 p)
{
#ifdef SPOT
	return -p.z;
#else
	vec3 a = abs(p);
	return max(a.x, max(a.y, a.z));
#endif
}

// That distance as the depth value in the map, and back.
float hpl_ShadowDistToDepth(float dist)
{
	float n = hpl_ShadowMap0.y, f = hpl_ShadowMap0.z;
	return 0.5 * ((f + n) - 2.0 * f * n / max(dist, n)) / (f - n) + 0.5;
}

float hpl_ShadowDepthToDist(float depth)
{
	float n = hpl_ShadowMap0.y, f = hpl_ShadowMap0.z;
	return 2.0 * f * n / ((f + n) - (2.0 * depth - 1.0) * (f - n));
}

#ifdef SPOT
vec2 hpl_ShadowUv(vec3 p)
{
	return 0.5 + p.xy / max(-p.z, 0.0001) * hpl_ShadowMap3.xy;
}
#endif

// 1 if nothing in the map, seen from the light towards p, is nearer than
// dist; filtered between the four texels around.
float hpl_ShadowCompare(vec3 p, float dist)
{
	float depth = hpl_ShadowDistToDepth(dist);
#ifdef SPOT
	return texture(hpl_ShadowMap, vec3(hpl_ShadowUv(p), depth));
#else
	return texture(hpl_ShadowMap, vec4(p, depth));
#endif
}

// The distance of what the light sees towards p.
float hpl_ShadowOccluderDist(vec3 p)
{
#ifdef SPOT
	return hpl_ShadowDepthToDist(texture(hpl_ShadowMapDepth, hpl_ShadowUv(p)).x);
#else
	return hpl_ShadowDepthToDist(texture(hpl_ShadowMapDepth, p).x);
#endif
}

// The normal of the surface a position is on, from how the position changes
// from pixel to pixel. Not inside a branch: the changes are only defined
// where every pixel takes the same path.
vec3 hpl_ShadowPlane(vec3 pos)
{
	vec3 normal = cross(dFdx(pos), dFdy(pos));
	float len = length(normal);
	return len > 0.0 ? normal / len : vec3(0.0);
}

// The distance to compare the map with for a sample at p, near the lit
// point: the nearer of p itself and of where the surface's plane is in that
// direction. With p alone a surface that slants away from the light shadows
// itself on its near side; with the plane alone the samples go behind the
// wall where a floor ends, and the wall counts as casting a shadow.
float hpl_ShadowSampleDist(vec3 p, vec3 normal, float planeDist, float bias)
{
	float along = dot(normal, p);
	float scale = abs(along) > 0.00001 ? clamp(planeDist / along, 0.5, 1.0) : 1.0;
	return hpl_ShadowDist(p) * scale - bias;
}

// The samples are evenly spread on a disc of radius 1 (Vogel's spiral):
// sample i of n is at the radius below, and a turn of the golden angle
// (2.3999632) further around than the one before.
const mat2 hpl_ShadowTurn = mat2(-0.7373688, 0.6754903, -0.6754903, -0.7373688);

float hpl_ShadowDiscRadius(int i, int n)
{
	return sqrt((float(i) + 0.5) / float(n));
}

// 0 to 1: how much of the light reaches pos. normal is hpl_ShadowPlane(pos).
float hpl_Shadow(vec3 pos, vec3 normal)
{
	int filterType = int(hpl_ShadowMap0.x);
	if(filterType == 0) return 1.0;

	float len = max(length(pos), 0.0001);
	vec3 dir = pos / len;
	float dist = hpl_ShadowDist(pos);
	float planeDist = dot(normal, pos);

	// The map has one depth for a texel, the surface a range of them: wider
	// the more it slants.
	float cosAngle = abs(dot(normal, dir));
	float slope = sqrt(max(1.0 - cosAngle * cosAngle, 0.0)) / max(cosAngle, 0.1);
	float texel = hpl_ShadowMap0.w * dist;
	float bias = hpl_ShadowMap2.x + hpl_ShadowMap2.y * texel * slope;

	if(filterType == 1) return hpl_ShadowCompare(pos, dist - bias);

	// The samples are on a disc around the point that faces the light.
	vec3 side = normalize(cross(dir, abs(dir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
	vec3 up = cross(dir, side);
	// Where the first sample is: turned differently in every pixel
	vec2 first = vec2(1.0, 0.0);
	if(hpl_ShadowMap3.z > 0.5)
	{
		float turn = 6.2831853 * fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
		first = vec2(cos(turn), sin(turn));
	}

	float widest = hpl_ShadowMap1.z * len;
	float blur = hpl_ShadowMap1.x * len;

	if(filterType == 3)
	{
		// What is between the light and the point, within the widest blur,
		// and how far along the way it is on average: a shadow is sharp
		// where it is cast from close by and wide where from far.
		int searchNum = int(hpl_ShadowMap2.w);
		float along = 0.0, found = 0.0;
		vec2 around = first;
		for(int i = 0; i < searchNum; ++i)
		{
			vec2 d = around * (hpl_ShadowDiscRadius(i, searchNum) * widest);
			around = hpl_ShadowTurn * around;
			vec3 p = pos + side * d.x + up * d.y;
			float occluder = hpl_ShadowOccluderDist(p);
			if(occluder < hpl_ShadowSampleDist(p, normal, planeDist, bias))
			{
				along += occluder / hpl_ShadowDist(p);
				found += 1.0;
			}
		}
		// Nothing found: lit, or the samples missed something thin. One
		// sample of the map tells, and does not flicker from pixel to pixel
		// as lit against blurred would.
		// Everything found: deep in the shadow, where the blur would give
		// the same as one sample too.
		if(found < 0.5 || found > float(searchNum) - 0.5) return hpl_ShadowCompare(pos, dist - bias);

		along = clamp(along / found, 0.001, 1.0);
		blur = min(hpl_ShadowMap1.y * (1.0 - along) / along, widest);
	}

	// A blur narrower than a texel: all samples would read the same four
	// texels, as one does.
	if(blur < 0.5 * texel) return hpl_ShadowCompare(pos, dist - bias);

	int sampleNum = int(hpl_ShadowMap2.z);
	float lit = 0.0;
	vec2 around = first;
	for(int i = 0; i < sampleNum; ++i)
	{
		vec2 d = around * (hpl_ShadowDiscRadius(i, sampleNum) * blur);
		around = hpl_ShadowTurn * around;
		vec3 p = pos + side * d.x + up * d.y;
		lit += hpl_ShadowCompare(p, hpl_ShadowSampleDist(p, normal, planeDist, bias));
	}
	return lit / float(sampleNum);
}
