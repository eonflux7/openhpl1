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
#include "impl/GLShadowTargets.h"
#include "impl/GLGraphicsHelpers.h"
#include "impl/GpuProfiler.h"
#include "system/LowLevelSystem.h"
#include "math/Math.h"
namespace hpl
{
	void cGLShadowTargets::Init()
	{
		// Valid initial bindings for both shadow sampler types before the first light.
		CreateShadowMap(false, 16);
		CreateShadowMap(true, 16);
	}
	void cGLShadowTargets::Reset()
	{
		if(mlShadowFramebuffer) glDeleteFramebuffers(1, &mlShadowFramebuffer);
		if(mvShadowMaps[0] || mvShadowMaps[1]) glDeleteTextures(2, mvShadowMaps);
		if(mvShadowSamplers[0] || mvShadowSamplers[1]) glDeleteSamplers(2, mvShadowSamplers);
		mlShadowFramebuffer = 0;
		mvShadowMaps[0] = mvShadowMaps[1] = 0;
		mvShadowMapSizes[0] = mvShadowMapSizes[1] = 0;
		mvShadowSamplers[0] = mvShadowSamplers[1] = 0;
	}

	void cGLShadowTargets::CreateShadowMap(bool abCube, int alSize)
	{
		const int lIdx = abCube ? 1 : 0;
		const GLenum lTarget = abCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

		if(mvShadowSamplers[0] == 0)
		{
			glGenSamplers(2, mvShadowSamplers);
			for(int i = 0; i < 2; ++i)
			{
				GLuint lSampler = mvShadowSamplers[i];
				GLenum lFilter = i == 0 ? GL_LINEAR : GL_NEAREST;
				glSamplerParameteri(lSampler, GL_TEXTURE_MIN_FILTER, lFilter);
				glSamplerParameteri(lSampler, GL_TEXTURE_MAG_FILTER, lFilter);
				// Past the edge of a spotlight's map there is nothing in the
				// way: the edge's texels repeated would stretch what they hold
				// outwards. (A cube map has no edge, its faces are seamless.)
				const GLfloat vBorder[4] = {1, 1, 1, 1};
				glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glSamplerParameterfv(lSampler, GL_TEXTURE_BORDER_COLOR, vBorder);
				if(i == 0)
				{
					glSamplerParameteri(lSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
					glSamplerParameteri(lSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				}
				glBindSampler(cGLSLProgram::kShadowMapUnit + i, lSampler);
			}
		}

		if(mvShadowMaps[lIdx]) glDeleteTextures(1, &mvShadowMaps[lIdx]);
		glGenTextures(1, &mvShadowMaps[lIdx]);
		mvShadowMapSizes[lIdx] = alSize;

		for(int i = 0; i < 2; ++i)
		{
			glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit + i);
			glBindTexture(lTarget, mvShadowMaps[lIdx]);
		}
		glTexStorage2D(lTarget, 1, GL_DEPTH_COMPONENT24, alSize, alSize);
		glTexParameteri(lTarget, GL_TEXTURE_MAX_LEVEL, 0);
		glActiveTexture(GL_TEXTURE0 + mState.GetActiveTextureUnit());

		cGpuProfiler::Label(GL_TEXTURE, mvShadowMaps[lIdx], abCube ? "Shadow cube map" : "Shadow map");
	}

	void cGLShadowTargets::BeginShadowMap(bool abCube, int alFace, int alSize)
	{
		const int lIdx = abCube ? 1 : 0;
		alSize = cMath::Clamp(alSize, 16, 8192);
		if(mvShadowMapSizes[lIdx] != alSize) CreateShadowMap(abCube, alSize);

		if(mlShadowFramebuffer == 0)
		{
			glGenFramebuffers(1, &mlShadowFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlShadowFramebuffer);
			cGpuProfiler::Label(GL_FRAMEBUFFER, mlShadowFramebuffer, "Shadow map");
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, mlShadowFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			abCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + alFace : GL_TEXTURE_2D,
			mvShadowMaps[lIdx], 0);
		mTargets.BindFramebuffer(mlShadowFramebuffer, cVector2l(alSize, alSize));
		mbInShadowMap = true;
		mState.Invalidate();

		glEnable(GL_DEPTH_CLAMP);
		glDepthMask(GL_TRUE);
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void cGLShadowTargets::EndShadowMap()
	{
		glDisable(GL_DEPTH_CLAMP);
		mbInShadowMap = false;
		mState.Invalidate();

		mTargets.RestoreAfterShadow();
	}
} // namespace hpl
