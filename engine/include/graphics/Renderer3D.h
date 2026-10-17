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
#ifndef HPL_RENDERER3D_H
#define HPL_RENDERER3D_H

#include "graphics/GraphicsTypes.h"
#include "math/MathTypes.h"
#include "graphics/Material.h"
#include "math/Frustum.h"

namespace hpl {


#define eRendererDebugFlag_DrawTangents			(0x00000001)
#define eRendererDebugFlag_DrawNormals			(0x00000002)
#define eRendererDebugFlag_DisableLighting		(0x00000004)
#define eRendererDebugFlag_LogRendering			(0x00000008)
#define eRendererDebugFlag_DrawBoundingBox		(0x00000010)
#define eRendererDebugFlag_DrawBoundingSphere	(0x00000020)
#define eRendererDebugFlag_DrawLightBoundingBox	(0x00000040)
#define eRendererDebugFlag_RenderLines			(0x00000080)

	typedef tFlag tRendererDebugFlag;

	class cResources;
	class iLowLevelGraphics;
	class iLowLevelResources;
	class iTexture;
	class cCamera3D;
	class cWorld3D;
	class iVertexBuffer;
	class cMeshCreator;
	class cRenderList;
	class iRenderable;
	class iLight3D;
	class cRendererPostEffects;
	class cSector;
	struct cTransparentDraw;
	class iRenderState;

	class cResources;

	//---------------------------------------------

	enum eRendererShowShadows
	{
		eRendererShowShadows_All,
		eRendererShowShadows_Static,
		eRendererShowShadows_None,
		eRendererShowShadows_LastEnum
	};

	//---------------------------------------------

	enum eShadowMapFilter
	{
		/// One sample, filtered between texels.
		eShadowMapFilter_Hard,
		/// The same blur everywhere.
		eShadowMapFilter_Soft,
		/// Sharp where the shadow is cast from close by, wider with the distance.
		eShadowMapFilter_SoftByDistance,
		eShadowMapFilter_LastEnum
	};

	/**
	 * Which sides of what casts a shadow are drawn into the map.
	 */
	enum eShadowMapFaces
	{
		eShadowMapFaces_Both,
		/// As the shadow volumes do it: a wall with one side casts no shadow from behind.
		eShadowMapFaces_TowardsLight,
		eShadowMapFaces_AwayFromLight,
		eShadowMapFaces_LastEnum
	};

	/**
	 * How the shadow maps are made and read. Lengths are in metres; a blur
	 * is its radius at 1 metre from the light.
	 */
	class cShadowMapSettings
	{
	public:
		eShadowMapFilter mFilter = eShadowMapFilter_SoftByDistance;
		/// Pixels a side: of a spotlight's map, and of a face of a point light's cube map.
		int mlSpotSize = 1024;
		int mlPointSize = 1024;

		/// eShadowMapFilter_Soft: the blur.
		float mfSoftness = 0.02f;
		/// eShadowMapFilter_SoftByDistance: the radius of the lights, and the
		/// widest blur. Wider, and what is cast from close to a light (the
		/// cage of a lamp) is blurred away.
		float mfLightSize = 0.05f;
		float mfMaxBlur = 0.03f;
		int mlSamples = 16;
		int mlBlockerSamples = 12;
		bool mbNoise = true;

		float mfBias = 0.005f;
		float mfSlopeBias = 1.5f;

		eShadowMapFaces mFaces = eShadowMapFaces_Both;
		/// Materials with holes cut by their alpha (fences, gratings) cast shadows with the holes.
		bool mbAlphaCasters = true;
		/// Nearer to a point light than this counts as at this distance.
		float mfPointNear = 0.05f;

		//Debug
		/// The light passes show how much light arrives, not the surfaces.
		bool mbShowOnly = false;
		/// If not empty, only the light of this name is drawn.
		tString msOnlyLight;
	};

	/**
	 * What the shadow maps of the last frame took.
	 */
	class cShadowMapStats
	{
	public:
		int mlLights = 0;
		/// Maps drawn: one for a spotlight, up to six for a point light.
		int mlMaps = 0;
		int mlCasters = 0;
	};

	//---------------------------------------------

	class cRenderSettings
	{
	public:
		cRenderSettings();

		void Clear();
		void Reset(iLowLevelGraphics *apLowLevel);

		//Setings that doesn't change:
		iLowLevelGraphics* mpLowLevel;

		//Setting that changes
		iLight3D* mpLight;
		cCamera3D* mpCamera;
		cFrustum* mpFrustum;

		cSector *mpSector;

		bool mbFogActive;
		bool mbFogCulling;
		float mfFogStart;
		float mfFogEnd;
		cColor mFogColor;

		eRendererShowShadows mShowShadows;
		bool mbLog;
		tRendererDebugFlag mDebugFlags;

		//State settings
		bool mbDepthTest;

		eMaterialAlphaMode mAlphaMode;
		eMaterialBlendMode mBlendMode;
		eMaterialChannelMode mChannelMode;

		iGpuProgram* mpProgram;
		iMaterialProgramSetup* mpProgramSetup;


		bool mbMatrixWasNULL;

		bool mbUsesLight;
		bool mbUsesEye;

		cColor mAmbientColor;


		iTexture* mpTexture[MAX_TEXTUREUNITS];

		iVertexBuffer* mpVtxBuffer;
	};

	//---------------------------------------------

	class cRenderer3D
	{
	public:
		cRenderer3D(iLowLevelGraphics *apLowLevelGraphics,cResources* apResources,
						cMeshCreator* apMeshCreator, cRenderList *apRenderList);
		~cRenderer3D();

		void UpdateRenderList(cWorld3D* apWorld, cCamera3D* apCamera, float afFrameTime);

