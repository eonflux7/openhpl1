// This file is part of OpenHPL1, licensed under the GNU GPL v3 or later.
#ifndef HPL_GLGRAPHICSSTATE_H
#define HPL_GLGRAPHICSSTATE_H
#include <glad/gl.h>
#include "graphics/LowLevelGraphics.h"
#include "impl/GLSLProgram.h"
#include <map>
#include <tuple>

namespace hpl
{
	// Snapshot of the destination facts that affect hpl_State. No resource ownership.
	struct cGLDrawTargetState
	{
		bool mbInScene;
		bool mbSceneResolved;
		bool mbInShadowMap;
		bool mbHasRenderTarget;
		int mlMultisampling;
	};

	// What a fullscreen pass must put back on a texture unit. A unit can
	// have a 2D and an array binding at once; the pass only changes the 2D
	// one, but both are restored so a shared unit stays as it was.
	struct cGLTextureUnitBinding
	{
		GLuint mlTexture2D = 0;
		GLuint mlTextureArray = 0;
		GLuint mlSampler = 0;
	};
	// Owns shader-visible state, default programs, UBOs, texture/sampler and VAO caches.
	// Construction needs no GL. Initialization/use and teardown of allocated resources
	// require the caller's current context; Reset/cleanup is idempotent.
	class cGLGraphicsState
	{
	public:
		cGLGraphicsState();
		~cGLGraphicsState();
		bool Init();
		void SetupGL();
		void Reset();
		void Invalidate() { mbStateChanged = true; }
		unsigned int GetActiveTextureUnit() const { return mlActiveTextureUnit; }
		GLuint GetUnitSampler(int unit) const { return mvUnitSampler[unit]; }
		cGLTextureUnitBinding GetTextureUnitBinding(int unit) const
		{
			return {mvUnitTexture2D[unit], mvUnitTextureArray[unit], mvUnitSampler[unit]};
		}
		void RestoreTextureUnit(int unit, const cGLTextureUnitBinding &aBinding);
		/// Re-read capabilities and write masks after raw GL calls. Clients
		/// must restore bindings, depth/blend functions and the viewport.
		void ResyncExternalState();
		bool GetDepthTestActive() const { return mbDepthTest; }
		bool GetDepthWriteActive() const { return mbDepthWrite; }
		bool GetBlendActive() const { return mbBlend; }
		bool GetScissorActive() const { return mbScissor; }
		bool GetCullActive() const { return mbCull; }
		bool GetAlphaToCoverageActive() const { return mbAlphaToCoverage; }
		const bool *GetColorWriteMask() const { return mvColorWrite; }
		void BindEmptyVertexArray() { BindVertexArray(mlEmptyVertexArray); }
		const cDrawMatrices &GetDrawMatrices() const { return mDrawMatrices; }
		const cLinearLightingTuning &GetLinearLightingTuning() const { return mLinearTuning; }
		void SetShading(const cShadingSettings &value) { mShading = value; }
		const cShadingSettings &GetShading() const { return mShading; }
		void SetEffects(const cEffectSettings &value) { mEffects = value; }
		const cEffectSettings &GetEffects() const { return mEffects; }
		float GetFrameTime() const { return mfFrameTime; }
		cGLGraphicsState(const cGLGraphicsState &) = delete;
		cGLGraphicsState &operator=(const cGLGraphicsState &) = delete;
		int GetCaps(eGraphicCaps aType);

		GLuint GetSampler(bool abMipMaps, GLenum aWrapS, GLenum aWrapT, GLenum aWrapR);

		void SetTextureFilter(eTextureFilter aFilter, float afAnisotropy);

		void BindSampler(unsigned int alUnit, GLuint alSampler);

		void TextureSamplerChanged(iTexture *apTex);

		void SetTexture(unsigned int alUnit, iTexture *apTex);

		void TextureStorageChanged(iTexture *apTex, GLuint alOldHandle);

		void ForgetTexture(iTexture *apTex);

		void RestoreSetTexture();

		void SetDrawMatrices(const cDrawMatrices &aMatrices);

