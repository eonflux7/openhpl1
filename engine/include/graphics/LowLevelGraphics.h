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
#ifndef HPL_LOWLEVELGRAPHICS_H
#define HPL_LOWLEVELGRAPHICS_H

#include <functional>

#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/Bitmap2D.h"
#include "graphics/Texture.h"
#include "graphics/GPUProgram.h"
#include "graphics/VertexBuffer.h"

namespace hpl {

	enum eBlendFunc
	{
		eBlendFunc_Zero,
		eBlendFunc_One,
		eBlendFunc_SrcColor,
		eBlendFunc_OneMinusSrcColor,
		eBlendFunc_DestColor,
		eBlendFunc_OneMinusDestColor,
		eBlendFunc_SrcAlpha,
		eBlendFunc_OneMinusSrcAlpha,
		eBlendFunc_DestAlpha,
		eBlendFunc_OneMinusDestAlpha,
		eBlendFunc_SrcAlphaSaturate,
		eBlendFunc_LastEnum
	};

	enum eTextureParam
	{
		eTextureParam_ColorFunc,
		eTextureParam_AlphaFunc,
		eTextureParam_ColorSource0,
		eTextureParam_ColorSource1,
		eTextureParam_ColorSource2,
		eTextureParam_AlphaSource0,
		eTextureParam_AlphaSource1,
		eTextureParam_AlphaSource2,
		eTextureParam_ColorOp0,
		eTextureParam_ColorOp1,
		eTextureParam_ColorOp2,
		eTextureParam_AlphaOp0,
		eTextureParam_AlphaOp1,
		eTextureParam_AlphaOp2,
		eTextureParam_ColorScale,
		eTextureParam_AlphaScale,
		eTextureParam_LastEnum
	};

	enum eTextureOp
	{
		eTextureOp_Color,
		eTextureOp_OneMinusColor,
		eTextureOp_Alpha,
		eTextureOp_OneMinusAlpha,
		eTextureOp_LasEnum
	};

	enum eTextureSource
	{
		eTextureSource_Texture,
		eTextureSource_Constant,
		eTextureSource_Primary,
		eTextureSource_Previous,
		eTextureSource_LastEnum
	};

	enum eTextureFunc
	{
		eTextureFunc_Modulate,
		eTextureFunc_Replace,
		eTextureFunc_Add,
		eTextureFunc_Substract,
		eTextureFunc_AddSigned,
		eTextureFunc_Interpolate,
		eTextureFunc_Dot3RGB,
		eTextureFunc_Dot3RGBA,
		eTextureFunc_LastEnum
	};

	enum eStencilFunc
	{
		eStencilFunc_Never,
		eStencilFunc_Less,
		eStencilFunc_LessOrEqual,
		eStencilFunc_Greater,
		eStencilFunc_GreaterOrEqual,
		eStencilFunc_Equal,
		eStencilFunc_NotEqual,
		eStencilFunc_Always,
		eStencilFunc_LastEnum
	};

	enum eStencilFace
	{
		eStencilFace_Front,
		eStencilFace_Back,
		eStencilFace_LastEnum,
	};

	enum eDepthTestFunc
	{
		eDepthTestFunc_Never,
		eDepthTestFunc_Less,
		eDepthTestFunc_LessOrEqual,
		eDepthTestFunc_Greater,
		eDepthTestFunc_GreaterOrEqual,
		eDepthTestFunc_Equal,
		eDepthTestFunc_NotEqual,
		eDepthTestFunc_Always,
		eDepthTestFunc_LastEnum
	};

	enum eAlphaTestFunc
	{
		eAlphaTestFunc_Never,
		eAlphaTestFunc_Less,
		eAlphaTestFunc_LessOrEqual,
		eAlphaTestFunc_Greater,
		eAlphaTestFunc_GreaterOrEqual,
		eAlphaTestFunc_Equal,
		eAlphaTestFunc_NotEqual,
		eAlphaTestFunc_Always,
		eAlphaTestFunc_LastEnum
	};

