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

#include <glad/gl.h>

#include <SDL3/SDL.h>

#include "graphics/LowLevelGraphics.h"
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

		void SetMultisamplingActive(bool abX) override;

		void SetGammaCorrection(float afX) override;
		float GetGammaCorrection() override;

		int GetMultisampling() override{ return mlMultisampling;}

		void SetClipPlane(int alIdx, const cPlanef& aPlane) override;
		cPlanef GetClipPlane(int alIdx, const cPlanef& aPlane) override;
		void SetClipPlaneActive(int alIdx, bool abX) override;

		cVector2f GetScreenSize() override;
		cVector2f GetRenderSize() override;
		void SetRenderScale(float afScale) override;
		float GetRenderScale() override;
		void BeginScene() override;
		void EndScene() override;
		cVector2f GetVirtualSize() override;
		void SetVirtualSize(cVector2f avSize) override;
		cVector2f GetVirtualOffset() override;

		iBitmap2D* CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp) override;
		iFontData* CreateFontData(const tString &asName) override;

		iTexture* CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture* CreateTexture(const tString &asName,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
		iTexture* CreateTexture(iBitmap2D* apBmp,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget) override;
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
		void RotateMatrix(eMatrix aMtxType, const cVector3f &avRot) override;
		void ScaleMatrix(eMatrix aMtxType, const cVector3f &avScale) override;

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
		};
		bool CreateFullscreenPass(cFullscreenPass &aPass);
		void DeleteFullscreenPass(cFullscreenPass &aPass);
		void DrawFullscreenPass(cFullscreenPass &aPass, GLuint alTexture, const cVector2l &avSize);

		//Gamma
		float mfGammaCorrection;
		cFullscreenPass mGammaPass;

		//Scene to output: clamps, scales and dithers
		cFullscreenPass mOutputPass;

		//Frame order: the 3D scene is drawn into the scene framebuffer at
		//the render size (between BeginScene() and EndScene()), EndScene()
		//copies it to the output framebuffer at the window size, the 2D
		//drawing goes there, and SwapBuffers() copies that to the window
		//through the gamma pass. With multisampling, the scene colour and
		//depth are multisampled and mlResolveFramebuffer holds the resolved
		//colour.
		cVector2l mvRenderSize;
		float mfRenderScale;
		cVector2l mvTargetSize;
		bool mbInScene;
		GLuint mlSceneFramebuffer;
		GLuint mlSceneColor;
		GLuint mlSceneDepthStencil;
		GLuint mlResolveFramebuffer;
		GLuint mlResolveColor;
		GLuint mlOutputFramebuffer;
		GLuint mlOutputColor;
		GLuint mlOutputDepthStencil;
		//Has the texture given to SetRenderTarget() attached.
		GLuint mlTargetFramebuffer;

		//Clipping
		cPlanef mvClipPlanes[kMaxClipPlanes];

		//SDL Variables
		SDL_Window *mpWindow;
		SDL_GLContext mGLContext;
		cSDLPixelFormat *mpPixelFormat;

		//Vertex Array variables
		//The vertex arrays used:
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

		//Multisample
		void CheckMultisampleCaps();

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
		void PresentFrame();

		bool DrawGammaPass();


		//Batch helper
		void SetUpBatchArrays();

		//Depth helper
		GLenum GetGLDepthTestFuncEnum(eDepthTestFunc aType);

		//Alpha Helper
		GLenum GetGLAlphaTestFuncEnum(eAlphaTestFunc aType);

		//Stencil helper
		GLenum GetGLStencilFuncEnum(eStencilFunc aType);
		GLenum GetGLStencilOpEnum(eStencilOp aType);

		//Matrix Helper
		void SetMatrixMode(eMatrix mType);

		//Texture helper
		GLenum GetGLTextureParamEnum(eTextureParam aType);
		GLenum GetGLTextureOpEnum(eTextureOp aType);
		GLenum GetGLTextureFuncEnum(eTextureFunc aType);
		GLenum GetGLTextureSourceEnum(eTextureSource aType);

		//Blend helper
		GLenum GetGLBlendEnum(eBlendFunc aType);

		//Vtx helper
		void SetVtxBatchStates(tVtxBatchFlag aFlags);
	};
};
#endif // HPL_LOWLEVELGRAPHICS_SDL_H
