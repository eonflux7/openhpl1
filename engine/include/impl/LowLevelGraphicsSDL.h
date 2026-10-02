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
#ifndef HPL_LOWLEVELGRAPHICS_SDL_H
#define HPL_LOWLEVELGRAPHICS_SDL_H

#include <map>
#include <tuple>

#include <glad/gl.h>

#include <SDL3/SDL.h>

#include "graphics/LowLevelGraphics.h"
#include "impl/DebugOverlay.h"
#include "impl/FixedFunctionShaders.h"
#include "impl/GLSLProgram.h"
#include "impl/GpuProfiler.h"
#include "impl/SDLPixelFormat.h"
#include "math/MathTypes.h"



namespace hpl {

	//-------------------------------------------------

	GLenum ColorFormatToGL(eColorDataFormat aFormat);

	GLenum TextureTargetToGL(eTextureTarget aTarget);

	//-------------------------------------------------

	class cLowLevelGraphicsSDL : public iLowLevelGraphics
	{
	public:
		cLowLevelGraphicsSDL();
		~cLowLevelGraphicsSDL() override;

		bool Init(	int alWidth, int alHeight, int alBpp, int abFullscreen, int alMultisampling,
					const tString& asWindowCaption, int alDisplay) override;

		int GetDisplayNum() override;
		tString GetDisplayName(int alDisplay) override;
		cVector2l GetDisplaySize(int alDisplay) override;
		void GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes) override;
		int GetDisplay() override;

		int GetCaps(eGraphicCaps aType) override;

		void ShowCursor(bool abX) override;

		void SetVsyncActive(bool abX) override;

		void SetGammaCorrection(float afX) override;
		float GetGammaCorrection() override;

		int GetMultisampling() override{ return mlMultisampling;}
		void SetMultisampling(int alSamples) override;
		void SetTextureFilter(eTextureFilter aFilter, float afAnisotropy) override;

		void SetClipPlane(int alIdx, const cPlanef& aPlane) override;
		cPlanef GetClipPlane(int alIdx, const cPlanef& aPlane) override;
		void SetClipPlaneActive(int alIdx, bool abX) override;

		cVector2f GetScreenSize() override;
		cVector2f GetRenderSize() override;
		void SetRenderScale(float afScale) override;
		float GetRenderScale() override;
		void SetLinearLighting(bool abX) override;
		bool GetLinearLighting() override;
		void SetLinearSource(bool abX) override;
		void SetLinearLightingTuning(const cLinearLightingTuning& aTuning) override;
		const cLinearLightingTuning& GetLinearLightingTuning() override{ return mLinearTuning; }
		void SetShading(const cShadingSettings& aSettings) override{ mShading = aSettings; }
		const cShadingSettings& GetShading() override{ return mShading; }
		void SetFilmGrain(const cFilmGrainSettings& aSettings) override{ mFilmGrain = aSettings; }
		const cFilmGrainSettings& GetFilmGrain() override{ return mFilmGrain; }
		void SetSmaa(const cSmaaSettings& aSettings) override{ mSmaa = aSettings; }
		const cSmaaSettings& GetSmaa() override{ return mSmaa; }
		void BeginScene() override;
		void ResolveScene() override;
		void ResolveSceneDepth() override;
		void EndScene() override;
		cVector2f GetVirtualSize() override;
		void SetVirtualSize(cVector2f avSize) override;
		cVector2f GetVirtualOffset() override;

