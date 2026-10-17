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
#include "graphics/RenderList.h"
#include <algorithm>
#include "graphics/Renderable.h"
#include "scene/MeshEntity.h"
#include "graphics/Material.h"
#include "scene/Camera3D.h"
#include "math/Math.h"
#include "graphics/RenderState.h"
#include "graphics/Renderer3D.h"
#include "graphics/RendererPostEffects.h"
#include "graphics/Graphics.h"


namespace hpl {


	int cRenderList::mlGlobalRenderCount = 0;


	//////////////////////////////////////////////////////////////////////////
	// FOG OBJECT COMPARE
	//////////////////////////////////////////////////////////////////////////

	bool cFogObject_Compare::operator()(	iRenderable* pObjectA,
												iRenderable* pObjectB) const
	{
		//TODO: perhaps Z here...

		return pObjectA->GetVertexBuffer() < pObjectB->GetVertexBuffer();
	}

	//////////////////////////////////////////////////////////////////////////
	// TRANSPERANT OBJECT COMPARE
	//////////////////////////////////////////////////////////////////////////

	bool cTransperantObject_Compare::operator()(iRenderable* pObjectA,iRenderable* pObjectB) const
	{
		if(pObjectA->GetMaterial()->GetDepthTest() != pObjectA->GetMaterial()->GetDepthTest())
		{
			return pObjectA->GetMaterial()->GetDepthTest() < pObjectA->GetMaterial()->GetDepthTest();
		}

		return pObjectA->GetZ() < pObjectB->GetZ();
	}


	//////////////////////////////////////////////////////////////////////////
	// DRAW QUEUES
	//////////////////////////////////////////////////////////////////////////

	void cRenderQueue::Sort()
	{
		std::stable_sort(mvDraws.begin(), mvDraws.end(), [](const cRenderDraw& a, const cRenderDraw& b){
			return a.mKey < b.mKey;
		});
	}

	eRenderStateType cRenderDraw::FirstChangedState(const cRenderDraw* previous) const
	{
		if(!previous) return eRenderStateType_Sector;
		// End of each state's part of the key, in application order.
		const size_t ends[] = {1, 2, 3, 4, 5, 7, 9,
			9 + MAX_TEXTUREUNITS, 10 + MAX_TEXTUREUNITS,
			11 + MAX_TEXTUREUNITS, 12 + MAX_TEXTUREUNITS};
		static_assert(sizeof(ends) / sizeof(ends[0]) == eRenderStateType_LastEnum, "Every state needs a key boundary");
		size_t start = 0;
		for(int mode = 0; mode < eRenderStateType_LastEnum; ++mode){
			for(size_t i = start; i < ends[mode]; ++i)
				if(mKey[i] != previous->mKey[i]) return static_cast<eRenderStateType>(mode);
			start = ends[mode];
		}
		return eRenderStateType_Render; // Even equal keys represent separate draws.
	}

