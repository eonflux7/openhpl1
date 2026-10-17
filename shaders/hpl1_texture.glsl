// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.

// A linked sampler variant selects either a normal 2D texture or one layer
// of an animation array, on the same texture unit. Filtering, LOD and wrap
// remain sampler state; the layer never interpolates between frames.
uniform float hpl_TextureLayer0;
#ifdef HPL_ARRAY_0
#define HPL_SAMPLER_0 sampler2DArray
#define HPL_UV_0(uv) vec3(uv, hpl_TextureLayer0)
#define HPL_PIXEL_0(pixel) ivec3(pixel, int(hpl_TextureLayer0))
#else
#define HPL_SAMPLER_0 sampler2D
#define HPL_UV_0(uv) uv
#define HPL_PIXEL_0(pixel) pixel
#endif
uniform float hpl_TextureLayer1;
#ifdef HPL_ARRAY_1
#define HPL_SAMPLER_1 sampler2DArray
#define HPL_UV_1(uv) vec3(uv, hpl_TextureLayer1)
#define HPL_PIXEL_1(pixel) ivec3(pixel, int(hpl_TextureLayer1))
#else
#define HPL_SAMPLER_1 sampler2D
#define HPL_UV_1(uv) uv
#define HPL_PIXEL_1(pixel) pixel
#endif
uniform float hpl_TextureLayer2;
#ifdef HPL_ARRAY_2
#define HPL_SAMPLER_2 sampler2DArray
#define HPL_UV_2(uv) vec3(uv, hpl_TextureLayer2)
#define HPL_PIXEL_2(pixel) ivec3(pixel, int(hpl_TextureLayer2))
#else
#define HPL_SAMPLER_2 sampler2D
#define HPL_UV_2(uv) uv
#define HPL_PIXEL_2(pixel) pixel
#endif
uniform float hpl_TextureLayer3;
#ifdef HPL_ARRAY_3
#define HPL_SAMPLER_3 sampler2DArray
#define HPL_UV_3(uv) vec3(uv, hpl_TextureLayer3)
#define HPL_PIXEL_3(pixel) ivec3(pixel, int(hpl_TextureLayer3))
#else
#define HPL_SAMPLER_3 sampler2D
#define HPL_UV_3(uv) uv
#define HPL_PIXEL_3(pixel) pixel
#endif
uniform float hpl_TextureLayer4;
#ifdef HPL_ARRAY_4
#define HPL_SAMPLER_4 sampler2DArray
#define HPL_UV_4(uv) vec3(uv, hpl_TextureLayer4)
#define HPL_PIXEL_4(pixel) ivec3(pixel, int(hpl_TextureLayer4))
#else
#define HPL_SAMPLER_4 sampler2D
#define HPL_UV_4(uv) uv
#define HPL_PIXEL_4(pixel) pixel
#endif
uniform float hpl_TextureLayer5;
#ifdef HPL_ARRAY_5
#define HPL_SAMPLER_5 sampler2DArray
#define HPL_UV_5(uv) vec3(uv, hpl_TextureLayer5)
#define HPL_PIXEL_5(pixel) ivec3(pixel, int(hpl_TextureLayer5))
#else
#define HPL_SAMPLER_5 sampler2D
#define HPL_UV_5(uv) uv
#define HPL_PIXEL_5(pixel) pixel
#endif
uniform float hpl_TextureLayer6;
#ifdef HPL_ARRAY_6
#define HPL_SAMPLER_6 sampler2DArray
#define HPL_UV_6(uv) vec3(uv, hpl_TextureLayer6)
#define HPL_PIXEL_6(pixel) ivec3(pixel, int(hpl_TextureLayer6))
#else
#define HPL_SAMPLER_6 sampler2D
#define HPL_UV_6(uv) uv
#define HPL_PIXEL_6(pixel) pixel
#endif
uniform float hpl_TextureLayer7;
#ifdef HPL_ARRAY_7
#define HPL_SAMPLER_7 sampler2DArray
#define HPL_UV_7(uv) vec3(uv, hpl_TextureLayer7)
#define HPL_PIXEL_7(pixel) ivec3(pixel, int(hpl_TextureLayer7))
#else
#define HPL_SAMPLER_7 sampler2D
#define HPL_UV_7(uv) uv
#define HPL_PIXEL_7(pixel) pixel
#endif

#define HPL_TEXTURE_2D(N) \
uniform HPL_SAMPLER_##N tex##N; \
vec4 hpl_Texture##N(vec2 uv) { return texture(tex##N, HPL_UV_##N(uv)); } \
vec4 hpl_TextureLod##N(vec2 uv, float lod) { return textureLod(tex##N, HPL_UV_##N(uv), lod); } \
vec4 hpl_TexelFetch##N(ivec2 pixel, int lod) { return texelFetch(tex##N, HPL_PIXEL_##N(pixel), lod); } \
ivec2 hpl_TextureSize##N(int lod) { return textureSize(tex##N, lod).xy; }
