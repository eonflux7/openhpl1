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

	/**
	 * The values the bloom is tuned with: what is brighter than a
	 * threshold, blurred widely through a chain of halvings.
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
		/// frame in half of its pixels: seen as ghosting in play, so off
		/// unless asked for.
		bool mbHalfRate = false;
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

		/// Slices and steps of a quality. Each has a fragment program of
		/// its own (PostEffect_AO_Occlusion_<slices>x<steps>).
		static const int* QualitySamples(int alQuality)
		{
			static const int vSamples[klQualityNum][2] = {{2,2}, {2,4}, {3,6}, {4,10}, {6,16}};
			return vSamples[alQuality];
		}
	};

	/**
	 * How the depth of field is drawn: every pixel spread over a disc as
	 * wide as it is out of focus, from the depth buffer. What is in focus
	 * is the game's to say (SetDepthOfFieldFocalPlane() and the others).
	 */
	class cDepthOfFieldSettings
	{
	public:
		/// The radius of the blur of what is fully out of focus, as a share
		/// of the picture's height.
		float mfRadius = 0.012f;
		/// Rings of samples around a pixel, 1 to 5. More: smoother discs,
		/// and the cost grows with their square.
		int mlRings = 3;

		//Debug
		/// The depth of field is on, focused as set here, whatever the
		/// game says.
		bool mbTest = false;
		/// The distance in focus, and how far from it the blur is full.
		float mfTestFocus = 2.0f;
		float mfTestRange = 1.5f;
	};

	/**
	 * How the motion blur is drawn: the camera's own movement, from the
	 * depth buffer, in one pass.
	 */
	class cMotionBlurSettings
	{
	public:
		/// How long the shutter is open, in seconds: the picture is
		/// blurred over the way it moves in that time, at any frame rate.
		float mfShutterTime = 1.0f / 120.0f;
		/// The longest blur, as a share of the picture's height.
		float mfMaxLength = 0.04f;
		/// Samples along the blur, 2 to 32.
		int mlSamples = 8;
		/// What is nearer than this, in metres, is not blurred: what the
		/// player holds moves with the camera.
		float mfNearLimit = 0.6f;
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
		/// Player trails share the pass, but cannot switch off a scripted save trail.
		void SetPlayerImageTrailActive(bool abX)
		{
			if(!mbPlayerImageTrailActive && abX) mImageTrailData.mbFirstPass = true;
			mbPlayerImageTrailActive = abX;
		}
		void SetPlayerImageTrailAmount(float afAmount) { mfPlayerImageTrailAmount = afAmount; }

		iTexture* GetScreenBuffer(int alNum){ return mpScreenBuffer[alNum];}

		void SetActive(bool abX){ mbActive = abX;}
		bool GetActive(){ return mbActive;}

		/**
		 * (Re)creates the screen copies and the bloom's levels at the
		 * current render size.
		 */
		void CreateScreenBuffers();

		void SetBloomActive(bool abX);
		bool GetBloomActive(){ return mbBloomActive;}

		void SetBloomTuning(const cBloomTuning& aTuning){ mBloomTuning = aTuning;}
		const cBloomTuning& GetBloomTuning(){ return mBloomTuning;}

		void SetMotionBlurActive(bool abX);
		bool GetMotionBlurActive(){ return mbMotionBlurActive;}

		void SetMotionBlurSettings(const cMotionBlurSettings& aSettings){ mMotionBlurSettings = aSettings;}
		const cMotionBlurSettings& GetMotionBlurSettings(){ return mMotionBlurSettings;}

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

		void SetDepthOfFieldSettings(const cDepthOfFieldSettings& aSettings){ mDofSettings = aSettings;}
		const cDepthOfFieldSettings& GetDepthOfFieldSettings(){ return mDofSettings;}

		iTexture* GetFreeScreenTexture(){ return mpScreenBuffer[mImageTrailData.mlCurrentBuffer==0?1:0];}

		//Switched on, or to half rate, there is no frame to keep pixels of
		void SetAmbientOcclusionActive(bool abX){ if(abX && mbAoActive==false) mbAoFilled = false; mbAoActive = abX;}
		bool GetAmbientOcclusionActive(){ return mbAoActive;}
		void SetAmbientOcclusionSettings(const cAmbientOcclusionSettings& aSettings)
		{
			if(aSettings.mbHalfRate && mAoSettings.mbHalfRate==false) mbAoFilled = false;
			mAoSettings = aSettings;
		}
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

	private:
		bool DrawSceneCopy(iTexture *apTarget);

		void RenderImageTrail();

		void RenderBloom();
		void DrawBloomPass(iGpuProgram *apFP, iTexture *apTarget, iTexture *apSource);

		void RenderMotionBlur();

		void RenderDepthOfField();
		bool CreateDepthOfFieldTextures();
		void DrawScreenPass(iTexture *apTarget, iTexture *apSource);

		bool CreateAmbientOcclusionTextures();
		void DestroyAmbientOcclusionTextures();
		void DrawAmbientOcclusionPass(iTexture *apTarget, iTexture *apSource);
		void DrawAmbientOcclusionQuad(const cVector2f& avMin, const cVector2f& avMax);

		iLowLevelGraphics *mpLowLevelGraphics;
		iLowLevelResources *mpLowLevelResources;
		cResources* mpResources;
		cGpuProgramManager* mpGpuManager;
		cRenderer3D *mpRenderer3D;

		cRenderList *mpRenderList;

		cVector2f mvScreenSize;

		iTexture* mpScreenBuffer[2];

		cImageTrailEffect mImageTrailData;
		iGpuProgram *mpImageTrailVP;
		iGpuProgram *mpImageTrailFP;
		bool mbPlayerImageTrailActive = false;
		float mfPlayerImageTrailAmount = 0;

		//The bloom: each level is half the size of the one before.
		//mvBloomUp[i] is level i blended with all the levels below it; the
		//last level has none below and is used as it is.
		iGpuProgram *mpBloomChainVP;
		iGpuProgram *mpBloomPrefilterFP;
		iGpuProgram *mpBloomDownFP;
		iGpuProgram *mpBloomUpFP;
		iGpuProgram *mpBloomCompositeFP;
		std::vector<iTexture*> mvBloomDown;
		std::vector<iTexture*> mvBloomUp;

		//The motion blur
		iGpuProgram *mpMotionCameraVP;
		iGpuProgram *mpMotionCameraFP;

		//The depth of field: its textures are half the render size
		//and made when first needed.
		iGpuProgram *mpBokehVP;
		iGpuProgram *mpBokehPrefilterFP;
		iGpuProgram *mpBokehGatherFP;
		iGpuProgram *mpBokehFillFP;
		iGpuProgram *mpBokehCompositeFP;
		iTexture *mpBokehTexture[2];

		//Ambient occlusion. The distance and normal textures are a tile
		//each (see the shader), the tiled texture is as large as the tiles
		//together and the others are of the render size. Made when first
		//needed.
		iGpuProgram *mpAoVP;
		iGpuProgram *mpAoDeinterleaveFP;
		iGpuProgram *mpAoOcclusionFP;
		//The same with the samples of a quality as constants, loaded when
		//first used
		iGpuProgram *mvAoQualityFP[cAmbientOcclusionSettings::klQualityNum];
		bool mvAoQualityTried[cAmbientOcclusionSettings::klQualityNum];
		iGpuProgram *mpAoFilterFP;
		iGpuProgram *mpAoCompositeFP;
		//The same reading the scene itself, with one sample a pixel and
		//multisampled: no copy of the scene is needed
		iGpuProgram *mpAoCompositeSceneFP[2];
		iTexture *mvAoDistanceTexture[16];
		iTexture *mvAoNormalTexture[16];
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
		cBloomTuning mBloomTuning;

		bool mbMotionBlurActive;
		bool mbMotionBlurFirstTime;
		cMotionBlurSettings mMotionBlurSettings;
		//The motion blur's frame before: what the camera saw, and when
		cMatrixf m_mtxMotionBlurPrevViewProj;
		float mfMotionBlurPrevTime;

		bool mbDofActive;
		float mfDofMaxBlur;
		float mfDofFocalPlane;
		float mfDofNearPlane;
		float mfDofFarPlane;
		cDepthOfFieldSettings mDofSettings;

		bool mbActive;
	};

};
#endif // HPL_RENDERER_POST_EFFECTS_H
