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
#ifndef HPL_RENDERER_POST_EFFECTS_H
#define HPL_RENDERER_POST_EFFECTS_H

#include <set>
#include <list>
#include <math.h>

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"

namespace hpl {

#define kFilterProgramNum (1)

	class iLowLevelGraphics;
	class iLowLevelResources;
	class iGpuProgram;
	class cResources;
	class iTexture;
	class cGpuProgramManager;
	class cRenderList;
	class cRenderer3D;

	enum ePostEffectFilter
	{
		ePostEffectFilter_Offset,
		ePostEffectFilter_LastEnum
	};

	enum ePostEffectProgram
	{
		ePostEffectProgram_Offset,
		ePostEffectProgram_LastEnum
	};

	enum eBloomType
	{
		/// The blurred scene, squared and added: the original.
		eBloomType_Original,
		/// What is brighter than a threshold, blurred widely through a chain of halvings.
		eBloomType_Soft,
		eBloomType_LastEnum
	};

	/**
	 * The values the soft bloom is tuned with.
	 */
	class cBloomTuning
	{
	public:
		/// The screen value (1 is white) at which things begin to bloom.
		float mfThreshold = 0.3f;
		/// How much of the blurred light is added to the scene.
		float mfIntensity = 1.55f;
		/// How far the light spreads, 0 to 1: the share each level takes from the wider one below it.
		float mfScatter = 0.55f;
	};

	/**
	 * How the ambient occlusion is made (hpl1_PostEffect_AO.fragment) and
	 * how the scene is darkened by it. Lengths are in metres.
	 */
	class cAmbientOcclusionSettings
	{
	public:
		/// Directions on screen each pixel is searched in, and steps to each
		/// side of a direction: the cost is their product.
		int mlSlices = 3;
		int mlSteps = 6;
		/// Only every other pixel is searched in a frame, in turns; the
		/// others keep what they had from the frame before. Half the cost,
		/// and the occlusion of what moves across the screen trails by a
		/// frame in half of its pixels.
		bool mbHalfRate = true;
		/// How far from a point things keep light from it.
		float mfRadius = 0.6f;
		/// If not 0, the radius is this share of the picture's height at
		/// every distance instead: what is far away is searched as widely on
		/// screen as what is near.
		float mfScreenRadius = 0;
		/// The power the share of light that arrives is raised to: 1 as
		/// computed, more darkens more, 0 not at all.
		float mfStrength = 1.6f;
		/// The distance at which it has faded away.
		float mfFadeDistance = 30.0f;
		/// How much less what is brightly lit is darkened, 0 to 1.
		float mfLightFade = 0.5f;
		/// Passes of the filter that takes out the pattern of the samples: 0 to 2.
		int mlFilter = 1;

		//Debug
		/// The occlusion is drawn alone, as if everything were white.
		bool mbShowOnly = false;

		/// Slices and steps as one setting, from low (0) to ultra.
		static const int klQualityNum = 5;
		/// \return klQualityNum when they are not one of the qualities.
		int GetQuality() const
		{
			for(int i=0; i<klQualityNum; ++i)
				if(QualitySamples(i)[0] == mlSlices && QualitySamples(i)[1] == mlSteps) return i;
			return klQualityNum;
		}
		void SetQuality(int alQuality)
		{
			if(alQuality < 0 || alQuality >= klQualityNum) return;
			mlSlices = QualitySamples(alQuality)[0];
			mlSteps = QualitySamples(alQuality)[1];
		}

	private:
		static const int* QualitySamples(int alQuality)
		{
			static const int vSamples[klQualityNum][2] = {{2,2}, {2,4}, {3,6}, {4,10}, {6,16}};
			return vSamples[alQuality];
		}
	};

	class cImageTrailEffect
	{
	public:
		cImageTrailEffect() : mbActive(false), mbFirstPass(false), mlCurrentBuffer(0) {}

		bool mbActive;
		bool mbFirstPass;
		int mlCurrentBuffer;
		float mfAmount;
	};

	class cResources;

	class cRendererPostEffects
	{
	public:
		cRendererPostEffects(iLowLevelGraphics *apLowLevelGraphics,cResources* apResources,
							cRenderList *apRenderList, cRenderer3D *apRenderer3D);
		~cRendererPostEffects();

		/**
		 * Render post effects, called by cScene
		 */
		void Render();

		void SetImageTrailActive(bool abX){
			if(!mImageTrailData.mbActive && abX) mImageTrailData.mbFirstPass = true;
			mImageTrailData.mbActive = abX;}
		bool GetImageTrailActive(){return mImageTrailData.mbActive;}
		/**
		 * Set the amount of blur
		 * \param afAmount 0.0 - 1.0 are valid
		 */
		void SetImageTrailAmount(float afAmount){mImageTrailData.mfAmount = afAmount;}

		iTexture* GetScreenBuffer(int alNum){ return mpScreenBuffer[alNum];}

		void SetActive(bool abX){ mbActive = abX;}
		bool GetActive(){ return mbActive;}

		/**
		 * (Re)creates the screen copies at the current render size, and
		 * the blur textures, in the format the lighting needs.
		 */
		void CreateScreenBuffers();

		void SetBloomActive(bool abX);
		bool GetBloomActive(){ return mbBloomActive;}

		void SetBloomSpread(float afX){mfBloomSpread = afX;}
		float GetBloomSpread(){ return mfBloomSpread;}

		void SetBloomType(eBloomType aType){ mBloomType = aType;}
		eBloomType GetBloomType(){ return mBloomType;}

		void SetBloomTuning(const cBloomTuning& aTuning){ mBloomTuning = aTuning;}
		const cBloomTuning& GetBloomTuning(){ return mBloomTuning;}