	void cRenderQueue::Render(cRenderSettings* apSettings)
	{
		const cRenderDraw* previous = nullptr;
		for(cRenderDraw& draw : mvDraws){
			for(int mode = draw.FirstChangedState(previous); mode < eRenderStateType_LastEnum; ++mode){
				if(mode == eRenderStateType_Sector && !mbSector) continue;
				if(mode == eRenderStateType_Depth) continue;
				draw.mState.mType = static_cast<eRenderStateType>(mode);
				draw.mState.SetMode(apSettings);
			}
			previous = &draw;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cRenderList::cRenderList(cGraphics *apGraphics)
	{
		mfFrameTime = 0;
		mDepthQueue.mbSector = true;

		mlRenderCount=0;
		mlLastRenderCount =0;

		mpGraphics = apGraphics;

	}

	//-----------------------------------------------------------------------

	cRenderList::~cRenderList()
	{
		Clear();
	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////


	//-----------------------------------------------------------------------

	void cRenderList::Clear()
	{
		++mlGlobalRenderCount;

		//Clear containers.
		m_setLights.clear();
		m_setObjects.clear();
		m_setFogObjects.clear();
		m_setTransperantObjects.clear();

		mDepthQueue.Clear();
		mDiffuseQueue.Clear();
		mTransQueue.Clear();

		for(int i=0;i<MAX_NUM_OF_LIGHTS;i++)
		{
			mvLightQueues[i].Clear();
		}

		mlLastRenderCount = mlRenderCount;
		mlRenderCount++;
	}

	//-----------------------------------------------------------------------

	cFogObjectIterator cRenderList::GetFogIterator()
	{
		return cFogObjectIterator(&m_setFogObjects);
	}

	//-----------------------------------------------------------------------

	cTransperantObjectIterator cRenderList::GetTransperantIterator()
	{
		return cTransperantObjectIterator(&m_setTransperantObjects);
	}

	//-----------------------------------------------------------------------

	cLight3DIterator cRenderList::GetLightIt()
	{
		return cLight3DIterator(&m_setLights);
	}

	//-----------------------------------------------------------------------

	cRenderableIterator cRenderList::GetObjectIt()
	{
		return cRenderableIterator(&m_setObjects);
	}

	//-----------------------------------------------------------------------

	int cRenderList::GetLightNum()
	{
		return (int)m_setLights.size();
	}

	int cRenderList::GetObjectNum()
	{
		return (int)m_setObjects.size();
	}

	//-----------------------------------------------------------------------

	cRenderQueue* cRenderList::GetQueue(eRenderListDrawType aObjectType, eMaterialRenderType aPassType, int alLightNum)
	{
		if(aObjectType== eRenderListDrawType_Normal)
		{
			if(aPassType == eMaterialRenderType_Z) return &mDepthQueue;
			if(aPassType == eMaterialRenderType_Diffuse) return &mDiffuseQueue;
			if(aPassType == eMaterialRenderType_Light) return &mvLightQueues[alLightNum];
			return nullptr;
		}
		else
		{
			return &mTransQueue;
		}
	}

	//-----------------------------------------------------------------------

	void cRenderList::Compile()
	{
		int lLightNum = (int)m_setLights.size();
		if(lLightNum > MAX_NUM_OF_LIGHTS) lLightNum = MAX_NUM_OF_LIGHTS;

		for(int i=0; i<lLightNum; ++i) mvObjectsPerLight[i] =0;

		// Iterate the objects to be rendered and build draw queues.
		tRenderableSetIt it = m_setObjects.begin();
		for(;it != m_setObjects.end();++it)
		{
			iRenderable* pObject = *it;
			iMaterial* pMat = pObject->GetMaterial();

			//Skip meshes...
			if(pObject->GetRenderType() == eRenderableType_Mesh) continue;

			//If the object is transparent add tot eh trans queue
			if(pMat->IsTransperant())
			{
				//Use the set cotainer instead for now:
				m_setTransperantObjects.insert(pObject);
			}
			//If object is not trans add to another queue.
			else
			{
				//If the object uses z pass add to z queue.
				if(pMat->UsesType(eMaterialRenderType_Z))
				{
					for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Z, nullptr);lPass++)
					{
						AddDraw(pObject,eRenderListDrawType_Normal,eMaterialRenderType_Z,
									0,nullptr, false,lPass);
					}
				}

				//If object uses light add the object to each light's queue.
				if(pMat->UsesType(eMaterialRenderType_Light))
				{
					//Light queues that the object will belong to.
					int lLightCount =0;

					tLight3DSetIt lightIt = m_setLights.begin();
					for(;lightIt != m_setLights.end();++lightIt)
					{
						iLight3D* pLight = *lightIt;

						if(	(pLight->GetOnlyAffectInSector()==false || pObject->IsInSector(pLight->GetCurrentSector())) &&
							pLight->CheckObjectIntersection(pObject))
						{
							if(lLightCount >= MAX_NUM_OF_LIGHTS) break;

							++mvObjectsPerLight[lLightCount];

							for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Light, pLight);++lPass)
							{
								AddDraw(pObject,eRenderListDrawType_Normal,
											eMaterialRenderType_Light,lLightCount,pLight, false,lPass);
							}
						}
						++lLightCount;
					}
				}

				//If it has a diffuse pass, add to the diffuse queue.
				if(pObject->GetMaterial()->UsesType(eMaterialRenderType_Diffuse))
				{
					for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Diffuse,nullptr);lPass++)
					{
						AddDraw(pObject,eRenderListDrawType_Normal,
									eMaterialRenderType_Diffuse,0,nullptr, false,lPass);
					}
				}
			}
		}
		mDepthQueue.Sort();
		mDiffuseQueue.Sort();
		mTransQueue.Sort();
		for(auto& queue : mvLightQueues) queue.Sort();

	}

	//-----------------------------------------------------------------------

	bool cRenderList::Add(iRenderable* apObject)
	{
		if(apObject->IsVisible()==false) return false;

		//Check if the object is culled by fog.
		cRenderer3D *pRenderer = mpGraphics->GetRenderer3D();
		if(pRenderer->GetFogActive() && pRenderer->GetFogCulling())
		{
			if(cMath::CheckCollisionBV(*pRenderer->GetFogBV(), *apObject->GetBoundingVolume()))
			{

			}
			else {
				return false;
			}
		}

		//Check if the object as already been added.
		if(mlRenderCount == apObject->GetRenderCount()) return false;
		apObject->SetRenderCount(mlRenderCount);

		//Update the graphics of object.
		apObject->UpdateGraphics(cRenderList::GetCamera(),mfFrameTime,this);

		//Set that the object is to be rendered.
		apObject->SetGlobalRenderCount(mlGlobalRenderCount);

		switch(apObject->GetRenderType())
		{
			//Add a normal renderable
		case eRenderableType_Normal: case eRenderableType_ParticleSystem:
			if(apObject->GetMaterial()->IsTransperant())
			{
				//Calculate the Z for the trans object
				cVector3f vCameraPos = cMath::MatrixMul(cRenderList::GetCamera()->GetViewMatrix(),
					apObject->GetBoundingVolume()->GetWorldCenter());
				apObject->SetZ(vCameraPos.z);

				m_setObjects.insert(apObject);
			}
			else
			{
				m_setObjects.insert(apObject);

				//Drawn again for the fog
				if(mpGraphics->GetRenderer3D()->GetRenderSettings()->mbFogActive)
				{
					m_setFogObjects.insert(apObject);
				}
			}

			break;
			//Add all sub meshes of the mesh
		case eRenderableType_Mesh:
			{
				cMeshEntity* pMesh = static_cast<cMeshEntity*>(apObject);
				for(int i=0;i<pMesh->GetSubMeshEntityNum();i++)
				{
					Add(pMesh->GetSubMeshEntity(i));
				}
				break;
			}
			//Add a light to a special container
		case eRenderableType_Light:
			iLight3D *pLight = static_cast<iLight3D*>(apObject);

			m_setLights.insert(pLight);
			break;
		}

		return true;

	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cRenderList::AddDraw(iRenderable* apObject,eRenderListDrawType aObjectType,
								eMaterialRenderType aPassType, int alLightNum,iLight3D* apLight,
								bool abUseDepth, int alPass)
	{

		cRenderQueue* queue = GetQueue(aObjectType, aPassType, alLightNum);
		queue->mvDraws.emplace_back();
		cRenderDraw& draw = queue->mvDraws.back();
		iRenderState* pTempState = &draw.mState;
		iMaterial* material = apObject->GetMaterial();

		pTempState->mpSector = aPassType == eMaterialRenderType_Z ? apObject->GetCurrentSector() : nullptr;
		pTempState->mlPass = alPass;
		pTempState->mbDepthTest = material->GetDepthTest();
		pTempState->mfZ = abUseDepth ? apObject->GetZ() : 0;
		pTempState->mAlphaMode = material->GetAlphaMode(aPassType, alPass, apLight);
		pTempState->mBlendMode = material->GetBlendMode(aPassType, alPass, apLight);
		pTempState->mChannelMode = material->GetChannelMode(aPassType, alPass, apLight);
		pTempState->mpProgram = material->GetProgram(aPassType, alPass, apLight);
		pTempState->mpProgramSetup = material->GetProgramSetup(aPassType, alPass, apLight);
		pTempState->mbUsesLight = material->ProgramUsesLight(aPassType, alPass, apLight);
		pTempState->mbUsesEye = material->ProgramUsesEye(aPassType, alPass, apLight);
		pTempState->mpLight = apLight;
		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
			pTempState->mpTexture[i] = material->GetTexture(i, aPassType, alPass, apLight);
		pTempState->mpVtxBuffer = apObject->GetVertexBuffer();
		pTempState->mpModelMatrix = apObject->GetModelMatrix(mpCamera);
		pTempState->mpInvModelMatrix = apObject->GetInvModelMatrix();
		pTempState->mvScale = apObject->GetCalcScale();
		pTempState->mpObject = apObject;

		auto& key = draw.mKey;
		size_t k = 0;
		key[k++] = reinterpret_cast<uintptr_t>(pTempState->mpSector);
		key[k++] = alPass;
		key[k++] = pTempState->mbDepthTest;
		key[k++] = 0; // Transparent draws keep their separate back-to-front queue.
		key[k++] = ~uintptr_t(pTempState->mAlphaMode);
		key[k++] = ~uintptr_t(pTempState->mChannelMode);
		key[k++] = ~uintptr_t(pTempState->mBlendMode);
		key[k++] = ~reinterpret_cast<uintptr_t>(pTempState->mpProgram);
		key[k++] = ~reinterpret_cast<uintptr_t>(pTempState->mpProgramSetup);
		for(auto* texture : pTempState->mpTexture) key[k++] = ~reinterpret_cast<uintptr_t>(texture);
		key[k++] = ~reinterpret_cast<uintptr_t>(pTempState->mpVtxBuffer);
		key[k++] = ~reinterpret_cast<uintptr_t>(pTempState->mpModelMatrix);
		key[k++] = ~reinterpret_cast<uintptr_t>(apObject);

	}

	//----------------------------------------------------------------------------------------------


	//-----------------------------------------------------------------------

}
