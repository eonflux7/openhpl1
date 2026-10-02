// hpl1_shading.glsl
//
// The improved shading (cShadingSettings in hpl_Frame): what a surface does
// with light beyond the original's texture x light x N.L.
//
// The game has no gloss maps. What a material has for specular (a colour
// map, or the alpha of its normal map) says how much it reflects, and is
// taken for how smooth it is too: bright is glossy, dark is dull.

#include "hpl1_frame.glsl"

// normalize() that gives a zero vector for a zero vector: some meshes have
// faces without tangents, and a NaN in the scene spreads through every blur.
vec3 hpl_Normalize(vec3 v)
{
	return v * inversesqrt(max(dot(v, v), 1.0e-12));
}

// Roughness (0 mirror, 1 chalk) for a specular map value as stored.
float hpl_Roughness(vec3 specularMap)
{
	float gloss = dot(specularMap, vec3(0.3, 0.58, 0.12));
	return clamp(mix(hpl_Shading1.x, hpl_Shading1.y, gloss), 0.05, 1.0);
}

// The share of light mirrored when the surface is faced straight on.
vec3 hpl_Reflectance(vec3 specularMap)
{
	return pow(specularMap, vec3(2.2)) * hpl_Shading0.z;
}

// Fresnel (Schlick): the share mirrored rises towards grazing angles. What
// reflects nothing straight on reflects nothing there either, so the black
// parts of a specular map stay matt.
vec3 hpl_Fresnel(vec3 reflectance, float cosAngle)
{
	float grazing = clamp(50.0 * max(reflectance.x, max(reflectance.y, reflectance.z)), 0.0, 1.0);
	float f = pow(1.0 - clamp(cosAngle, 0.0, 1.0), 5.0);
	return reflectance + (vec3(grazing) - reflectance) * f;
}

// The share of an even light from all around that a surface mirrors
// towards the eye at a slant, beyond what it mirrors faced straight on
// (that much is taken to be in the texture already, and there is no
// picture of the surroundings to mirror). The rise stops well short of a
// full mirror, sooner on rough surfaces and on what has little specular: a
// third of the specular materials have no map and count as white, and a
// full rise turns those a flat grey from below or along a wall.
vec3 hpl_AmbientMirror(vec3 specularMap, float cosAngle)
{
	vec3 reflectance = hpl_Reflectance(specularMap);
	float gloss = dot(specularMap, vec3(0.3, 0.58, 0.12));
	float grazing = 0.5 * (1.0 - hpl_Roughness(specularMap)) * gloss;
	float f = pow(1.0 - clamp(cosAngle, 0.0, 1.0), 5.0);
	return max(vec3(grazing) - reflectance, 0.0) * f;
}

// The highlight of a light of brightness 1, as an amount of light: a
// microfacet lobe (GGX, with the Smith visibility term), times N.L. A
// diffuse surface is "texture x light" in this renderer, without the
// division by pi, so the lobe has no pi either.
//
// All vectors in one space and normalised, except the normal: it comes
// from a filtered normal map. Where that normal changes from pixel to
// pixel the lobe is widened by as much, which keeps bumps too small for
// the screen from sparkling (Tokuyoshi and Kaplanyan 2019).
vec3 hpl_Highlight(vec3 normal, vec3 toLight, vec3 toEye, vec3 specularMap)
{
	vec3 n = hpl_Normalize(normal);
	vec3 h = hpl_Normalize(toLight + toEye);
	float nl = clamp(dot(n, toLight), 0.0, 1.0);
	float nv = clamp(dot(n, toEye), 0.0, 1.0) + 0.0001;
	float nh = clamp(dot(n, h), 0.0, 1.0);
	float lh = clamp(dot(toLight, h), 0.0, 1.0);

	float rough = hpl_Roughness(specularMap);
	float a = rough * rough;
	vec3 dx = dFdx(n), dy = dFdy(n);
	float a2 = clamp(a * a + min(0.5 * (dot(dx, dx) + dot(dy, dy)), 0.18), 0.0, 1.0);

	float d = nh * nh * (a2 - 1.0) + 1.0;
	float lobe = a2 / (d * d);
	float visible = 0.5 / max(mix(2.0 * nl * nv, nl + nv, a), 0.0001);

	return hpl_Fresnel(hpl_Reflectance(specularMap), lh) * (lobe * visible * nl * hpl_Shading0.y);
}