		void SetMotionBlurActive(bool abX);
		bool GetMotionBlurActive(){ return mbMotionBlurActive;}

		void SetMotionBlurAmount(float afX){ mfMotionBlurAmount = afX;}
		float GetMotionBlurAmount(){ return mfMotionBlurAmount;}

		void SetDepthOfFieldActive(bool abX){ mbDofActive = abX;}
		void SetDepthOfFieldMaxBlur(float afX){ mfDofMaxBlur = afX;}
		void SetDepthOfFieldFocalPlane(float afX){ mfDofFocalPlane = afX;}
		void SetDepthOfFieldNearPlane(float afX){ mfDofNearPlane = afX;}
		void SetDepthOfFieldFarPlane(float afX){ mfDofFarPlane = afX;}

		bool GetDepthOfFieldActive(){ return mbDofActive;}
		float GetDepthOfFieldMaxBlur(){ return mfDofMaxBlur;}
		float GetDepthOfFieldFocalPlane(){ return mfDofFocalPlane;}
		float GetDepthOfFieldNearPlane(){ return mfDofNearPlane;}
		float GetDepthOfFieldFarPlane(){ return mfDofFarPlane;}

		iTexture* GetFreeScreenTexture(){ return mpScreenBuffer[mImageTrailData.mlCurrentBuffer==0?1:0];}

		void SetAmbientOcclusionActive(bool abX){ mbAoActive = abX;}
		bool GetAmbientOcclusionActive(){ return mbAoActive;}
		void SetAmbientOcclusionSettings(const cAmbientOcclusionSettings& aSettings){ mAoSettings = aSettings;}
		const cAmbientOcclusionSettings& GetAmbientOcclusionSettings(){ return mAoSettings;}
		/**
		 * Darkens the scene as drawn so far by the ambient occlusion, made
		 * from the depth buffer as it is. Called by cRenderer3D between the
		 * lights and what is not lit; not one of the effects of Render().
		 * Leaves the low level's programs, textures 0 and 1, blending,
		 * depth test, colour mask and projection changed.
		 * 
eturn false if nothing was drawn
		 */
		bool RenderAmbientOcclusion();

		void RenderBlurTexture(iTexture *apDestination, iTexture *apSource,float afBlurAmount);

	private:
		void RenderImageTrail();

		void RenderBloom();
		void RenderBloomChain();
		void DrawBloomPass(iGpuProgram *apFP, iTexture *apTarget, iTexture *apSource, bool abRectSource);

		void RenderMotionBlur();

		void RenderDepthOfField();

		bool CreateAmbientOcclusionTextures();
		void DestroyAmbientOcclusionTextures();
		void DrawAmbientOcclusionPass(iTexture *apTarget, iTexture *apSource, int alTurn=-1);

		iLowLevelGraphics *mpLowLevelGraphics;
		iLowLevelResources *mpLowLevelResources;
		cResources* mpResources;
		cGpuProgramManager* mpGpuManager;
		cRenderer3D *mpRenderer3D;

		cRenderList *mpRenderList;

		cVector2f mvScreenSize;

		iTexture* mpScreenBuffer[2];

		cImageTrailEffect mImageTrailData;

		iGpuProgram *mpBlurVP;
		iGpuProgram *mpBlur2dFP;
		iGpuProgram *mpBlurRectFP;
		//Holds the horizontal pass of RenderBlurTexture(), same size as the
		//blur textures.
		iTexture *mpBlurTempTexture;

		iGpuProgram *mpBloomVP;
		iGpuProgram *mpBloomFP;

		iTexture *mpBloomBlurTexture;

		//The soft bloom: each level is half the size of the one before.
		//mvBloomUp[i] is level i blended with all the levels below it; the
		//last level has none below and is used as it is.
		iGpuProgram *mpBloomChainVP;
		iGpuProgram *mpBloomPrefilterFP;
		iGpuProgram *mpBloomDownFP;
		iGpuProgram *mpBloomUpFP;
		iGpuProgram *mpBloomCompositeFP;
		std::vector<iTexture*> mvBloomDown;
		std::vector<iTexture*> mvBloomUp;

		iGpuProgram *mpMotionBlurVP;
		iGpuProgram *mpMotionBlurFP;

		iGpuProgram *mpDepthOfFieldVP;
		iGpuProgram *mpDepthOfFieldFP;
		iTexture *mpDofBlurTexture;

		//Ambient occlusion. The first two textures are in tiles (see the
		//shader) and as large as the tiles together; the last two are of
		//the render size. Made when first needed.
		iGpuProgram *mpAoVP;
		iGpuProgram *mpAoDeinterleaveFP;
		iGpuProgram *mpAoOcclusionFP;
		iGpuProgram *mpAoFilterFP;
		iGpuProgram *mpAoCompositeFP;
		iTexture *mpAoDistanceTexture;
		iTexture *mpAoTiledTexture;
		iTexture *mpAoTexture[2];
		cVector2l mvAoTileSize;
		//Frames drawn, for taking turns; and if the textures hold a frame
		int mlAoFrame;
		bool mbAoFilled;
		bool mbAoActive;
		cAmbientOcclusionSettings mAoSettings;

		tVertexVec mvTexRectVtx;

		bool mbBloomActive;
		float mfBloomSpread;
		eBloomType mBloomType;
		cBloomTuning mBloomTuning;

		bool mbMotionBlurActive;
		float mfMotionBlurAmount;
		bool mbMotionBlurFirstTime;

		bool mbDofActive;
		float mfDofMaxBlur;
		float mfDofFocalPlane;
		float mfDofNearPlane;
		float mfDofFarPlane;

		bool mbActive;
	};

};
#endif // HPL_RENDERER_POST_EFFECTS_H
