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
#include "graphics/Renderer3D.h"

#include "math/Math.h"
#include "graphics/Texture.h"
#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "resources/Resources.h"
#include "resources/LowLevelResources.h"
#include "resources/TextureManager.h"
#include "graphics/VertexBuffer.h"
#include "graphics/MeshCreator.h"
#include "scene/Camera3D.h"
#include "scene/Entity3D.h"
#include "graphics/RenderList.h"
#include "graphics/Renderable.h"
#include "scene/World3D.h"
#include "scene/MeshEntity.h"
#include "scene/SubMeshEntity.h"
#include "scene/RenderableContainer.h"
#include "scene/Light3D.h"
#include "scene/Light3DSpot.h"
#include "math/BoundingVolume.h"
#include "resources/GpuProgramManager.h"
#include "graphics/GPUProgram.h"
#include "graphics/RendererPostEffects.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// RENDER SETTINGS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cRenderSettings::cRenderSettings()
	{
		mbFogActive = false;
		mfFogStart = 5.0f;
		mfFogEnd = 5.0f;
		mFogColor = cColor(1,1);
		mbFogCulling = false;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cRenderer3D::cRenderer3D(iLowLevelGraphics *apLowLevelGraphics,cResources* apResources,
							cMeshCreator* apMeshCreator, cRenderList *apRenderList)
	{
		Log("  Creating Renderer3D\n");


		mpLowLevelGraphics = apLowLevelGraphics;
		mpLowLevelResources = apResources->GetLowLevel();
		mpResources = apResources;
		mpMeshCreator = apMeshCreator;

		mpSkyBoxTexture = nullptr;
		mbAutoDestroySkybox = false;
		mbSkyBoxActive = false;
		mSkyBoxColor = cColor(1,1);

		mRenderSettings.mAmbientColor = cColor(0,1);

		mpRenderList = apRenderList;

		mDebugFlags = 0;

		mbLog = false;

		mfRenderTime =0;

		mbRefractionUsed = true;

		mvVtxRect.resize(4);
		mvVtxRect[0] = cVertex(cVector3f(0,0,0),cVector2f(0,1),cColor(1,1) ) ;
		mvVtxRect[1] = cVertex(cVector3f(1,0,0),cVector2f(1,1),cColor(1,1)) ;
		mvVtxRect[2] = cVertex(cVector3f(1,1,0),cVector2f(1,0),cColor(1,1)) ;
		mvVtxRect[3] = cVertex(cVector3f(0,1,40),cVector2f(0,0),cColor(1,1));


		//Set up render settings.
		mRenderSettings.mpLowLevel = mpLowLevelGraphics;
		mRenderSettings.mbLog = false;
		mRenderSettings.mShowShadows = eRendererShowShadows_All;

		Log("   Load Renderer3D gpu programs:\n");

		cGpuProgramManager *pProgramManager = apResources->GetGpuProgramManager();

		///////////////////////////////////
		//Load diffuse program, for stuff like query rendering
		Log("    Diffuse Vertex\n");
		mpDiffuseVtxProgram = pProgramManager->CreateProgram("Diffuse_Color_vp.cg","main",
																eGpuProgramType_Vertex);
		if(mpDiffuseVtxProgram==nullptr)
		{
			Error("Couldn't load 'Diffuse_Color_vp.cg'\n");
		}

		///////////////////////////////////
		//Load diffuse frag program, used where possible. Else fixed funcion.
		Log("    Diffuse Fragment\n");
		mpDiffuseFragProgram = pProgramManager->CreateProgram("Diffuse_Color_fp.cg","main",	eGpuProgramType_Fragment);

		///////////////////////////////////
		//Fog Vertex Program Init
		Log("    Fog\n");
		mpSolidFogVtxProgram = pProgramManager->CreateProgram("Fog_Solid_vp.cg","main",eGpuProgramType_Vertex);
		if(mpSolidFogVtxProgram==nullptr)
			Error("Couldn't load 'Fog_Solid_vp.cg'\n");

		mpSolidFogFragProgram = pProgramManager->CreateProgram("Fog_Solid_fp.cg","main",eGpuProgramType_Fragment);


		///////////////////////////////////
		//Fog Texture Init

		Log("   Creating fog textures: ");
		unsigned char* pFogArray = hplNewArray(unsigned char,256 *2);


		//Solid
		Log("Solid ");
		iTexture *pTex = mpLowLevelGraphics->CreateTexture("FogLinearSolid",
														false,eTextureType_Normal,eTextureTarget_1D);

		for(int i=0; i<256; ++i) {
			pFogArray[i*2 + 0] = 255;
			pFogArray[i*2 + 1] = 255 - (unsigned char)i;
		}

		pTex->CreateFromArray(pFogArray,2,cVector3l(256,1,1));
		pTex->SetWrapR(eTextureWrap_ClampToEdge);
		pTex->SetWrapS(eTextureWrap_ClampToEdge);
		pTex->SetWrapT(eTextureWrap_ClampToEdge);

		mpFogLinearSolidTexture = pTex;

		//Additive
		Log("Additive ");
		pTex = mpLowLevelGraphics->CreateTexture("FogLinearAdd",
												false,eTextureType_Normal,eTextureTarget_1D);
		for(int i=0; i<256; ++i) {
			pFogArray[i*2 + 0] = (unsigned char)i;
			pFogArray[i*2 + 1] = (unsigned char)i;
		}

		pTex->CreateFromArray(pFogArray,2,cVector3l(256,1,1));
		pTex->SetWrapR(eTextureWrap_ClampToEdge); pTex->SetWrapS(eTextureWrap_ClampToEdge);

		mpFogLinearAddTexture = pTex;

		//Alpha
		Log("Alpha ");
		pTex = mpLowLevelGraphics->CreateTexture("FogLinearAlpha",
													false,eTextureType_Normal,eTextureTarget_1D);
		for(int i=0; i<256; ++i) {
			pFogArray[i*2 + 0] = 255;
			pFogArray[i*2 + 1] = (unsigned char)i;
		}

		pTex->CreateFromArray(pFogArray,2,cVector3l(256,1,1));
		pTex->SetWrapR(eTextureWrap_ClampToEdge); pTex->SetWrapS(eTextureWrap_ClampToEdge);

		mpFogLinearAlphaTexture = pTex;

		hplDeleteArray(pFogArray);

		Log("\n");

		/////////////////////////////////////////////
		//Create Refraction programs
		mbRefractionAvailable = true;

		mpRefractVtxProgram = pProgramManager->CreateProgram("refract_vp.cg","main",eGpuProgramType_Vertex);
		mpRefractFragProgram = pProgramManager->CreateProgram("refract_fp.cg","main",eGpuProgramType_Fragment);
		mpRefractSpecFragProgram = pProgramManager->CreateProgram("refract_special_fp.cg","main",eGpuProgramType_Fragment);

		if(mpRefractFragProgram==nullptr || mpRefractVtxProgram==nullptr || mpRefractSpecFragProgram==nullptr)
		{
			mbRefractionAvailable = false;
			Log("   Refraction will not be supported!\n");
		}


		/////////////////////////////////////////////
		//Create sky box graphics.

		Log("   init sky box\n");
		InitSkyBox();

		Log("  Renderer3D created\n");
	}

	//-----------------------------------------------------------------------

	cRenderer3D::~cRenderer3D()
	{
		if(mpDiffuseVtxProgram) mpResources->GetGpuProgramManager()->Destroy(mpDiffuseVtxProgram);
		if(mpDiffuseFragProgram) mpResources->GetGpuProgramManager()->Destroy(mpDiffuseFragProgram);

		if(mpSolidFogVtxProgram)mpResources->GetGpuProgramManager()->Destroy(mpSolidFogVtxProgram);
		if(mpSolidFogFragProgram)mpResources->GetGpuProgramManager()->Destroy(mpSolidFogFragProgram);

		if(mpRefractVtxProgram)mpResources->GetGpuProgramManager()->Destroy(mpRefractVtxProgram);
		if(mpRefractFragProgram)mpResources->GetGpuProgramManager()->Destroy(mpRefractFragProgram);
		if(mpRefractSpecFragProgram)mpResources->GetGpuProgramManager()->Destroy(mpRefractSpecFragProgram);


		if(mpSkyBox) hplDelete(mpSkyBox);
		if(mpSkyBoxTexture && mbAutoDestroySkybox)
		{
			mpResources->GetTextureManager()->Destroy(mpSkyBoxTexture);
		}

		if(mpFogLinearSolidTexture) hplDelete(mpFogLinearSolidTexture);
		if(mpFogLinearAddTexture) hplDelete(mpFogLinearAddTexture);
		if(mpFogLinearAlphaTexture) hplDelete(mpFogLinearAlphaTexture);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cRenderSettings::Clear()
	{
		mbDepthTest = true;

		mAlphaMode = eMaterialAlphaMode_Solid;
		mBlendMode = eMaterialBlendMode_None;
		mChannelMode = eMaterialChannelMode_RGBA;

		mpVertexProgram = nullptr;
		mbVertexUseLight = false;
		mpFragmentProgram = nullptr;

		mpSector = nullptr;

		mbUsesLight = false;
		mbUsesEye = false;

		mbMatrixWasNULL = false;

		for(int i=0;i<MAX_TEXTUREUNITS;i++)
		{
			mpTexture[i] = nullptr;
			mTextureBlend[i] = eMaterialBlendMode_None;
		}

		mpVtxBuffer = nullptr;
	}

	void cRenderSettings::Reset(iLowLevelGraphics *apLowLevel)
	{
		if(mpVertexProgram)mpVertexProgram->UnBind();
		if(mpFragmentProgram)mpFragmentProgram->UnBind();
		if(mpVtxBuffer) mpVtxBuffer->UnBind();

		for(int i=0;i<MAX_TEXTUREUNITS;i++)
		{
			if(mpTexture[i])
			{
				apLowLevel->SetTexture(i,nullptr);
			}
		}

		Clear();
	}

	//-----------------------------------------------------------------------

	eRendererShowShadows cRenderer3D::GetShowShadows()
	{
		return mRenderSettings.mShowShadows;
	}
	void cRenderer3D::SetShowShadows(eRendererShowShadows aState)
	{
		mRenderSettings.mShowShadows = aState;
	}

	//-----------------------------------------------------------------------


	void cRenderer3D::UpdateRenderList(cWorld3D* apWorld, cCamera3D* apCamera, float afFrameTime)
	{
		//Clear all objects to be rendereded
		mpRenderList->Clear();

		//Set some variables
		mpRenderList->SetFrameTime(afFrameTime);
		mpRenderList->SetCamera(apCamera);

		//Set the frustum
		mRenderSettings.mpFrustum = apCamera->GetFrustum();

		//Setup fog BV
		if(mRenderSettings.mbFogActive && mRenderSettings.mbFogCulling)
		{
			//This is becuase the fog line is a stright line infront of the camera.
			// The distance to a corner of the view at the fog end. This was
			// 2 * end / cos(fov * aspect / 2), which stops working when that
			// angle gets near 90 degrees (wide windows, large FOV).
			float fTanY = tan(apCamera->GetScaledFOV()*0.5f);
			float fTanX = fTanY * apCamera->GetAspect();
			float fCornerDist = (mRenderSettings.mfFogEnd *2.0f) * sqrt(1.0f + fTanX*fTanX + fTanY*fTanY);

			mFogBV.SetSize(fCornerDist);
			mFogBV.SetPosition(apCamera->GetPosition());
		}

		//Add all objects to be rendered
		apWorld->GetRenderContainer()->GetVisible(mRenderSettings.mpFrustum, mpRenderList);

		//Compile an optimized render list.
		mpRenderList->Compile();
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderWorld(cWorld3D* apWorld, cCamera3D* apCamera, float afFrameTime)
	{
		mfRenderTime += afFrameTime;

		//////////////////////////////
		//Setup render settings and logging
		if(mDebugFlags & eRendererDebugFlag_LogRendering){
			mbLog = true;
			mRenderSettings.mbLog = true;
		}
		else if(mbLog)
		{
			mbLog = false;
			mRenderSettings.mbLog = false;
		}
		mRenderSettings.mDebugFlags = mDebugFlags;

		/////////////////////////////////
		//Set up rendering
		BeginRendering(apCamera);

		mRenderSettings.Clear();

		////////////////////////////
		// Render Z
		mpLowLevelGraphics->SetColorWriteActive(true, true, true,true);
		mRenderSettings.mChannelMode = eMaterialChannelMode_RGBA;

		if(mbLog) Log("Rendering ZBuffer:\n");
		mpLowLevelGraphics->BeginGpuSection("Z");
		RenderZ(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		////////////////////////////
		//Render Occlusion Queries
		mpLowLevelGraphics->SetColorWriteActive(false, false, false,false);
		mRenderSettings.mChannelMode = eMaterialChannelMode_Z;

		if(mbLog) Log("Rendering Occlusion Queries:\n");
		mpLowLevelGraphics->SetDepthWriteActive(false);
		mpLowLevelGraphics->BeginGpuSection("Occlusion queries");
		RenderOcclusionQueries(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		////////////////////////////
		//Render lighting
		mRenderSettings.mChannelMode = eMaterialChannelMode_RGBA;
		mpLowLevelGraphics->SetColorWriteActive(true, true, true,true);

		mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_Equal);

		if(mbLog) Log("Rendering Lighting:\n");
		mpLowLevelGraphics->BeginGpuSection("Lights");
		RenderLight(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		////////////////////////////
		//Ambient occlusion: darkens what is lit, before what glows by
		//itself, the fog and what is see-through are drawn
		if(mbLog) Log("Rendering Ambient occlusion:\n");
		mpLowLevelGraphics->BeginGpuSection("Ambient occlusion");
		bool bOcclusionOnly = RenderAmbientOcclusion(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		////////////////////////////
		//Render Diffuse
		if(mbLog) Log("Rendering Diffuse:\n");
		mpLowLevelGraphics->BeginGpuSection("Diffuse");
		if(bOcclusionOnly==false) RenderDiffuse(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		////////////////////////////
		//Render fog
		if(mbLog) Log("Rendering fog:\n");
		mpLowLevelGraphics->BeginGpuSection("Fog");
		if(bOcclusionOnly==false) RenderFog(apCamera);
		mpLowLevelGraphics->EndGpuSection();


		////////////////////////////
		//Render sky box
		mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_LessOrEqual);

		////////////////////////////
		if(mbLog) Log("Rendering Skybox:\n");
		mpLowLevelGraphics->BeginGpuSection("Sky box");
		if(bOcclusionOnly==false) RenderSkyBox(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		//Render transparent
		if(mbLog) Log("Rendering Transperant:\n");
		mpLowLevelGraphics->BeginGpuSection("Transparent");
		{
			// Their shaders read the depth of what is behind them
			const cEffectSettings &effects = mpLowLevelGraphics->GetEffects();
			if(effects.mbHalos || effects.mbShafts || effects.mbParticles) mpLowLevelGraphics->ResolveSceneDepth();
		}
		if(bOcclusionOnly==false) RenderTrans(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		mRenderSettings.Reset(mpLowLevelGraphics);

		////////////////////////////
		//Render debug
		mpLowLevelGraphics->BeginGpuSection("Debug");
		RenderDebug(apCamera);
		mpLowLevelGraphics->EndGpuSection();

		mpLowLevelGraphics->SetDepthWriteActive(true);
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::SetSkyBox(iTexture *apTexture, bool abAutoDestroy)
	{
		if(mpSkyBoxTexture && mbAutoDestroySkybox)
		{
			mpResources->GetTextureManager()->Destroy(mpSkyBoxTexture);
		}

		mbAutoDestroySkybox = abAutoDestroy;
		mpSkyBoxTexture = apTexture;
		if(mpSkyBoxTexture)
		{
			mpSkyBoxTexture->SetWrapS(eTextureWrap_ClampToEdge);
			mpSkyBoxTexture->SetWrapT(eTextureWrap_ClampToEdge);
		}
	}

	void cRenderer3D::SetSkyBoxActive(bool abX)
	{
		mbSkyBoxActive = abX;
	}

	void cRenderer3D::SetSkyBoxColor(const cColor& aColor)
	{
		if(mSkyBoxColor == aColor) return;

		mSkyBoxColor = aColor;

		float *pColors = mpSkyBox->GetArray(eVertexFlag_Color0);

		int lNum = mpSkyBox->GetVertexNum();
		for(int i=0; i<lNum;++i)
		{
			pColors[0] = mSkyBoxColor.r;
			pColors[1] = mSkyBoxColor.g;
			pColors[2] = mSkyBoxColor.b;
			pColors[3] = mSkyBoxColor.a;
			pColors+=4;
		}

		mpSkyBox->UpdateData(eVertexFlag_Color0,false);
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::SetFogActive(bool abX)
	{
		if(mpSolidFogVtxProgram)
			mRenderSettings.mbFogActive = abX;
	}
	void cRenderer3D::SetFogStart(float afX)
	{
		mRenderSettings.mfFogStart = afX;
	}

	void cRenderer3D::SetFogEnd(float afX)
	{
		mRenderSettings.mfFogEnd = afX;
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::FetchOcclusionQueries()
	{
		if(mbLog) Log("Fetching Occlusion Queries Result:\n");

		//With depth test
		cOcclusionQueryObjectIterator it = mpRenderList->GetQueryIterator();
		while(it.HasNext())
		{
			cOcclusionQueryObject *pObject = it.Next();

			// Takes the newest finished result, without waiting
			pObject->mpQuery->FetchResults();

			if(mbLog) Log(" Query: %d SampleCount: %d\n",	pObject->mpQuery,
															pObject->mpQuery->GetSampleCount());
		}

		if(mbLog) Log("Done fetching queries\n");
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cRenderer3D::BeginRendering(cCamera3D* apCamera)
	{
		//////////////////////////////////////////////////
		/////Setup for clearing the screen
		//turn of scissor test.
		mpLowLevelGraphics->SetScissorActive(false);

		mpLowLevelGraphics->SetClearColor(cColor(0,0));
		mpLowLevelGraphics->SetClearColorActive(true);
		mpLowLevelGraphics->SetClearDepthActive(true);
		mpLowLevelGraphics->SetClearDepth(1);

		mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_LessOrEqual);

		mpLowLevelGraphics->ClearScreen();

		//////////////////////////////////////////////////
		// Cull and depth mode.
		mpLowLevelGraphics->SetCullActive(true);

		mpLowLevelGraphics->SetCullMode(eCullMode_CounterClockwise);
		mpLowLevelGraphics->SetDepthTestActive(true);

		mpLowLevelGraphics->SetMatrix(eMatrix_Projection, apCamera->GetProjectionMatrix());
		mpLowLevelGraphics->SetFrameUniforms(apCamera->GetViewMatrix(), apCamera->GetProjectionMatrix(),
											apCamera->GetPosition(), apCamera->GetNearClipPlane(),
											apCamera->GetFarClipPlane(), mfRenderTime);

		mRenderSettings.mpCamera = apCamera;

		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
			mpLowLevelGraphics->SetTexture(i,nullptr);
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::InitSkyBox()
	{
		mpSkyBox = mpMeshCreator->CreateSkyBoxVertexBuffer(1);
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderFog(cCamera3D *apCamera)
	{
		if(mRenderSettings.mbFogActive==false || mpSolidFogVtxProgram==nullptr) return;
		int i;
		//////////////////////////////////
		//Set textures to NULL
		for(i=0; i<MAX_TEXTUREUNITS; ++i)
		{
			if(mRenderSettings.mpTexture[i])
			{
				mpLowLevelGraphics->SetTexture(i,nullptr);
				mRenderSettings.mpTexture[i] = nullptr;
			}
		}


		//////////////////////////////////
		//Set vertex program
		if(mRenderSettings.mpVertexProgram) mRenderSettings.mpVertexProgram->UnBind();
		mRenderSettings.mpVertexProgram = nullptr;
		mpSolidFogVtxProgram->Bind();

		mpSolidFogVtxProgram->SetColor3f("fogColor",mRenderSettings.mFogColor);
		mpSolidFogVtxProgram->SetFloat("fogStart",mRenderSettings.mfFogStart);
		mpSolidFogVtxProgram->SetFloat("fogEnd",mRenderSettings.mfFogEnd);


		//////////////////////////////////
		//Set fragment program
		if(mRenderSettings.mpFragmentProgram) mRenderSettings.mpFragmentProgram->UnBind();
		if(mpSolidFogFragProgram) mpSolidFogFragProgram->Bind();
		mRenderSettings.mpFragmentProgram = nullptr;


		//////////////////////////////////
		// Blend mode
		mpLowLevelGraphics->SetBlendActive(true);
		mpLowLevelGraphics->SetBlendFunc(eBlendFunc_SrcAlpha, eBlendFunc_OneMinusSrcAlpha);
		mRenderSettings.mBlendMode = eMaterialBlendMode_LastEnum;

		//////////////////////////////////////
		// Texture
		mpLowLevelGraphics->SetTexture(0,mpFogLinearSolidTexture);
		mRenderSettings.mpTexture[0] = mpFogLinearSolidTexture;


		//////////////////////////////////
		// Render objects
		cFogObjectIterator it = mpRenderList->GetFogIterator();

		while(it.HasNext())
		{
			iRenderable *pObject = it.Next();
			cMatrixf *pMtx = pObject->GetModelMatrix(apCamera);

			//////////////////
			//Non static models
			if(pMtx)
			{
				mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,cMath::MatrixMul(apCamera->GetViewMatrix(),
																				*pMtx));

				mpSolidFogVtxProgram->SetMatrixf("worldViewProj",	eGpuProgramMatrix_ViewProjection,
																eGpuProgramMatrixOp_Identity);
			}
			//////////////////
			//NULL Model view matrix (static)
			else
			{
				mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());

				mpSolidFogVtxProgram->SetMatrixf("worldViewProj",	eGpuProgramMatrix_ViewProjection,
																eGpuProgramMatrixOp_Identity);
			}

			pObject->GetVertexBuffer()->Bind();
			pObject->GetVertexBuffer()->Draw();
			pObject->GetVertexBuffer()->UnBind();
		}

		mpSolidFogVtxProgram->UnBind();
		if(mpSolidFogFragProgram) mpSolidFogFragProgram->UnBind();
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderSkyBox(cCamera3D *apCamera)
	{
		if(mbSkyBoxActive==false) return;

		if(mbLog) Log(" Drawing skybox\n");

		if(mRenderSettings.mpVertexProgram)
		{
			mRenderSettings.mpVertexProgram->UnBind();
			mRenderSettings.mpVertexProgram = nullptr;
			if(mbLog) Log(" Setting Vertex program: NULL\n");
		}
		if(mRenderSettings.mpFragmentProgram)
		{
			mRenderSettings.mpFragmentProgram->UnBind();
			mRenderSettings.mpFragmentProgram = nullptr;
			if(mbLog) Log(" Setting Fragment program: NULL\n");
		}
		if(mRenderSettings.mpVtxBuffer)
		{
			mRenderSettings.mpVtxBuffer->UnBind();
			mRenderSettings.mpVtxBuffer = nullptr;
			if(mbLog) Log(" Setting Vertex Buffer: NULL\n");
		}

		for(int i=1; i<MAX_TEXTUREUNITS; ++i)
		{
			if(mRenderSettings.mpTexture[i])
			{
				mpLowLevelGraphics->SetTexture(i,nullptr);
				mRenderSettings.mpTexture[i] = nullptr;
				if(mbLog) Log(" Setting Texture %d : NULL\n",i);
			}
		}

		mRenderSettings.mbMatrixWasNULL = false;

		cMatrixf mtxSky = cMatrixf::Identity;

		//Calculate the size of the sky box need to just touch the far clip plane.
		float fFarClip = apCamera->GetFarClipPlane();
		float fSide = sqrt((fFarClip*fFarClip) / 3) *0.95f;
		mtxSky.m[0][0] = fSide;
		mtxSky.m[1][1] = fSide;
		mtxSky.m[2][2] = fSide;

		mtxSky = cMath::MatrixMul(mtxSky,apCamera->GetViewMatrix());

		mtxSky.SetTranslation(0);

		mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,mtxSky);

		mpLowLevelGraphics->SetTexture(0,mpSkyBoxTexture);
		mRenderSettings.mpTexture[0] = mpSkyBoxTexture;

		mpLowLevelGraphics->SetBlendActive(false);
		mRenderSettings.mBlendMode = eMaterialBlendMode_Replace;

		mpSkyBox->Bind();
		mpSkyBox->Draw();
		mpSkyBox->UnBind();
	}
	//-----------------------------------------------------------------------

	void cRenderer3D::RenderZ(cCamera3D *apCamera)
	{
		cRenderNode* pNode = mpRenderList->GetRootNode(eRenderListDrawType_Normal,
														eMaterialRenderType_Z, 0);

		mRenderSettings.mpLight = nullptr;
		pNode->Render(&mRenderSettings);

		// The improved ambient pass reads more than the diffuse texture;
		// those units are emptied again for what is drawn next.
		for(int i=1; i<MAX_TEXTUREUNITS; ++i)
		{
			if(mRenderSettings.mpTexture[i]==nullptr) continue;
			mpLowLevelGraphics->SetTexture(i,nullptr);
			mRenderSettings.mpTexture[i] = nullptr;
		}
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderOcclusionQueries(cCamera3D *apCamera)
	{
		//////////////////////////////////////////////////
		// Reset any vertex buffers,fragment or vertex programs.
		if(mRenderSettings.mpFragmentProgram) mRenderSettings.mpFragmentProgram->UnBind();

		mRenderSettings.mpFragmentProgram = nullptr;

		////////////////////////////
		// Vertex program
		if(mRenderSettings.mpVertexProgram != mpDiffuseVtxProgram)
		{
			if(mRenderSettings.mpVertexProgram)mRenderSettings.mpVertexProgram->UnBind();
			mRenderSettings.mpVertexProgram = mpDiffuseVtxProgram;

			mpDiffuseVtxProgram->Bind();
			if(mbLog) Log(" Binding vertex program %d\n",mpDiffuseVtxProgram);
		}

		////////////////////////
		// Reset texture unit 0
		mpLowLevelGraphics->SetTexture(0,nullptr);
		mRenderSettings.mpTexture[0] = nullptr;

		////////////////////////
		// Keep track of what has been set
		iVertexBuffer *pPrevBuffer = mRenderSettings.mpVtxBuffer;
		cMatrixf *pPrevMatrix = nullptr;
		bool bFirstRound = true;
		bool bPrevDepthTest = true;

		//////////////////////////////////
		//Iterate the query objects
		cOcclusionQueryObjectIterator it = mpRenderList->GetQueryIterator();
		while(it.HasNext())
		{
			cOcclusionQueryObject *pObject = it.Next();


			/////////////////////
			//Set depth test
			if(bPrevDepthTest != pObject->mbDepthTest)
			{
				if(pObject->mbDepthTest)
					mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_LessOrEqual);
				else
					mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_Always);

				bPrevDepthTest = pObject->mbDepthTest;
				if(mbLog) Log(" Setting depth test %d\n",pObject->mbDepthTest?1:0);
			}

			/////////////////////
			//Set matrix
			if(pPrevMatrix != pObject->mpMatrix || bFirstRound)
			{
				if(pObject->mpMatrix)
				{
					mpLowLevelGraphics->SetMatrix(eMatrix_ModelView, cMath::MatrixMul(apCamera->GetViewMatrix(),*pObject->mpMatrix));
				}
				else
				{
					mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());
				}
				pPrevMatrix = pObject->mpMatrix;
				//Set the vertex program matrix.
				mpDiffuseVtxProgram->SetMatrixf("worldViewProj",eGpuProgramMatrix_ViewProjection,
																eGpuProgramMatrixOp_Identity);

				if(mbLog) Log(" Setting matrix %d\n",pObject->mpMatrix);
			}
			/////////////////////
			//Set Vertex buffer and draw
			if(pPrevBuffer != pObject->mpVtxBuffer)
			{
				if(pPrevBuffer) pPrevBuffer->UnBind();
				pObject->mpVtxBuffer->Bind();
				pPrevBuffer = pObject->mpVtxBuffer;

				if(mbLog) Log(" Setting vtx buffer %d\n",pObject->mpVtxBuffer);
			}

			pObject->mpQuery->Begin();
			pObject->mpVtxBuffer->Draw();
			pObject->mpQuery->End();

			if(mbLog) Log(" Render with query: %d\n",pObject->mpQuery);

			bFirstRound = false;
		}

		mRenderSettings.mpVtxBuffer = pPrevBuffer;


	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderLight(cCamera3D *apCamera)
	{
		mShadowMapStats = cShadowMapStats();

		if(mDebugFlags & eRendererDebugFlag_DisableLighting) return;

		UpdateShadowCasterGraphics(apCamera);

		cLight3DIterator lightIt = mpRenderList->GetLightIt();

		int lLightCount=0;
		while(lightIt.HasNext())
		{
			if(lLightCount >= MAX_NUM_OF_LIGHTS) break;

			iLight3D* pLight = lightIt.Next();

			if(mpRenderList->GetLightObjects(lLightCount)==0 ||
				(mShadowMapSettings.msOnlyLight != "" && pLight->GetName() != mShadowMapSettings.msOnlyLight))
			{
				lLightCount++;
				continue;
			}

			if(mbLog) Log("-----Light %s/%d ------\n",pLight->GetName().c_str(), (size_t)pLight);

			// The light's shadow map, which its passes read
			cShadowMapParams shadowParams;
			shadowParams.mbShowOnly = mShadowMapSettings.mbShowOnly;
			RenderShadowMap(pLight, apCamera, shadowParams);
			mpLowLevelGraphics->SetShadowMapParams(shadowParams);
			mpLowLevelGraphics->SetLightLook(pLight->GetLook());

			cRenderNode* pNode = mpRenderList->GetRootNode(eRenderListDrawType_Normal,
															eMaterialRenderType_Light, lLightCount);

			if(pLight->BeginDraw(&mRenderSettings, mpLowLevelGraphics))
			{
				pNode->Render(&mRenderSettings);
			}
			pLight->EndDraw(&mRenderSettings, mpLowLevelGraphics);

			lLightCount++;
		}

		mpLowLevelGraphics->SetShadowMapParams(cShadowMapParams());
	}

	//-----------------------------------------------------------------------

	/**
	 * Brings the moving things that cast a shadow into the picture without
	 * being in it themselves to the pose they have now. The render list does
	 * this for what it holds (cRenderList::Add); an animated mesh that is not
	 * in it would cast the shadow of the pose it was last seen in.
	 */
	void cRenderer3D::UpdateShadowCasterGraphics(cCamera3D *apCamera)
	{
		if(mRenderSettings.mShowShadows != eRendererShowShadows_All) return;

		const int lRenderCount = mpRenderList->GetRenderCount();

		cLight3DIterator lightIt = mpRenderList->GetLightIt();
		while(lightIt.HasNext())
		{
			iLight3D *pLight = lightIt.Next();
			if(pLight->GetCastShadows()==false) continue;

			for(iRenderable *pObject : *pLight->GetDynamicCasters())
			{
				// The count of what is in the list, and of what was done here
				// for another light
				if(pObject->GetRenderCount() == lRenderCount) continue;
				if(pObject->GetEntityType() != "SubMesh") continue;

				// The bones first, then the vertices they move
				cMeshEntity *pMesh = static_cast<cSubMeshEntity*>(pObject)->GetMeshEntity();
				if(pMesh->GetRenderCount() != lRenderCount)
				{
					pMesh->SetRenderCount(lRenderCount);
					pMesh->UpdateGraphics(apCamera, 0, mpRenderList);
				}
				pObject->SetRenderCount(lRenderCount);
				pObject->UpdateGraphics(apCamera, 0, mpRenderList);
			}
		}
	}

	//-----------------------------------------------------------------------

	// The directions the faces of a cube map look in, and what is up in them.
	static const cVector3f gvCubeFaceDir[6] = {
		cVector3f(1,0,0), cVector3f(-1,0,0), cVector3f(0,1,0),
		cVector3f(0,-1,0), cVector3f(0,0,1), cVector3f(0,0,-1)};
	static const cVector3f gvCubeFaceUp[6] = {
		cVector3f(0,-1,0), cVector3f(0,-1,0), cVector3f(0,0,1),
		cVector3f(0,0,-1), cVector3f(0,-1,0), cVector3f(0,-1,0)};

	static cMatrixf CubeFaceViewMatrix(int alFace, const cVector3f &avPos)
	{
		const cVector3f &vForward = gvCubeFaceDir[alFace];
		cVector3f vRight = cMath::Vector3Cross(vForward, gvCubeFaceUp[alFace]);
		cVector3f vUp = cMath::Vector3Cross(vRight, vForward);

		return cMatrixf(
			vRight.x, vRight.y, vRight.z, -cMath::Vector3Dot(vRight, avPos),
			vUp.x, vUp.y, vUp.z, -cMath::Vector3Dot(vUp, avPos),
			-vForward.x, -vForward.y, -vForward.z, cMath::Vector3Dot(vForward, avPos),
			0,0,0,1);
	}

	// A view of 90 degrees, as wide as high.
	static cMatrixf CubeFaceProjectionMatrix(float afNear, float afFar)
	{
		return cMatrixf(
			1,0,0,0,
			0,1,0,0,
			0,0,-(afFar + afNear)/(afFar - afNear), -(2.0f*afFar*afNear)/(afFar - afNear),
			0,0,-1,0);
	}

	// If a sphere, given relative to the middle of a cube map, shows in a face.
	static bool SphereInCubeFace(int alFace, const cVector3f &avCenter, float afRadius)
	{
		const cVector3f &vDir = gvCubeFaceDir[alFace];
		float fAlong = cMath::Vector3Dot(vDir, avCenter);
		if(fAlong < -afRadius) return false;

		// The sides of the face's view are at 45 degrees to its direction
		float fReach = afRadius * 1.4142136f;
		for(int i=0; i<3; ++i)
		{
			if(vDir.v[i] != 0) continue;
			if(fAlong - std::abs(avCenter.v[i]) < -fReach) return false;
		}
		return true;
	}

	//-----------------------------------------------------------------------

	/**
	 * Draws what casts shadows from a light into the light's shadow map and
	 * fills in what the light passes need to read it. Leaves the render
	 * settings telling what is set. False if the light has no shadow map.
	 */
	bool cRenderer3D::RenderShadowMap(iLight3D *apLight, cCamera3D *apCamera, cShadowMapParams &aParams)
	{
		if(apLight->GetCastShadows()==false || mRenderSettings.mShowShadows == eRendererShowShadows_None ||
			mpDiffuseVtxProgram==nullptr || mpDiffuseFragProgram==nullptr)
		{
			return false;
		}

		const cShadowMapSettings &settings = mShadowMapSettings;
		const bool bCube = apLight->GetLightType() == eLight3DType_Point;
		// As large as BeginShadowMap() makes it
		const int lSize = cMath::Clamp(bCube ? settings.mlPointSize : settings.mlSpotSize, 16, 8192);
		const float fFar = apLight->GetFarAttenuation();

		mpLowLevelGraphics->BeginGpuSection("Shadow maps");
		if(mbLog) Log("Rendering shadow map:\n");

		///////////////////////////////
		// Programs and state
		if(mRenderSettings.mpVertexProgram != mpDiffuseVtxProgram)
		{
			if(mRenderSettings.mpVertexProgram) mRenderSettings.mpVertexProgram->UnBind();
			mpDiffuseVtxProgram->Bind();
			mRenderSettings.mpVertexProgram = mpDiffuseVtxProgram;
		}
		if(mRenderSettings.mpFragmentProgram != mpDiffuseFragProgram)
		{
			if(mRenderSettings.mpFragmentProgram) mRenderSettings.mpFragmentProgram->UnBind();
			mpDiffuseFragProgram->Bind();
			mRenderSettings.mpFragmentProgram = mpDiffuseFragProgram;
		}

		mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_LessOrEqual);
		mpLowLevelGraphics->SetCullActive(settings.mFaces != eShadowMapFaces_Both);
		if(settings.mFaces == eShadowMapFaces_AwayFromLight)
			mpLowLevelGraphics->SetCullMode(eCullMode_Clockwise);

		///////////////////////////////
		// The maps
		mShadowMapStats.mlLights++;

		if(bCube)
		{
			const cVector3f vPos = apLight->GetLightPosition();
			const float fNear = cMath::Min(settings.mfPointNear, fFar * 0.5f);
			mpLowLevelGraphics->SetMatrix(eMatrix_Projection, CubeFaceProjectionMatrix(fNear, fFar));

			// How far from a lit point the map is read: the blur, and the
			// texels around a sample that the filtering takes in.
			float fBlur = 0;
			if(settings.mFilter == eShadowMapFilter_Soft)				fBlur = settings.mfSoftness;
			else if(settings.mFilter == eShadowMapFilter_SoftByDistance)	fBlur = settings.mfMaxBlur;
			const float fMargin = fFar * (cMath::Max(fBlur, 0.0f) + 4.0f / (float)lSize);
			const float fReach = fFar + fMargin;

			for(int lFace=0; lFace<6; ++lFace)
			{
				// A face shows in the picture if the part of the light's
				// reach it covers is not all on the outer side of one of
				// the planes the camera's view ends at, by more than the
				// margin: a face that is not drawn holds another light's
				// map, which a sample near the edge of the picture would read.
				bool bInView = true;
				for(int lPlane=0; lPlane<5 && bInView; ++lPlane)
				{
					cPlanef plane = mRenderSettings.mpFrustum->GetPlane((eFrustumPlane)lPlane);
					bool bOutside = cMath::PlaneToPointDist(plane, vPos) < -fMargin;
					cVector3f vRight = cMath::Vector3Cross(gvCubeFaceDir[lFace], gvCubeFaceUp[lFace]);
					for(int lCorner=0; lCorner<4 && bOutside; ++lCorner)
					{
						cVector3f vCorner = vPos + (gvCubeFaceDir[lFace] +
									vRight * (lCorner & 1 ? 1.0f : -1.0f) +
									gvCubeFaceUp[lFace] * (lCorner & 2 ? 1.0f : -1.0f)) * fReach;
						bOutside = cMath::PlaneToPointDist(plane, vCorner) < -fMargin;
					}
					if(bOutside) bInView = false;
				}
				if(bInView==false) continue;

				mpLowLevelGraphics->BeginShadowMap(true, lFace, lSize);
				mShadowMapStats.mlCasters += RenderShadowCasters(apLight, CubeFaceViewMatrix(lFace, vPos), lFace);
				mShadowMapStats.mlMaps++;
			}

			aParams.mfNear = fNear;
			aParams.mfFar = fFar;
			aParams.mfTexelSize = 2.0f / (float)lSize;
		}
		else
		{
			cLight3DSpot *pSpot = static_cast<cLight3DSpot*>(apLight);
			mpLowLevelGraphics->SetMatrix(eMatrix_Projection, pSpot->GetProjectionMatrix());

			mpLowLevelGraphics->BeginShadowMap(false, 0, lSize);
			mShadowMapStats.mlCasters += RenderShadowCasters(apLight, pSpot->GetViewMatrix(), -1);
			mShadowMapStats.mlMaps++;

			// Near and far as the projection has them: the map is read as
			// it was drawn, whatever the light's reach is now.
			const cMatrixf &mtxProj = pSpot->GetProjectionMatrix();
			float fTan = tan(pSpot->GetFOV() * 0.5f);
			aParams.mfNear = mtxProj.m[2][3] / (mtxProj.m[2][2] - 1.0f);
			aParams.mfFar = mtxProj.m[2][3] / (mtxProj.m[2][2] + 1.0f);
			aParams.mfTexelSize = 2.0f * fTan / (float)lSize;
			aParams.mvUvScale = cVector2f(0.5f / (fTan * pSpot->GetAspect()), 0.5f / fTan);
		}

		mpLowLevelGraphics->EndShadowMap();

		///////////////////////////////
		// Back to how the light passes are drawn
		mpLowLevelGraphics->SetMatrix(eMatrix_Projection, apCamera->GetProjectionMatrix());
		mRenderSettings.mbMatrixWasNULL = false;

		mpLowLevelGraphics->SetDepthWriteActive(false);
		mpLowLevelGraphics->SetDepthTestFunc(eDepthTestFunc_Equal);
		mpLowLevelGraphics->SetCullActive(true);
		mpLowLevelGraphics->SetCullMode(eCullMode_CounterClockwise);
		mpLowLevelGraphics->SetAlphaCutoff(mRenderSettings.mAlphaMode != eMaterialAlphaMode_Solid ? 0.6f : 0);

		aParams.mlFilter = (int)settings.mFilter + 1;
		aParams.mfSoftness = settings.mfSoftness;
		aParams.mfLightSize = settings.mfLightSize;
		aParams.mfMaxBlur = settings.mfMaxBlur;
		aParams.mlSamples = cMath::Clamp(settings.mlSamples, 1, 64);
		aParams.mlBlockerSamples = cMath::Clamp(settings.mlBlockerSamples, 1, 64);
		aParams.mbNoise = settings.mbNoise;
		aParams.mfBias = settings.mfBias;
		aParams.mfSlopeBias = settings.mfSlopeBias;

		mpLowLevelGraphics->EndGpuSection();
		return true;
	}

	//-----------------------------------------------------------------------

	// What casts shadows from the light, as seen in a_mtxView with the
	// projection that is set; alFace is the face of a cube map, or -1.
	// Returns how many were drawn.
	int cRenderer3D::RenderShadowCasters(iLight3D *apLight, const cMatrixf &a_mtxView, int alFace)
	{
		const cShadowMapSettings &settings = mShadowMapSettings;
		const cVector3f vLightPos = apLight->GetLightPosition();
		int lDrawn = 0;
		bool bAlphaTest = false;
		mpLowLevelGraphics->SetAlphaCutoff(0);

		tCasterCacheSet *vSets[2] = {apLight->GetStaticCasters(), apLight->GetDynamicCasters()};
		int lSetNum = mRenderSettings.mShowShadows == eRendererShowShadows_All ? 2 : 1;

		for(int lSet=0; lSet<lSetNum; ++lSet)
		for(iRenderable *pObject : *vSets[lSet])
		{
			iVertexBuffer *pVtxBuffer = pObject->GetVertexBuffer();
			if(pVtxBuffer==nullptr) continue;

			if(alFace >= 0)
			{
				cBoundingVolume *pBV = pObject->GetBoundingVolume();
				if(SphereInCubeFace(alFace, pBV->GetWorldCenter() - vLightPos, pBV->GetRadius())==false)
					continue;
			}

			// Holes cut by the alpha of the texture
			iMaterial *pMaterial = pObject->GetMaterial();
			bool bAlpha = pMaterial && pMaterial->HasAlpha();
			if(bAlpha && settings.mbAlphaCasters==false) continue;
			if(bAlpha)
			{
				iTexture *pTexture = pMaterial->GetTexture(eMaterialTexture_Diffuse);
				if(pTexture==nullptr) continue;
				if(mRenderSettings.mpTexture[0] != pTexture)
				{
					mpLowLevelGraphics->SetTexture(0, pTexture);
					mRenderSettings.mpTexture[0] = pTexture;
				}
			}
			if(bAlpha != bAlphaTest)
			{
				bAlphaTest = bAlpha;
				mpLowLevelGraphics->SetAlphaCutoff(bAlpha ? 0.6f : 0);
			}

			cMatrixf *pModelMtx = pObject->GetModelMatrix(nullptr);
			if(pModelMtx)	mpLowLevelGraphics->SetMatrix(eMatrix_ModelView, cMath::MatrixMul(a_mtxView, *pModelMtx));
			else			mpLowLevelGraphics->SetMatrix(eMatrix_ModelView, a_mtxView);
			mpDiffuseVtxProgram->SetMatrixf("worldViewProj", eGpuProgramMatrix_ViewProjection,
											eGpuProgramMatrixOp_Identity);

			if(mRenderSettings.mpVtxBuffer != pVtxBuffer)
			{
				if(mRenderSettings.mpVtxBuffer) mRenderSettings.mpVtxBuffer->UnBind();
				pVtxBuffer->Bind();
				mRenderSettings.mpVtxBuffer = pVtxBuffer;
			}
			pVtxBuffer->Draw();
			++lDrawn;
		}

		return lDrawn;
	}

	//-----------------------------------------------------------------------

	/**
	 * Darkens the lit scene by the ambient occlusion
	 * (cRendererPostEffects::RenderAmbientOcclusion) and leaves the render
	 * settings telling what is set. True if the occlusion was drawn alone,
	 * in place of the scene.
	 */
	bool cRenderer3D::RenderAmbientOcclusion(cCamera3D *apCamera)
	{
		if(mpPostEffects==nullptr || mpPostEffects->GetAmbientOcclusionActive()==false) return false;

		///////////////////////////////
		// Let go of what the light passes left set
		if(mRenderSettings.mpVtxBuffer) mRenderSettings.mpVtxBuffer->UnBind();
		if(mRenderSettings.mpVertexProgram) mRenderSettings.mpVertexProgram->UnBind();
		if(mRenderSettings.mpFragmentProgram) mRenderSettings.mpFragmentProgram->UnBind();
		mRenderSettings.mpVtxBuffer = nullptr;
		mRenderSettings.mpVertexProgram = nullptr;
		mRenderSettings.mpFragmentProgram = nullptr;
		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
		{
			if(mRenderSettings.mpTexture[i]==nullptr) continue;
			mpLowLevelGraphics->SetTexture(i,nullptr);
			mRenderSettings.mpTexture[i] = nullptr;
		}

		if(mpPostEffects->RenderAmbientOcclusion()==false) return false;

		///////////////////////////////
		// Back to how the passes after the lights are drawn
		mpLowLevelGraphics->SetMatrix(eMatrix_Projection, apCamera->GetProjectionMatrix());
		mRenderSettings.mbMatrixWasNULL = false;

		mpLowLevelGraphics->SetDepthTestActive(mRenderSettings.mbDepthTest);
		mpLowLevelGraphics->SetAlphaCutoff(mRenderSettings.mAlphaMode != eMaterialAlphaMode_Solid ? 0.6f : 0);
		// Blending and the colour mask are set anew by what is drawn next
		mRenderSettings.mBlendMode = eMaterialBlendMode_LastEnum;
		mRenderSettings.mChannelMode = eMaterialChannelMode_LastEnum;

		return mpPostEffects->GetAmbientOcclusionSettings().mbShowOnly;
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderDiffuse(cCamera3D *apCamera)
	{
		cRenderNode* pNode = mpRenderList->GetRootNode(eRenderListDrawType_Normal,
														eMaterialRenderType_Diffuse, 0);
		mRenderSettings.mpLight = nullptr;
		pNode->Render(&mRenderSettings);
	}

	//-----------------------------------------------------------------------


	void cRenderer3D::RenderTrans(cCamera3D *apCamera)
	{
		mpLowLevelGraphics->SetColorWriteActive(true, true, true, true);
		mpLowLevelGraphics->SetDepthWriteActive(false);

		bool bLog = mRenderSettings.mbLog;

		iTexture *vTextures[MAX_TEXTUREUNITS];


		cVector3f vForward = apCamera->GetForward();

		//////////////////////////////////
		//Iterate the query objects
		cTransperantObjectIterator it = mpRenderList->GetTransperantIterator();
		while(it.HasNext())
		{
			iRenderable *pObject = it.Next();

			if(bLog) Log(" Rendering '%s'\n",pObject->GetName().c_str());


			/////////////////////////////////////////
			// Get all data needed
			iMaterial *pMaterial = pObject->GetMaterial();

			bool bDepthTest = pMaterial->GetDepthTest();

			eMaterialAlphaMode alphaMode = pMaterial->GetAlphaMode(eMaterialRenderType_Diffuse,0,nullptr);

			eMaterialBlendMode blendMode = pMaterial->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr);

			iGpuProgram *pVtxProgram = pMaterial->GetVertexProgram(eMaterialRenderType_Diffuse,0,nullptr);
			iMaterialProgramSetup* pVtxProgramSetup = pMaterial->GetVertexProgramSetup(eMaterialRenderType_Diffuse,0,nullptr);

			iGpuProgram *pFragProgram = pMaterial->GetFragmentProgram(eMaterialRenderType_Diffuse,0,nullptr);

			for(int i=0; i<MAX_TEXTUREUNITS; ++i) vTextures[i] = pMaterial->GetTexture(i,eMaterialRenderType_Diffuse,0,nullptr);

			iVertexBuffer *pVtxBuffer = pObject->GetVertexBuffer();

			cMatrixf *pModelMatrix = pObject->GetModelMatrix(apCamera);
			cMatrixf *pInvModelMatrix = pObject->GetInvModelMatrix();

			iTexture *pRefraction = pMaterial->GetTexture(eMaterialTexture_Refraction);

			/////////////////////////////////////////
			// Halos, shafts and particles that draw themselves
			cTransDraw ownDraw;
			bool bOwnDraw = pObject->GetTransDraw(apCamera, ownDraw);
			if(bOwnDraw)
			{
				pVtxProgram = ownDraw.mpVtxProgram;
				pVtxProgramSetup = nullptr;
				pFragProgram = ownDraw.mpFragProgram;
				if(ownDraw.mpVtxBuffer) pVtxBuffer = ownDraw.mpVtxBuffer;
				if(ownDraw.mpModelMatrix) pModelMatrix = ownDraw.mpModelMatrix;
				bDepthTest = ownDraw.mbDepthTest;
			}

			if(mRenderSettings.mbDepthTest != bDepthTest)
			{
				mpLowLevelGraphics->SetDepthTestActive(bDepthTest);
				mRenderSettings.mbDepthTest = bDepthTest;

				if(bLog) Log("  Set depth test %d\n",bDepthTest);
			}

			/////////////////////////////////////////
			//Refraction Rendering
			if(mbRefractionAvailable && pRefraction && mbRefractionUsed)
			{
				if(bLog) Log("  Start render refraction\n");

				if(bLog) Log("   Unbind vtx buffer, vtx program and frag program\n");
				if(mRenderSettings.mpVtxBuffer)			mRenderSettings.mpVtxBuffer->UnBind();
				if(mRenderSettings.mpVertexProgram)		mRenderSettings.mpVertexProgram->UnBind();
				if(mRenderSettings.mpFragmentProgram)	mRenderSettings.mpFragmentProgram->UnBind();

				/////////////////////////////////////
				//Alpha and blend mode
				mRenderSettings.mAlphaMode = eMaterialAlphaMode_Solid;
				mpLowLevelGraphics->SetAlphaCutoff(0);

				mRenderSettings.mBlendMode = eMaterialBlendMode_None;
				mpLowLevelGraphics->SetBlendActive(false);

				//////////////////////////////
				//Get Screen Clip space
				cVector2f vScreenSizeFloat = mpLowLevelGraphics->GetRenderSize();
				cVector2l vScreenSize(	(int)mpLowLevelGraphics->GetRenderSize().x,
										(int)mpLowLevelGraphics->GetRenderSize().y);
				iTexture *pScreen = mpPostEffects->GetFreeScreenTexture();

				//Get the cliprect objects bounding volume
				cRect2l clipRect;
				int lScale = cMath::Abs((int)pMaterial->GetValue()) + 1;
				bool bHasClipRect = cMath::GetClipRectFromBV(clipRect,*pObject->GetBoundingVolume(),
										apCamera->GetViewMatrix(), apCamera->GetProjectionMatrix(),
										apCamera->GetNearClipPlane(),vScreenSize);
				if(bHasClipRect)
				{
					clipRect.x -= lScale;
					if(clipRect.x <0){
						clipRect.w += clipRect.x;
						clipRect.x =0;
					}
					clipRect.y -= lScale;
					if(clipRect.y <0){
						clipRect.h += clipRect.y;
						clipRect.y =0;
					}

					clipRect.w += lScale*2;
					if(clipRect.w + clipRect.x > vScreenSize.x)
						clipRect.w = vScreenSize.x-clipRect.x;

					clipRect.h += lScale*2;
					if(clipRect.h + clipRect.y > vScreenSize.y)
						clipRect.h = vScreenSize.y-clipRect.y;
				}


				//////////////////////////////
				//Render to alpha if water, (shitty test this is...)
				if(pMaterial->GetRefractionSkipsStandardTrans())
				{
					if(bLog) Log("   Render alpha to screen\n");

					//NULL all textuer
					for(int i=0;i<MAX_TEXTUREUNITS;i++)
					{
						if(mRenderSettings.mpTexture[i] != nullptr)
						{
							mpLowLevelGraphics->SetTexture(i, nullptr);
							mRenderSettings.mpTexture[i] = nullptr;

							if(bLog) Log("    Set texture[%d] NULL\n",i);
						}
					}

					//Only write to alpha
					mpLowLevelGraphics->SetColorWriteActive(false,false,false, true);

					////////////////////////////////////////////////
					//Clear alpha using 2D quad.
					mpLowLevelGraphics->SetOrthoProjection(mpLowLevelGraphics->GetRenderSize(),-1000,1000);
					mpLowLevelGraphics->SetIdentityMatrix(eMatrix_ModelView);

					if(bHasClipRect)
					{
						cVector2f vOffset = cVector2f((float)clipRect.x,(float)clipRect.y);
						cVector2f vSize = cVector2f((float)clipRect.w,(float)clipRect.h);

						mvVtxRect[0].pos = cVector3f(vOffset.x,				vOffset.y,0);
						mvVtxRect[1].pos = cVector3f(vOffset.x + vSize.x,	vOffset.y,0);
						mvVtxRect[2].pos = cVector3f(vOffset.x + vSize.x,	vOffset.y + vSize.y,0);
						mvVtxRect[3].pos = cVector3f(vOffset.x,				vOffset.y + vSize.y,0);
					}
					else
					{
						mvVtxRect[0].pos = cVector3f(0,0,0);
						mvVtxRect[1].pos = cVector3f(vScreenSizeFloat.x,0,0);
						mvVtxRect[2].pos = cVector3f(vScreenSizeFloat.x,vScreenSizeFloat.y,0);
						mvVtxRect[3].pos = cVector3f(0,vScreenSizeFloat.y,0);
					}

					for(int i=0; i<4; ++i) mvVtxRect[i].col.a = 0;
					mpLowLevelGraphics->DrawQuad(mvVtxRect);

					//Set back to ordinary projection...
					mpLowLevelGraphics->SetMatrix(eMatrix_Projection, apCamera->GetProjectionMatrix());

					//Set Model matrix
					if(pModelMatrix) {
						cMatrixf mtxModel = cMath::MatrixMul(apCamera->GetViewMatrix(),	*pModelMatrix);
						mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,mtxModel);
						mRenderSettings.mbMatrixWasNULL = false;
					}
					else {
						mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());
						mRenderSettings.mbMatrixWasNULL = true;
					}

					pVtxBuffer->Bind();

					if(bLog) Log("    Drawing vtx buffer %d\n",pVtxBuffer);
					pVtxBuffer->Draw();

					pVtxBuffer->UnBind();

					//Reset stuff
					mpLowLevelGraphics->SetColorWriteActive(true,true,true, true);
				}

				//////////////////////////////
				//The scene, to read while it is drawn over
				if(bHasClipRect)
				{
					cVector2l vOffset = cVector2l(clipRect.x,clipRect.y);
					cVector2l vSize = cVector2l(clipRect.w,clipRect.h);

					mpLowLevelGraphics->DrawSceneCopy(pScreen,vOffset,vSize);
					if(bLog) Log("   Copy clipped screen texture\n");
				}
				else
				{
					mpLowLevelGraphics->DrawSceneCopy(pScreen,0,vScreenSize);
					if(bLog) Log("   Copy full screen screen texture\n");
				}

				//////////////////////////////
				//Set up programs
				bool bSpecial = false;
				if(	pMaterial->GetTexture(eMaterialTexture_Specular)!=nullptr &&
					pMaterial->GetRefractionDiffuseTexture() == eMaterialTexture_Diffuse)
				{
					bSpecial = true;
					if(bLog) Log("   Special = true\n");
				}

				iGpuProgram *pRefractVtxProgram = pMaterial->GetRefractionVertexProgam();
				iGpuProgram *pRefractFragProgram = pMaterial->GetRefractionFragmentProgam();

				if(pRefractFragProgram==nullptr) {
					if(bSpecial)	pRefractFragProgram = mpRefractSpecFragProgram;
					else			pRefractFragProgram = mpRefractFragProgram;
				}
				if(pRefractVtxProgram==nullptr) pRefractVtxProgram = mpRefractVtxProgram;


				/////////////////////////////////////
				//Vertex program
				mRenderSettings.mpVertexProgram = pRefractVtxProgram;
				pRefractVtxProgram->Bind();
				if(bLog) Log("   Binding vtx program '%s' (%d)\n",pRefractVtxProgram->GetName().c_str(), pRefractVtxProgram);
				mRenderSettings.mbMatrixWasNULL = false;


				//Model matrix
				if(pModelMatrix)
				{
					cMatrixf mtxModel = cMath::MatrixMul(apCamera->GetViewMatrix(),	*pModelMatrix);
					mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,mtxModel);
					mRenderSettings.mbMatrixWasNULL = false;
				}
				else
				{
					mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());
					mRenderSettings.mbMatrixWasNULL = true;
				}

				pRefractVtxProgram->SetMatrixf("worldViewProj",
												eGpuProgramMatrix_ViewProjection,
												eGpuProgramMatrixOp_Identity);

				//Eye position
				if(pMaterial->GetRefractionUsesEye())
				{
					if(pModelMatrix)
					{
						cVector3f vLocalEye =  cMath::MatrixMul(*pInvModelMatrix,apCamera->GetEyePosition());
						pRefractVtxProgram->SetVec3f("EyePos",vLocalEye);
					}
					else
					{
						pRefractVtxProgram->SetVec3f("EyePos",apCamera->GetEyePosition());
					}
				}

				/////////////////////////////////////
				//Fragment program
				if(bLog) Log("   Binding frag program '%s' (%d)\n",pRefractFragProgram->GetName().c_str(), pRefractFragProgram);
				pRefractFragProgram->Bind();

				mRenderSettings.mpFragmentProgram = pRefractFragProgram;

				if(bSpecial)
				{
					pRefractFragProgram->SetFloat("t", mfRenderTime * pRefraction->GetFrameTime());
				}
				else if(pMaterial->GetRefractionUsesTime())
				{
					pRefractFragProgram->SetFloat("t", mfRenderTime);
				}

				pRefractFragProgram->SetFloat("scale", pMaterial->GetValue());

				////////////////////////////////////
				//Textures
				for(int i=0;i<MAX_TEXTUREUNITS;i++)
				{
					iTexture *pTex = nullptr;
					switch(i)
					{
						case 0: pTex = pScreen; break;
						case 1: pTex = pRefraction; break;
						case 2: if(bSpecial)
								{
									pTex = pMaterial->GetTexture(eMaterialTexture_Specular);
								}
								else if(pMaterial->GetRefractionUsesDiffuse())
								{
									pTex = pMaterial->GetTexture(pMaterial->GetRefractionDiffuseTexture());
								}
					}

					if(mRenderSettings.mpTexture[i] != pTex)
					{
						mpLowLevelGraphics->SetTexture(i, pTex);
						mRenderSettings.mpTexture[i] = pTex;
						if(bLog) {
							if(pTex)	Log("   Set texture[%d] %s (%d)\n",i,  pTex->GetName().c_str(), pTex);
							else		Log("   Set texture[%d] NULL\n",i);
						}

					}
				}

				/////////////////////////////////////
				//Draw
				if(bLog) Log("   Drawing vtx buffer %d\n",pVtxBuffer);

				pVtxBuffer->Bind();

				pVtxBuffer->Draw();

				pVtxBuffer->UnBind();

				mRenderSettings.mpVtxBuffer = pVtxBuffer;

				if(pMaterial->GetRefractionSkipsStandardTrans() || pMaterial->GetTexture(eMaterialTexture_Diffuse)==nullptr)
				{
					if(bLog) Log("   Skipping normal trans rendering!\n");
					continue;
				}
			}

			/////////////////////////////////////////////////
			// Alpha mode
			if(alphaMode != mRenderSettings.mAlphaMode)
			{
				mRenderSettings.mAlphaMode = alphaMode;

				if(alphaMode == eMaterialAlphaMode_Solid)
				{
					mpLowLevelGraphics->SetAlphaCutoff(0);
					if(bLog) Log("  Set alpha test off!\n");
				}
				else
				{
					mpLowLevelGraphics->SetAlphaCutoff(0.6f);
					if(bLog) Log("  Set alpha test on!\n");
				}
			}

			/////////////////////////////////////////////////
			// Blend mode
			if(blendMode != mRenderSettings.mBlendMode)
			{
				mRenderSettings.mBlendMode = blendMode;

				if(blendMode == eMaterialBlendMode_None)
				{
					mpLowLevelGraphics->SetBlendActive(false);
					if(bLog) Log("  Set blend mode off!\n");
				}
				else
				{
					mpLowLevelGraphics->SetBlendActive(true);

					switch(blendMode)
					{
					case eMaterialBlendMode_Add:
						mpLowLevelGraphics->SetBlendFunc(eBlendFunc_One,eBlendFunc_One);
						if(bLog) Log("  Set blend mode one-one!\n");
						break;
					case eMaterialBlendMode_Replace:
						mpLowLevelGraphics->SetBlendFunc(eBlendFunc_One,eBlendFunc_Zero);
						if(bLog) Log("  Set blend mode one-zero!\n");
						break;
					case eMaterialBlendMode_Mul:
						mpLowLevelGraphics->SetBlendFunc(eBlendFunc_Zero,eBlendFunc_SrcColor);
						if(bLog) Log("  Set blend mode zero-srccolor!\n");
						break;
					case eMaterialBlendMode_MulX2:
						mpLowLevelGraphics->SetBlendFunc(eBlendFunc_DestColor,eBlendFunc_SrcColor);
						if(bLog) Log("  Set blend mode destcolor-srccolor!\n");
						break;
					case eMaterialBlendMode_Alpha:
						mpLowLevelGraphics->SetBlendFunc(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha);
						if(bLog) Log("  Set blend mode srcalpha-oneminussrcalpha!\n");
						break;
					case eMaterialBlendMode_DestAlphaAdd:
						mpLowLevelGraphics->SetBlendFunc(eBlendFunc_DestAlpha,eBlendFunc_One);
						if(bLog) Log("  Set blend mode destalpha-one!\n");
						break;
					}
				}
			}

			/////////////////////////////////////////////////
			// Vertex program
			if(pVtxProgram != mRenderSettings.mpVertexProgram)
			{
				if(bLog){
					if(pVtxProgram)	Log("  Set vtx program '%s' (%d)\n",pVtxProgram->GetName().c_str(), pVtxProgram);
					else			Log("  Set vtx program NULL\n");
				}
				if(mRenderSettings.mpVertexProgram)// && pVtxProgram==NULL) //Why null??
				{
					mRenderSettings.mpVertexProgram->UnBind();
				}
				mRenderSettings.mpVertexProgram = pVtxProgram;

				if(pVtxProgram)
				{
					pVtxProgram->Bind();

					if(pVtxProgramSetup)
					{
						pVtxProgramSetup->Setup(pVtxProgram, &mRenderSettings);
					}
					mRenderSettings.mpVtxProgramSetup = pVtxProgramSetup;

					//reset this so all matrix setting are set to vertex program.
					mRenderSettings.mbMatrixWasNULL = false;
				}
			}

			/////////////////////////////////////////////////
			// Fragment program
			if(pFragProgram != mRenderSettings.mpFragmentProgram)
			{
				if(bLog){
					if(pFragProgram)Log("  Set frag program '%s' (%d)\n",pFragProgram->GetName().c_str(), pFragProgram);
					else			Log("  Set frag program NULL\n");
				}
				if(mRenderSettings.mpFragmentProgram) mRenderSettings.mpFragmentProgram->UnBind();

				mRenderSettings.mpFragmentProgram = pFragProgram;

				if(pFragProgram)
				{
					pFragProgram->Bind();
				}
			}

			/////////////////////////////////////////////////
			// Texture
			for(int i=0;i<MAX_TEXTUREUNITS;i++)
			{
				if(mRenderSettings.mpTexture[i] != vTextures[i])
				{
					mpLowLevelGraphics->SetTexture(i,vTextures[i]);
					mRenderSettings.mpTexture[i] = vTextures[i];

					if(bLog) {
						if(vTextures[i])	Log("   Set texture[%d] %s (%d)\n",i,  vTextures[i]->GetName().c_str(), vTextures[i]);
						else				Log("   Set texture[%d] NULL\n",i);
					}
				}
			}

			/////////////////////////////////////////////////
			// Vertex buffer
			if(pVtxBuffer != mRenderSettings.mpVtxBuffer)
			{
				if(mRenderSettings.mpVtxBuffer) mRenderSettings.mpVtxBuffer->UnBind();
				mRenderSettings.mpVtxBuffer = pVtxBuffer;

				if(pVtxBuffer)	pVtxBuffer->Bind();

				if(bLog) Log("  Set vtx buffer %d\n",pVtxBuffer);
			}

			/////////////////////////////////////////////////
			// Matrix
			//It is a normal matrix
			bool bSetVtxProgMatrix =false;
			if(pModelMatrix)
			{
				cMatrixf mtxModel = cMath::MatrixMul(apCamera->GetViewMatrix(),	*pModelMatrix);

				mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,mtxModel);

				mRenderSettings.mbMatrixWasNULL = false;
				bSetVtxProgMatrix =true;
			}
			//NULL matrix
			else if(mRenderSettings.mbMatrixWasNULL==false)
			{
				mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());

				mRenderSettings.mbMatrixWasNULL = true;
				bSetVtxProgMatrix =true;
			}

			if(mRenderSettings.mpVertexProgram && bSetVtxProgMatrix)
			{
				//Might be quicker if this is set directly
				mRenderSettings.mpVertexProgram->SetMatrixf("worldViewProj",
														eGpuProgramMatrix_ViewProjection,
														eGpuProgramMatrixOp_Identity);
				if(mRenderSettings.mpVtxProgramSetup)
				{
					mRenderSettings.mpVtxProgramSetup->SetupMatrix(pModelMatrix,&mRenderSettings);
				}
			}

			///////////////////////////////////////////
			/// Draw
			if(bLog) Log("  Draw\n");
			if(bOwnDraw)
			{
				pObject->SetupTransDraw(apCamera, &mRenderSettings, ownDraw);
				if(ownDraw.mbCull==false) mpLowLevelGraphics->SetCullActive(false);
				pVtxBuffer->Draw();
				if(ownDraw.mbCull==false) mpLowLevelGraphics->SetCullActive(true);
			}
			else
			{
				pVtxBuffer->Draw();
			}

		}

		if(mRenderSettings.mpVtxBuffer) mRenderSettings.mpVtxBuffer->UnBind();
		if(mRenderSettings.mpVertexProgram) mRenderSettings.mpVertexProgram->UnBind();
		if(mRenderSettings.mpFragmentProgram) mRenderSettings.mpFragmentProgram->UnBind();
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderDebug(cCamera3D *apCamera)
	{
		if(mDebugFlags)
		{
			mpLowLevelGraphics->SetDepthWriteActive(false);


			//Render Debug for objects
			cRenderableIterator objectIt = mpRenderList->GetObjectIt();
			while(objectIt.HasNext())
			{
				iRenderable* pObject = objectIt.Next();

				RenderDebugObject(apCamera,pObject,nullptr,0,nullptr,eMaterialRenderType_Diffuse,nullptr);
			}

			//Render debug for lights.

			if(mDebugFlags & eRendererDebugFlag_DrawLightBoundingBox)
			{
				mpLowLevelGraphics->SetDepthTestActive(false);
				mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());

				cLight3DIterator lightIt = mpRenderList->GetLightIt();
				while(lightIt.HasNext())
				{
					iLight3D* pLight = lightIt.Next();
					cBoundingVolume *pBV = pLight->GetBoundingVolume();

					cColor Col = pLight->GetDiffuseColor();

					mpLowLevelGraphics->DrawSphere(pLight->GetWorldPosition(),0.1f,Col);
					mpLowLevelGraphics->DrawBoxMaxMin(pBV->GetMax(), pBV->GetMin(),Col);
				}

				mpLowLevelGraphics->SetDepthTestActive(true);
			}

			mpLowLevelGraphics->SetDepthWriteActive(true);
		}
	}

	//-----------------------------------------------------------------------

	void cRenderer3D::RenderDebugObject(cCamera3D *apCamera,iRenderable* &apObject, iMaterial* apPrevMat,
		int alPrevMatId,iVertexBuffer* apPrevVtxBuff,
		eMaterialRenderType aRenderType, iLight3D* apLight)
	{
		iVertexBuffer* pVtxBuffer = apObject->GetVertexBuffer();

		if(mDebugFlags & eRendererDebugFlag_DrawBoundingBox)
		{
			mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());

			cBoundingVolume *pBV = apObject->GetBoundingVolume();

			mpLowLevelGraphics->DrawBoxMaxMin(pBV->GetMax(), pBV->GetMin(),cColor(1,1.0f,1.0f,1));
		}

		if(mDebugFlags & eRendererDebugFlag_DrawBoundingSphere)
		{
			mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,apCamera->GetViewMatrix());

			cBoundingVolume *pBV = apObject->GetBoundingVolume();

			mpLowLevelGraphics->DrawSphere(pBV->GetWorldCenter(), pBV->GetRadius(),cColor(1,1.0f,1.0f,1));
		}

		cMatrixf mtxModel;
		cMatrixf *pModelMtx = apObject->GetModelMatrix(apCamera);

		if(pModelMtx)
			mtxModel = cMath::MatrixMul(apCamera->GetViewMatrix(),*pModelMtx);
		else
			mtxModel = cMath::MatrixMul(apCamera->GetViewMatrix(),cMatrixf::Identity);

		mpLowLevelGraphics->SetMatrix(eMatrix_ModelView,mtxModel);

		//Draw the debug graphics for the object.
		for(int i=0; i< pVtxBuffer->GetVertexNum(); i++)
		{
			cVector3f vPos = pVtxBuffer->GetVector3(eVertexFlag_Position,i);

			if(mDebugFlags & eRendererDebugFlag_DrawNormals)
			{
				cVector3f vNormal = pVtxBuffer->GetVector3(eVertexFlag_Normal,i);

				mpLowLevelGraphics->DrawLine(vPos,vPos+(vNormal*0.1f),cColor(0.5f,0.5f,1,1));
			}
			if(mDebugFlags & eRendererDebugFlag_DrawTangents)
			{
				cVector3f vTan = pVtxBuffer->GetVector4(eVertexFlag_Texture1,i);

				mpLowLevelGraphics->DrawLine(vPos,vPos+(vTan*0.2f),cColor(1,0.0f,0.0f,1));
			}
		}
	}


	//-----------------------------------------------------------------------

}
