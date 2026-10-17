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
#include "graphics/Beam.h"

#include "tinyxml.h"

#include "resources/Resources.h"
#include "resources/MaterialManager.h"
#include "resources/FileSearcher.h"
#include "graphics/VertexBuffer.h"
#include "graphics/Material.h"
#include "graphics/Graphics.h"
#include "graphics/LowLevelGraphics.h"
#include "scene/Camera3D.h"
#include "scene/World3D.h"
#include "scene/Scene.h"
#include "math/Math.h"

#include "game/Game.h"

namespace hpl {

	static cColor BeamVertexColor(const cColor &color, bool multiplyAlpha)
	{
		return multiplyAlpha ? cColor(color.r * color.a, color.g * color.a, color.b * color.a, color.a) : color;
	}


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cBeam::cBeam(const tString asName, cResources *apResources,cGraphics *apGraphics) :
	iRenderable(asName)
	{
		mpMaterialManager = apResources->GetMaterialManager();
		mpFileSearcher = apResources->GetFileSearcher();
		mpLowLevelGraphics = apGraphics->GetLowLevel();

		msFileName = "";

		mvSize = 1;

		mbTileHeight = true;
		mbMultiplyAlphaWithColor = false;

		mColor = cColor(1,1,1,1);

		mpMaterial = nullptr;

		mlLastRenderCount = -1;


		mpVtxBuffer = mpLowLevelGraphics->CreateVertexBuffer(0,
			eVertexBufferDrawType_Tri, eVertexBufferUsageType_Dynamic);
		mpVtxBuffer->UpdateQuadInstances(1)[0].mfKind = 2;
		mpVtxBuffer->Compile(0);

		mpEnd = hplNew( cBeamEnd, (asName + "_end",this));
		mpEnd->AddCallback(&mEndCallback);

		//Some temp setup
		mBoundingVolume.SetSize(cVector3f(mvSize.x, mvSize.y, mvSize.x));

		mbApplyTransformToBV = false;
	}

	//-----------------------------------------------------------------------

