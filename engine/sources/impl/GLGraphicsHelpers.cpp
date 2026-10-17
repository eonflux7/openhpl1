/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "impl/GLGraphicsHelpers.h"
#include "system/LowLevelSystem.h"
namespace hpl
{
	GLuint CreateColorTexture(int alW, int alH, GLenum aFormat, GLenum aFilter)
	{
		GLuint lTex = 0;
		glGenTextures(1, &lTex);
		glBindTexture(GL_TEXTURE_2D, lTex);
		glTexStorage2D(GL_TEXTURE_2D, 1, aFormat, alW, alH);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, aFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		return lTex;
	}

	bool CheckFramebuffer(const char *asName)
	{
		GLenum lStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(lStatus == GL_FRAMEBUFFER_COMPLETE) return true;
		Error("%s framebuffer incomplete: 0x%x\n", asName, lStatus);
		return false;
	}

	GLuint CreateDepthStencilTexture(int alW, int alH)
	{
		GLuint lTex = 0;
		glGenTextures(1, &lTex);
		glBindTexture(GL_TEXTURE_2D, lTex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, alW, alH);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		return lTex;
	}

	GLenum ColorFormatToGL(eColorDataFormat aFormat)
	{
		switch(aFormat)
		{
			case eColorDataFormat_RGB:
				return GL_RGB;
			case eColorDataFormat_RGBA:
				return GL_RGBA;
			// One-channel textures are red, read as luminance (cSDLTexture)
			case eColorDataFormat_ALPHA:
				return GL_RED;
			case eColorDataFormat_BGR:
				return GL_BGR;
			case eColorDataFormat_BGRA:
				return GL_BGRA;
		};

		return 0;
	}
	GLenum TextureTargetToGL(eTextureTarget aTarget)
	{
		switch(aTarget)
		{
			case eTextureTarget_1D:
				return GL_TEXTURE_1D;
			case eTextureTarget_2D:
				return GL_TEXTURE_2D;
			case eTextureTarget_CubeMap:
				return GL_TEXTURE_CUBE_MAP;
			case eTextureTarget_3D:
				return GL_TEXTURE_3D;
			case eTextureTarget_2DArray:
				return GL_TEXTURE_2D_ARRAY;
		}
		return 0;
	}
} // namespace hpl