		void RenderWorld(cWorld3D* apWorld, cCamera3D* apCamera, float afFrameTime);

		void SetSkyBox(iTexture *apTexture, bool abAutoDestroy);
		void SetSkyBoxActive(bool abX);
		void SetSkyBoxColor(const cColor& aColor);

		iTexture* GetSkyBox(){return mpSkyBoxTexture;}
		bool GetSkyBoxActive(){ return mbSkyBoxActive;}
		cColor GetSkyBoxColor(){ return mSkyBoxColor;}

		void SetAmbientColor(const cColor& aColor){ mRenderSettings.mAmbientColor = aColor;}
		cColor GetAmbientColor(){ return mRenderSettings.mAmbientColor;}

		iTexture* GetFogSolidTexture(){ return mpFogLinearSolidTexture;}
		iTexture* GetFogAddTexture(){ return mpFogLinearAddTexture;}
		iTexture* GetFogAlphaTexture(){ return mpFogLinearAlphaTexture;}

		void SetRefractionUsed(bool abX){ mbRefractionUsed = abX;}
		bool GetRefractionUsed(){  return mbRefractionUsed;}

		//Fog properties
		void SetFogActive(bool abX);
		void SetFogStart(float afX);
		void SetFogEnd(float afX);
		void SetFogColor(const cColor& aCol){ mRenderSettings.mFogColor = aCol;}
		void SetFogCulling(bool abX){ mRenderSettings.mbFogCulling = abX;}


		bool GetFogActive(){ return mRenderSettings.mbFogActive;}
		float GetFogStart(){ return mRenderSettings.mfFogStart;}
		float GetFogEnd(){ return mRenderSettings.mfFogEnd;}
		cColor GetFogColor(){ return mRenderSettings.mFogColor;}
		bool GetFogCulling(){ return mRenderSettings.mbFogCulling;}

		cBoundingVolume* GetFogBV(){return &mFogBV;}

		void SetPostEffects(cRendererPostEffects *apPostEffects){ mpPostEffects = apPostEffects;}


		//Debug setup
		void SetDebugFlags(tRendererDebugFlag aFlags){ mDebugFlags = aFlags;}
		tRendererDebugFlag GetDebugFlags(){ return mDebugFlags;}

		cRenderList* GetRenderList(){ return mpRenderList;}
		/// Seconds of rendering: the frame times given to RenderWorld(), added up.
		float GetRenderTime(){ return mfRenderTime;}
		cRenderSettings* GetRenderSettings(){return &mRenderSettings;}

		eRendererShowShadows GetShowShadows();
		void SetShowShadows(eRendererShowShadows aState);

		const cShadowMapSettings& GetShadowMapSettings(){ return mShadowMapSettings; }
		void SetShadowMapSettings(const cShadowMapSettings& aSettings){ mShadowMapSettings = aSettings; }
		const cShadowMapStats& GetShadowMapStats(){ return mShadowMapStats; }


	private:
		inline void BeginRendering(cCamera3D* apCamera);

		void InitSkyBox();

		//Render steps
		void RenderFog(cCamera3D *apCamera);

		void RenderSkyBox(cCamera3D *apCamera);

		void RenderZ(cCamera3D *apCamera);


		void RenderLight(cCamera3D *apCamera);

		bool RenderShadowMap(iLight3D *apLight, cCamera3D *apCamera, cShadowMapParams &aParams);
		int RenderShadowCasters(iLight3D *apLight, const cMatrixf &a_mtxView, const cMatrixf &a_mtxProjection, int alFace);
		void UpdateShadowCasterGraphics(cCamera3D *apCamera);

		bool RenderAmbientOcclusion(cCamera3D *apCamera);

		void RenderDiffuse(cCamera3D *apCamera);

		void RenderTrans(cCamera3D *apCamera);
		bool RenderTransparentRefraction(cCamera3D *apCamera, const cTransparentDraw &aDraw,
			const iRenderState &aState);
		void RenderRefractionMask(cCamera3D *apCamera, const iRenderState &aState,
			const cRect2l &aClipRect);

		void RenderDebug(cCamera3D *apCamera);

		inline void RenderDebugObject(cCamera3D *apCamera,iRenderable* &apObject, iMaterial* apPrevMat,
					int alPrevMatId,iVertexBuffer* apPrevVtxBuff,
					eMaterialRenderType aRenderType, iLight3D* apLight);

		iLowLevelGraphics *mpLowLevelGraphics;
		iLowLevelResources *mpLowLevelResources;

		cRendererPostEffects *mpPostEffects;

		bool mbLog;

		float mfRenderTime;

		iGpuProgram *mpDiffuseProgram;
		iGpuProgram *mpSolidFogProgram;

		iTexture *mpFogLinearSolidTexture;

		iTexture *mpFogLinearAddTexture;
		iTexture *mpFogLinearAlphaTexture;

		iGpuProgram *mpRefractProgram;
		iGpuProgram *mpRefractSpecialProgram;
		bool mbRefractionAvailable;
		bool mbRefractionUsed;

		cRenderSettings mRenderSettings;

		cShadowMapSettings mShadowMapSettings;
		cShadowMapStats mShadowMapStats;

		cBoundingVolume mFogBV;

		tVertexVec mvVtxRect;

		cMeshCreator* mpMeshCreator;
		cRenderList *mpRenderList;

		iVertexBuffer* mpSkyBox;
		iTexture* mpSkyBoxTexture;
		bool mbAutoDestroySkybox;
		bool mbSkyBoxActive;
		cColor mSkyBoxColor;

		cResources* mpResources;

		tRendererDebugFlag mDebugFlags;
	};

};
#endif // HPL_RENDERER3D_H
