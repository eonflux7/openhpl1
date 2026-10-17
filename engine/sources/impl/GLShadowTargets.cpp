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
#include <algorithm>
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
		mpSelectedMap = nullptr;
		while(!mCache.empty()) Evict(mCache.begin());
		mlPendingFaceMask = 0;
		mlUseSerial = 0;
		mbDrawFailed = false;
		mbInShadowMap = false;
		if(mlShadowFramebuffer) glDeleteFramebuffers(1, &mlShadowFramebuffer);
		if(mvShadowMaps[0] || mvShadowMaps[1]) glDeleteTextures(2, mvShadowMaps);
		if(mvShadowSamplers[0] || mvShadowSamplers[1]) glDeleteSamplers(2, mvShadowSamplers);
		mlShadowFramebuffer = 0;
		mvShadowMaps[0] = mvShadowMaps[1] = 0;
		mvShadowMapSizes[0] = mvShadowMapSizes[1] = 0;
		mvShadowSamplers[0] = mvShadowSamplers[1] = 0;
	}

	void cGLShadowTargets::BindShadowMap(bool abCube, GLuint alTexture)
	{
		const GLenum lTarget = abCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
		for(int i = 0; i < 2; ++i)
		{
			glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit + i);
			glBindTexture(lTarget, alTexture);
		}
		glActiveTexture(GL_TEXTURE0 + mState.GetActiveTextureUnit());
	}

	GLuint cGLShadowTargets::AllocateShadowMap(bool abCube, int alSize)
	{
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

		GLuint lTexture = 0;
		glGenTextures(1, &lTexture);
		BindShadowMap(abCube, lTexture);
		// BindShadowMap restores the active material unit; allocation must
		// happen on a reserved unit without replacing a material texture.
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
		glTexStorage2D(lTarget, 1, GL_DEPTH_COMPONENT24, alSize, alSize);
		glTexParameteri(lTarget, GL_TEXTURE_MAX_LEVEL, 0);
		glActiveTexture(GL_TEXTURE0 + mState.GetActiveTextureUnit());

		cGpuProfiler::Label(GL_TEXTURE, lTexture, abCube ? "Shadow cube map" : "Shadow map");
		return lTexture;
	}

	void cGLShadowTargets::CreateShadowMap(bool abCube, int alSize)
	{
		const int lIndex = abCube ? 1 : 0;
		if(mvShadowMaps[lIndex]) glDeleteTextures(1, &mvShadowMaps[lIndex]);
		mvShadowMaps[lIndex] = AllocateShadowMap(abCube, alSize);
		mvShadowMapSizes[lIndex] = alSize;
	}

	void cGLShadowTargets::Evict(std::map<uint64_t, cCachedMap>::iterator aEntry)
	{
		mlCacheBytes -= aEntry->second.mlBytes;
		glDeleteTextures(1, &aEntry->second.mlTexture);
		mCache.erase(aEntry);
	}

	unsigned cGLShadowTargets::SelectShadowMap(uint64_t alLightId, bool abCube, int alResolution,
		const std::vector<uint64_t> &avSignature, size_t alBudgetBytes)
	{
		mpSelectedMap = nullptr;
		mlPendingFaceMask = 0;
		alResolution = cMath::Clamp(alResolution, 16, 8192);
		// DEPTH_COMPONENT24 commonly occupies four bytes. Include the CPU
		// signature in the budget and separately cap small-entry metadata.
		const size_t lEntryBytes = size_t(alResolution) * alResolution * (abCube ? 6 : 1) * 4 + avSignature.size() * sizeof(uint64_t);
		auto cacheIt = mCache.find(alLightId);
		if(cacheIt != mCache.end() && (cacheIt->second.mlResolution != alResolution || cacheIt->second.mbCube != abCube))
		{
			Evict(cacheIt);
			cacheIt = mCache.end();
		}
		if(cacheIt != mCache.end())
		{
			// Caster membership changes the signature length, not the texture
			// layout. Reaccount it without reallocating depth storage.
			mlCacheBytes -= cacheIt->second.mlBytes;
			cacheIt->second.mlBytes = lEntryBytes;
			mlCacheBytes += lEntryBytes;
		}
		const bool bRetain = alLightId != 0 && lEntryBytes <= alBudgetBytes;
		GLuint lRecycledTexture = 0;
		while(!mCache.empty() &&
			(mlCacheBytes + (bRetain && cacheIt == mCache.end() ? lEntryBytes : 0) > alBudgetBytes ||
				(bRetain && cacheIt == mCache.end() && mCache.size() >= 64)))
		{
			auto oldestIt = std::min_element(mCache.begin(), mCache.end(), [](const auto &aLeft, const auto &aRight)
			{
				return aLeft.second.mlLastUseSerial < aRight.second.mlLastUseSerial;
			});
			if(oldestIt == cacheIt) cacheIt = mCache.end();
			if(bRetain && cacheIt == mCache.end() && !lRecycledTexture && oldestIt->second.mlResolution == alResolution && oldestIt->second.mbCube == abCube)
			{
				// A working set larger than the budget should redraw into reused
				// storage rather than allocate a fresh GPU texture on every miss.
				lRecycledTexture = oldestIt->second.mlTexture;
				oldestIt->second.mlTexture = 0;
			}
			Evict(oldestIt);
		}
		if(!bRetain)
		{
			const int lIndex = abCube ? 1 : 0;
			if(mvShadowMapSizes[lIndex] != alResolution) CreateShadowMap(abCube, alResolution);
			BindShadowMap(abCube, mvShadowMaps[lIndex]);
			return 0;
		}
		if(cacheIt == mCache.end())
		{
			cCachedMap cachedMap;
			cachedMap.mlTexture = lRecycledTexture ? lRecycledTexture : AllocateShadowMap(abCube, alResolution);
			cachedMap.mlResolution = alResolution;
			cachedMap.mbCube = abCube;
			cachedMap.mlBytes = lEntryBytes;
			mlCacheBytes += lEntryBytes;
			cacheIt = mCache.emplace(alLightId, std::move(cachedMap)).first;
		}
		auto &cachedMap = cacheIt->second;
		if(cachedMap.mvSignature != avSignature)
		{
			if(cachedMap.mvSignature.size() != avSignature.size())
				std::vector<uint64_t>(avSignature).swap(cachedMap.mvSignature);
			else
				cachedMap.mvSignature = avSignature;
			cachedMap.mlValidFaceMask = 0;
		}
		cachedMap.mlLastUseSerial = ++mlUseSerial;
		mpSelectedMap = &cachedMap;
		BindShadowMap(abCube, cachedMap.mlTexture);
		return cachedMap.mlValidFaceMask;
	}

	void cGLShadowTargets::BeginShadowMap(bool abCube, int alFace, int alSize)
	{
		if(!mbInShadowMap) mbDrawFailed = false;
		const int lIdx = abCube ? 1 : 0;
		alSize = cMath::Clamp(alSize, 16, 8192);
		if(mpSelectedMap && (mpSelectedMap->mbCube != abCube || mpSelectedMap->mlResolution != alSize))
			mpSelectedMap = nullptr;
		if(!mpSelectedMap && mvShadowMapSizes[lIdx] != alSize) CreateShadowMap(abCube, alSize);
		const GLuint lTexture = mpSelectedMap ? mpSelectedMap->mlTexture : mvShadowMaps[lIdx];
		BindShadowMap(abCube, lTexture);
		mlPendingFaceMask |= 1u << (abCube ? alFace : 0);
		if(mpSelectedMap) mpSelectedMap->mlValidFaceMask &= ~(1u << (abCube ? alFace : 0));

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
			lTexture, 0);
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
		if(mpSelectedMap && !mbDrawFailed) mpSelectedMap->mlValidFaceMask |= mlPendingFaceMask;
		mlPendingFaceMask = 0;
		glDisable(GL_DEPTH_CLAMP);
		// Do not keep an evicted texture alive through an unbound FBO attachment.
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		mbInShadowMap = false;
		mState.Invalidate();

		mTargets.RestoreAfterShadow();
	}
} // namespace hpl