	enum eStencilOp
	{
		eStencilOp_Keep,
		eStencilOp_Zero,
		eStencilOp_Replace,
		eStencilOp_Increment,
		eStencilOp_Decrement,
		eStencilOp_Invert,
		eStencilOp_IncrementWrap,
		eStencilOp_DecrementWrap,
		eStencilOp_LastEnum
	};

	enum eCullMode
	{
		eCullMode_Clockwise,
		eCullMode_CounterClockwise,
		eCullMode_LastEnum
	};

	enum eGraphicCaps
	{
		eGraphicCaps_MaxTextureImageUnits,

		eGraphicCaps_AnisotropicFiltering,
		eGraphicCaps_MaxAnisotropicFiltering,

		eGraphicCaps_LastEnum
	};

	typedef tFlag tVtxBatchFlag;

	#define eVtxBatchFlag_Normal		(0x00000001)
	#define eVtxBatchFlag_Position		(0x00000002)
	#define eVtxBatchFlag_Color0		(0x00000004)
	#define eVtxBatchFlag_Texture0		(0x00000008)
	#define eVtxBatchFlag_Texture1		(0x00000010)
	#define eVtxBatchFlag_Texture2		(0x00000020)

	#define kMaxClipPlanes (6)

	class iFontData;
	class iOcclusionQuery;

	/**
	 * The values linear lighting is tuned with. The defaults were chosen by
	 * playing; 2.2, 1, 1, a blend match of 0 and a shoulder of 1 leave
	 * everything as the conversion gives it.
	 */
	class cLinearLightingTuning
	{
	public:
		/**
		 * The power the amount of light (N.L and the falloff ramp, which
		 * were made to look right on screen values) is raised to. 2.2 gives
		 * one light the look it has without linear lighting; 1.0 takes them
		 * as amounts of light, which is wider and flatter.
		 */
		float mfLightExp = 1.8f;
		/// Scales the light of the finished scene before it is converted for the screen.
		float mfExposure = 1.3f;
		/**
		 * Scales the light of what is added to the scene without being lit:
		 * halos, illumination maps, additive materials and particles. The
		 * full scale is for what is white; darker parts get less of it,
		 * so faint edges stay faint.
		 */
		float mfGlow = 1.45f;
		/**
		 * How far the alpha of what is blended over the scene (particles,
		 * alpha materials, fog) is changed to cover as much as it does
		 * without linear lighting: 0 leaves it, 1 matches it.
		 */
		float mfBlendMatch = 1.0f;
		/**
		 * Tonemapping: the screen value (0 to 1) above which bright values
		 * are rolled off towards white instead of clipping at it. Below it
		 * the picture is untouched. 1 is no tonemapping.
		 */
		float mfShoulder = 1.0f;
		/// Tonemapping: the amount of light, in times white, that becomes white on screen.
		float mfWhite = 4.0f;
	};

	/**
	 * What the light shaders need to read the shadow map of the light that
	 * is drawn. Lengths are in world units (metres); a blur is given as its
	 * radius at a distance of 1 from the light, and grows with the distance.
	 */
	class cShadowMapParams
	{
	public:
		/**
		 * 0: the light has no shadow map. 1: hard, one filtered sample.
		 * 2: blurred by mfSoftness. 3: blurred by how far what casts the
		 * shadow is from what it falls on, for a light of mfLightSize.
		 */
		int mlFilter = 0;
		/// The depth range the map was drawn with.
		float mfNear = 0.05f;
		float mfFar = 1;
		/// The width of one texel at a distance of 1 from the light.
		float mfTexelSize = 0;
		/// Spotlights: texture coordinates per unit of x/z and y/z in the light's view.
		cVector2f mvUvScale = cVector2f(1,1);

		float mfSoftness = 0.02f;
		/// Radius of the light source.
		float mfLightSize = 0.05f;
		/// The widest blur.
		float mfMaxBlur = 0.03f;
		/// Samples of the blur, and of the search for what casts the shadow (filter 3).
		int mlSamples = 16;
		int mlBlockerSamples = 12;
		/// The samples are turned by a different angle in every pixel.
		bool mbNoise = true;

		/// What a surface is moved towards the light by before it is compared.
		float mfBias = 0.01f;
		/// The same, in texels of the map, for a surface at 45 degrees to the light.
		float mfSlopeBias = 1.5f;