		void SetLinearLightingTuning(const cLinearLightingTuning &aTuning);

		void ClearScreen();

		void SetClearColor(const cColor &aCol);

		void SetClearDepth(float afDepth);

		void SetClearColorActive(bool abX);

		void SetClearDepthActive(bool abX);

		void SetColorWriteActive(bool abR, bool abG, bool abB, bool abA);

		void SetDepthWriteActive(bool abX);

		void SetDepthTestActive(bool abX);

		void SetDepthTestFunc(eDepthTestFunc aFunc);

		void SetAlphaCutoff(float afMin);

		void SetCullActive(bool abX);

		void SetCullMode(eCullMode aMode);

		void SetScissorActive(bool abX);

		void SetBlendActive(bool abX);

		void SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor);

		void SetLightLook(const cLightLook &aLook);

		void SetShadowMapParams(const cShadowMapParams &aParams);

		void BindVertexArray(GLuint alVertexArray);

		void SetCurrentAttrib(int alAttrib, float afX, float afY, float afZ, float afW);

		void SetFrameUniforms(const cMatrixf &a_mtxView, const cMatrixf &a_mtxProjection,
			const cVector3f &avEyePos, float afNear, float afFar,
			float afTime, const cVector2l &avRenderSize);

		bool PrepareDraw(const cGLDrawTargetState &aTarget);
		/// True when a completed draw with the current masks can write depth.
		bool WritesDepth() const { return mbDepthTest && mbDepthWrite; }
		bool GetClearDepthActive() const { return mbClearDepth; }

	private:
		void SetSamplerFilter(GLuint alSampler, bool abMipMaps);

		GLenum GetGLBlendEnum(eBlendFunc aType);

		GLenum GetGLDepthTestFuncEnum(eDepthTestFunc aType);
		GLuint mlFrameUniformBuffer = 0;
		GLuint mlStateUniformBuffer = 0;
		// hpl_Frame/hpl_State are bound at 0/1; linked programs own material binding 2.
		static constexpr int kStateUniformFloats = 36;
		cGLSLProgram *mpDefaultProgram = nullptr;
		cGLSLProgram *mpDefaultTextureProgram = nullptr;
		cGLSLProgram *mpDefaultCubeProgram = nullptr;
		unsigned int mlActiveTextureUnit = 0;
		// Stream, mesh and fullscreen draws all bind through this cache.
		GLuint mlBoundVertexArray = 0;
		GLuint mlEmptyVertexArray = 0;
		iTexture *mpCurrentTexture[MAX_TEXTUREUNITS] = {};
		std::map<std::tuple<bool, GLenum, GLenum, GLenum>, GLuint> m_mapSamplers;
		GLuint mvUnitSampler[MAX_TEXTUREUNITS] = {};
		GLuint mvUnitTexture2D[MAX_TEXTUREUNITS] = {};
		GLuint mvUnitTextureArray[MAX_TEXTUREUNITS] = {};
		eTextureFilter mTextureFilter = eTextureFilter_Trilinear;
		float mfTextureAnisotropy = 1.0f;
		float mfAlphaCutoff = 0;
		bool mbAlphaToCoverage = false;
		bool mbStateChanged = true;
		bool mbBlend = false;
		bool mbScissor = false;
		bool mbCull = false;
		bool mvColorWrite[4] = {true, true, true, true};
		eBlendFunc mBlendSrc = eBlendFunc_One;
		eBlendFunc mBlendDest = eBlendFunc_Zero;
		bool mbClearColor = true;
		bool mbClearDepth = true;
		bool mbDepthTest = true;
		bool mbDepthWrite = true;
		cLinearLightingTuning mLinearTuning;
		cShadingSettings mShading;
		cEffectSettings mEffects;
		cLightLook mLightLook;
		cShadowMapParams mShadowParams;
		float mfFrameTime = 0;
		cDrawMatrices mDrawMatrices;
		float mvCurrentAttrib[cGLSLProgram::eVertexAttrib_LastEnum][4];
		unsigned int mlCurrentAttribChanged = ~0u;
	};
} // namespace hpl
#endif
