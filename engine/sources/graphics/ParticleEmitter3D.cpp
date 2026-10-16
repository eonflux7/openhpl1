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
		mpVtxBuffer = apGraphics->GetLowLevel()->CreateVertexBuffer(
							eVertexFlag_Position | eVertexFlag_Color0 | eVertexFlag_Texture0,
							eVertexBufferDrawType_Tri, eVertexBufferUsageType_Stream,
							alMaxParticles*4, alMaxParticles*6);

		//Fill the indices with quads
		for(int i=0;i<(int)alMaxParticles;i++)
		{
			int lStart = i*4;
			for(int j=0;j<3;j++) mpVtxBuffer->AddIndex(lStart + j);
			for(int j=2;j<5;j++) mpVtxBuffer->AddIndex(lStart + (j==4?0:j));
		}

		//Fill with tetxure coords (will do for most particle systems)
		for(int i=0;i<(int)alMaxParticles;i++)
		{
			mpVtxBuffer->AddVertex(eVertexFlag_Texture0, cVector3f(1,1,0));
			mpVtxBuffer->AddVertex(eVertexFlag_Texture0, cVector3f(0,1,0));
			mpVtxBuffer->AddVertex(eVertexFlag_Texture0, cVector3f(0,0,0));
			mpVtxBuffer->AddVertex(eVertexFlag_Texture0, cVector3f(1,0,0));
		}

		//Set default values for pos and col
		for(int i=0;i<(int)alMaxParticles*4;i++){
			mpVtxBuffer->AddVertex(eVertexFlag_Position, 0);
			mpVtxBuffer->AddColor(eVertexFlag_Color0, cColor(1,1));
		}

		mpVtxBuffer->Compile(0);

		mpSoftVtxProgram = nullptr;
		mpSoftFragProgram = nullptr;

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
		if(mpSoftVtxProgram) pProgramManager->Destroy(mpSoftVtxProgram);
		if(mpSoftFragProgram) pProgramManager->Destroy(mpSoftFragProgram);
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

	static inline void SetCol(float *apCol, const cColor &aCol)
	{
		apCol[0] = aCol.r;
		apCol[1] = aCol.g;
		apCol[2] = aCol.b;
		apCol[3] = aCol.a;
	}

	static inline void SetPos(float *apPos, const cVector3f &aPos)
	{
		apPos[0] = aPos.x;
		apPos[1] = aPos.y;
		apPos[2] = aPos.z;
	}

	static inline void SetTex(float *apTex, const cVector3f &aPos)
	{
		apTex[0] = aPos.x;
		apTex[1] = aPos.y;
		apTex[2] = aPos.z;
	}

	void iParticleEmitter3D::UpdateGraphics(cCamera3D *apCamera,float afFrameTime, cRenderList *apRenderList)
	{
		if(apCamera==nullptr) return;

		if(mbUpdateGfx)
		{
			float *pPosArray = mpVtxBuffer->GetArray(eVertexFlag_Position);
			float *pColArray = mpVtxBuffer->GetArray(eVertexFlag_Color0);
// NEW
			if ( mPEType == ePEType_Beam)
			{


			}
			else
			{
			//////////////////////////////////////////////////
			// SUB DIVISION SET UP
				if(mvSubDivUV.size() > 1)
				{
					float *pTexArray = mpVtxBuffer->GetArray(eVertexFlag_Texture0);

					for(int i=0;i<(int)mlNumOfParticles;i++)
					{
						cParticle *pParticle = mvParticles[i];

						cPESubDivision &subDiv = mvSubDivUV[pParticle->mlSubDivNum];

						SetTex(&pTexArray[i*12 + 0*3],subDiv.mvUV[0]);
						SetTex(&pTexArray[i*12 + 1*3],subDiv.mvUV[1]);
						SetTex(&pTexArray[i*12 + 2*3],subDiv.mvUV[2]);
						SetTex(&pTexArray[i*12 + 3*3],subDiv.mvUV[3]);

					}
				}

				//////////////////////////////////////////////////
				// FIXED POINT
				if(mDrawType == eParticleEmitter3DType_FixedPoint)
				{
					cVector3f vAdd[4] = {
						cVector3f( mvDrawSize.x,-mvDrawSize.y,0),
						cVector3f(-mvDrawSize.x,-mvDrawSize.y,0),
						cVector3f(-mvDrawSize.x, mvDrawSize.y,0),
						cVector3f( mvDrawSize.x, mvDrawSize.y,0)
					};

				// NEW

				// ---


					int lVtxStride = kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)];
					int lVtxQuadSize = lVtxStride*4;

					for(int i=0;i<(int)mlNumOfParticles;i++)
					{
						cParticle *pParticle = mvParticles[i];

						//This is not the fastest thing possible...
						cVector3f vParticlePos = pParticle->mvPos;

						if(mCoordSystem == eParticleEmitter3DCoordSystem_Local){
							vParticlePos = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), vParticlePos);
						}

						cVector3f vPos = cMath::MatrixMul(apCamera->GetViewMatrix(), vParticlePos);


						SetPos(&pPosArray[i*lVtxQuadSize + 0*lVtxStride], vPos + vAdd[0]);
						SetCol(&pColArray[i*16 + 0*4], pParticle->mColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 1*lVtxStride], vPos + vAdd[1]);
						SetCol(&pColArray[i*16 + 1*4], pParticle->mColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 2*lVtxStride], vPos + vAdd[2]);
						SetCol(&pColArray[i*16 + 2*4], pParticle->mColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 3*lVtxStride], vPos + vAdd[3]);
						SetCol(&pColArray[i*16 + 3*4], pParticle->mColor);
					}
				}
				//////////////////////////////////////////////////
				// DYNAMIC POINT
				else if(mDrawType == eParticleEmitter3DType_DynamicPoint)
				{
					cVector3f vAdd[4] = {
						cVector3f( mvDrawSize.x,-mvDrawSize.y,0),
							cVector3f(-mvDrawSize.x,-mvDrawSize.y,0),
							cVector3f(-mvDrawSize.x, mvDrawSize.y,0),
							cVector3f( mvDrawSize.x, mvDrawSize.y,0)
					};

					int lVtxStride = kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)];
					int lVtxQuadSize = lVtxStride*4;

					for(int i=0;i<(int)mlNumOfParticles;i++)
					{
						cParticle *pParticle = mvParticles[i];

						//This is not the fastest thing possible
						cVector3f vParticlePos = pParticle->mvPos;


						if(mCoordSystem == eParticleEmitter3DCoordSystem_Local){
							vParticlePos = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), vParticlePos);
						}

						cVector3f vPos = cMath::MatrixMul(apCamera->GetViewMatrix(), vParticlePos);


						// NEW

						cVector3f vParticleSize = pParticle->mvSize;
						cColor& colParticleColor = pParticle->mColor;

						if ( mbUsePartSpin )
						{
							cMatrixf mtxRotationMatrix = cMath::MatrixRotateZ(pParticle->mfSpin);


							SetPos(&pPosArray[i*lVtxQuadSize + 0*lVtxStride], vPos + cMath::MatrixMul(mtxRotationMatrix, vAdd[0]*vParticleSize));
							SetCol(&pColArray[i*16 + 0*4], colParticleColor);

							SetPos(&pPosArray[i*lVtxQuadSize + 1*lVtxStride], vPos + cMath::MatrixMul(mtxRotationMatrix, vAdd[1]*vParticleSize));
							SetCol(&pColArray[i*16 + 1*4], colParticleColor);

							SetPos(&pPosArray[i*lVtxQuadSize + 2*lVtxStride], vPos + cMath::MatrixMul(mtxRotationMatrix, vAdd[2]*vParticleSize));
							SetCol(&pColArray[i*16 + 2*4], colParticleColor);

							SetPos(&pPosArray[i*lVtxQuadSize + 3*lVtxStride], vPos + cMath::MatrixMul(mtxRotationMatrix, vAdd[3]*vParticleSize));
							SetCol(&pColArray[i*16 + 3*4], colParticleColor);

						}
						else
						{
						//--

							SetPos(&pPosArray[i*lVtxQuadSize + 0*lVtxStride], vPos + vAdd[0]*vParticleSize);
							SetCol(&pColArray[i*16 + 0*4], colParticleColor);

							SetPos(&pPosArray[i*lVtxQuadSize + 1*lVtxStride], vPos + vAdd[1]*vParticleSize);
							SetCol(&pColArray[i*16 + 1*4], colParticleColor);

							SetPos(&pPosArray[i*lVtxQuadSize + 2*lVtxStride], vPos + vAdd[2]*vParticleSize);
							SetCol(&pColArray[i*16 + 2*4], colParticleColor);

							SetPos(&pPosArray[i*lVtxQuadSize + 3*lVtxStride], vPos + vAdd[3]*vParticleSize);
							SetCol(&pColArray[i*16 + 3*4], colParticleColor);

						}
					}
				}
				//////////////////////////////////////////////////
				// LINE
				else if(mDrawType == eParticleEmitter3DType_Line)
				{
					cVector3f vAdd[4] = {
						cVector3f( mvDrawSize.x,-mvDrawSize.y,0),
						cVector3f(-mvDrawSize.x,-mvDrawSize.y,0),
						cVector3f(-mvDrawSize.x, mvDrawSize.y,0),
						cVector3f( mvDrawSize.x, mvDrawSize.y,0)
					};

					int lVtxStride = kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)];
					int lVtxQuadSize = lVtxStride*4;

					for(int i=0;i<(int)mlNumOfParticles;i++)
					{
						cParticle *pParticle = mvParticles[i];

						//This is not the fastest thing possible...

						cVector3f vParticlePos1 = pParticle->mvPos;
						cVector3f vParticlePos2 = pParticle->mvLastPos;

						if(mCoordSystem == eParticleEmitter3DCoordSystem_Local){
							vParticlePos1 = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), vParticlePos1);
							vParticlePos2 = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), vParticlePos2);
						}

						cVector3f vPos1 = cMath::MatrixMul(apCamera->GetViewMatrix(), vParticlePos1);
						cVector3f vPos2 = cMath::MatrixMul(apCamera->GetViewMatrix(), vParticlePos2);

						cVector3f vDirY;
						cVector2f vDirX;

						if(vPos1 == vPos2)
						{
							vDirY = cVector3f(0,1,0);
							vDirX = cVector2f(1,0);
						}
						else
						{
							vDirY = vPos1 - vPos2;
							vDirY.Normalise();
							vDirX = cVector2f(vDirY.y,-vDirY.x);
							vDirX.Normalise();
						}

						vDirX = vDirX * mvDrawSize.x * pParticle->mvSize.x;
						vDirY = vDirY * mvDrawSize.y * pParticle->mvSize.y;

						SetPos(&pPosArray[i*lVtxQuadSize + 0*lVtxStride], vPos2 + vDirY*-1 + vDirX);
						SetCol(&pColArray[i*16 + 0*4], pParticle->mColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 1*lVtxStride], vPos2 + vDirY*-1 + vDirX*-1);
						SetCol(&pColArray[i*16 + 1*4], pParticle->mColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 2*lVtxStride], vPos1 + vDirY + vDirX*-1);
						SetCol(&pColArray[i*16 + 2*4], pParticle->mColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 3*lVtxStride], vPos1 + vDirY + vDirX);
						SetCol(&pColArray[i*16 + 3*4], pParticle->mColor);
					}
				}
				//////////////////////////////////////////////////
				// AXIS
				else if(mDrawType == eParticleEmitter3DType_Axis)
				{
					if(mlAxisDrawUpdateCount != GetMatrixUpdateCount())
					{
						mlAxisDrawUpdateCount = GetMatrixUpdateCount();
						cMatrixf mtxInv = cMath::MatrixInverse(GetWorldMatrix());
						mvRight = mtxInv.GetRight();
						mvForward = mtxInv.GetForward();
					}

					cVector3f vAdd[4];

					int lVtxStride = kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)];
					int lVtxQuadSize = lVtxStride*4;

					for(int i=0;i<(int)mlNumOfParticles;i++)
					{
						cParticle *pParticle = mvParticles[i];

						//This is not the fastest thing possible
						cVector3f vParticlePos = pParticle->mvPos;


						if(mCoordSystem == eParticleEmitter3DCoordSystem_Local){
							vParticlePos = cMath::MatrixMul(mpParentSystem->GetWorldMatrix(), vParticlePos);
						}

						cVector3f vPos = vParticlePos;//cMath::MatrixMul(apCamera->GetViewMatrix(), vParticlePos);
						cVector2f &vSize = pParticle->mvSize;

						vAdd[0] = mvRight	* vSize.x	 +	mvForward * vSize.y;
						vAdd[1] = mvRight * -vSize.x	 +	mvForward * vSize.y;
						vAdd[2] = mvRight * -vSize.x	 +	mvForward * -vSize.y;
						vAdd[3] = mvRight	* vSize.x	 +	mvForward * -vSize.y;

						cColor& colParticleColor = pParticle->mColor;

						SetPos(&pPosArray[i*lVtxQuadSize + 0*lVtxStride], vPos + vAdd[0]);
						SetCol(&pColArray[i*16 + 0*4], colParticleColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 1*lVtxStride], vPos + vAdd[1]);
						SetCol(&pColArray[i*16 + 1*4], colParticleColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 2*lVtxStride], vPos + vAdd[2]);
						SetCol(&pColArray[i*16 + 2*4], colParticleColor);

						SetPos(&pPosArray[i*lVtxQuadSize + 3*lVtxStride], vPos + vAdd[3]);
						SetCol(&pColArray[i*16 + 3*4], colParticleColor);
					}
				}

				mpVtxBuffer->SetElementNum(mlNumOfParticles * 6);

				mbUpdateGfx = false;

				//Update the vertex buffer data

				if(mvSubDivUV.size() > 1)
					mpVtxBuffer->UpdateData(eVertexFlag_Position | eVertexFlag_Color0 | eVertexFlag_Texture0, false);
				else
					mpVtxBuffer->UpdateData(eVertexFlag_Position | eVertexFlag_Color0, false);

				}
			}

	}

	//-----------------------------------------------------------------------

	bool iParticleEmitter3D::GetTransDraw(cCamera3D *apCamera, cTransDraw &aDraw)
	{
		if(mpGraphics->GetLowLevel()->GetEffects().mbParticles==false) return false;

		iMaterial *pMaterial = GetMaterial();
		eMaterialBlendMode blend = pMaterial->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr);
		if(blend != eMaterialBlendMode_Alpha && blend != eMaterialBlendMode_Add) return false;

		if(mpSoftVtxProgram==nullptr)
		{
			cGpuProgramManager *pProgramManager = mpResources->GetGpuProgramManager();
			mpSoftVtxProgram = pProgramManager->CreateProgram("Particle_vp.cg","main",eGpuProgramType_Vertex);
			mpSoftFragProgram = pProgramManager->CreateProgram("Particle_fp.cg","main",eGpuProgramType_Fragment);
		}
		if(mpSoftVtxProgram==nullptr || mpSoftFragProgram==nullptr) return false;

		aDraw.mpVtxProgram = mpSoftVtxProgram;
		aDraw.mpFragProgram = mpSoftFragProgram;
		aDraw.mbDepthTest = pMaterial->GetDepthTest();
		return true;
	}

	//-----------------------------------------------------------------------

	void iParticleEmitter3D::SetupTransDraw(cCamera3D *apCamera, cRenderSettings *apSettings, cTransDraw &aDraw)
	{
		static const int kLights = 6;
		static tString vPosNames[kLights], vColorNames[kLights], vDirNames[kLights];
		if(vPosNames[0] == "")
		{
			for(int i=0; i<kLights; ++i)
			{
				vPosNames[i] = "partLightPos[" + cString::ToString(i) + "]";
				vColorNames[i] = "partLightColor[" + cString::ToString(i) + "]";
				vDirNames[i] = "partLightDir[" + cString::ToString(i) + "]";
			}
		}

		const cEffectSettings &effects = mpGraphics->GetLowLevel()->GetEffects();
		bool bAdded = GetMaterial()->GetBlendMode(eMaterialRenderType_Diffuse,0,nullptr) == eMaterialBlendMode_Add;

		mpSoftVtxProgram->SetMatrixf("partToWorld", *GetModelMatrix(apCamera));
		mpSoftVtxProgram->SetVec2f("fogRange", apSettings->mfFogStart, apSettings->mfFogEnd);

		// The fade takes about the size of a particle: wisps of smoke
		// fade over more than drops of water do
		float fSoft = cMath::Clamp(cMath::Max(mvMaxDrawSize.x, mvMaxDrawSize.y), 0.03f, 0.6f) * effects.mfParticleSoft;
		mpSoftFragProgram->SetVec4f("partLook", fSoft, apSettings->mbFogActive ? 1.0f : 0.0f, bAdded ? 1.0f : 0.0f, 0);

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

				mpSoftVtxProgram->SetVec4f(vPosNames[i], vPos.x, vPos.y, vPos.z, pLight->GetFarAttenuation());
				mpSoftVtxProgram->SetVec4f(vColorNames[i], col.r, col.g, col.b, fEdge);
				mpSoftVtxProgram->SetVec4f(vDirNames[i], vDir.x, vDir.y, vDir.z, 0);
			}
		}

		// What the map's own light adds to the brightness in the dark
		const cColor &ambient = apSettings->mAmbientColor;
		float fDark = cMath::Min(effects.mfParticleDark + (ambient.r + ambient.g + ambient.b) / 3.0f, 1.0f);
		mpSoftVtxProgram->SetVec4f("partLight", (float)lCount, bAdded ? 0.0f : effects.mfParticleLight, fDark, 0);
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
