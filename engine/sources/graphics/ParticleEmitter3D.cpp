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
#include "graphics/ParticleEmitter3D.h"
#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "graphics/Graphics.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/GPUProgram.h"
#include "graphics/Material.h"
#include "graphics/Renderer3D.h"
#include "resources/GpuProgramManager.h"
#include "scene/Light3D.h"
#include "scene/Light3DSpot.h"
#include "math/BoundingVolume.h"
#include "scene/Camera3D.h"
#include "math/Math.h"

#include "game/Game.h"
#include "scene/Scene.h"
#include "scene/World3D.h"

#include "graphics/ParticleSystem3D.h"


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iParticleEmitter3D::iParticleEmitter3D(tString asName,tMaterialVec *avMaterials,
									unsigned int alMaxParticles, cVector3f avSize,
									cGraphics *apGraphics,cResources *apResources)
	:iRenderable(asName),iParticleEmitter(avMaterials, alMaxParticles,avSize,apGraphics,apResources)
	{
		mpVtxBuffer = apGraphics->GetLowLevel()->CreateVertexBuffer(0,
			eVertexBufferDrawType_Tri, eVertexBufferUsageType_Stream);
		mpVtxBuffer->SetQuadInstances({});
		mpVtxBuffer->Compile(0);
		mpSoftProgram = nullptr;

		//Setup vars
		mlDirectionUpdateCount = -1;
		mvDirection = cVector3f(0,0,0);

		mvMaxDrawSize =0;

		mlAxisDrawUpdateCount = -1;

		mbApplyTransformToBV = false;

		mlUpdateCount =0;
		mfTimeStepAccum =0;

		//If Direction should be udpdated
		mbUsesDirection = false;

		mDrawType = eParticleEmitter3DType_FixedPoint;

		mCoordSystem = eParticleEmitter3DCoordSystem_World;


	}

	//-----------------------------------------------------------------------

	iParticleEmitter3D::~iParticleEmitter3D()
	{
		hplDelete(mpVtxBuffer);

		cGpuProgramManager *pProgramManager = mpResources->GetGpuProgramManager();
		if(mpSoftProgram) pProgramManager->Destroy(mpSoftProgram);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::SetSubDivUV(const cVector2l &avSubDiv)
	{
		//Check so that there is any subdivision and that no sub divison axis is
		//equal or below zero
		if( (avSubDiv.x > 1 || avSubDiv.y > 1) && (avSubDiv.x >0 && avSubDiv.y >0))
		{
			int lSubDivNum = avSubDiv.x * avSubDiv.y;

			mvSubDivUV.resize(lSubDivNum);

			float fInvW = 1.0f / (float)avSubDiv.x;
			float fInvH = 1.0f / (float)avSubDiv.y;

			for(int x=0; x < avSubDiv.x; ++x)
			for(int y=0; y < avSubDiv.y; ++y)
			{
				int lIdx = y*avSubDiv.x + x;

				float fX = (float)x;
				float fY = (float)y;

				cPESubDivision *pSubDiv = &mvSubDivUV[lIdx];

				pSubDiv->mvUV[0] = cVector3f( (fX +1)*fInvW,	(fY +1)*fInvH,0);	//1,1
				pSubDiv->mvUV[1] = cVector3f( fX*fInvW,			(fY +1)*fInvH,0);	//0,1
				pSubDiv->mvUV[2] = cVector3f( fX*fInvW,			fY*fInvH,0);		//0,0
				pSubDiv->mvUV[3] = cVector3f( (fX +1)*fInvW,	fY*fInvH,0);		//1,0
			}
		}

	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::UpdateLogic(float afTimeStep)
	{
		if(IsActive()==false) return;

		//Skip as it has never been sued before right?

		Update(afTimeStep);
	}

	//-----------------------------------------------------------------------

	iMaterial* iParticleEmitter3D::GetMaterial()
	{
		return (*mvMaterials)[(int)mfFrame];
	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::UpdateGraphics(cCamera3D *apCamera,float afFrameTime, cRenderList *apRenderList)
	{
		if(apCamera==nullptr || !mbUpdateGfx) return;
		// PEType=Beam has never supplied geometry (the simulation branch is
		// empty as well). Keep it empty rather than inventing a new effect.
		if(mPEType == ePEType_Beam) return;

		if(mDrawType == eParticleEmitter3DType_Axis && mlAxisDrawUpdateCount != GetMatrixUpdateCount())
		{
			mlAxisDrawUpdateCount = GetMatrixUpdateCount();
			const cMatrixf inverse = cMath::MatrixInverse(GetWorldMatrix());
			mvRight = inverse.GetRight();
			mvForward = inverse.GetForward();
		}

		auto &instances = mpVtxBuffer->UpdateQuadInstances(mlNumOfParticles);
		for(unsigned int i=0; i<mlNumOfParticles; ++i)
		{
			const cParticle &particle = *mvParticles[i];
			cQuadInstance &sheet = instances[i];
			sheet = cQuadInstance();
			sheet.mfKind = 3;
			sheet.mColor = sheet.mEndColor = particle.mColor;
			sheet.mvUV[1] = 1;
			sheet.mvUV[3] = 0;
			if(mvSubDivUV.size() > 1)
			{
				const cPESubDivision &sub = mvSubDivUV[particle.mlSubDivNum];
				sheet.mvUV[0] = sub.mvUV[1].x;
				sheet.mvUV[1] = sub.mvUV[1].y;
				sheet.mvUV[2] = sub.mvUV[3].x;
				sheet.mvUV[3] = sub.mvUV[3].y;
			}

			cVector3f position = particle.mvPos;
			if(mCoordSystem == eParticleEmitter3DCoordSystem_Local)
				position = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), position);
			if(mDrawType == eParticleEmitter3DType_Axis)
			{
				sheet.mvCentre = position;
				sheet.mvRight = mvRight * particle.mvSize.x;
				sheet.mvUp = mvForward * -particle.mvSize.y;
				continue;
			}

			position = cMath::MatrixMul(apCamera->GetViewMatrix(), position);
			sheet.mvCentre = position;
			cVector2f size = mvDrawSize;
			if(mDrawType != eParticleEmitter3DType_FixedPoint) size = size * particle.mvSize;
			sheet.mvRight = cVector3f(size.x,0,0);
			sheet.mvUp = cVector3f(0,size.y,0);
			if(mDrawType == eParticleEmitter3DType_DynamicPoint && mbUsePartSpin)
			{
				const cMatrixf rotation = cMath::MatrixRotateZ(particle.mfSpin);
				sheet.mvRight = cMath::MatrixMul(rotation, sheet.mvRight);
				sheet.mvUp = cMath::MatrixMul(rotation, sheet.mvUp);
			}
			else if(mDrawType == eParticleEmitter3DType_Line)
			{
				cVector3f last = particle.mvLastPos;
				if(mCoordSystem == eParticleEmitter3DCoordSystem_Local)
					last = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), last);
				last = cMath::MatrixMul(apCamera->GetViewMatrix(), last);
				cVector3f along(0,1,0);
				cVector2f across(1,0);
				if(position != last)
				{
					along = position - last;
					along.Normalise();
					across = cVector2f(along.y,-along.x);
					across.Normalise();
				}
				sheet.mvCentre = (position + last) * 0.5f;
				sheet.mvRight = cVector3f(across.x * size.x, across.y * size.x,0);
				sheet.mvUp = (position - last) * 0.5f + along * size.y;
			}
		}
		mbUpdateGfx = false;
	}

	//-----------------------------------------------------------------------

	bool iParticleEmitter3D::GetTransDraw(cCamera3D *apCamera, cTransDraw &aDraw)
	{
		iMaterial *pMaterial = GetMaterial();
		eMaterialBlendMode blend = pMaterial->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr);
		if(blend != eMaterialBlendMode_Alpha && blend != eMaterialBlendMode_Add) return false;

		if(mpSoftProgram==nullptr)
		{
			cGpuProgramManager *pProgramManager = mpResources->GetGpuProgramManager();
			mpSoftProgram = pProgramManager->CreateProgram("Particle");
		}
		if(mpSoftProgram==nullptr) return false;

		aDraw.mpProgram = mpSoftProgram;
		aDraw.mbDepthTest = pMaterial->GetDepthTest();
		return true;
	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::SetupTransDraw(cCamera3D *apCamera, cRenderSettings *apSettings, cTransDraw &aDraw)
	{
		static const int kLights = 6;

		const cEffectSettings &effects = mpGraphics->GetLowLevel()->GetEffects();
		bool bAdded = GetMaterial()->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr) == eMaterialBlendMode_Add;

		mpSoftProgram->SetMatrixf(eProgramParam::partToWorld, *GetModelMatrix(apCamera));
		mpSoftProgram->SetVec2f(eProgramParam::fogRange, apSettings->mfFogStart, apSettings->mfFogEnd);

		// The fade takes about the size of a particle: wisps of smoke
		// fade over more than drops of water do
		float fSoft = cMath::Clamp(cMath::Max(mvMaxDrawSize.x, mvMaxDrawSize.y), 0.03f, 0.6f) * effects.mfParticleSoft;
		mpSoftProgram->SetVec4f(eProgramParam::partLook, fSoft, apSettings->mbFogActive ? 1.0f : 0.0f, bAdded ? 1.0f : 0.0f, 0);

		/////////////////////////////////////
		// The lights that reach the particles: the strongest few
		int lCount = 0;
		if(bAdded==false && effects.mfParticleLight > 0 && mpWorld)
		{
			iLight3D *vLights[kLights];
			float vScores[kLights];
			cBoundingVolume *pBV = GetBoundingVolume();
			cVector3f vCentre = pBV->GetWorldCenter();

			cLight3DListIterator lightIt = mpWorld->GetLightIterator();
			while(lightIt.HasNext())
			{
				iLight3D *pLight = lightIt.Next();
				if(pLight->IsVisible()==false) continue;
				const cColor &col = pLight->GetDiffuseColor();
				float fBright = cMath::Max(col.r, cMath::Max(col.g, col.b)) * pLight->GetLook().mfBrightness;
				if(fBright <= 0.01f || pLight->GetFarAttenuation() <= 0) continue;
				if(pLight->CollidesWithBV(pBV)==false) continue;

				float fDist = cMath::Vector3Dist(pLight->GetLightPosition(), vCentre);
				float fScore = fBright * cMath::Max(1.0f - fDist / (pLight->GetFarAttenuation() + pBV->GetRadius()), 0.05f);

				// Kept in order of score
				int lAt = lCount;
				while(lAt > 0 && vScores[lAt-1] < fScore) --lAt;
				if(lAt >= kLights) continue;
				for(int i=cMath::Min(lCount, kLights-1); i>lAt; --i)
				{
					vLights[i] = vLights[i-1];
					vScores[i] = vScores[i-1];
				}
				vLights[lAt] = pLight;
				vScores[lAt] = fScore;
				if(lCount < kLights) ++lCount;
			}

			for(int i=0; i<lCount; ++i)
			{
				iLight3D *pLight = vLights[i];
				const cLightLook &look = pLight->GetLook();
				cVector3f vPos = pLight->GetLightPosition();
				cColor col = pLight->GetDiffuseColor() * look.mTint * look.mfBrightness;

				float fEdge = -2.0f;
				cVector3f vDir(0,0,-1);
				if(pLight->GetLightType() == eLight3DType_Spot)
				{
					cLight3DSpot *pSpot = static_cast<cLight3DSpot*>(pLight);
					// A spotlight shines along its own -z
					vDir = pSpot->GetViewMatrix().GetForward() * -1.0f;
					fEdge = std::cos(pSpot->GetFOV() * 0.5f * (look.mbBeam ? look.mfBeamWidth : 1.0f));
				}

				mpSoftProgram->SetVec4f(iGpuProgram::Element(eProgramParam::partLightPos, i), vPos.x, vPos.y, vPos.z, pLight->GetFarAttenuation());
				mpSoftProgram->SetVec4f(iGpuProgram::Element(eProgramParam::partLightColor, i), col.r, col.g, col.b, fEdge);
				mpSoftProgram->SetVec4f(iGpuProgram::Element(eProgramParam::partLightDir, i), vDir.x, vDir.y, vDir.z, 0);
			}
		}

		// What the map's own light adds to the brightness in the dark
		const cColor &ambient = apSettings->mAmbientColor;
		float fDark = cMath::Min(effects.mfParticleDark + (ambient.r + ambient.g + ambient.b) / 3.0f, 1.0f);
		mpSoftProgram->SetVec4f(eProgramParam::partLight, (float)lCount, bAdded ? 0.0f : effects.mfParticleLight, fDark, 0);
	}

	//-----------------------------------------------------------------------

	iVertexBuffer* iParticleEmitter3D::GetVertexBuffer()
	{
		return mpVtxBuffer;
	}

	//-----------------------------------------------------------------------

	bool iParticleEmitter3D::IsVisible()
	{
		if(IsActive()==false) return false;
		return IsRendered();
	}

	//-----------------------------------------------------------------------

	cBoundingVolume* iParticleEmitter3D::GetBoundingVolume()
	{
		if(mbUpdateBV)
		{
			cVector3f vMin(100000, 100000, 100000);
			cVector3f vMax(-100000, -100000, -100000);

			for(int i=0;i<(int)mlNumOfParticles;i++)
			{
				cParticle *pParticle = mvParticles[i];

				//X
				if(pParticle->mvPos.x < vMin.x)		 vMin.x = pParticle->mvPos.x;
				else if(pParticle->mvPos.x > vMax.x) vMax.x = pParticle->mvPos.x;

				//Y
				if(pParticle->mvPos.y < vMin.y)		 vMin.y = pParticle->mvPos.y;
				else if(pParticle->mvPos.y > vMax.y) vMax.y = pParticle->mvPos.y;

				//Z
				if(pParticle->mvPos.z < vMin.z)		 vMin.z = pParticle->mvPos.z;
				else if(pParticle->mvPos.z > vMax.z) vMax.z = pParticle->mvPos.z;
			}

			vMax += cVector3f(mvMaxDrawSize.x,mvMaxDrawSize.y, mvMaxDrawSize.x);
			vMin -= cVector3f(mvMaxDrawSize.x,mvMaxDrawSize.y, mvMaxDrawSize.x);

			mBoundingVolume.SetLocalMinMax(vMin, vMax);


			if(mCoordSystem == eParticleEmitter3DCoordSystem_Local)
			{
				mBoundingVolume.SetTransform(mpParentSystem->GetWorldMatrix());
			}

			mbUpdateBV = false;
		}

		return &mBoundingVolume;
	}

	//-----------------------------------------------------------------------

	cMatrixf* iParticleEmitter3D::GetModelMatrix(cCamera3D *apCamera)
	{
		if(apCamera)
		{
			if(mDrawType == eParticleEmitter3DType_Axis)
			{
				m_mtxTemp = cMatrixf::Identity;
			}
			//This is really not good...
			else if(mCoordSystem == eParticleEmitter3DCoordSystem_World)
			{
				m_mtxTemp = cMath::MatrixInverse(apCamera->GetViewMatrix());
			}
			else
			{
				m_mtxTemp = cMath::MatrixInverse(apCamera->GetViewMatrix());
			}


			return &m_mtxTemp;

		}
		else
		{
			return &GetWorldMatrix();
		}
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PROTECTED METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// SAVE OBJECT STUFF
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	kBeginSerialize(cSaveData_iParticleEmitter3D,cSaveData_iRenderable)
	/*kSerializeVar(msDataName, eSerializeType_String)
	kSerializeVar(mvDataSize, eSerializeType_Vector3f)
	kSerializeVar(mlTime, eSerializeType_Int32)
	kSerializeVar(mbDying, eSerializeType_Bool)
	kSerializeVar(mfFrame, eSerializeType_Float32)*/
	kEndSerialize()

	//-----------------------------------------------------------------------

	iSaveObject* cSaveData_iParticleEmitter3D::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
	{
		return nullptr;
	}

	//-----------------------------------------------------------------------

	int cSaveData_iParticleEmitter3D::GetSaveCreatePrio()
	{
		return 3;
	}

	//-----------------------------------------------------------------------

	iSaveData* iParticleEmitter3D::CreateSaveData()
	{
		return nullptr;
	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::SaveToSaveData(iSaveData *apSaveData)
	{
	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::LoadFromSaveData(iSaveData *apSaveData)
	{
	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
	}

	//-----------------------------------------------------------------------
}