		/// The light passes draw how much of the light arrives instead of the lit surface.
		bool mbShowOnly = false;
	};

	class iLowLevelGraphics
	{
	public:
		virtual ~iLowLevelGraphics(){}
		/**
		 * Sets the video mode. Must only be called ONCE!
		 * \param alWidth
		 * \param alHeight
		 * \param alBpp
		 * \param abFullscreen
		 * \param alMultiSampling The amount of multisamplimg, 0 = off.
		 * \param alDisplay The display to open on, see GetDisplayNum(). -1 = pick one.
		 * \return
		 */
		virtual bool Init(int alWidth, int alHeight, int alBpp, int abFullscreen, int alMultisampling,
							const tString& asWindowCaption, int alDisplay)=0;

		/**
		 * The displays (monitors) that are connected. Can be used before Init().
		 */
		virtual int GetDisplayNum()=0;
		virtual tString GetDisplayName(int alDisplay)=0;
		/**
		 * The size of the desktop on a display.
		 */
		virtual cVector2l GetDisplaySize(int alDisplay)=0;
		/**
		 * The fullscreen sizes of a display, smallest first.
		 */
		virtual void GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes)=0;
		/**
		 * The display the window was opened on.
		 */
		virtual int GetDisplay()=0;


		/**
		 * Get the capabilities of the graphics. Th return value depends on the capability
		 * \param aType
		 * \return
		 */
		virtual int GetCaps(eGraphicCaps aType)=0;
		/**
		 * Show the cursor or not. Default is false
		 * \param aX
		 */
		virtual void ShowCursor(bool abX)=0;

		virtual int GetMultisampling()=0;
		/**
		 * Get Size of screen
		 * \return
		 */
		/**
		 * Size of the window in pixels, which the 2D drawing covers.
		 */
		virtual cVector2f GetScreenSize()=0;
		/**
		 * Size in pixels the 3D scene is drawn at, between BeginScene() and
		 * EndScene(). Post effects and screen copies work at this size.
		 */
		virtual cVector2f GetRenderSize()=0;
		/**
		 * Sets the render size to the window size times afScale (0.25 to
		 * 2). Not while the scene is drawn.
		 */
		virtual void SetRenderScale(float afScale)=0;
		virtual float GetRenderScale()=0;
		/**
		 * Linear lighting: the 3D scene is added up and blended on values
		 * proportional to light, and converted for the screen when it is
		 * finished. Off, it is done on the screen values, as originally.
		 */
		virtual void SetLinearLighting(bool abX)=0;
		virtual bool GetLinearLighting()=0;
		/**
		 * Set while drawing something into the scene that holds linear
		 * values already (a copy of the scene), so that it is not converted
		 * again. Only matters with linear lighting.
		 */
		virtual void SetLinearSource(bool abX)=0;
		/**
		 * How linear lighting looks; see cLinearLightingTuning.
		 */
		virtual void SetLinearLightingTuning(const cLinearLightingTuning& aTuning)=0;
		virtual const cLinearLightingTuning& GetLinearLightingTuning()=0;
		/**
		 * Starts drawing the 3D scene.
		 */
		virtual void BeginScene()=0;
		/**
		 * Ends drawing the 3D scene: it is scaled to the window, and the
		 * drawing that follows (HUD, menus) goes on top of it.
		 */
		virtual void EndScene()=0;
		virtual cVector2f GetVirtualSize()=0;
		/**
		 * Sets the virtual screen size. Default is 0-1
		 * \param avSize
		 */
		virtual void SetVirtualSize(cVector2f avSize)=0;
		/**
		 * How far the screen reaches past the virtual size on each side, in
		 * virtual units. The virtual canvas keeps its aspect ratio and is
		 * centred, so this is non-zero on one axis when the window has
		 * another aspect ratio: the screen spans -offset to size+offset.
		 */
		virtual cVector2f GetVirtualOffset()=0;

		/**
		 * Set if vsync should eb active
		 * \param abX
		 */
		virtual void SetVsyncActive(bool abX)=0;

		virtual void SetMultisamplingActive(bool abX)=0;

