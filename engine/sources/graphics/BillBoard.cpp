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
#include "graphics/BillBoard.h"

#include "tinyxml.h"

#include "resources/Resources.h"
#include "resources/MaterialManager.h"
#include "resources/FileSearcher.h"
#include "resources/GpuProgramManager.h"
#include "resources/LowLevelResources.h"
#include "graphics/GPUProgram.h"
#include "graphics/Bitmap2D.h"
#include "graphics/Texture.h"
#include "graphics/Renderer3D.h"
#include "graphics/VertexBuffer.h"
#include "graphics/Material.h"
#include "graphics/Graphics.h"
#include "graphics/MeshCreator.h"
#include "graphics/LowLevelGraphics.h"
#include "scene/Camera3D.h"
#include "scene/World3D.h"
#include "scene/Scene.h"
#include "math/Math.h"
#include "math/BoundingVolume.h"

#include "game/Game.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cBillboard::cBillboard(const tString asName,const cVector2f& avSize, cResources *apResources,
							cGraphics *apGraphics) :
	iRenderable(asName)
	{
		mpMaterialManager = apResources->GetMaterialManager();
		mpFileSearcher = apResources->GetFileSearcher();
		mpLowLevelGraphics = apGraphics->GetLowLevel();
		mpMeshCreator = apGraphics->GetMeshCreator();
		mpResources = apResources;
		mpProgramManager = apResources->GetGpuProgramManager();

		mpHaloVtxProgram = nullptr;
		mpHaloFragProgram = nullptr;
		mpHaloProfile = nullptr;
		mpShaftVtxProgram = nullptr;
		mpShaftFragProgram = nullptr;
		mpShaftBuffer = nullptr;

		mpHaloSourceBuffer = mpMeshCreator->CreateBoxVertexBuffer(1);

		mvSize = avSize;
		mvAxis = cVector3f(0,1,0);

		mColor = cColor(1,1,1,1);
		mfForwardOffset =0;
		mfHaloAlpha = 1.0f;

		mType = eBillboardType_Point;

		mpMaterial = nullptr;

		mlLastRenderCount = -1;

		mpVtxBuffer = mpLowLevelGraphics->CreateVertexBuffer(
								eVertexFlag_Position | eVertexFlag_Color0 | eVertexFlag_Texture0 |
								eVertexFlag_Normal,
								eVertexBufferDrawType_Tri, eVertexBufferUsageType_Dynamic,4,6);



		cVector3f vCoords[4] = {cVector3f((mvSize.x/2),-(mvSize.y/2),0),
								cVector3f(-(mvSize.x/2),-(mvSize.y/2),0),
								cVector3f(-(mvSize.x/2),(mvSize.y/2),0),
								cVector3f((mvSize.x/2),(mvSize.y/2),0)};

		cVector3f vTexCoords[4] = {cVector3f(1,-1,0),
								cVector3f(-1,-1,0),
								cVector3f(-1,1,0),
								cVector3f(1,1,0)};
		for(int i=0;i<4;i++)
		{
			mpVtxBuffer->AddVertex(eVertexFlag_Position, vCoords[i]);
			mpVtxBuffer->AddColor(eVertexFlag_Color0, cColor(1,1,1,1));
			mpVtxBuffer->AddVertex(eVertexFlag_Texture0, (vTexCoords[i] + cVector2f(1,1))/2 );
			mpVtxBuffer->AddVertex(eVertexFlag_Normal,cVector3f(0,0,1));
		}

		for(int i=0;i<3;i++) mpVtxBuffer->AddIndex(i);
		for(int i=2;i<5;i++) mpVtxBuffer->AddIndex(i==4?0:i);

		mpVtxBuffer->Compile(eVertexCompileFlag_CreateTangents);

		mbIsHalo = false;
		mbHaloSourceIsParent = false;
		mvHaloSourceSize = 1;


		mBoundingVolume.SetSize(cVector3f(mvSize.x, mvSize.y, mvSize.x));
	}

	//-----------------------------------------------------------------------

	cBillboard::~cBillboard()
	{
		if(mpMaterial) mpMaterialManager->Destroy(mpMaterial);
		if(mpVtxBuffer) hplDelete(mpVtxBuffer);
		if(mpHaloSourceBuffer) hplDelete(mpHaloSourceBuffer);
		if(mpShaftBuffer) hplDelete(mpShaftBuffer);

		if(mpHaloVtxProgram) mpProgramManager->Destroy(mpHaloVtxProgram);
		if(mpHaloFragProgram) mpProgramManager->Destroy(mpHaloFragProgram);
		if(mpShaftVtxProgram) mpProgramManager->Destroy(mpShaftVtxProgram);
		if(mpShaftFragProgram) mpProgramManager->Destroy(mpShaftFragProgram);

		if(mQueryObject.mpQuery)
			mpLowLevelGraphics->DestroyOcclusionQuery(mQueryObject.mpQuery);
		if(mMaxQueryObject.mpQuery)
			mpLowLevelGraphics->DestroyOcclusionQuery(mMaxQueryObject.mpQuery);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cBillboard::SetSize(const cVector2f& avSize)
	{
		mvSize = avSize;
		mBoundingVolume.SetSize(cVector3f(mvSize.x, mvSize.y, mvSize.x));

		float *pPos = mpVtxBuffer->GetArray(eVertexFlag_Position);

		cVector3f vCoords[4] = {cVector3f((mvSize.x/2),-(mvSize.y/2),0),
								cVector3f(-(mvSize.x/2),-(mvSize.y/2),0),
								cVector3f(-(mvSize.x/2),(mvSize.y/2),0),
								cVector3f((mvSize.x/2),(mvSize.y/2),0)};

		for(int i=0; i<4;++i)
		{
			pPos[0] = vCoords[i].x;
			pPos[1] = vCoords[i].y;
			pPos[2] = vCoords[i].z;
			pPos+=4;
		}

		mpVtxBuffer->UpdateData(eVertexFlag_Position,false);

		if(mType == eBillboardType_Axis) SetAxis(mvAxis);

		SetTransformUpdated();
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetType(eBillboardType aType)
	{
		mType = aType;

		SetAxis(mvAxis);
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetAxis(const cVector3f& avAxis)
	{
		mvAxis = avAxis;
		mvAxis.Normalise();

		//This is a quick fix so the bounding box is correct for non up-pointing axises
		if(mType == eBillboardType_Axis && mvAxis != cVector3f(0,1,0))
		{
			float fMax = mvSize.x;
			if(fMax < mvSize.y) fMax = mvSize.y;

			fMax *= kSqrt2f;

			mBoundingVolume.SetSize(fMax);

			SetTransformUpdated();
		}
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetColor(const cColor &aColor)
	{
		if(mColor == aColor) return;

		mColor = aColor;

		float *pColors = mpVtxBuffer->GetArray(eVertexFlag_Color0);

		for(int i=0; i<4;++i)
		{
			pColors[0] = mColor.r * mfHaloAlpha;
			pColors[1] = mColor.g * mfHaloAlpha;
			pColors[2] = mColor.b * mfHaloAlpha;
			pColors[3] = mColor.a * mfHaloAlpha;
			pColors+=4;
		}

		mpVtxBuffer->UpdateData(eVertexFlag_Color0,false);
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetHaloAlpha(float afX)
	{
		if(mfHaloAlpha == afX)
		{
			return;
		}

		mfHaloAlpha = afX;

		float *pColors = mpVtxBuffer->GetArray(eVertexFlag_Color0);

		for(int i=0; i<4;++i)
		{
			pColors[0] = mColor.r * mfHaloAlpha;
			pColors[1] = mColor.g * mfHaloAlpha;
			pColors[2] = mColor.b * mfHaloAlpha;
			pColors[3] = mColor.a * mfHaloAlpha;
			pColors+=4;
		}

		mpVtxBuffer->UpdateData(eVertexFlag_Color0,false);
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetForwardOffset(float afOffset)
	{
		mfForwardOffset = afOffset;
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetMaterial(iMaterial * apMaterial)
	{
		mpMaterial = apMaterial;
	}

	//-----------------------------------------------------------------------

	cBoundingVolume* cBillboard::GetBoundingVolume()
	{
		return &mBoundingVolume;
	}

	//-----------------------------------------------------------------------

	void cBillboard::UpdateGraphics(cCamera3D *apCamera,float afFrameTime, cRenderList *apRenderList)
	{
		if(mbIsHalo==false) return;

		// The halo's shader finds out how much of the lamp is in view
		if(DrawnAsHalo())
		{
			SetHaloAlpha(1);
			return;
		}

		////////////////////////
		//Set the alpha
		float fAlpha = 0;

		if(mlLastRenderCount == apRenderList->GetLastRenderCount())
		{
			if(mMaxQueryObject.mpQuery->GetSampleCount()>0)
			{
				fAlpha =	(float)mQueryObject.mpQuery->GetSampleCount() /
							(float)mMaxQueryObject.mpQuery->GetSampleCount();
			}
		}
		SetHaloAlpha(fAlpha);
		mlLastRenderCount = apRenderList->GetRenderCount();

		////////////////////////
		//Add the queries
		if(mbHaloSourceIsParent)
		{
			iRenderable *pParent = static_cast<iRenderable*>(GetEntityParent());
			if(pParent==nullptr)
			{
				Error("Billboard %s has no parent!\n",msName.c_str());
				return;
			}

			iVertexBuffer *pVtxBuffer = pParent->GetVertexBuffer();
			if(pVtxBuffer==nullptr)
			{
				Error("Billboard '%s' parent '%s' as NULL vertex buffer!\n",msName.c_str(),
																		pParent->GetName().c_str());
				return;
			}

			mQueryObject.mpMatrix = pParent->GetModelMatrix(apCamera);
			mQueryObject.mpVtxBuffer = pVtxBuffer;

			mMaxQueryObject.mpMatrix = pParent->GetModelMatrix(apCamera);
			mMaxQueryObject.mpVtxBuffer = pVtxBuffer;
		}
		else
		{
			mQueryObject.mpMatrix = &GetWorldMatrix();
			mQueryObject.mpVtxBuffer = mpHaloSourceBuffer;

			mMaxQueryObject.mpMatrix = &GetWorldMatrix();
			mMaxQueryObject.mpVtxBuffer = mpHaloSourceBuffer;
		}

		mQueryObject.mbDepthTest = true;
		mMaxQueryObject.mbDepthTest = false;

		apRenderList->AddOcclusionQuery(&mQueryObject);
		apRenderList->AddOcclusionQuery(&mMaxQueryObject);

	}

	//-----------------------------------------------------------------------

	cMatrixf* cBillboard::GetModelMatrix(cCamera3D *apCamera)
	{
		if(apCamera==nullptr)return &GetWorldMatrix();

		m_mtxTempTransform = GetWorldMatrix();
		cVector3f vForward, vRight, vUp;

		cVector3f vCameraForward = apCamera->GetPosition() - GetWorldPosition();
		vCameraForward.Normalise();

		if(mType == eBillboardType_Point)
		{
			vForward = vCameraForward;
			vRight = cMath::Vector3Cross(apCamera->GetViewMatrix().GetUp(), vForward);
			vUp = cMath::Vector3Cross(vForward,vRight);
		}
		else if(mType == eBillboardType_Axis)
		{
			vUp = cMath::MatrixMul(GetWorldMatrix().GetRotation(),mvAxis);
			vUp.Normalise();

			if(vUp == vForward)
			{
				vRight = cMath::Vector3Cross(vUp, vCameraForward);
				Warning("Billboard Right vector is not correct! Contact programmer!\n");
			}
			else
				vRight = cMath::Vector3Cross(vUp, vCameraForward);

			vRight.Normalise();
			vForward = cMath::Vector3Cross(vRight, vUp);

			//vForward.Normalise();
			//vUp.Normalise();
		}

		if(mfForwardOffset!=0)
		{
			cVector3f vPos = m_mtxTempTransform.GetTranslation();
			vPos +=  vCameraForward * mfForwardOffset;
			m_mtxTempTransform.SetTranslation(vPos);
		}

		//Set right vector
		m_mtxTempTransform.m[0][0] = vRight.x;
		m_mtxTempTransform.m[1][0] = vRight.y;
		m_mtxTempTransform.m[2][0] = vRight.z;

		//Set up vector
		m_mtxTempTransform.m[0][1] = vUp.x;
		m_mtxTempTransform.m[1][1] = vUp.y;
		m_mtxTempTransform.m[2][1] = vUp.z;

		//Set forward vector
		m_mtxTempTransform.m[0][2] = vForward.x;
		m_mtxTempTransform.m[1][2] = vForward.y;
		m_mtxTempTransform.m[2][2] = vForward.z;

		return &m_mtxTempTransform;
	}

	//-----------------------------------------------------------------------

	bool cBillboard::DrawnAsHalo()
	{
		if(mType != eBillboardType_Point || mpMaterial==nullptr) return false;
		if(mpLowLevelGraphics->GetEffects().mbHalos==false) return false;
		return mpMaterial->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr) == eMaterialBlendMode_Add;
	}

	bool cBillboard::DrawnAsShaft()
	{
		if(mType != eBillboardType_Axis || mpMaterial==nullptr) return false;
		// The small ones are glints on things held in the hand (the
		// flashlight's lens), not air
		if(mvSize.y < 0.5f) return false;
		if(mpLowLevelGraphics->GetEffects().mbShafts==false) return false;
		return mpMaterial->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr) == eMaterialBlendMode_Add;
	}

	//-----------------------------------------------------------------------

	/**
	 * The colour of the material's picture at kHaloKnots distances from its
	 * middle, evenly spaced up to the corner, each the average round the
	 * ring. Measured once for each picture.
	 */
	const cColor* cBillboard::GetHaloProfile()
	{
		if(mpHaloProfile) return mpHaloProfile;

		static std::map<tString, std::vector<cColor> > mapProfiles;

		iTexture *pTexture = mpMaterial->GetTexture(eMaterialTexture_Diffuse);
		// Textures are named after their file; one with frames after the
		// first frame's file less its number
		tString sName = pTexture ? pTexture->GetName() : "";
		tString sPath = sName != "" ? mpFileSearcher->GetFilePath(sName) : "";
		if(sPath == "" && sName != "")
		{
			static const char *vExts[] = {"jpg","tga","png","bmp"};
			tString sBase = cString::SetFileExt(sName,"");
			for(int i=0; i<8 && sPath == ""; ++i)
				sPath = mpFileSearcher->GetFilePath(sBase + (i < 4 ? "." : "01.") + vExts[i % 4]);
		}

		std::map<tString, std::vector<cColor> >::iterator it = mapProfiles.find(sPath);
		if(it == mapProfiles.end())
		{
			std::vector<cColor> vProfile(kHaloKnots, cColor(0,0));
			std::vector<float> vWeight(kHaloKnots, 0.0f);

			iBitmap2D *pBmp = sPath != "" ? mpResources->GetLowLevel()->LoadBitmap2D(sPath) : nullptr;
			int lChannels = pBmp ? pBmp->GetNumChannels() : 0;
			if(lChannels >= 3)
			{
				const unsigned char *pData = static_cast<const unsigned char*>(pBmp->GetRawData());
				int lW = pBmp->GetWidth(), lH = pBmp->GetHeight();
				for(int y=0; y<lH; ++y)
				for(int x=0; x<lW; ++x)
				{
					float fX = ((float)x + 0.5f) / (float)lW * 2.0f - 1.0f;
					float fY = ((float)y + 0.5f) / (float)lH * 2.0f - 1.0f;
					float fKnot = std::sqrt(fX*fX + fY*fY) / kSqrt2f * (float)(kHaloKnots-1);
					int lKnot = cMath::Min((int)fKnot, kHaloKnots-2);
					float fT = cMath::Min(fKnot - (float)lKnot, 1.0f);

					const unsigned char *pPixel = pData + ((size_t)y * lW + x) * lChannels;
					cColor col(pPixel[0] / 255.0f, pPixel[1] / 255.0f, pPixel[2] / 255.0f, 1);
					vProfile[lKnot] = vProfile[lKnot] + col * (1.0f - fT);
					vWeight[lKnot] += 1.0f - fT;
					vProfile[lKnot+1] = vProfile[lKnot+1] + col * fT;
					vWeight[lKnot+1] += fT;
				}
				for(int i=0; i<kHaloKnots; ++i)
					if(vWeight[i] > 0) vProfile[i] = vProfile[i] * (1.0f / vWeight[i]);
			}
			else
			{
				// No glow known: the shader adds all of the picture as "the rest"
				Warning("Billboard '%s': the picture '%s' could not be read for its halo\n",
						msName.c_str(), sName.c_str());
			}
			if(pBmp) hplDelete(pBmp);

			it = mapProfiles.insert(std::make_pair(sPath, vProfile)).first;
		}

		mpHaloProfile = it->second.data();
		return mpHaloProfile;
	}

	//-----------------------------------------------------------------------

	bool cBillboard::GetTransDraw(cCamera3D *apCamera, cTransDraw &aDraw)
	{
		if(DrawnAsHalo())
		{
			if(mpHaloVtxProgram==nullptr)
			{
				mpHaloVtxProgram = mpProgramManager->CreateProgram("Halo_vp.cg","main",eGpuProgramType_Vertex);
				mpHaloFragProgram = mpProgramManager->CreateProgram("Halo_fp.cg","main",eGpuProgramType_Fragment);
			}
			if(mpHaloVtxProgram==nullptr || mpHaloFragProgram==nullptr) return false;

			aDraw.mpVtxProgram = mpHaloVtxProgram;
			aDraw.mpFragProgram = mpHaloFragProgram;
			aDraw.mbDepthTest = mpMaterial->GetDepthTest();
			return true;
		}

		if(DrawnAsShaft())
		{
			if(mpShaftVtxProgram==nullptr)
			{
				mpShaftVtxProgram = mpProgramManager->CreateProgram("Shaft_vp.cg","main",eGpuProgramType_Vertex);
				mpShaftFragProgram = mpProgramManager->CreateProgram("Shaft_fp.cg","main",eGpuProgramType_Fragment);
			}
			if(mpShaftVtxProgram==nullptr || mpShaftFragProgram==nullptr) return false;

			if(mpShaftBuffer==nullptr || mvShaftBufferSize != mvSize)
			{
				if(mpShaftBuffer) hplDelete(mpShaftBuffer);
				mpShaftBuffer = mpMeshCreator->CreateBoxVertexBuffer(cVector3f(mvSize.x, mvSize.y, mvSize.x));
				mvShaftBufferSize = mvSize;
			}

			// y along the axis; the body is round, so any two directions
			// across it will do
			cVector3f vUp = cMath::MatrixMul(GetWorldMatrix().GetRotation(),mvAxis);
			vUp.Normalise();
			cVector3f vRight = cMath::Vector3Cross(vUp, std::abs(vUp.y) < 0.9f ? cVector3f(0,1,0) : cVector3f(1,0,0));
			vRight.Normalise();
			cVector3f vForward = cMath::Vector3Cross(vRight, vUp);

			m_mtxShaft = cMatrixf::Identity;
			m_mtxShaft.m[0][0] = vRight.x;	m_mtxShaft.m[1][0] = vRight.y;	m_mtxShaft.m[2][0] = vRight.z;
			m_mtxShaft.m[0][1] = vUp.x;		m_mtxShaft.m[1][1] = vUp.y;		m_mtxShaft.m[2][1] = vUp.z;
			m_mtxShaft.m[0][2] = vForward.x;m_mtxShaft.m[1][2] = vForward.y;m_mtxShaft.m[2][2] = vForward.z;
			m_mtxShaft.SetTranslation(GetWorldPosition());

			aDraw.mpVtxProgram = mpShaftVtxProgram;
			aDraw.mpFragProgram = mpShaftFragProgram;
			aDraw.mpVtxBuffer = mpShaftBuffer;
			aDraw.mpModelMatrix = &m_mtxShaft;
			// The shader cuts the shaft off at what is in front of it, and
			// the box is drawn by the sides turned away from the viewer
			aDraw.mbDepthTest = false;
			aDraw.mbCull = false;
			return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetupTransDraw(cCamera3D *apCamera, cRenderSettings *apSettings, cTransDraw &aDraw)
	{
		const cEffectSettings &effects = mpLowLevelGraphics->GetEffects();
		bool bFog = apSettings->mbFogActive;

		if(aDraw.mpVtxProgram == mpHaloVtxProgram)
		{
			// The lamp: the box the original asks about, or the box
			// round the parent where the original draws the parent itself
			cMatrixf mtxBox = GetWorldMatrix();
			cVector3f vHalf = mvHaloSourceSize * 0.5f;
			iRenderable *pParent = mbHaloSourceIsParent ? static_cast<iRenderable*>(GetEntityParent()) : nullptr;
			if(pParent)
			{
				cBoundingVolume *pBV = pParent->GetBoundingVolume();
				mtxBox = cMatrixf::Identity;
				mtxBox.SetTranslation((pBV->GetMax() + pBV->GetMin()) * 0.5f);
				vHalf = (pBV->GetMax() - pBV->GetMin()) * 0.5f;
			}
			vHalf.x = cMath::Max(vHalf.x, 0.005f);
			vHalf.y = cMath::Max(vHalf.y, 0.005f);
			vHalf.z = cMath::Max(vHalf.z, 0.005f);
			for(int i=0; i<3; ++i)
			{
				mtxBox.m[i][0] *= vHalf.x;
				mtxBox.m[i][1] *= vHalf.y;
				mtxBox.m[i][2] *= vHalf.z;
			}
			cVector3f vEye = cMath::MatrixMul(cMath::MatrixInverse(mtxBox), apCamera->GetEyePosition());

			mpHaloVtxProgram->SetMatrixf("haloBox", mtxBox);
			mpHaloVtxProgram->SetVec3f("haloEye", vEye);
			// The original draws the parent itself, all its faces, and the
			// ones behind others fail the test: a lamp in full view comes
			// out at about 0.6 (measured on the game's lamps). The box
			// round it would come out at 1.
			float fWhole = pParent ? 0.65f : 1.0f;
			mpHaloVtxProgram->SetVec4f("haloParams", 0, mbIsHalo ? 1.0f : 0.0f, effects.mfHaloBrightness * fWhole, 0);
			mpHaloVtxProgram->SetVec2f("fogRange", apSettings->mfFogStart, apSettings->mfFogEnd);

			static tString vKnotNames[kHaloKnots];
			if(vKnotNames[0] == "")
				for(int i=0; i<kHaloKnots; ++i) vKnotNames[i] = "haloProfile[" + cString::ToString(i) + "]";

			const cColor *pProfile = GetHaloProfile();
			for(int i=0; i<kHaloKnots; ++i)
				mpHaloFragProgram->SetVec4f(vKnotNames[i], pProfile[i].r, pProfile[i].g, pProfile[i].b, 0);
			mpHaloFragProgram->SetVec4f("haloLook", effects.mfHaloStreaks, bFog ? 1.0f : 0.0f, 0, 0);
		}
		else if(aDraw.mpVtxProgram == mpShaftVtxProgram)
		{
			cMatrixf mtxInv = cMath::MatrixInverse(m_mtxShaft);
			cVector3f vEye = cMath::MatrixMul(mtxInv, apCamera->GetEyePosition());
			// The viewer looks along -z
			cMatrixf mtxView = cMath::MatrixMul(apCamera->GetViewMatrix(), m_mtxShaft);

			mpShaftFragProgram->SetVec3f("shaftEye", vEye);
			mpShaftFragProgram->SetVec4f("shaftViewZ", -mtxView.m[2][0], -mtxView.m[2][1], -mtxView.m[2][2], -mtxView.m[2][3]);
			mpShaftFragProgram->SetVec4f("shaftSize", mvSize.x * 0.5f, mvSize.y * 0.5f, 0, 0);
			// As the sheet's corners are coloured
			mpShaftFragProgram->SetVec4f("shaftColor", mColor.r * mfHaloAlpha, mColor.g * mfHaloAlpha,
													mColor.b * mfHaloAlpha, mColor.a * mfHaloAlpha);
			mpShaftFragProgram->SetVec4f("shaftLook", effects.mfShaftBrightness, effects.mfShaftDust, bFog ? 1.0f : 0.0f, 0);
			mpShaftFragProgram->SetVec2f("fogRange", apSettings->mfFogStart, apSettings->mfFogEnd);
		}
	}

	//-----------------------------------------------------------------------

	int cBillboard::GetMatrixUpdateCount()
	{
		return GetTransformUpdateCount();
	}

	//-----------------------------------------------------------------------


	void cBillboard::LoadXMLProperties(const tString asFile)
	{
		tString sNewFile = cString::SetFileExt(asFile,"bnt");
		tString sPath = mpFileSearcher->GetFilePath(sNewFile);
		if(sPath != "")
		{
			TiXmlDocument *pDoc = hplNew( TiXmlDocument, (sPath.c_str()) );
			if(pDoc->LoadFile())
			{
				TiXmlElement *pRootElem = pDoc->RootElement();

				TiXmlElement *pMainElem = pRootElem->FirstChildElement("MAIN");
				if(pMainElem!=nullptr)
				{
					mType = ToType(pMainElem->Attribute("Type"));
					tString sMaterial = cString::ToString(pMainElem->Attribute("Material"),"");
					bool bUsesOffset = cString::ToBool(pMainElem->Attribute("UseOffset"),false);
					if(bUsesOffset==false) mfForwardOffset =0;

					/////////////////
					//Halo stuff
					bool bIsHalo = cString::ToBool(pMainElem->Attribute("IsHalo"),false);
					SetIsHalo(bIsHalo);

					if(bIsHalo)
					{
						bool bHaloSourceIsParent  = cString::ToBool(pMainElem->Attribute("HaloSourceIsParent"),false);
						SetHaloSourceIsParent(bHaloSourceIsParent);

						if(bHaloSourceIsParent == false)
						{
							tString sSizeVec = cString::ToString(pMainElem->Attribute("HaloSourceSize"),"1 1 1");
							tFloatVec vSizeValues;
							cString::GetFloatVec(sSizeVec,vSizeValues,nullptr);
							SetHaloSourceSize(cVector3f(vSizeValues[0],vSizeValues[1],vSizeValues[2]));
						}
					}

					/////////////////
					//Load material
					iMaterial *pMat = mpMaterialManager->CreateMaterial(sMaterial);
					if(pMat)
					{
						SetMaterial(pMat);
					}
					else
					{
						Error("Couldn't load material '%s' in billboard file '%s'",
													sMaterial.c_str(), sNewFile.c_str());
					}
				}
				else
				{
					Error("Cannot find main element in %s\n",sNewFile.c_str());
				}
			}
			else
			{
				Error("Couldn't load file '%s'\n",sNewFile.c_str());
			}
			hplDelete(pDoc);
		}
		else
		{
			Error("Couldn't find file '%s'\n",sNewFile.c_str());
		}

	}

	//-----------------------------------------------------------------------

	bool cBillboard::IsVisible()
	{
		if(mColor.r <= 0 && mColor.g <= 0 && mColor.b <= 0) return false;

		return IsRendered();
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetIsHalo(bool abX)
	{
		mbIsHalo = abX;

		if(mbIsHalo)
		{
			mQueryObject.mpQuery = mpLowLevelGraphics->CreateOcclusionQuery();
			mMaxQueryObject.mpQuery = mpLowLevelGraphics->CreateOcclusionQuery();

			mfHaloAlpha = 1; //THis is to make sure that the new alpha is set to the mesh.
			SetHaloAlpha(0);
		}
		else if(mQueryObject.mpQuery)
		{
			mpLowLevelGraphics->DestroyOcclusionQuery(mQueryObject.mpQuery);
			mpLowLevelGraphics->DestroyOcclusionQuery(mMaxQueryObject.mpQuery);
		}
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetHaloSourceSize(const cVector3f &avSize)
	{
		mvHaloSourceSize = avSize;

		UpdateSourceBufferSize();
	}

	//-----------------------------------------------------------------------

	void cBillboard::SetHaloSourceIsParent(bool abX)
	{
		mbHaloSourceIsParent = abX;
	}

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	eBillboardType cBillboard::ToType(const char* apString)
	{
		if(apString==nullptr) return eBillboardType_Point;

		tString sType = cString::ToLowerCase(apString);

		if(sType == "point") return eBillboardType_Point;
		else if(sType == "axis") return eBillboardType_Axis;

		Warning("Invalid billboard type '%s'\n",apString);

		return eBillboardType_Point;
	}

	//-----------------------------------------------------------------------

	void cBillboard::UpdateSourceBufferSize()
	{
		int lNum = mpHaloSourceBuffer->GetVertexNum();
		float *pPositions = mpHaloSourceBuffer->GetArray(eVertexFlag_Position);

		for(int i=0; i<lNum;++i)
		{
			//X
			if(pPositions[0] < 0)
				pPositions[0] = -mvHaloSourceSize.x*0.5f;
			else
				pPositions[0] = mvHaloSourceSize.x*0.5f;
			//Y
			if(pPositions[1] < 0)
				pPositions[1] = -mvHaloSourceSize.y*0.5f;
			else
				pPositions[1] = mvHaloSourceSize.y*0.5f;
			//Z
			if(pPositions[2] < 0)
				pPositions[2] = -mvHaloSourceSize.z*0.5f;
			else
				pPositions[2] = mvHaloSourceSize.z*0.5f;

			pPositions += 4;
		}

		mpHaloSourceBuffer->UpdateData(eVertexFlag_Position, false);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// SAVE OBJECT STUFF
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	kBeginSerialize(cSaveData_cBillboard,cSaveData_iRenderable)
	kSerializeVar(msMaterial,eSerializeType_String)

	kSerializeVar(mType,eSerializeType_Int32)
	kSerializeVar(mvSize,eSerializeType_Vector3f)
	kSerializeVar(mvAxis,eSerializeType_Vector3f)
	kSerializeVar(mfForwardOffset,eSerializeType_Float32)
	kSerializeVar(mColor,eSerializeType_Color)
	kSerializeVar(mfHaloAlpha, eSerializeType_Float32)

	kSerializeVar(mbIsHalo, eSerializeType_Bool)
	kSerializeVar(mvHaloSourceSize, eSerializeType_Vector3f)
	kSerializeVar(mbHaloSourceIsParent, eSerializeType_Bool)
	kEndSerialize()

	//-----------------------------------------------------------------------

	iSaveObject* cSaveData_cBillboard::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
	{
		cWorld3D *pWorld = apGame->GetScene()->GetWorld3D();

		cBillboard *pBill = pWorld->CreateBillboard(msName,mvSize,msMaterial);

		return pBill;
	}

	//-----------------------------------------------------------------------

	int cSaveData_cBillboard::GetSaveCreatePrio()
	{
		return 3;
	}

	//-----------------------------------------------------------------------

	iSaveData* cBillboard::CreateSaveData()
	{
		return hplNew( cSaveData_cBillboard, () );
	}

	//-----------------------------------------------------------------------

	void cBillboard::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(cBillboard);

		pData->msMaterial = mpMaterial==nullptr ? "" : mpMaterial->GetName();

		kSaveData_SaveTo(mType);
		kSaveData_SaveTo(mvSize);
		kSaveData_SaveTo(mvAxis);
		kSaveData_SaveTo(mfForwardOffset);
		kSaveData_SaveTo(mColor);
		kSaveData_SaveTo(mfHaloAlpha);

		kSaveData_SaveTo(mbIsHalo);
		kSaveData_SaveTo(mvHaloSourceSize);
		kSaveData_SaveTo(mbHaloSourceIsParent);
	}

	//-----------------------------------------------------------------------

	void cBillboard::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(cBillboard);

		mType = (eBillboardType) pData->mType;
		kSaveData_LoadFrom(mvSize);
		kSaveData_LoadFrom(mvAxis);
		kSaveData_LoadFrom(mfForwardOffset);
		kSaveData_LoadFrom(mColor);
		kSaveData_LoadFrom(mfHaloAlpha);

		SetIsHalo(pData->mbIsHalo);
		SetHaloSourceSize(pData->mvHaloSourceSize);
		SetHaloSourceIsParent(pData->mbHaloSourceIsParent);
	}

	//-----------------------------------------------------------------------

	void cBillboard::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(cBillboard);

		if(mbIsHalo)
		{
			//Log("Setting up halo billboard '%s'\n",msName.c_str());
		}
	}

	//-----------------------------------------------------------------------
}
