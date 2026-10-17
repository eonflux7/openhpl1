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
#include "impl/GLGraphicsState.h"
#include "impl/GLGraphicsHelpers.h"
#include "impl/SDLTexture.h"
#include "impl/GpuProfiler.h"
#include "system/LowLevelSystem.h"
#include "math/Math.h"
#include <cassert>
#include <cstring>
namespace hpl
{
	// std140 layout of hpl_Frame in hpl1_frame.glsl; matrices column-major.
	struct cFrameUniformData
	{
		float mvView[16];
		float mvProjection[16];
		float mvViewProjection[16];
		float mvEyePos[4];
		float mvRenderSize[4];
		float mvDepthRange[4];
		float mvTime[4];
		float mvShading0[4];
		float mvShading1[4];
	};

	static void CopyColumnMajor(float *apDest, const cMatrixf &a_mtx)
	{
		cMatrixf mtxTranspose = a_mtx.GetTranspose();
		memcpy(apDest, mtxTranspose.v, sizeof(float) * 16);
	}

	//-------------------------------------------------

	// An extension until OpenGL 4.6, with the same values
	static bool HasAnisotropicFiltering()
	{
		return GLAD_GL_VERSION_4_6 || GLAD_GL_EXT_texture_filter_anisotropic;
	}

	cGLGraphicsState::cGLGraphicsState()
	{
		for(int i = 0; i < cGLSLProgram::eVertexAttrib_LastEnum; ++i) SetCurrentAttrib(i, 0, 0, 0, 1);
		SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Normal, 0, 0, 1, 1);
		SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, 1, 1, 1, 1);
	}
	cGLGraphicsState::~cGLGraphicsState()
	{
		Reset();
	}
	bool cGLGraphicsState::Init()
	{
		glGenBuffers(1, &mlFrameUniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, mlFrameUniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(cFrameUniformData), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, cGLSLProgram::kFrameBlockBinding, mlFrameUniformBuffer);
		cGpuProfiler::Label(GL_BUFFER, mlFrameUniformBuffer, "hpl_Frame");

		glGenBuffers(1, &mlStateUniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, mlStateUniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * kStateUniformFloats, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, cGLSLProgram::kStateBlockBinding, mlStateUniformBuffer);
		cGpuProfiler::Label(GL_BUFFER, mlStateUniformBuffer, "hpl_State");

		mpDefaultProgram = hplNew(cGLSLProgram, ("Default"));
		mpDefaultTextureProgram = hplNew(cGLSLProgram, ("Default TEXTURE"));
		mpDefaultCubeProgram = hplNew(cGLSLProgram, ("Default CUBE"));
		if(!mpDefaultProgram->CreateFromFile("Default") ||
			!mpDefaultTextureProgram->CreateFromFile("Default", "TEXTURE") ||
			!mpDefaultCubeProgram->CreateFromFile("Default", "CUBE"))
		{
			FatalError("Couldn't create the default programs (hpl1_Default)!\n");
			return false;
		}

		glGenVertexArrays(1, &mlEmptyVertexArray);
		return true;
	}
	void cGLGraphicsState::Reset()
	{
		for(auto &sampler : m_mapSamplers) glDeleteSamplers(1, &sampler.second);
		m_mapSamplers.clear();
		if(mlFrameUniformBuffer) glDeleteBuffers(1, &mlFrameUniformBuffer);
		if(mlStateUniformBuffer) glDeleteBuffers(1, &mlStateUniformBuffer);
		if(mlEmptyVertexArray) glDeleteVertexArrays(1, &mlEmptyVertexArray);
		mlFrameUniformBuffer = mlStateUniformBuffer = mlEmptyVertexArray = 0;
		if(mpDefaultProgram) hplDelete(mpDefaultProgram);
		if(mpDefaultTextureProgram) hplDelete(mpDefaultTextureProgram);
		if(mpDefaultCubeProgram) hplDelete(mpDefaultCubeProgram);
		mpDefaultProgram = mpDefaultTextureProgram = mpDefaultCubeProgram = nullptr;
	}

	int cGLGraphicsState::GetCaps(eGraphicCaps aType)
	{
		switch(aType)
		{
			//Max Texture Image Units
			case eGraphicCaps_MaxTextureImageUnits:
			{
				int lUnits;
				glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, (GLint *)&lUnits);
				return lUnits;
			}

			//Texture Anisotropy
			case eGraphicCaps_AnisotropicFiltering:
			{
				return HasAnisotropicFiltering() ? 1 : 0;
			}

			//Texture Anisotropy
			case eGraphicCaps_MaxAnisotropicFiltering:
			{
				if(!HasAnisotropicFiltering()) return 0;

				float fMax;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fMax);
				return (int)fMax;
			}

			//The scene is drawn into multisampled textures
			case eGraphicCaps_MaxMultisampling:
			{
				GLint lSamples = 0, lColorSamples = 0, lDepthSamples = 0;
				glGetIntegerv(GL_MAX_SAMPLES, &lSamples);
				glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &lColorSamples);
				glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &lDepthSamples);
				return cMath::Min(lSamples, cMath::Min(lColorSamples, lDepthSamples));
			}
		}

		return 0;
	}

	void cGLGraphicsState::SetSamplerFilter(GLuint alSampler, bool abMipMaps)
	{
		GLenum lMinFilter = GL_LINEAR;
		if(abMipMaps)
			lMinFilter = mTextureFilter == eTextureFilter_Bilinear ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
		glSamplerParameteri(alSampler, GL_TEXTURE_MIN_FILTER, lMinFilter);
		glSamplerParameteri(alSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if(HasAnisotropicFiltering())
			glSamplerParameterf(alSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, abMipMaps ? mfTextureAnisotropy : 1.0f);
	}

	GLuint cGLGraphicsState::GetSampler(bool abMipMaps, GLenum aWrapS, GLenum aWrapT, GLenum aWrapR)
	{
		GLuint &lSampler = m_mapSamplers[std::make_tuple(abMipMaps, aWrapS, aWrapT, aWrapR)];
		if(lSampler == 0)
		{
			glGenSamplers(1, &lSampler);
			glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_S, aWrapS);
			glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_T, aWrapT);
			glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_R, aWrapR);
			SetSamplerFilter(lSampler, abMipMaps);
		}
		return lSampler;
	}

	void cGLGraphicsState::SetTextureFilter(eTextureFilter aFilter, float afAnisotropy)
	{
		mTextureFilter = aFilter;
		mfTextureAnisotropy = cMath::Clamp(afAnisotropy, 1.0f, (float)cMath::Max(GetCaps(eGraphicCaps_MaxAnisotropicFiltering), 1));

		for(auto &sampler : m_mapSamplers)
			if(std::get<0>(sampler.first)) SetSamplerFilter(sampler.second, true);
	}

	void cGLGraphicsState::BindSampler(unsigned int alUnit, GLuint alSampler)
	{
		if(alUnit >= MAX_TEXTUREUNITS || mvUnitSampler[alUnit] == alSampler) return;
		mvUnitSampler[alUnit] = alSampler;
		glBindSampler(alUnit, alSampler);
	}

	void cGLGraphicsState::TextureSamplerChanged(iTexture *apTex)
	{
		for(unsigned int lUnit = 0; lUnit < MAX_TEXTUREUNITS; lUnit++)
			if(mpCurrentTexture[lUnit] == apTex) BindSampler(lUnit, static_cast<cSDLTexture *>(apTex)->GetSampler());
	}

	void cGLGraphicsState::SetTexture(unsigned int alUnit, iTexture *apTex)
	{
		if(apTex == mpCurrentTexture[alUnit]) return;

		GLenum NewTarget = 0;
		if(apTex)
			NewTarget = TextureTargetToGL(apTex->GetTarget());

		glActiveTexture(GL_TEXTURE0 + alUnit);

		// A unit without a texture keeps the last one bound
		if(apTex)
		{
			cSDLTexture *pSDLTex = static_cast<cSDLTexture *>(apTex);

			glBindTexture(NewTarget, pSDLTex->GetTextureHandle());
			BindSampler(alUnit, pSDLTex->GetSampler());
		}

		mpCurrentTexture[alUnit] = apTex;
		mlActiveTextureUnit = alUnit;
	}

	void cGLGraphicsState::TextureStorageChanged(iTexture *apTex)
	{
		GLint active = GL_TEXTURE0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
		for(unsigned int unit = 0; unit < MAX_TEXTUREUNITS; ++unit)
		{
			if(mpCurrentTexture[unit] != apTex) continue;
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(TextureTargetToGL(apTex->GetTarget()), static_cast<cSDLTexture *>(apTex)->GetTextureHandle());
		}
		glActiveTexture(active);
	}

	void cGLGraphicsState::ForgetTexture(iTexture *apTex)
	{
		for(unsigned int lUnit = 0; lUnit < MAX_TEXTUREUNITS; lUnit++)
			if(mpCurrentTexture[lUnit] == apTex) mpCurrentTexture[lUnit] = nullptr;
	}

	void cGLGraphicsState::RestoreSetTexture()
	{
		GLint lActiveUnit = GL_TEXTURE0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &lActiveUnit);

		int lUnit = lActiveUnit - GL_TEXTURE0;
		if(lUnit < 0 || lUnit >= MAX_TEXTUREUNITS) return;

		iTexture *pTex = mpCurrentTexture[lUnit];
		if(pTex == nullptr) return;

		GLenum target = TextureTargetToGL(pTex->GetTarget());
		glBindTexture(target, static_cast<cSDLTexture *>(pTex)->GetTextureHandle());
	}

	void cGLGraphicsState::SetDrawMatrices(const cDrawMatrices &aMatrices)
	{
		mDrawMatrices = aMatrices;
	}

	void cGLGraphicsState::SetLinearLightingTuning(const cLinearLightingTuning &aTuning)
	{
		mLinearTuning = aTuning;
		mbStateChanged = true;
	}

	void cGLGraphicsState::ClearScreen()
	{
		GLbitfield bitmask = 0;

		if(mbClearColor) bitmask |= GL_COLOR_BUFFER_BIT;
		if(mbClearDepth) bitmask |= GL_DEPTH_BUFFER_BIT;

		glClear(bitmask);
	}

	void cGLGraphicsState::SetClearColor(const cColor &aCol)
	{
		glClearColor(aCol.r, aCol.g, aCol.b, aCol.a);
	}

	void cGLGraphicsState::SetClearDepth(float afDepth)
	{
		glClearDepth(afDepth);
	}

	void cGLGraphicsState::SetClearColorActive(bool abX)
	{
		mbClearColor = abX;
	}

	void cGLGraphicsState::SetClearDepthActive(bool abX)
	{
		mbClearDepth = abX;
	}

	void cGLGraphicsState::SetColorWriteActive(bool abR, bool abG, bool abB, bool abA)
	{
		glColorMask(abR, abG, abB, abA);
	}

	void cGLGraphicsState::SetDepthWriteActive(bool abX)
	{
		glDepthMask(abX);
	}

	void cGLGraphicsState::SetDepthTestActive(bool abX)
	{
		if(abX)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void cGLGraphicsState::SetDepthTestFunc(eDepthTestFunc aFunc)
	{
		glDepthFunc(GetGLDepthTestFuncEnum(aFunc));
	}

	void cGLGraphicsState::SetAlphaCutoff(float afMin)
	{
		afMin = cMath::Clamp(afMin, 0.0f, 1.0f);
		if(mfAlphaCutoff == afMin) return;
		mfAlphaCutoff = afMin;
		mbStateChanged = true;
	}

	void cGLGraphicsState::SetCullActive(bool abX)
	{
		if(abX)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void cGLGraphicsState::SetCullMode(eCullMode aMode)
	{
		glCullFace(GL_BACK);
		if(aMode == eCullMode_Clockwise)
			glFrontFace(GL_CCW);
		else
			glFrontFace(GL_CW);
	}

	void cGLGraphicsState::SetScissorActive(bool abX)
	{
		if(abX)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

	void cGLGraphicsState::SetBlendActive(bool abX)
	{
		if(abX)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);

		if(mbBlend != abX) mbStateChanged = true;
		mbBlend = abX;
	}

	void cGLGraphicsState::SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor)
	{
		glBlendFunc(GetGLBlendEnum(aSrcFactor), GetGLBlendEnum(aDestFactor));

		if(mBlendSrc != aSrcFactor || mBlendDest != aDestFactor) mbStateChanged = true;
		mBlendSrc = aSrcFactor;
		mBlendDest = aDestFactor;
	}

	void cGLGraphicsState::SetLightLook(const cLightLook &aLook)
	{
		mLightLook = aLook;
		mbStateChanged = true;
	}

	void cGLGraphicsState::SetShadowMapParams(const cShadowMapParams &aParams)
	{
		mShadowParams = aParams;
		mbStateChanged = true;
	}

	void cGLGraphicsState::BindVertexArray(GLuint alVertexArray)
	{
		if(alVertexArray == mlBoundVertexArray) return;
		glBindVertexArray(alVertexArray);
		mlBoundVertexArray = alVertexArray;
	}

	void cGLGraphicsState::SetCurrentAttrib(int alAttrib, float afX, float afY, float afZ, float afW)
	{
		float *pVal = mvCurrentAttrib[alAttrib];
		pVal[0] = afX;
		pVal[1] = afY;
		pVal[2] = afZ;
		pVal[3] = afW;
		mlCurrentAttribChanged |= 1u << alAttrib;
	}

	GLenum cGLGraphicsState::GetGLBlendEnum(eBlendFunc aType)
	{
		switch(aType)
		{
			case eBlendFunc_Zero:
				return GL_ZERO;
			case eBlendFunc_One:
				return GL_ONE;
			case eBlendFunc_SrcColor:
				return GL_SRC_COLOR;
			case eBlendFunc_OneMinusSrcColor:
				return GL_ONE_MINUS_SRC_COLOR;
			case eBlendFunc_DestColor:
				return GL_DST_COLOR;
			case eBlendFunc_OneMinusDestColor:
				return GL_ONE_MINUS_DST_COLOR;
			case eBlendFunc_SrcAlpha:
				return GL_SRC_ALPHA;
			case eBlendFunc_OneMinusSrcAlpha:
				return GL_ONE_MINUS_SRC_ALPHA;
			case eBlendFunc_DestAlpha:
				return GL_DST_ALPHA;
			case eBlendFunc_OneMinusDestAlpha:
				return GL_ONE_MINUS_DST_ALPHA;
			case eBlendFunc_SrcAlphaSaturate:
				return GL_SRC_ALPHA_SATURATE;
		}
		return 0;
	}

	GLenum cGLGraphicsState::GetGLDepthTestFuncEnum(eDepthTestFunc aType)
	{
		switch(aType)
		{
			case eDepthTestFunc_Never:
				return GL_NEVER;
			case eDepthTestFunc_Less:
				return GL_LESS;
			case eDepthTestFunc_LessOrEqual:
				return GL_LEQUAL;
			case eDepthTestFunc_Greater:
				return GL_GREATER;
			case eDepthTestFunc_GreaterOrEqual:
				return GL_GEQUAL;
			case eDepthTestFunc_Equal:
				return GL_EQUAL;
			case eDepthTestFunc_NotEqual:
				return GL_NOTEQUAL;
			case eDepthTestFunc_Always:
				return GL_ALWAYS;
		}
		return 0;
	}

	void cGLGraphicsState::SetFrameUniforms(const cMatrixf &a_mtxView, const cMatrixf &a_mtxProjection,
		const cVector3f &avEyePos, float afNear, float afFar,
		float afTime, const cVector2l &avRenderSize)
	{
		cVector2f vSize((float)avRenderSize.x, (float)avRenderSize.y);

		cFrameUniformData data;
		CopyColumnMajor(data.mvView, a_mtxView);
		CopyColumnMajor(data.mvProjection, a_mtxProjection);
		CopyColumnMajor(data.mvViewProjection, cMath::MatrixMul(a_mtxProjection, a_mtxView));
		data.mvEyePos[0] = avEyePos.x;
		data.mvEyePos[1] = avEyePos.y;
		data.mvEyePos[2] = avEyePos.z;
		data.mvEyePos[3] = 1;
		data.mvRenderSize[0] = vSize.x;
		data.mvRenderSize[1] = vSize.y;
		data.mvRenderSize[2] = 1.0f / vSize.x;
		data.mvRenderSize[3] = 1.0f / vSize.y;
		data.mvDepthRange[0] = afNear;
		data.mvDepthRange[1] = afFar;
		data.mvDepthRange[2] = data.mvDepthRange[3] = 0;
		data.mvTime[0] = afTime;
		mfFrameTime = afTime;
		data.mvTime[1] = data.mvTime[2] = data.mvTime[3] = 0;
		data.mvShading0[0] = 0;
		data.mvShading0[1] = mShading.mfSpecular;
		data.mvShading0[2] = mShading.mfReflectance;
		data.mvShading0[3] = mShading.mfAmbientSheen;
		data.mvShading1[0] = mShading.mfRoughDull;
		data.mvShading1[1] = mShading.mfRoughShiny;
		data.mvShading1[2] = mShading.mfAmbientUp;
		data.mvShading1[3] = mShading.mfAmbientFacing;

		glBindBuffer(GL_UNIFORM_BUFFER, mlFrameUniformBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	bool cGLGraphicsState::PrepareDraw(const cGLDrawTargetState &aTarget)
	{
		cGLSLProgram *program = cGLSLProgram::GetBoundProgram();
		if(!program)
		{
			eTextureTarget target = mpCurrentTexture[0] ? mpCurrentTexture[0]->GetTarget() : eTextureTarget_LastEnum;
			program = (target == eTextureTarget_2D || target == eTextureTarget_2DArray) ? mpDefaultTextureProgram : (target == eTextureTarget_CubeMap ? mpDefaultCubeProgram : mpDefaultProgram);
		}
		program->SetMatrixf(eProgramParam::worldViewProj, mDrawMatrices.GetModelViewProjection());
		unsigned int arrayMask = 0;
		float layers[8] = {};
		for(int i = 0; i < 8; ++i)
			if(mpCurrentTexture[i] && mpCurrentTexture[i]->GetTarget() == eTextureTarget_2DArray)
			{
				arrayMask |= 1u << i;
				layers[i] = static_cast<cSDLTexture *>(mpCurrentTexture[i])->GetAnimationFrame();
			}
		if(!program->Apply(arrayMask, layers)) return false;

		if(mlCurrentAttribChanged)
		{
			for(int i = 0; i < cGLSLProgram::eVertexAttrib_LastEnum; ++i)
				if(mlCurrentAttribChanged & (1u << i)) glVertexAttrib4fv(i, mvCurrentAttrib[i]);
			mlCurrentAttribChanged = 0;
		}

		if(mbStateChanged)
		{
			// Colours are converted for the scene, which holds linear
			// values. "Modulate x2" (destination x source x 2) doubles on
			// encoded values: on linear ones that is 2^2.2, of which the
			// blend supplies 2. What is added to the scene gets the glow
			// value (the light passes are added too, but do their own
			// conversion).
			bool bConvert = aTarget.mbInScene;
			bool bMulX2 = mbBlend && mBlendSrc == eBlendFunc_DestColor && mBlendDest == eBlendFunc_SrcColor;
			bool bAdd = mbBlend && mBlendDest == eBlendFunc_One &&
				(mBlendSrc == eBlendFunc_One || mBlendSrc == eBlendFunc_SrcAlpha);
			float fScale = bMulX2 ? 2.2974f : 1.0f;
			float fGlow = bConvert && bAdd ? mLinearTuning.mfGlow : 1.0f;
			bool bAlpha = mbBlend && mBlendSrc == eBlendFunc_SrcAlpha && mBlendDest == eBlendFunc_OneMinusSrcAlpha;
			float fAlphaMatch = bConvert && bAlpha ? cMath::Clamp(mLinearTuning.mfBlendMatch, 0.0f, 1.0f) : 0.0f;
			// In the multisampled scene the test decides for each sample:
			// the fragment's alpha becomes the share of the pixel's samples
			// it covers (kAlphaToCoverage in the shaders)
			bool bAlphaTest = mfAlphaCutoff > 0;
			bool bToCoverage = bAlphaTest && aTarget.mlMultisampling > 0 && aTarget.mbInScene && aTarget.mbSceneResolved == false &&
				!aTarget.mbHasRenderTarget && aTarget.mbInShadowMap == false;
			if(bToCoverage != mbAlphaToCoverage)
			{
				if(bToCoverage)
					glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
				else
					glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
				mbAlphaToCoverage = bToCoverage;
			}
			float fAlphaTest = bToCoverage ? (float)cGLSLProgram::kAlphaToCoverage : bAlphaTest ? 1.0f
																								: 0.0f;
			const cShadowMapParams &shadow = mShadowParams;
			// The colour is scaled as a screen value: brightness is an
			// amount of light. Exactly 1 for a light as the game made it.
			const cLightLook &look = mLightLook;
			float fLookScale = look.mfBrightness == 1.0f ? 1.0f : powf(cMath::Max(look.mfBrightness, 0.0f), 1.0f / 2.2f);
			float vState[kStateUniformFloats] = {
				fAlphaTest, mfAlphaCutoff, fAlphaMatch, fGlow,
				bConvert ? 1.0f : 0.0f, fScale, mLinearTuning.mfLightExp,
				0.0f,
				(float)shadow.mlFilter, shadow.mfNear, shadow.mfFar, shadow.mfTexelSize,
				shadow.mfSoftness, shadow.mfLightSize, shadow.mfMaxBlur, shadow.mbShowOnly ? 1.0f : 0.0f,
				shadow.mfBias, shadow.mfSlopeBias, (float)shadow.mlSamples, (float)shadow.mlBlockerSamples,
				shadow.mvUvScale.x, shadow.mvUvScale.y, shadow.mbNoise ? 1.0f : 0.0f, 0.0f,
				look.mbDistanceFalloff ? 1.0f : 0.0f, look.mfEvenAt, look.mfNearBoost, look.HasTube() ? 1.0f : 0.0f,
				look.mbBeam ? 1.0f : 0.0f, look.mfBeamHotspot, look.mfBeamField, look.mfBeamWidth,
				look.mTint.r * fLookScale, look.mTint.g * fLookScale, look.mTint.b * fLookScale, 0.0f};
			glBindBuffer(GL_UNIFORM_BUFFER, mlStateUniformBuffer);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(vState), vState);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			mbStateChanged = false;
		}
		return true;
	}
	void cGLGraphicsState::SetupGL()
	{
		//Inits GL stuff
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		// Each fragment's colour is clamped to 0..1 before blending, as an
		// 8-bit buffer did (by the fragment shaders, see cGLSLProgram); only
		// the sums of blending go past 1 in the float scene buffer.

		//Depth Test setup
		glClearDepth(1.0f); //VAlues buffer is cleared with
		glEnable(GL_DEPTH_TEST); //enable depth testing
		glDepthFunc(GL_LEQUAL); //function to do depth test with


		//Stencil setup
		glClearStencil(0);

		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		SetDrawMatrices(cDrawMatrices{});

		// Cube maps are filtered across the edges of their faces
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		//Show some info
		Log("  Max texture image units: %d\n", GetCaps(eGraphicCaps_MaxTextureImageUnits));
		Log("  Anisotropic filtering: %d\n", GetCaps(eGraphicCaps_AnisotropicFiltering));
		if(GetCaps(eGraphicCaps_AnisotropicFiltering))
			Log("  Max Anisotropic degree: %d\n", GetCaps(eGraphicCaps_MaxAnisotropicFiltering));
	}
} // namespace hpl
