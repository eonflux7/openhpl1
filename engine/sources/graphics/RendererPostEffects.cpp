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
#include "graphics/RendererPostEffects.h"
#include "graphics/Texture.h"
#include "scene/Scene.h"
#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/GPUProgram.h"
#include "resources/LowLevelResources.h"
#include "math/Math.h"
#include "resources/Resources.h"
#include "resources/GpuProgramManager.h"
#include "graphics/RenderList.h"
#include "graphics/Renderer3D.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cRendererPostEffects::cRendererPostEffects(iLowLevelGraphics *apLowLevelGraphics,cResources* apResources,
												cRenderList *apRenderList,cRenderer3D *apRenderer3D)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		mpLowLevelResources = apResources->GetLowLevel();
		mpResources = apResources;
		mpRenderer3D = apRenderer3D;

		mpGpuManager = mpResources->GetGpuProgramManager();

		mpRenderList = apRenderList;

		mpScreenBuffer = nullptr;
		for(iTexture *&pTexture : mvAoDistanceTexture) pTexture = nullptr;
		for(iTexture *&pTexture : mvAoNormalTexture) pTexture = nullptr;
		mpAoTiledTexture = nullptr;
		mpAoTexture[0] = nullptr;
		mpAoTexture[1] = nullptr;
		mpBokehTexture[0] = nullptr;
		mpBokehTexture[1] = nullptr;
		CreateScreenBuffers();

		///////////////////////////////////////////
		// Create programs

		Log(" Creating programs\n");

		/////////////////
		// Bloom programs
		mpBloomChainVP = mpGpuManager->CreateProgram("PostEffect_BloomChain_vp.cg","main",eGpuProgramType_Vertex);
		mpBloomPrefilterFP = mpGpuManager->CreateProgram("PostEffect_BloomChain_Prefilter_fp.cg","main",eGpuProgramType_Fragment);
		mpBloomDownFP = mpGpuManager->CreateProgram("PostEffect_BloomChain_Down_fp.cg","main",eGpuProgramType_Fragment);
		mpBloomUpFP = mpGpuManager->CreateProgram("PostEffect_BloomChain_Up_fp.cg","main",eGpuProgramType_Fragment);
		mpBloomCompositeFP = mpGpuManager->CreateProgram("PostEffect_BloomChain_Composite_fp.cg","main",eGpuProgramType_Fragment);
		if(!mpBloomChainVP || !mpBloomPrefilterFP || !mpBloomDownFP || !mpBloomUpFP || !mpBloomCompositeFP)
			Error("Couldn't load the 'PostEffect_BloomChain' programs!\n");

		/////////////////
		// Ambient occlusion programs
		mpAoVP = mpGpuManager->CreateProgram("PostEffect_AO_vp.cg","main",eGpuProgramType_Vertex);
		mpAoDeinterleaveFP = mpGpuManager->CreateProgram("PostEffect_AO_Deinterleave_fp.cg","main",eGpuProgramType_Fragment);
		mpAoOcclusionFP = mpGpuManager->CreateProgram("PostEffect_AO_Occlusion_fp.cg","main",eGpuProgramType_Fragment);
		mpAoFilterFP = mpGpuManager->CreateProgram("PostEffect_AO_Filter_fp.cg","main",eGpuProgramType_Fragment);
		mpAoCompositeFP = mpGpuManager->CreateProgram("PostEffect_AO_Composite_fp.cg","main",eGpuProgramType_Fragment);
		mpAoCompositeSceneFP[0] = mpGpuManager->CreateProgram("PostEffect_AO_Composite_Scene_fp.cg","main",eGpuProgramType_Fragment);
		mpAoCompositeSceneFP[1] = mpGpuManager->CreateProgram("PostEffect_AO_Composite_SceneMS_fp.cg","main",eGpuProgramType_Fragment);
		if(!mpAoVP || !mpAoDeinterleaveFP || !mpAoOcclusionFP || !mpAoFilterFP || !mpAoCompositeFP)
			Error("Couldn't load the 'PostEffect_AO' programs!\n");
		for(int i=0; i<cAmbientOcclusionSettings::klQualityNum; ++i)
		{
			mvAoQualityFP[i] = nullptr;
			mvAoQualityTried[i] = false;
		}

		/////////////////
		// MotionBlur programs
		mpMotionCameraVP = mpGpuManager->CreateProgram("PostEffect_MotionCamera_vp.cg","main",eGpuProgramType_Vertex);
		mpMotionCameraFP = mpGpuManager->CreateProgram("PostEffect_MotionCamera_fp.cg","main",eGpuProgramType_Fragment);
		if(!mpMotionCameraVP || !mpMotionCameraFP) Error("Couldn't load the 'PostEffect_MotionCamera' programs!\n");

		/////////////////
		// Depth of Field programs
		mpBokehVP = mpGpuManager->CreateProgram("PostEffect_Bokeh_vp.cg","main",eGpuProgramType_Vertex);
		mpBokehPrefilterFP = mpGpuManager->CreateProgram("PostEffect_Bokeh_Prefilter_fp.cg","main",eGpuProgramType_Fragment);
		mpBokehGatherFP = mpGpuManager->CreateProgram("PostEffect_Bokeh_Gather_fp.cg","main",eGpuProgramType_Fragment);
		mpBokehFillFP = mpGpuManager->CreateProgram("PostEffect_Bokeh_Fill_fp.cg","main",eGpuProgramType_Fragment);
		mpBokehCompositeFP = mpGpuManager->CreateProgram("PostEffect_Bokeh_Composite_fp.cg","main",eGpuProgramType_Fragment);
		if(!mpBokehVP || !mpBokehPrefilterFP || !mpBokehGatherFP || !mpBokehFillFP || !mpBokehCompositeFP)
			Error("Couldn't load the 'PostEffect_Bokeh' programs!\n");

		Log("  RendererPostEffects created\n");

		////////////////////////////////////
		// Variable setup

		//General
		mbActive = false;
		mvTexRectVtx.resize(4);

		//Bloom
		mbBloomActive = false;

		//Ambient occlusion
		mbAoActive = false;
		mlAoFrame = 0;
		mbAoFilled = false;

		//Motion blur
		mbMotionBlurActive = false;
		mbMotionBlurFirstTime = true;
		mfMotionBlurPrevTime = 0;

		//Depth of Field
		mbDofActive = false;
		mfDofMaxBlur = 1.0f;
		mfDofFocalPlane = 1.0f;
		mfDofNearPlane = 2.0f;
		mfDofFarPlane = 3.0f;
	}

	//-----------------------------------------------------------------------

	cRendererPostEffects::~cRendererPostEffects()
	{
		if(mpScreenBuffer) hplDelete(mpScreenBuffer);

		iGpuProgram *vChainPrograms[] = {mpBloomChainVP, mpBloomPrefilterFP, mpBloomDownFP, mpBloomUpFP, mpBloomCompositeFP};
		for(iGpuProgram *pProgram : vChainPrograms) if(pProgram) mpGpuManager->Destroy(pProgram);
		for(iTexture *pTexture : mvBloomDown) hplDelete(pTexture);
		for(iTexture *pTexture : mvBloomUp) hplDelete(pTexture);

		iGpuProgram *vAoPrograms[] = {mpAoVP, mpAoDeinterleaveFP, mpAoOcclusionFP, mpAoFilterFP, mpAoCompositeFP,
										mpAoCompositeSceneFP[0], mpAoCompositeSceneFP[1]};
		for(iGpuProgram *pProgram : vAoPrograms) if(pProgram) mpGpuManager->Destroy(pProgram);
		for(iGpuProgram *pProgram : mvAoQualityFP) if(pProgram) mpGpuManager->Destroy(pProgram);
		DestroyAmbientOcclusionTextures();

		iGpuProgram *vNewPrograms[] = {mpMotionCameraVP, mpMotionCameraFP, mpBokehVP, mpBokehPrefilterFP,
										mpBokehGatherFP, mpBokehFillFP, mpBokehCompositeFP};
		for(iGpuProgram *pProgram : vNewPrograms) if(pProgram) mpGpuManager->Destroy(pProgram);
		for(iTexture *pTexture : mpBokehTexture) if(pTexture) hplDelete(pTexture);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cRendererPostEffects::CreateScreenBuffers()
	{
		if(mpScreenBuffer) hplDelete(mpScreenBuffer);
		mpScreenBuffer = nullptr;

		// The copy holds linear values, which need more than 8 bits in
		// the dark end: floating point
		int lBpp = 64;

		mvScreenSize = mpLowLevelGraphics->GetRenderSize();
		Log(" Creating screen buffer size %s\n",mvScreenSize.ToString().c_str());
		mpScreenBuffer = mpLowLevelGraphics->CreateTexture(cVector2l(
			(int)mvScreenSize.x,(int)mvScreenSize.y),lBpp,cColor(0,0,0,0),false,
			eTextureType_Normal, eTextureTarget_2D);
		if(mpScreenBuffer)
		{
			mpScreenBuffer->SetWrapS(eTextureWrap_ClampToEdge);
			mpScreenBuffer->SetWrapT(eTextureWrap_ClampToEdge);
		}
		else
		{
			Error("Couldn't create screenbuffer!\n");
		}

		//The levels of the bloom, from half the render size down to
		//a few pixels: the widest level covers the same part of the picture
		//at any resolution. Floating point always, they hold linear values.
		for(iTexture *pTexture : mvBloomDown) hplDelete(pTexture);
		for(iTexture *pTexture : mvBloomUp) hplDelete(pTexture);
		mvBloomDown.clear();
		mvBloomUp.clear();
		cVector2l vLevelSize((int)mvScreenSize.x / 2, (int)mvScreenSize.y / 2);
		while(vLevelSize.x >= 6 && vLevelSize.y >= 6 && mvBloomDown.size() < 12)
		{
			iTexture *vTextures[2];
			for(iTexture *&pTexture : vTextures)
			{
				pTexture = mpLowLevelGraphics->CreateTexture(vLevelSize,64,cColor(0,0,0,0),false,
															eTextureType_Normal, eTextureTarget_2D);
				if(pTexture == nullptr) continue;
				pTexture->SetWrapS(eTextureWrap_ClampToEdge);
				pTexture->SetWrapT(eTextureWrap_ClampToEdge);
			}
			if(vTextures[0] == nullptr || vTextures[1] == nullptr)
			{
				Error("Couldn't create bloom texture!\n");
				if(vTextures[0]) hplDelete(vTextures[0]);
				if(vTextures[1]) hplDelete(vTextures[1]);
				break;
			}
			mvBloomDown.push_back(vTextures[0]);
			mvBloomUp.push_back(vTextures[1]);

			vLevelSize = cVector2l(vLevelSize.x / 2, vLevelSize.y / 2);
		}

		//Made again, at the new size, when next needed
		DestroyAmbientOcclusionTextures();
		for(iTexture *&pTexture : mpBokehTexture)
		{
			if(pTexture) hplDelete(pTexture);
			pTexture = nullptr;
		}

		//The old frames are gone
		mbMotionBlurFirstTime = true;
	}

	//-----------------------------------------------------------------------

	void cRendererPostEffects::SetBloomActive(bool abX)
	{
		mbBloomActive = abX;
	}

	//-----------------------------------------------------------------------

	void cRendererPostEffects::SetMotionBlurActive(bool abX)
	{
		mbMotionBlurActive = abX;
		mbMotionBlurFirstTime = true;
	}

	//-----------------------------------------------------------------------

	void cRendererPostEffects::Render()
	{
		if(mpScreenBuffer == nullptr) return;

		// The occlusion alone is not a scene to blur or make glow
		if(mbAoActive && mAoSettings.mbShowOnly) return;

		// The panel's test of the depth of field does not wait for the game
		bool bDofTest = mDofSettings.mbTest;
		if(bDofTest)
		{
			float fRange = cMath::Max(mDofSettings.mfTestRange, 0.01f);
			mfDofFocalPlane = cMath::Max(mDofSettings.mfTestFocus, 0.0f);
			mfDofNearPlane = cMath::Max(mfDofFocalPlane - fRange, 0.0f);
			mfDofFarPlane = mfDofFocalPlane + fRange;
			mfDofMaxBlur = 1;
		}

		if(mbActive==false ||
			(	mbBloomActive==false &&
				mbMotionBlurActive==false && mbDofActive==false && bDofTest==false) )
		{
			return;
		}

		mvScreenSize = mpLowLevelGraphics->GetRenderSize();

		// A multisampled scene has one sample a pixel from here on: the
		// effects are drawn over all of it and copy it, neither of which
		// more samples are of use to.
		mpLowLevelGraphics->ResolveScene();

		mpLowLevelGraphics->BeginGpuSection("Depth of field");
		if(mbDofActive || bDofTest) RenderDepthOfField();
		mpLowLevelGraphics->EndGpuSection();
		mpLowLevelGraphics->BeginGpuSection("Motion blur");
		RenderMotionBlur();
		mpLowLevelGraphics->EndGpuSection();

		mpLowLevelGraphics->SetDepthTestActive(false);
		mpLowLevelGraphics->PushMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetIdentityMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetOrthoProjection(mpLowLevelGraphics->GetVirtualSize(),-1000,1000);

		mpLowLevelGraphics->BeginGpuSection("Bloom");
		RenderBloom();
		mpLowLevelGraphics->EndGpuSection();

		mpLowLevelGraphics->PopMatrix(eMatrix_ModelView);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	// One pass of the effects: a quad over apTarget (the scene when
	// it is null) with apSource in unit 0. The fragment programs take their
	// positions from the pixel drawn. Expects an orthographic projection of
	// the virtual screen size.
	void cRendererPostEffects::DrawScreenPass(iTexture *apTarget, iTexture *apSource)
	{
		mpLowLevelGraphics->SetRenderTarget(apTarget);
		mpLowLevelGraphics->SetTexture(0,apSource);

		cVector2f vVirtSize = mpLowLevelGraphics->GetVirtualSize();
		mvTexRectVtx[0] = cVertex(cVector3f(0,0,40),cVector2f(0,1),cColor(1,1.0f) );
		mvTexRectVtx[1] = cVertex(cVector3f(vVirtSize.x,0,40),cVector2f(1,1),cColor(1,1.0f));
		mvTexRectVtx[2] = cVertex(cVector3f(vVirtSize.x,vVirtSize.y,40),cVector2f(1,0),cColor(1,1.0f));
		mvTexRectVtx[3] = cVertex(cVector3f(0,vVirtSize.y,40),cVector2f(0,0),cColor(1,1.0f));
		mpLowLevelGraphics->DrawQuad(mvTexRectVtx);
	}

	//-----------------------------------------------------------------------

	// The textures of the depth of field, if they are not there yet.
	bool cRendererPostEffects::CreateDepthOfFieldTextures()
	{
		if(mpBokehTexture[1]) return true;

		cVector2l vSize(((int)mvScreenSize.x + 1) / 2, ((int)mvScreenSize.y + 1) / 2);
		for(iTexture *&pTexture : mpBokehTexture)
		{
			if(pTexture) hplDelete(pTexture);
			pTexture = mpLowLevelGraphics->CreateFloatTexture(vSize, 4, eTextureTarget_2D);
			if(pTexture==nullptr) continue;
			pTexture->SetWrapS(eTextureWrap_ClampToEdge);
			pTexture->SetWrapT(eTextureWrap_ClampToEdge);
		}
		if(!mpBokehTexture[0] || !mpBokehTexture[1])
		{
			Error("Couldn't create the depth of field textures!\n");
			for(iTexture *&pTexture : mpBokehTexture)
			{
				if(pTexture) hplDelete(pTexture);
				pTexture = nullptr;
			}
			return false;
		}
		return true;
	}

	//-----------------------------------------------------------------------

	// See hpl1_PostEffect_Bokeh.fragment.
	void cRendererPostEffects::RenderDepthOfField()
	{
		if(!mpBokehVP || !mpBokehPrefilterFP || !mpBokehGatherFP || !mpBokehFillFP || !mpBokehCompositeFP) return;

		// The blur of what is fully out of focus, in pixels of the scene
		float fRadius = cMath::Clamp(mDofSettings.mfRadius, 0.0f, 0.05f) * mvScreenSize.y;
		// The game fades the blur in and out evenly. A blur of half the
		// width still reads as out of focus, so the fade is squared: the
		// picture is nearly sharp half way through.
		float fMaxBlur = cMath::Clamp(mfDofMaxBlur, 0.0f, 1.0f);
		fMaxBlur *= fMaxBlur;
		if(fRadius * fMaxBlur < 1.0f) return;

		if(CreateDepthOfFieldTextures()==false) return;
		mpLowLevelGraphics->ResolveSceneDepth();

		cCamera3D *pCam = mpRenderList->GetCamera();
		iTexture *pScreenTexture = GetFreeScreenTexture();
		DrawSceneCopy(pScreenTexture);

		mpLowLevelGraphics->SetDepthWriteActive(false);
		mpLowLevelGraphics->SetDepthTestActive(false);
		mpLowLevelGraphics->SetBlendActive(false);
		mpLowLevelGraphics->PushMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetIdentityMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetOrthoProjection(mpLowLevelGraphics->GetVirtualSize(),-1000,1000);

		mpBokehVP->Bind();
		mpBokehVP->SetMatrixf("worldViewProj",eGpuProgramMatrix_ViewProjection,eGpuProgramMatrixOp_Identity);

		cVector3f vPlanes(mfDofNearPlane, mfDofFocalPlane, mfDofFarPlane);

		///////////////////////////////////////////
		// The scene at half size, with how far out of focus each pixel is
		mpBokehPrefilterFP->Bind();
		mpBokehPrefilterFP->SetVec3f("planes", vPlanes);
		mpBokehPrefilterFP->SetVec2f("blur", fMaxBlur, fRadius);
		DrawScreenPass(mpBokehTexture[0], pScreenTexture);
		mpBokehPrefilterFP->UnBind();

		///////////////////////////////////////////
		// Every pixel gathers what is spread over it
		mpBokehGatherFP->Bind();
		mpBokehGatherFP->SetVec3f("disc", fRadius * 0.5f, (float)cMath::Clamp(mDofSettings.mlRings, 1, 5),
									fRadius * 0.5f * fMaxBlur);
		DrawScreenPass(mpBokehTexture[1], mpBokehTexture[0]);
		mpBokehGatherFP->UnBind();

		///////////////////////////////////////////
		// The gaps between the samples smoothed over
		mpBokehFillFP->Bind();
		DrawScreenPass(mpBokehTexture[0], mpBokehTexture[1]);
		mpBokehFillFP->UnBind();

		///////////////////////////////////////////
		// Over the scene, where it is out of focus. The pass reads the
		// scene's depth, so the scene is drawn into without it; destination
		// alpha is the renderer's own.
		mpLowLevelGraphics->SetSceneColorOnly(true);
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, false);
		mpBokehCompositeFP->Bind();
		mpBokehCompositeFP->SetVec3f("planes", vPlanes);
		mpBokehCompositeFP->SetVec2f("blur", fMaxBlur, fRadius);
		mpLowLevelGraphics->SetTexture(1,pScreenTexture);
		DrawScreenPass(nullptr, mpBokehTexture[0]);
		mpBokehCompositeFP->UnBind();
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, true);
		mpLowLevelGraphics->SetSceneColorOnly(false);

		mpBokehVP->UnBind();
		mpLowLevelGraphics->SetTexture(0,nullptr);
		mpLowLevelGraphics->SetTexture(1,nullptr);
		mpLowLevelGraphics->PopMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetMatrix(eMatrix_Projection, pCam->GetProjectionMatrix());
		mpLowLevelGraphics->SetDepthWriteActive(true);
	}

	//-----------------------------------------------------------------------

	// See hpl1_PostEffect_MotionCamera.fragment.
	void cRendererPostEffects::RenderMotionBlur()
	{
		if(mbMotionBlurActive==false) return;
		if(!mpMotionCameraVP || !mpMotionCameraFP) return;

		///////////////////////////////////////////
		// Where the camera was the frame before, and how long ago
		cCamera3D *pCam = mpRenderList->GetCamera();
		cMatrixf mtxViewProj = cMath::MatrixMul(pCam->GetProjectionMatrix(), pCam->GetViewMatrix());
		cMatrixf mtxPrevViewProj = m_mtxMotionBlurPrevViewProj;
		float fTime = mpRenderer3D->GetRenderTime();
		float fFrameTime = fTime - mfMotionBlurPrevTime;
		bool bFirstTime = mbMotionBlurFirstTime;

		m_mtxMotionBlurPrevViewProj = mtxViewProj;
		mfMotionBlurPrevTime = fTime;
		mbMotionBlurFirstTime = false;

		if(bFirstTime || fFrameTime < 0.0001f || mtxPrevViewProj == mtxViewProj) return;

		const cMotionBlurSettings &settings = mMotionBlurSettings;
		// The share of the way since the frame before that the shutter is open for
		float fShutter = cMath::Max(settings.mfShutterTime, 0.0f) / fFrameTime;
		float fMaxLength = cMath::Clamp(settings.mfMaxLength, 0.0f, 0.25f) * mvScreenSize.y;
		if(fShutter <= 0 || fMaxLength < 1.0f) return;

		// From a place in this frame's view to its place on the screen of
		// the frame before
		cMatrixf mtxReproject = cMath::MatrixMul(mtxPrevViewProj, cMath::MatrixInverse(pCam->GetViewMatrix()));

		iTexture *pScreenTexture = GetFreeScreenTexture();
		DrawSceneCopy(pScreenTexture);

		mpLowLevelGraphics->SetDepthWriteActive(false);
		mpLowLevelGraphics->SetDepthTestActive(false);
		mpLowLevelGraphics->SetBlendActive(false);
		mpLowLevelGraphics->PushMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetIdentityMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetOrthoProjection(mpLowLevelGraphics->GetVirtualSize(),-1000,1000);

		// The pass reads the scene's depth, so the scene is drawn into
		// without it; destination alpha is the renderer's own.
		mpLowLevelGraphics->SetSceneColorOnly(true);
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, false);

		mpMotionCameraVP->Bind();
		mpMotionCameraVP->SetMatrixf("worldViewProj",eGpuProgramMatrix_ViewProjection,eGpuProgramMatrixOp_Identity);
		mpMotionCameraFP->Bind();
		mpMotionCameraFP->SetMatrixf("reproject", mtxReproject);
		mpMotionCameraFP->SetVec4f("shutter", fShutter, fMaxLength, (float)cMath::Clamp(settings.mlSamples, 2, 32),
									cMath::Max(settings.mfNearLimit, 0.0f));
		DrawScreenPass(nullptr, pScreenTexture);
		mpMotionCameraFP->UnBind();
		mpMotionCameraVP->UnBind();

		mpLowLevelGraphics->SetColorWriteActive(true, true, true, true);
		mpLowLevelGraphics->SetSceneColorOnly(false);
		mpLowLevelGraphics->SetTexture(0,nullptr);
		mpLowLevelGraphics->PopMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetMatrix(eMatrix_Projection, pCam->GetProjectionMatrix());
		mpLowLevelGraphics->SetDepthWriteActive(true);
	}

	//-----------------------------------------------------------------------

	// One pass of the bloom: a quad over apTarget (the scene when it
	// is null) with apSource in unit 0. The fragment programs take their
	// positions from the pixel drawn. Expects an orthographic projection of
	// the virtual screen size.
	void cRendererPostEffects::DrawBloomPass(iGpuProgram *apFP, iTexture *apTarget, iTexture *apSource)
	{
		cVector2f vTargetSize = apTarget ?	cVector2f((float)apTarget->GetWidth(), (float)apTarget->GetHeight()) :
											mvScreenSize;
		cVector2f vSourceSize((float)apSource->GetWidth(), (float)apSource->GetHeight());

		mpLowLevelGraphics->SetRenderTarget(apTarget);
		mpLowLevelGraphics->SetTexture(0,apSource);

		apFP->SetVec2f("sourceTexel", 1.0f / vSourceSize.x, 1.0f / vSourceSize.y);
		apFP->SetVec2f("targetToSource", 1.0f / vTargetSize.x, 1.0f / vTargetSize.y);

		cVector2f vVirtSize = mpLowLevelGraphics->GetVirtualSize();
		mvTexRectVtx[0] = cVertex(cVector3f(0,0,40),cVector2f(0,1),cColor(1,1.0f) );
		mvTexRectVtx[1] = cVertex(cVector3f(vVirtSize.x,0,40),cVector2f(1,1),cColor(1,1.0f));
		mvTexRectVtx[2] = cVertex(cVector3f(vVirtSize.x,vVirtSize.y,40),cVector2f(1,0),cColor(1,1.0f));
		mvTexRectVtx[3] = cVertex(cVector3f(0,vVirtSize.y,40),cVector2f(0,0),cColor(1,1.0f));
		mpLowLevelGraphics->DrawQuad(mvTexRectVtx);
	}

	//-----------------------------------------------------------------------

	// See hpl1_PostEffect_BloomChain.fragment.
	void cRendererPostEffects::RenderBloom()
	{
		if(mbBloomActive==false || mvBloomDown.empty()) return;
		if(!mpBloomChainVP || !mpBloomPrefilterFP || !mpBloomDownFP || !mpBloomUpFP || !mpBloomCompositeFP) return;

		iTexture *pScreenTexture = GetFreeScreenTexture();
		DrawSceneCopy(pScreenTexture);

		mpLowLevelGraphics->SetBlendActive(false);

		mpBloomChainVP->Bind();
		mpBloomChainVP->SetMatrixf("worldViewProj",eGpuProgramMatrix_ViewProjection,eGpuProgramMatrixOp_Identity);

		///////////////////////////////////////////
		// The bright part of the scene, at half size. The threshold is set
		// as a screen value and used on light.
		float fThreshold = powf(cMath::Max(mBloomTuning.mfThreshold, 0.0f), 2.2f);
		mpBloomPrefilterFP->Bind();
		mpBloomPrefilterFP->SetVec2f("threshold", fThreshold, fThreshold * 0.5f);
		DrawBloomPass(mpBloomPrefilterFP, mvBloomDown[0], pScreenTexture);
		mpBloomPrefilterFP->UnBind();

		///////////////////////////////////////////
		// Halved down to the last level
		size_t lLevels = mvBloomDown.size();
		mpBloomDownFP->Bind();
		for(size_t i = 1; i < lLevels; ++i)
			DrawBloomPass(mpBloomDownFP, mvBloomDown[i], mvBloomDown[i-1]);
		mpBloomDownFP->UnBind();

		///////////////////////////////////////////
		// And blended back up: each level with the wider ones below it
		iTexture *pBelow = mvBloomDown[lLevels-1];
		mpBloomUpFP->Bind();
		mpBloomUpFP->SetFloat("scatter", cMath::Clamp(mBloomTuning.mfScatter, 0.0f, 1.0f));
		for(int i = (int)lLevels - 2; i >= 0; --i)
		{
			mpLowLevelGraphics->SetTexture(1,mvBloomDown[i]);
			DrawBloomPass(mpBloomUpFP, mvBloomUp[i], pBelow);
			pBelow = mvBloomUp[i];
		}
		mpBloomUpFP->UnBind();
		mpLowLevelGraphics->SetTexture(1,nullptr);

		///////////////////////////////////////////
		// Added to the scene
		mpLowLevelGraphics->SetBlendActive(true);
		mpLowLevelGraphics->SetBlendFunc(eBlendFunc_One,eBlendFunc_One);
		mpBloomCompositeFP->Bind();
		mpBloomCompositeFP->SetFloat("intensity", mBloomTuning.mfIntensity);
		DrawBloomPass(mpBloomCompositeFP, nullptr, pBelow);
		mpBloomCompositeFP->UnBind();
		mpLowLevelGraphics->SetBlendActive(false);

		mpBloomChainVP->UnBind();
		mpLowLevelGraphics->SetTexture(0,nullptr);
	}

	//-----------------------------------------------------------------------

	void cRendererPostEffects::DestroyAmbientOcclusionTextures()
	{
		iTexture **vTextures[] = {&mpAoTiledTexture, &mpAoTexture[0], &mpAoTexture[1]};
		for(iTexture **pTexture : vTextures)
		{
			if(*pTexture) hplDelete(*pTexture);
			*pTexture = nullptr;
		}
		for(int i=0; i<16; ++i)
		{
			if(mvAoDistanceTexture[i]) hplDelete(mvAoDistanceTexture[i]);
			if(mvAoNormalTexture[i]) hplDelete(mvAoNormalTexture[i]);
			mvAoDistanceTexture[i] = mvAoNormalTexture[i] = nullptr;
		}
	}

	// The textures of the ambient occlusion, if they are not there yet.
	bool cRendererPostEffects::CreateAmbientOcclusionTextures()
	{
		if(mpAoTexture[1]) return true;
		DestroyAmbientOcclusionTextures();

		// 4x4 tiles of every fourth pixel; the last ones of a row or column
		// are not full when the size does not divide by four.
		cVector2l vSize((int)mvScreenSize.x, (int)mvScreenSize.y);
		mvAoTileSize = cVector2l((vSize.x + 3) / 4, (vSize.y + 3) / 4);
		cVector2l vTiledSize(mvAoTileSize.x * 4, mvAoTileSize.y * 4);

		// A tile's distances in one channel and its normals in two;
		// occlusion and distance in two
		bool bTiles = true;
		for(int i=0; i<16; ++i)
		{
			mvAoDistanceTexture[i] = mpLowLevelGraphics->CreateFloatTexture(mvAoTileSize, 1, eTextureTarget_2D);
			mvAoNormalTexture[i] = mpLowLevelGraphics->CreateFloatTexture(mvAoTileSize, 2, eTextureTarget_2D);
			if(!mvAoDistanceTexture[i] || !mvAoNormalTexture[i]) bTiles = false;
		}
		mpAoTiledTexture = mpLowLevelGraphics->CreateFloatTexture(vTiledSize, 2, eTextureTarget_2D);
		mpAoTexture[0] = mpLowLevelGraphics->CreateFloatTexture(vSize, 2, eTextureTarget_2D);
		mpAoTexture[1] = mpLowLevelGraphics->CreateFloatTexture(vSize, 2, eTextureTarget_2D);

		if(!bTiles || !mpAoTiledTexture || !mpAoTexture[0] || !mpAoTexture[1])
		{
			Error("Couldn't create the ambient occlusion textures!\n");
			DestroyAmbientOcclusionTextures();
			return false;
		}
		mbAoFilled = false;
		return true;
	}

	//-----------------------------------------------------------------------

	// One pass of the ambient occlusion: a quad over apTarget (the scene
	// when it is null) with apSource in unit 0. The fragment programs take
	// their positions from the pixel drawn. Expects an orthographic
	// projection of the virtual screen size.
	// It is one triangle that reaches past the target, not the two of a
	// quad: along the line where two meet, a multisampled pixel is drawn
	// once for each, and the second would read what the first wrote where
	// the scene is read as it is drawn over (BeginSceneColorRead).
	void cRendererPostEffects::DrawAmbientOcclusionPass(iTexture *apTarget, iTexture *apSource)
	{
		mpLowLevelGraphics->SetRenderTarget(apTarget);
		mpLowLevelGraphics->SetTexture(0,apSource);

		cVector2f vSize = mpLowLevelGraphics->GetVirtualSize();
		mvTexRectVtx[0] = cVertex(cVector3f(0,0,40),cVector2f(0,1),cColor(1,1.0f) );
		mvTexRectVtx[1] = cVertex(cVector3f(vSize.x * 2,0,40),cVector2f(2,1),cColor(1,1.0f));
		mvTexRectVtx[2] = cVertex(cVector3f(0,vSize.y * 2,40),cVector2f(0,-1),cColor(1,1.0f));
		mvTexRectVtx[3] = mvTexRectVtx[2];
		mpLowLevelGraphics->DrawQuad(mvTexRectVtx);
	}

	void cRendererPostEffects::DrawAmbientOcclusionQuad(const cVector2f& avMin, const cVector2f& avMax)
	{
		mvTexRectVtx[0] = cVertex(cVector3f(avMin.x,avMin.y,40),cVector2f(0,1),cColor(1,1.0f) );
		mvTexRectVtx[1] = cVertex(cVector3f(avMax.x,avMin.y,40),cVector2f(1,1),cColor(1,1.0f));
		mvTexRectVtx[2] = cVertex(cVector3f(avMax.x,avMax.y,40),cVector2f(1,0),cColor(1,1.0f));
		mvTexRectVtx[3] = cVertex(cVector3f(avMin.x,avMax.y,40),cVector2f(0,0),cColor(1,1.0f));
		mpLowLevelGraphics->DrawQuad(mvTexRectVtx);
	}

	//-----------------------------------------------------------------------

	// See hpl1_PostEffect_AO.fragment.
	bool cRendererPostEffects::RenderAmbientOcclusion()
	{
		if(mbAoActive==false) return false;
		if(!mpAoVP || !mpAoDeinterleaveFP || !mpAoOcclusionFP || !mpAoFilterFP || !mpAoCompositeFP) return false;

		mvScreenSize = mpLowLevelGraphics->GetRenderSize();
		if(CreateAmbientOcclusionTextures()==false) return false;
		mpLowLevelGraphics->ResolveSceneDepth();

		const cAmbientOcclusionSettings &settings = mAoSettings;
		cVector2l vSize((int)mvScreenSize.x, (int)mvScreenSize.y);
		cVector2f vTileSize((float)mvAoTileSize.x, (float)mvAoTileSize.y);

		///////////////////////////////////////////
		// What is brightly lit is told from the scene itself where it can
		// be read while it is drawn over, or else from a copy of it
		float fLightFade = cMath::Clamp(settings.mfLightFade, 0.0f, 1.0f);
		if(settings.mbShowOnly) fLightFade = 0;
		iGpuProgram *pCompositeFP = mpAoCompositeFP;
		iTexture *pScreenTexture = nullptr;
		if(fLightFade > 0)
		{
			iGpuProgram *pSceneFP = mpAoCompositeSceneFP[mpLowLevelGraphics->GetMultisampling() > 0 ? 1 : 0];
			if(pSceneFP && mpLowLevelGraphics->BeginSceneColorRead())
			{
				pCompositeFP = pSceneFP;
			}
			else
			{
				pScreenTexture = GetFreeScreenTexture();
				if(pScreenTexture && DrawSceneCopy(pScreenTexture)==false) pScreenTexture = nullptr;
				if(pScreenTexture==nullptr) fLightFade = 0;
			}
		}

		mpLowLevelGraphics->SetDepthTestActive(false);
		mpLowLevelGraphics->SetAlphaCutoff(0);
		mpLowLevelGraphics->SetBlendActive(false);
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, true);
		mpLowLevelGraphics->PushMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetIdentityMatrix(eMatrix_ModelView);
		mpLowLevelGraphics->SetOrthoProjection(mpLowLevelGraphics->GetVirtualSize(),-1000,1000);

		mpAoVP->Bind();
		mpAoVP->SetMatrixf("worldViewProj",eGpuProgramMatrix_ViewProjection,eGpuProgramMatrixOp_Identity);

		///////////////////////////////////////////
		// The depth buffer as distances and normals in tiles, a row of
		// four tiles at once
		mpLowLevelGraphics->BeginGpuSection("Depth tiles");
		mpAoDeinterleaveFP->Bind();
		mpLowLevelGraphics->SetTexture(0,nullptr);
		for(int y=0; y<4; ++y)
		{
			iTexture *vTargets[8];
			for(int x=0; x<4; ++x)
			{
				vTargets[x] = mvAoDistanceTexture[x + y * 4];
				vTargets[4 + x] = mvAoNormalTexture[x + y * 4];
			}
			mpLowLevelGraphics->SetRenderTargets(vTargets, 8);
			mpAoDeinterleaveFP->SetFloat("tileRow", (float)y);
			DrawAmbientOcclusionQuad(0, mpLowLevelGraphics->GetVirtualSize());
		}
		mpAoDeinterleaveFP->UnBind();
		mpLowLevelGraphics->EndGpuSection();

		///////////////////////////////////////////
		// The occlusion, tile by tile
		mpLowLevelGraphics->BeginGpuSection("Search");
		int lSlices = cMath::Clamp(settings.mlSlices, 1, 16);
		// The program of the quality, if the samples are those of one
		iGpuProgram *pOcclusionFP = mpAoOcclusionFP;
		int lQuality = settings.GetQuality();
		if(lQuality < cAmbientOcclusionSettings::klQualityNum)
		{
			if(mvAoQualityTried[lQuality]==false)
			{
				const int *pSamples = cAmbientOcclusionSettings::QualitySamples(lQuality);
				char sName[64];
				snprintf(sName, sizeof(sName), "PostEffect_AO_Occlusion_%dx%d_fp.cg", pSamples[0], pSamples[1]);
				mvAoQualityFP[lQuality] = mpGpuManager->CreateProgram(sName,"main",eGpuProgramType_Fragment);
				mvAoQualityTried[lQuality] = true;
			}
			if(mvAoQualityFP[lQuality]) pOcclusionFP = mvAoQualityFP[lQuality];
		}
		pOcclusionFP->Bind();
		pOcclusionFP->SetVec2f("tileSize", vTileSize);
		pOcclusionFP->SetVec4f("reach", cMath::Max(settings.mfRadius, 0.01f),
									cMath::Clamp(settings.mfScreenRadius, 0.0f, 1.0f),
									cMath::Max(settings.mfFadeDistance, 0.01f),
									cMath::Max(settings.mfStrength, 0.0f));
		if(pOcclusionFP == mpAoOcclusionFP)
			pOcclusionFP->SetVec2f("samples", (float)lSlices, (float)cMath::Clamp(settings.mlSteps, 1, 32));
		mpLowLevelGraphics->SetRenderTarget(mpAoTiledTexture);

		// At half rate the tiles are drawn in turns, like the squares of
		// one colour of a chess board: every other pixel of the picture.
		// Not before there is a frame for the others to keep.
		int lTurn = settings.mbHalfRate && mbAoFilled ? (mlAoFrame & 1) : -1;
		// The 16 tiles in an order that spreads any run of them evenly
		// (Bayer): every tile has its own turn of the directions and shift
		// of the steps
		static const int vOrder[16] = {0, 8, 2, 10,  12, 4, 14, 6,  3, 11, 1, 9,  15, 7, 13, 5};
		float fSliceAngle = kPif / (float)lSlices;
		cVector2f vVirtSize = mpLowLevelGraphics->GetVirtualSize();
		cVector2f vQuad(vVirtSize.x / 4.0f, vVirtSize.y / 4.0f);
		for(int y=0; y<4; ++y)
		for(int x=0; x<4; ++x)
		{
			if(lTurn >= 0 && ((x + y) & 1) != lTurn) continue;

			int lOrder = vOrder[x + y * 4];
			float fAngle = (float)lOrder / 16.0f * fSliceAngle;
			float fShift = ((float)((lOrder * 5 + 3) % 16) + 0.5f) / 16.0f;
			pOcclusionFP->SetVec4f("tilePlace", (float)x, (float)y, fShift, 0);
			pOcclusionFP->SetVec4f("tileTurn", cos(fAngle), sin(fAngle), cos(fSliceAngle), sin(fSliceAngle));
			mpLowLevelGraphics->SetTexture(0,mvAoDistanceTexture[x + y * 4]);
			mpLowLevelGraphics->SetTexture(1,mvAoNormalTexture[x + y * 4]);

			// Row 0 of the tiles is the lowest in the texture, and the
			// last in the projection
			cVector2f vMin(vQuad.x * (float)x, vQuad.y * (float)(3 - y));
			DrawAmbientOcclusionQuad(vMin, vMin + vQuad);
		}
		mpLowLevelGraphics->SetTexture(1,nullptr);
		++mlAoFrame;
		mbAoFilled = true;
		pOcclusionFP->UnBind();
		mpLowLevelGraphics->EndGpuSection();

		///////////////////////////////////////////
		// Back to the picture's layout, with the pattern filtered out. The
		// last pass of the filter is the blend's.
		int lFilter = cMath::Clamp(settings.mlFilter, 0, 2);
		iTexture *pOcclusion = mpAoTiledTexture;
		mpLowLevelGraphics->BeginGpuSection("Filter");
		if(lFilter > 0)
		{
			mpAoFilterFP->Bind();
			mpAoFilterFP->SetVec2f("tileSize", vTileSize);
			// The pixels in their places first: the filter's 16 looks would
			// each go to another tile
			mpAoFilterFP->SetVec3f("window", 1, 0, 0);
			DrawAmbientOcclusionPass(mpAoTexture[0], mpAoTiledTexture);
			pOcclusion = mpAoTexture[0];
			if(lFilter > 1)
			{
				mpAoFilterFP->SetVec3f("window", 0, -2, 1);
				DrawAmbientOcclusionPass(mpAoTexture[1], pOcclusion);
				pOcclusion = mpAoTexture[1];
			}
			mpAoFilterFP->UnBind();
		}
		mpLowLevelGraphics->EndGpuSection();

		///////////////////////////////////////////
		// The scene multiplied by it, or replaced by it to show it alone.
		// Destination alpha is the renderer's own.
		mpLowLevelGraphics->BeginGpuSection("Blend");
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, false);
		if(settings.mbShowOnly==false)
		{
			mpLowLevelGraphics->SetBlendActive(true);
			mpLowLevelGraphics->SetBlendFunc(eBlendFunc_Zero,eBlendFunc_SrcColor);
		}
		pCompositeFP->Bind();
		pCompositeFP->SetVec2f("tileSize", vTileSize);
		// Of two passes the second window is the first mirrored: together
		// they are centred on the pixel
		if(lFilter == 0)	pCompositeFP->SetVec3f("window", 1, 0, 0);
		else				pCompositeFP->SetVec3f("window", 0, lFilter > 1 ? -1.0f : -2.0f, 1);
		pCompositeFP->SetFloat("lightFade", fLightFade);
		mpLowLevelGraphics->SetRenderTarget(nullptr);
		if(pScreenTexture) mpLowLevelGraphics->SetTexture(1,pScreenTexture);
		if(pCompositeFP != mpAoCompositeFP) mpLowLevelGraphics->BeginSceneColorRead();
		DrawAmbientOcclusionPass(nullptr, pOcclusion);
		pCompositeFP->UnBind();
		mpLowLevelGraphics->EndGpuSection();

		mpAoVP->UnBind();
		mpLowLevelGraphics->SetBlendActive(false);
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, true);
		mpLowLevelGraphics->SetTexture(0,nullptr);
		mpLowLevelGraphics->SetTexture(1,nullptr);
		mpLowLevelGraphics->PopMatrix(eMatrix_ModelView);

		return true;
	}

	//-----------------------------------------------------------------------

	// The scene into apTarget, a screen buffer, to be read while the scene
	// is drawn over.
	bool cRendererPostEffects::DrawSceneCopy(iTexture *apTarget)
	{
		return mpLowLevelGraphics->DrawSceneCopy(apTarget, 0, cVector2l((int)mvScreenSize.x,(int)mvScreenSize.y));
	}

	//-----------------------------------------------------------------------

}