		virtual void SetGammaCorrection(float afX)=0;
		virtual float GetGammaCorrection()=0;

		virtual void SetClipPlane(int alIdx, const cPlanef& aPlane)=0;
		virtual cPlanef GetClipPlane(int alIdx, const cPlanef& aPlane)=0;
		virtual void SetClipPlaneActive(int alIdx, bool abX)=0;

		virtual iBitmap2D* CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp)=0;
		virtual iFontData* CreateFontData(const tString &asName)=0;

		virtual iTexture* CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)=0;
		virtual iTexture* CreateTexture(const tString &asName,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)=0;
		virtual iTexture* CreateTexture(iBitmap2D* apBmp,bool abUseMipMaps, eTextureType aType,
										eTextureTarget aTarget)=0;
		virtual iTexture* CreateTexture(const cVector2l& avSize,int alBpp,cColor aFillCol,
									bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)=0;


		virtual iPixelFormat* GetPixelFormat()=0;

		virtual iGpuProgram* CreateGpuProgram(const tString& asName, eGpuProgramType aType)=0;

		//TODO: Kinda quick and diry, better to have a screen to Bitmap.
		// and then a save as in the Bitmap.
		virtual void SaveScreenToBMP(const tString& asFile)=0;
		/**
		 * Saves the next frame as a PNG, as it is when it is shown: after
		 * everything has been drawn and just before the buffers are swapped.
		 */
		virtual void SaveNextFrameToPNG(const tString& asFile)=0;

		/////////// MATRIX METHODS //////////////////////////

		virtual void PushMatrix(eMatrix aMtxType)=0;
		virtual void PopMatrix(eMatrix aMtxType)=0;
		virtual void SetIdentityMatrix(eMatrix aMtxType)=0;

		virtual void SetMatrix(eMatrix aMtxType, const cMatrixf& a_mtxA)=0;

		virtual void TranslateMatrix(eMatrix aMtxType, const cVector3f &avPos)=0;
		virtual void ScaleMatrix(eMatrix aMtxType, const cVector3f &avScale)=0;

		/// The current matrix, kept on the CPU (no GL read back).
		virtual const cMatrixf& GetMatrix(eMatrix aMtxType)=0;

		/**
		 * Per-frame values for shaders, the hpl_Frame block in
		 * hpl1_frame.glsl; the render size is added. Set when the 3D scene
		 * starts. afTime is in seconds.
		 */
		virtual void SetFrameUniforms(const cMatrixf& a_mtxView, const cMatrixf& a_mtxProjection,
									const cVector3f& avEyePos, float afNear, float afFar,
									float afTime)=0;

		/////////// DEBUGGING //////////////////////////

		/**
		 * Starts a named section of the frame: a GL debug group (RenderDoc
		 * shows them), timed on the GPU while the debug overlay is shown.
		 * Sections nest; EndGpuSection() ends the last one begun. The name
		 * must stay valid until the frame is shown (use a string literal).
		 */
		virtual void BeginGpuSection(const char *asName)=0;
		virtual void EndGpuSection()=0;

		/// The overlay with frame times, GPU sections and draw counts.
		virtual void SetDebugOverlayActive(bool abX)=0;
		virtual bool GetDebugOverlayActive()=0;

		/**
		 * A function that draws Dear ImGui windows of the game's own, called
		 * every frame whether the overlay is shown or not; an empty function
		 * removes it.
		 */
		virtual void SetDebugPanel(std::function<void()> aDraw)=0;
		/// Gives ImGui the mouse and the keyboard, and frees the pointer.
		virtual void SetDebugPanelInput(bool abX)=0;

		virtual void SetOrthoProjection(const cVector2f& avSize, float afMin, float afMax)=0;
		/**
		 * Ortho projection of the whole screen in virtual coordinates,
		 * -GetVirtualOffset() to GetVirtualSize()+GetVirtualOffset().
		 */
		virtual void SetVirtualOrthoProjection(float afMin, float afMax)=0;

		/////////// DRAWING METHODS /////////////////////////

		//OCCLUSION
		virtual iOcclusionQuery* CreateOcclusionQuery()=0;
		virtual void DestroyOcclusionQuery(iOcclusionQuery *apQuery)=0;

		// CLEARING THE FRAMEBUFFER
		virtual void ClearScreen()=0;

		virtual void SetClearColor(const cColor& aCol)=0;
		virtual void SetClearDepth(float afDepth)=0;
		virtual void SetClearStencil(int alVal)=0;

		virtual void SetClearColorActive(bool abX)=0;
		virtual void SetClearDepthActive(bool abX)=0;
		virtual void SetClearStencilActive(bool abX)=0;

		virtual void SetColorWriteActive(bool abR,bool abG,bool abB,bool abA)=0;
		virtual void SetDepthWriteActive(bool abX)=0;

		virtual void SetCullActive(bool abX)=0;
		virtual void SetCullMode(eCullMode aMode)=0;

		//DEPTH
		virtual void SetDepthTestActive(bool abX)=0;
		virtual void SetDepthTestFunc(eDepthTestFunc aFunc)=0;

		//ALPHA
		virtual void SetAlphaTestActive(bool abX)=0;
		virtual void SetAlphaTestFunc(eAlphaTestFunc aFunc,float afRef)=0;

		//STENCIL
		virtual void SetStencilActive(bool abX)=0;
		/*virtual void SetStencilTwoSideActive(bool abX)=0;
		virtual void SetStencilFace(eStencilFace aFace)=0;
		virtual void SetStencilFunc(eStencilFunc aFunc,int alRef, unsigned int aMask)=0;
		virtual void SetStencilOp(eStencilOp aFailOp,eStencilOp aZFailOp,eStencilOp aZPassOp)=0;*/
		virtual void SetStencil(eStencilFunc aFunc,int alRef, unsigned int aMask,
						eStencilOp aFailOp,eStencilOp aZFailOp,eStencilOp aZPassOp)=0;
		virtual void SetStencilTwoSide(eStencilFunc aFrontFunc,eStencilFunc aBackFunc,
						int alRef, unsigned int aMask,
						eStencilOp aFrontFailOp,eStencilOp aFrontZFailOp,eStencilOp aFrontZPassOp,
						eStencilOp aBackFailOp,eStencilOp aBackZFailOp,eStencilOp aBackZPassOp)=0;
		virtual void SetStencilTwoSide(bool abX)=0;


		//SCISSOR
		virtual void SetScissorActive(bool abX)=0;
		virtual void SetScissorRect(const cRect2l &aRect)=0;

		//TEXTURE
		virtual void SetTexture(unsigned int alUnit,iTexture* apTex)=0;
		virtual void SetActiveTextureUnit(unsigned int alUnit)=0;
		virtual void SetTextureEnv(eTextureParam aParam, int alVal)=0;
		virtual void SetTextureConstantColor(const cColor &aColor)=0;

		//COLOR
		virtual void SetColor(const cColor &aColor)=0;

		//BLENDING
		virtual void SetBlendActive(bool abX)=0;
		virtual void SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor)=0;
		virtual void SetBlendFuncSeparate(eBlendFunc aSrcFactorColor, eBlendFunc aDestFactorColor,
										eBlendFunc aSrcFactorAlpha, eBlendFunc aDestFactorAlpha)=0;

		// POLYGONS
		virtual iVertexBuffer* CreateVertexBuffer(tVertexFlag aFlags, eVertexBufferDrawType aDrawType,
								eVertexBufferUsageType aUsageType,
								int alReserveVtxSize=0,int alReserveIdxSize=0)=0;

		virtual void DrawRect(const cVector2f &avPos,const cVector2f &avSize,float afZ)=0;

		virtual void DrawTri(const tVertexVec &avVtx)=0;
		virtual void DrawTri(const cVertex* avVtx)=0;

		virtual void DrawQuad(const tVertexVec &avVtx)=0;
		virtual void DrawQuad(const tVertexVec &avVtx, const cColor aCol)=0;
		virtual void DrawQuad(const tVertexVec &avVtx,const float afZ)=0;
		virtual void DrawQuad(const tVertexVec &avVtx,const float afZ,const cColor &aCol)=0;
		virtual void DrawQuadMultiTex(const tVertexVec &avVtx,const tVector3fVec &avExtraUvs)=0;

		//VERTEX BATCHER
		virtual void AddVertexToBatch(const cVertex *apVtx)=0;
		virtual void AddVertexToBatch(const cVertex *apVtx, const cVector3f* avTransform)=0;
		virtual void AddVertexToBatch(const cVertex *apVtx, const cMatrixf* aMtx)=0;

		virtual void AddVertexToBatch_Size2D(const cVertex *apVtx, const cVector3f* avTransform,
									const cColor* apCol,const float& mfW, const float& mfH)=0;

		virtual void AddVertexToBatch_Raw(	const cVector3f& avPos, const cColor &aColor,
											const cVector3f& avTex)=0;


		virtual void AddIndexToBatch(int alIndex)=0;

		virtual void AddTexCoordToBatch(unsigned int alUnit,const cVector3f *apCoord)=0;
		virtual void SetBatchTextureUnitActive(unsigned int alUnit,bool abActive)=0;

		//Add more ways to add Vertex to the batch?
		//Index array, vtxArray, etc perhaps?

		virtual void FlushTriBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear=true)=0;
		virtual void FlushQuadBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear=true)=0;
		virtual void ClearBatch()=0;

		//some primitive:
		virtual void DrawLine(const cVector3f& avBegin, const cVector3f& avEnd, cColor aCol)=0;
		virtual void DrawBoxMaxMin(const cVector3f& avMax, const cVector3f& avMin, cColor aCol)=0;
		virtual void DrawSphere(const cVector3f& avPos, float afRadius, cColor aCol)=0;


		virtual void DrawLine2D(const cVector2f& avBegin, const cVector2f& avEnd, float afZ, cColor aCol)=0;
		virtual void DrawLineRect2D(const cRect2f& aRect, float afZ, cColor aCol)=0;
		virtual void DrawLineCircle2D(const cVector2f& avCenter, float afRadius, float afZ, cColor aCol)=0;

		virtual void DrawFilledRect2D(const cRect2f& aRect, float afZ, cColor aCol)=0;
		// GENERAL
		/**
		 * All further drawing operations are rendered to this texture, which
		 * fills the viewport. There is no depth or stencil buffer.
		 * \param pTex Texture to render to. NULL = the screen (the scene
		 * framebuffer)
		 */
		virtual void SetRenderTarget(iTexture* pTex)=0;

		/**
		 * All further drawing goes to the depth of a shadow map, cleared:
		 * the one of spotlights, or a face (0 to 5: +x -x +y -y +z -z) of
		 * the cube map of point lights. There is one of each, alSize pixels
		 * a side, and the light shaders read them. Nearer than the near
		 * plane counts as on it.
		 */
		virtual void BeginShadowMap(bool abCube, int alFace, int alSize)=0;
		/**
		 * Back to the target before BeginShadowMap().
		 */
		virtual void EndShadowMap()=0;
		/**
		 * For the light passes drawn from now on.
		 */
		virtual void SetShadowMapParams(const cShadowMapParams& aParams)=0;

		/**
		 * Copies the current frame buffer to a texture.
		 * \param apTex The texture the framebuffer is copied to.
		 * \param &avPos The Screenpositon
		 * \param &avSize The size of the screen.
		 * \param &avTexOffset The position on the texture.
		 */
		virtual void CopyContextToTexure(iTexture* apTex, const cVector2l &avPos,
										const cVector2l &avSize, const cVector2l &avTexOffset=0)=0;
		virtual void FlushRendering()=0;
		virtual void SwapBuffers()=0;
	};

	/// Begins a GPU section and ends it at the end of the scope.
	class cGpuSection
	{
	public:
		cGpuSection(iLowLevelGraphics *apLowLevel, const char *asName) : mpLowLevel(apLowLevel)
		{
			mpLowLevel->BeginGpuSection(asName);
		}
		~cGpuSection(){ mpLowLevel->EndGpuSection(); }

	private:
		iLowLevelGraphics *mpLowLevel;
	};
};
#endif // HPL_LOWLEVELGRAPHICS_H