		iBitmap2D* CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp) override;
		iFontData* CreateFontData(const tString &asName) override;

		iTexture* CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture* CreateTexture(const tString &asName,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture* CreateTexture(iBitmap2D* apBmp,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture* CreateFloatTexture(const cVector2l& avSize, int alChannels, eTextureTarget aTarget) override;
		iTexture* CreateTexture(const cVector2l& avSize,int alBpp,cColor aFillCol,
								bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;


		iPixelFormat* GetPixelFormat() override;

		iGpuProgram* CreateGpuProgram(const tString& asName, eGpuProgramType aType) override;

		void SaveScreenToBMP(const tString& asFile) override;
		void SaveNextFrameToPNG(const tString& asFile) override { msSaveNextFrame = asFile; }

		/////////// MATRIX METHODS /////////////////////////

		void PushMatrix(eMatrix aMtxType) override;
		void PopMatrix(eMatrix aMtxType) override;
		void SetIdentityMatrix(eMatrix aMtxType) override;

		void SetMatrix(eMatrix aMtxType, const cMatrixf& a_mtxA) override;

		void TranslateMatrix(eMatrix aMtxType, const cVector3f &avPos) override;
		void ScaleMatrix(eMatrix aMtxType, const cVector3f &avScale) override;

		const cMatrixf& GetMatrix(eMatrix aMtxType) override{ return mvMatrixStack[aMtxType].back(); }

		void SetFrameUniforms(const cMatrixf& a_mtxView, const cMatrixf& a_mtxProjection,
							const cVector3f& avEyePos, float afNear, float afFar,
							float afTime) override;

		void BeginGpuSection(const char *asName) override{ mGpuProfiler.Begin(asName); }
		void EndGpuSection() override{ mGpuProfiler.End(); }
		void SetDebugOverlayActive(bool abX) override{ mDebugOverlay.SetActive(abX); }
		bool GetDebugOverlayActive() override{ return mDebugOverlay.IsActive(); }
		void SetDebugPanel(std::function<void()> aDraw) override{ mDebugOverlay.SetPanel(std::move(aDraw)); }
		void SetDebugPanelInput(bool abX) override{ mDebugOverlay.SetInput(abX); }

		cGpuProfiler* GetGpuProfiler(){ return &mGpuProfiler; }

		/**
		 * Called before every draw: puts the bound engine programs, or the
		 * fixed-function shaders for the stages without one, in the program
		 * pipeline, and brings the shaders' state up to date.
		 */
		void PrepareDraw();

		/// Every draw binds its vertex array through this.
		void BindVertexArray(GLuint alVertexArray);
		/**
		 * Copies indices to the stream index buffer and binds it to the
		 * bound vertex array. Returns what glDrawElements takes as the
		 * pointer.
		 */
		const void* StreamIndices(const unsigned int *apIndices, int alCount);

		void SetOrthoProjection(const cVector2f& avSize, float afMin, float afMax) override;
		void SetVirtualOrthoProjection(float afMin, float afMax) override;

		/////////// DRAWING METHODS /////////////////////////

		// OCCLUSION
		iOcclusionQuery* CreateOcclusionQuery() override;
		void DestroyOcclusionQuery(iOcclusionQuery *apQuery) override;

		// CLEARING THE FRAMEBUFFER
		void ClearScreen() override;

		void SetClearColor(const cColor& aCol) override;
		void SetClearDepth(float afDepth) override;
		void SetClearStencil(int alVal) override;

		void SetClearColorActive(bool abX) override;
		void SetClearDepthActive(bool abX) override;
		void SetClearStencilActive(bool abX) override;

		void SetColorWriteActive(bool abR,bool abG,bool abB,bool abA) override;
		void SetDepthWriteActive(bool abX) override;

		void SetCullActive(bool abX) override;
		void SetCullMode(eCullMode aMode) override;

		//DEPTH
		void SetDepthTestActive(bool abX) override;
		void SetDepthTestFunc(eDepthTestFunc aFunc) override;

		//ALPHA
		void SetAlphaTestActive(bool abX) override;
		void SetAlphaTestFunc(eAlphaTestFunc aFunc,float afRef) override;

		//STENCIL
		void SetStencilActive(bool abX) override;
		/*void SetStencilTwoSideActive(bool abX);
		void SetStencilFace(eStencilFace aFace);
		void SetStencilFunc(eStencilFunc aFunc,int alRef, unsigned int aMask);
		void SetStencilOp(eStencilOp aFailOp,eStencilOp aZFailOp,eStencilOp aZPassOp);*/
		void SetStencil(eStencilFunc aFunc,int alRef, unsigned int aMask,
				eStencilOp aFailOp,eStencilOp aZFailOp,eStencilOp aZPassOp) override;
		void SetStencilTwoSide(eStencilFunc aFrontFunc,eStencilFunc aBackFunc,
				int alRef, unsigned int aMask,
				eStencilOp aFrontFailOp,eStencilOp aFrontZFailOp,eStencilOp aFrontZPassOp,
				eStencilOp aBackFailOp,eStencilOp aBackZFailOp,eStencilOp aBackZPassOp) override;
		void SetStencilTwoSide(bool abX) override;


		//SCISSOR
		void SetScissorActive(bool abX) override;
		void SetScissorRect(const cRect2l &aRect) override;

		//BLENDING
		void SetBlendActive(bool abX) override;
		void SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor) override;
		void SetBlendFuncSeparate(eBlendFunc aSrcFactorColor, eBlendFunc aDestFactorColor,
			eBlendFunc aSrcFactorAlpha, eBlendFunc aDestFactorAlpha) override;


		// TEXTURE
		void SetTexture(unsigned int alUnit,iTexture* apTex) override;
		/**
		 * Called by a texture being destroyed, so that SetTexture does not
		 * take a new texture at the same address for the one still set.
		 */
		void ForgetTexture(iTexture* apTex);
		/**
		 * Called by a texture after it has bound itself to create or update
		 * its data: binds and enables the texture set with SetTexture on the
		 * active unit again.
		 */
		void RestoreSetTexture();
		/**
		 * The sampler object for textures with these wrap modes, with
		 * mipmaps or without. A texture is read through the sampler bound
		 * to its unit, so the filter of all textures is changed by
		 * changing a few samplers (SetTextureFilter()).
		 */
		GLuint GetSampler(bool abMipMaps, GLenum aWrapS, GLenum aWrapT, GLenum aWrapR);
		/// Binds a sampler object to a texture unit, 0 for none: the
		/// texture's own parameters are used then.
		void BindSampler(unsigned int alUnit, GLuint alSampler);
		/// Called by a texture whose sampler is another one now.
		void TextureSamplerChanged(iTexture* apTex);
		void SetActiveTextureUnit(unsigned int alUnit) override;
		void SetTextureEnv(eTextureParam aParam, int alVal) override;
		void SetTextureConstantColor(const cColor &aColor) override;

		void SetColor(const cColor &aColor) override;

		// POLYGONS
		iVertexBuffer* CreateVertexBuffer(tVertexFlag aFlags, eVertexBufferDrawType aDrawType,
										eVertexBufferUsageType aUsageType,
										int alReserveVtxSize=0,int alReserveIdxSize=0) override;

		void DrawRect(const cVector2f &avPos,const cVector2f &avSize,float afZ) override;

		void DrawTri(const tVertexVec &avVtx) override;
		void DrawTri(const cVertex* avVtx) override;

		void DrawQuad(const tVertexVec &avVtx) override;
		void DrawQuad(const tVertexVec &avVtx, const cColor aCol) override;
		void DrawQuad(const tVertexVec &avVtx,const float afZ) override;
		void DrawQuad(const tVertexVec &avVtx,const float afZ,const cColor &aCol) override;
		void DrawQuadMultiTex(const tVertexVec &avVtx,const tVector3fVec &avExtraUvs) override;

		void AddVertexToBatch(const cVertex *apVtx) override;
		void AddVertexToBatch(const cVertex *apVtx, const cVector3f* avTransform) override;
		void AddVertexToBatch(const cVertex *apVtx, const cMatrixf* aMtx) override;

		void AddVertexToBatch_Size2D(const cVertex *apVtx, const cVector3f* avTransform,
										const cColor* apCol,const float& mfW, const float& mfH) override;

		void AddVertexToBatch_Raw(	const cVector3f& avPos, const cColor &aColor,
									const cVector3f& avTex) override;


		void AddTexCoordToBatch(unsigned int alUnit,const cVector3f *apCoord) override;
		void SetBatchTextureUnitActive(unsigned int alUnit,bool abActive) override;

		void AddIndexToBatch(int alIndex) override;

		void FlushTriBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear=true) override;
		void FlushQuadBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear=true) override;
		void ClearBatch() override;

		//PRIMITIVES
		void DrawLine(const cVector3f& avBegin, const cVector3f& avEnd, cColor aCol) override;
		void DrawBoxMaxMin(const cVector3f& avMax, const cVector3f& avMin, cColor aCol) override;
		void DrawSphere(const cVector3f& avPos, float afRadius, cColor aCol) override;

		void DrawLine2D(const cVector2f& avBegin, const cVector2f& avEnd, float afZ, cColor aCol) override;
		void DrawLineRect2D(const cRect2f& aRect, float afZ, cColor aCol) override;
		void DrawLineCircle2D(const cVector2f& avCenter, float afRadius, float afZ, cColor aCol) override;

		void DrawFilledRect2D(const cRect2f& aRect, float afZ, cColor aCol) override;

		//FRAMEBUFFER
		void CopyContextToTexure(iTexture* apTex, const cVector2l &avPos,
			const cVector2l &avSize, const cVector2l &avTexOffset=0) override;
		void SetRenderTarget(iTexture* pTex) override;
		void SetSceneColorOnly(bool abX) override;
		void BeginShadowMap(bool abCube, int alFace, int alSize) override;
		void EndShadowMap() override;
		void SetShadowMapParams(const cShadowMapParams& aParams) override;
		void SetLightLook(const cLightLook& aLook) override;

		void FlushRendering() override;
		void SwapBuffers() override;

		///// SDL Specific ////////////////////////////

		iBitmap2D* CreateBitmap2DFromSurface(SDL_Surface* apSurface,const tString& asType);

		SDL_Window* GetWindow(){ return mpWindow;}

		void SetupGL();

		GLenum GetGLTextureTargetEnum(eTextureTarget aType);

	private:
		cVector2l mvScreenSize;
		cVector2f mvVirtualSize;
		int mlMultisampling;
		int mlBpp;
		int mlDisplay;

		//A shader pass that draws a texture over the whole viewport
		struct cFullscreenPass
		{
			tString msName;
			bool mbFailed = false;
			GLuint mlPipeline = 0;
			GLuint mlVertex = 0;
			GLuint mlFragment = 0;
			//Defines the fragment shader is compiled with, separated by spaces
			tString msDefines;
			cFullscreenPass(){}
			cFullscreenPass(const tString &asName, const tString &asDefines="") : msName(asName), msDefines(asDefines){}
		};
		bool CreateFullscreenPass(cFullscreenPass &aPass);
		void DeleteFullscreenPass(cFullscreenPass &aPass);
		void DrawFullscreenPass(cFullscreenPass &aPass, GLuint alTexture, const cVector2l &avSize,
								GLuint alTexture1=0, GLuint alTexture2=0, bool abStencil=false);
		void SetPassUniform(cFullscreenPass &aPass, const char *asName, float afX, float afY=0, float afZ=0, float afW=0);

		//Gamma
		float mfGammaCorrection;
		cFullscreenPass mGammaPass;

		//Scene to output: clamps, scales and dithers
		cFullscreenPass mOutputPass;
		//The same without scaling, grain and dithering: the picture SMAA works on
		cFullscreenPass mEncodePass;

		//SMAA: the scene's edges, how much the pixels on them are blended,
		//and the blend. Its textures are of the render size and made when
		//it is first used; the stencil buffer marks the pixels with edges.
		cSmaaSettings mSmaa;
		cFullscreenPass mSmaaEdgesPass;
		cFullscreenPass mSmaaWeightsPass;
		cFullscreenPass mSmaaWeightsNoDiagPass;
		cFullscreenPass mSmaaBlendPass;
		GLuint mlSmaaColor;
		GLuint mlSmaaEdges;
		GLuint mlSmaaWeights;
		GLuint mlSmaaStencil;
		GLuint mvSmaaFramebuffers[3];
		GLuint mlSmaaAreaTexture;
		GLuint mlSmaaSearchTexture;
		bool CreateSmaaTextures();
		void DeleteSmaaTextures();
		bool DrawSmaa(GLuint alColor, GLuint alColorFramebuffer);

		//Frame order: the 3D scene is drawn into the scene framebuffer at
		//the render size (between BeginScene() and EndScene()), EndScene()
		//copies it to the output framebuffer at the window size, the 2D
		//drawing goes there, and SwapBuffers() copies that to the window
		//through the gamma pass. With multisampling, the scene colour and
		//depth are multisampled until ResolveScene(), which copies them to
		//the textures of mlResolveFramebuffer, where the rest is drawn.
		cVector2l mvRenderSize;
		float mfRenderScale;
		cVector2l mvTargetSize;
		bool mbInScene;
		bool mbSceneResolved;
		bool mbInShadowMap;
		GLuint mlSceneFramebuffer;
		//The scene's colour without its depth and stencil, and if that is
		//what the scene is drawn into (SetSceneColorOnly())
		GLuint mlSceneColorFramebuffer;
		bool mbSceneColorOnly;
		GLuint GetSceneFramebuffer()
		{
			if(mlMultisampling > 0 && mbSceneResolved)
				return mbSceneColorOnly ? mlResolveColorFramebuffer : mlResolveFramebuffer;
			return mbSceneColorOnly ? mlSceneColorFramebuffer : mlSceneFramebuffer;
		}
		GLuint mlSceneColor;
		GLuint mlSceneDepthStencil;
		GLuint mlResolveFramebuffer;
		GLuint mlResolveColorFramebuffer;
		GLuint mlResolveColor;
		GLuint mlResolveDepthStencil;
		void BlitScene(GLbitfield aMask, int alX0, int alY0, int alX1, int alY1);
		void BindCurrentFramebuffer();
		GLuint mlOutputFramebuffer;
		GLuint mlOutputColor;
		GLuint mlOutputDepthStencil;
		//Has the texture given to SetRenderTarget() attached.
		GLuint mlTargetFramebuffer;

		//Shadow maps: a 2D depth texture for spotlights ([0]) and a cube
		//map for point lights ([1]). Both stay bound to two texture units
		//past the engine's, kShadowMapUnit with a sampler that compares
		//and the next with one that gives the depth.
		GLuint mlShadowFramebuffer;
		GLuint mvShadowMaps[2];
		int mvShadowMapSizes[2];
		GLuint mvShadowSamplers[2];
		cShadowMapParams mShadowParams;
		cLightLook mLightLook;

		// hpl_Frame uniform block, bound to uniform buffer binding 0
		GLuint mlFrameUniformBuffer;

		cGpuProfiler mGpuProfiler;
		cDebugOverlay mDebugOverlay;

		//Clipping
		cPlanef mvClipPlanes[kMaxClipPlanes];

		//Drawing without engine programs
		cFixedFunctionShaders mFixedFunction;
		bool mbMatricesChanged;
		unsigned int mlActiveTextureUnit;

		//Alpha test, done by the fragment shaders: hpl_State, uniform
		//buffer binding cGLSLProgram::kStateBlockBinding. In the
		//multisampled scene the test decides for each sample (alpha to
		//coverage).
		bool mbAlphaTest;
		bool mbAlphaToCoverage;
		eAlphaTestFunc mAlphaTestFunc;
		float mfAlphaTestRef;
		bool mbStateChanged;
		GLuint mlStateUniformBuffer;
		static const int kStateUniformFloats = 36;

		//Linear lighting: also in hpl_State
		bool mbLinearLighting;
		bool mbLinearSource;
		cLinearLightingTuning mLinearTuning;
		//In hpl_Frame
		cShadingSettings mShading;
		cFilmGrainSettings mFilmGrain;
		//Seconds of rendering, as last given to SetFrameUniforms()
		float mfFrameTime;
		bool mbBlend;
		eBlendFunc mBlendSrc;
		eBlendFunc mBlendDest;

		//SDL Variables
		SDL_Window *mpWindow;
		SDL_GLContext mGLContext;
		cSDLPixelFormat *mpPixelFormat;

		//Vertices of draws without a vertex buffer (DrawQuad and the like,
		//the batch) are copied to the stream buffer and drawn from there
		//with mlStreamVertexArray. The fullscreen passes have no vertex data.
		GLuint mlBoundVertexArray;
		GLuint mlStreamVertexArray;
		GLuint mlEmptyVertexArray;
		GLuint mlStreamVertexBuffer;
		GLuint mlStreamIndexBuffer;
		size_t mlStreamVertexSize;
		size_t mlStreamVertexOffset;
		size_t mlStreamIndexSize;
		size_t mlStreamIndexOffset;
		//Bit per attribute whose array is enabled in mlStreamVertexArray
		unsigned int mlStreamArrays;

		void StreamArray(unsigned int &alArrays, int alAttrib, int alElements, size_t alStride, size_t alOffset);
		void SetStreamArrays(unsigned int alArrays);

		//One draw's vertices: position, colour and mlImmediateTexUnits
		//texture coordinates of 3 floats each
		std::vector<float> mvImmediate;
		int mlImmediateTexUnits;
		void ImmediateBegin(int alTexUnits);
		void ImmediateVertex(const cVector3f &avPos, const cColor &aCol, const cVector3f *apTex=nullptr);
		void ImmediateDraw(GLenum aMode);

		//The values of attributes without an array, as set by SetColor()
		//and left by the last vertex of an immediate draw (what glColor and
		//glTexCoord did)
		float mvCurrentAttrib[cGLSLProgram::eVertexAttrib_LastEnum][4];
		unsigned int mlCurrentAttribChanged;
		void SetCurrentAttrib(int alAttrib, float afX, float afY, float afZ, float afW);

		std::vector<unsigned int> mvQuadIndices;
		void DrawBatch(tVtxBatchFlag aTypeFlags, bool abQuads);

		//The batch:
		float* mpVertexArray;
		unsigned int mlVertexCount;
		unsigned int* mpIndexArray;
		unsigned int mlIndexCount;

		unsigned int mlBatchStride;

		float *mpTexCoordArray[MAX_TEXTUREUNITS];
		bool mbTexCoordArrayActive[MAX_TEXTUREUNITS];
		unsigned int mlTexCoordArrayCount[MAX_TEXTUREUNITS];

		unsigned int mlBatchArraySize;

		//Clearing
		bool mbClearColor;
		bool mbClearDepth;
		bool mbClearStencil;

		//Rendertarget variables
		iTexture* mpRenderTarget;

		//Texture
		iTexture* mpCurrentTexture[MAX_TEXTUREUNITS];

		//Samplers, by mipmaps and the three wrap modes
		std::map<std::tuple<bool,GLenum,GLenum,GLenum>, GLuint> m_mapSamplers;
		GLuint mvUnitSampler[MAX_TEXTUREUNITS];
		eTextureFilter mTextureFilter;
		float mfTextureAnisotropy;
		void SetSamplerFilter(GLuint alSampler, bool abMipMaps);

		SDL_DisplayID ChooseDisplay(int alDisplay);
		SDL_DisplayID GetDisplayID(int alDisplay);
		void DebugDumpFrame();
		bool SaveFrameToPNG(const tString& asFile);
		tString msSaveNextFrame;

		bool CreateSceneFramebuffers();
		bool CreateOutputFramebuffer();
		void DeleteSceneFramebuffers();
		void DeleteFramebuffers();
		void BindFramebuffer(GLuint alFramebuffer, const cVector2l& avSize);
		void CreateShadowMap(bool abCube, int alSize);
		void PresentFrame();

		bool DrawGammaPass();


		//Depth helper
		GLenum GetGLDepthTestFuncEnum(eDepthTestFunc aType);


		//Stencil helper
		GLenum GetGLStencilFuncEnum(eStencilFunc aType);
		GLenum GetGLStencilOpEnum(eStencilOp aType);

		//Matrix Helper
		// Notes that the top of the CPU stack changed, for the
		// fixed-function shaders.
		void LoadMatrix(eMatrix aMtxType);
		// Top is the current matrix; never empty.
		std::vector<cMatrixf> mvMatrixStack[eMatrix_LastEnum];

		//Blend helper
		GLenum GetGLBlendEnum(eBlendFunc aType);
	};
};
#endif // HPL_LOWLEVELGRAPHICS_SDL_H