	cBeam::~cBeam()
	{
		hplDelete(mpEnd);
		if(mpMaterial) mpMaterialManager->Destroy(mpMaterial);
		if(mpVtxBuffer) hplDelete(mpVtxBuffer);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cBeam::SetSize(const cVector2f& avSize)
	{
		mvSize = avSize;
		mBoundingVolume.SetSize(cVector3f(mvSize.x, mvSize.y, mvSize.x));

		SetTransformUpdated();
	}

	//-----------------------------------------------------------------------

	void cBeam::SetTileHeight(bool abX)
	{
		if(mbTileHeight == abX) return;

		mbTileHeight = abX;

		SetTransformUpdated();
	}

	//-----------------------------------------------------------------------

	void cBeam::SetMultiplyAlphaWithColor(bool abX)
	{
		if(mbMultiplyAlphaWithColor == abX) return;

		mbMultiplyAlphaWithColor = abX;
	}

	//-----------------------------------------------------------------------

	void cBeam::SetColor(const cColor &aColor)
	{
		if(mColor == aColor) return;

		mColor = aColor;
		mpVtxBuffer->UpdateQuadInstances(1)[0].mColor = BeamVertexColor(mColor, mbMultiplyAlphaWithColor);
	}

	//-----------------------------------------------------------------------

	void cBeam::SetMaterial(iMaterial * apMaterial)
	{
		mpMaterial = apMaterial;
	}

	//-----------------------------------------------------------------------

	cBoundingVolume* cBeam::GetBoundingVolume()
	{
		if(mbUpdateBoundingVolume)
		{
			cVector3f vMax = GetWorldPosition();
			cVector3f vMin = vMax;
			cVector3f vEnd = mpEnd->GetWorldPosition();

			if(vMax.x < vEnd.x) vMax.x = vEnd.x;
			if(vMax.y < vEnd.y) vMax.y = vEnd.y;
			if(vMax.z < vEnd.z) vMax.z = vEnd.z;

			if(vMin.x > vEnd.x) vMin.x = vEnd.x;
			if(vMin.y > vEnd.y) vMin.y = vEnd.y;
			if(vMin.z > vEnd.z) vMin.z = vEnd.z;

			vMin -= cVector3f(mvSize.x);
			vMax += cVector3f(mvSize.x);

			mBoundingVolume.SetLocalMinMax(vMin,vMax);

			mbUpdateBoundingVolume = false;
		}

		return &mBoundingVolume;
	}

	//-----------------------------------------------------------------------

	void cBeam::UpdateGraphics(cCamera3D *apCamera,float afFrameTime, cRenderList *apRenderList)
	{
		UpdateInstance();
	}

	void cBeam::UpdateInstance()
	{
		mvAxis = mpEnd->GetWorldPosition() - GetWorldPosition();
		mvMidPosition = GetWorldPosition() + mvAxis * 0.5f;
		const float distance = mvAxis.Length();
		mvAxis.Normalise();

		cQuadInstance &sheet = mpVtxBuffer->UpdateQuadInstances(1)[0];
		sheet.mfKind = 2;
		sheet.mvRight = cVector3f(mvSize.x * 0.5f,0,0);
		sheet.mvUp = cVector3f(0,distance * 0.5f,0);
		sheet.mvUV[1] = 1;
		sheet.mvUV[3] = mbTileHeight ? -distance / mvSize.y : 0;
	}

	//-----------------------------------------------------------------------

	cMatrixf* cBeam::GetModelMatrix(cCamera3D *apCamera)
	{
		if(apCamera==nullptr)return &GetWorldMatrix();

		m_mtxTempTransform = GetWorldMatrix();
		cVector3f vForward, vRight, vUp;

		cVector3f vCameraForward = apCamera->GetPosition() - GetWorldPosition();
		vCameraForward.Normalise();

		vUp = mvAxis;//cMath::MatrixMul(GetWorldMatrix().GetRotation(),mvAxis);

		if(vUp == vForward)
		{
			vRight = cMath::Vector3Cross(vUp, vCameraForward);
			Warning("Beam Right vector is not correct! Contact programmer!\n");
		}
		else
			vRight = cMath::Vector3Cross(vUp, vCameraForward);

		vRight.Normalise();
		vForward = cMath::Vector3Cross(vRight, vUp);

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

		m_mtxTempTransform.SetTranslation(mvMidPosition);

		return &m_mtxTempTransform;
	}

	//-----------------------------------------------------------------------

	int cBeam::GetMatrixUpdateCount()
	{
		return GetTransformUpdateCount();
	}

	//-----------------------------------------------------------------------


	bool cBeam::LoadXMLProperties(const tString asFile)
	{
		msFileName = asFile;

		tString sNewFile = cString::SetFileExt(asFile,"beam");
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
					tString sMaterial = cString::ToString(pMainElem->Attribute("Material"),"");
					cVector2f vSize = cString::ToVector2f(pMainElem->Attribute("Size"),1);

					bool bTileHeight = cString::ToBool(pMainElem->Attribute("TileHeight"),true);
					bool bMultiplyAlphaWithColor = cString::ToBool(pMainElem->Attribute("MultiplyAlphaWithColor"),false);

					cColor StartColor = cString::ToColor(pMainElem->Attribute("StartColor"),cColor(1,1));
					cColor EndColor = cString::ToColor(pMainElem->Attribute("EndColor"),cColor(1,1));


					SetSize(vSize);
					SetTileHeight(bTileHeight);
					SetMultiplyAlphaWithColor(bMultiplyAlphaWithColor);
					SetColor(StartColor);
					mpEnd->SetColor(EndColor);

					/////////////////
					//Load material
					iMaterial *pMat = mpMaterialManager->CreateMaterial(sMaterial);
					if(pMat)	{
						SetMaterial(pMat);
					}
					else{
						Error("Couldn't load material '%s' in Beam file '%s'",
											sMaterial.c_str(), sNewFile.c_str());
						return false;
					}
				}
				else
				{
					Error("Cannot find main element in %s\n",sNewFile.c_str());
					return false;
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
			return false;
		}

		return true;
	}

	//-----------------------------------------------------------------------

	bool cBeam::IsVisible()
	{
		if(mColor.r <= 0 && mColor.g <= 0 && mColor.b <= 0) return false;

		return IsRendered();
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// BEAM END TRANSFORM UPDATE CALLBACK
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cBeamEnd_UpdateCallback::OnTransformUpdate(iEntity3D * apEntity)
	{
		cBeamEnd *pEnd = static_cast<cBeamEnd*>(apEntity);

		pEnd->mpBeam->SetTransformUpdated(true);
	}

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// BEAM END
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cBeamEnd::SetColor(const cColor &aColor)
	{
		if(mColor == aColor) return;

		mColor = aColor;

		mpBeam->mpVtxBuffer->UpdateQuadInstances(1)[0].mEndColor = BeamVertexColor(mColor, mpBeam->mbMultiplyAlphaWithColor);
	}

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// SAVE OBJECT STUFF
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

}
