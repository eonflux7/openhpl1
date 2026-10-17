// This file is part of Overture Remade, licensed under the GNU GPL v3 or later.
#ifndef HPL_GL_GRAPHICS_HELPERS_H
#define HPL_GL_GRAPHICS_HELPERS_H
#include <glad/gl.h>
#include "graphics/GraphicsTypes.h"
#include "graphics/Texture.h"
namespace hpl
{
	GLenum ColorFormatToGL(eColorDataFormat aFormat);
	GLenum TextureTargetToGL(eTextureTarget aTarget);
	// Preserve sums of light passes until the final output transform.
	constexpr GLenum kSceneColorFormat = GL_RGBA16F;
	GLuint CreateColorTexture(int alW, int alH, GLenum aFormat, GLenum aFilter);
	GLuint CreateDepthStencilTexture(int alW, int alH);
	bool CheckFramebuffer(const char *asName);
} // namespace hpl
#endif
