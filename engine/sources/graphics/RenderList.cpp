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

	iRenderState cTransparentDraw::GetState()
	{
		iRenderState state{};
		state.mpObject = mpObject;
		state.mbDepthTest = mbDepthTest;
		state.mAlphaMode = mAlphaMode;
		state.mBlendMode = mBlendMode;
		state.mChannelMode = eMaterialChannelMode_RGBA;
		state.mpProgram = mpProgram;
		state.mpProgramSetup = mpProgramSetup;
		for(int i = 0; i < MAX_TEXTUREUNITS; ++i) state.mpTexture[i] = mpTexture[i];
		state.mpVtxBuffer = mpVtxBuffer;
		state.mpModelMatrix = mbHasModelMatrix ? &mtxModelMatrix : nullptr;
		state.mpInvModelMatrix = mbHasInvModelMatrix ? &mtxInvModelMatrix : nullptr;
		return state;
	}

	bool cTransparentDraw_Compare(const cTransparentDraw &aDrawA, const cTransparentDraw &aDrawB)
	{
		if(aDrawA.mbDepthTest != aDrawB.mbDepthTest)
		{
			// Depth-disabled before depth-enabled, as the original intended.
			return aDrawA.mbDepthTest < aDrawB.mbDepthTest;
		}

		// Ascending camera-space Z is back-to-front for this camera. Equal
		// depths keep submission order, which Compile() preserves with a
		// stable sort of the retained submission list.
		return aDrawA.mfZ < aDrawB.mfZ;
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
		const size_t ends[] = {1, 2, 3, 4, 6, 8,
			8 + MAX_TEXTUREUNITS, 9 + MAX_TEXTUREUNITS, 10 + MAX_TEXTUREUNITS,
			11 + MAX_TEXTUREUNITS};
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
		mvTransparentDraws.clear();

		mDepthQueue.Clear();
		mDiffuseQueue.Clear();

		// Keep the light entries and their queue storage for the next frame.
		for(int i = 0; i < mlLightNum; ++i)
		{
			mvLights[i].mQueue.Clear();
			mvLights[i].mlObjectCount = 0;
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

	int cRenderList::GetObjectNum()
	{
		return (int)m_setObjects.size();
	}

	//-----------------------------------------------------------------------

	cRenderQueue* cRenderList::GetQueue(eMaterialRenderType aPassType, int alLightNum)
	{
		if(aPassType == eMaterialRenderType_Z) return &mDepthQueue;
		if(aPassType == eMaterialRenderType_Diffuse) return &mDiffuseQueue;
		if(aPassType == eMaterialRenderType_Light) return &mvLights[alLightNum].mQueue;
		return nullptr;
	}

	//-----------------------------------------------------------------------

	void cRenderList::Compile()
	{
		mlLightNum = (int)m_setLights.size();
		// Grow only: a many-light frame keeps storage for a later one.
		if((int)mvLights.size() < mlLightNum) mvLights.resize(mlLightNum);
		int lLightIndex = 0;
		for(tLight3DSetIt lightIt = m_setLights.begin(); lightIt != m_setLights.end();
			++lightIt, ++lLightIndex)
		{
			mvLights[lLightIndex].mpLight = *lightIt;
			mvLights[lLightIndex].mlObjectCount = 0;
			mvLights[lLightIndex].mQueue.Clear();
		}

		// Iterate the objects to be rendered and build draw queues.
		tRenderableSetIt it = m_setObjects.begin();
		for(;it != m_setObjects.end();++it)
		{
			iRenderable* pObject = *it;
			iMaterial* pMat = pObject->GetMaterial();

			//Skip meshes...
			if(pObject->GetRenderType() == eRenderableType_Mesh) continue;

			// See-through objects were recorded in submission order by Add().
			if(pMat->IsTransparent() == false)
			{
				//If the object uses z pass add to z queue.
				if(pMat->UsesType(eMaterialRenderType_Z))
				{
					for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Z, nullptr);lPass++)
					{
						AddDraw(pObject,eMaterialRenderType_Z,0,nullptr,lPass);
					}
				}

				//If object uses light add the object to each light's queue.
				if(pMat->UsesType(eMaterialRenderType_Light))
				{
					// Every eligible light keeps its own compiled queue.
					int lLightIndex = 0;
					tLight3DSetIt lightIt = m_setLights.begin();
					for(;lightIt != m_setLights.end();++lightIt, ++lLightIndex)
					{
						iLight3D* pLight = *lightIt;

						if(	(pLight->GetOnlyAffectInSector()==false || pObject->IsInSector(pLight->GetCurrentSector())) &&
							pLight->CheckObjectIntersection(pObject))
						{
							++mvLights[lLightIndex].mlObjectCount;

							for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Light, pLight);++lPass)
							{
								AddDraw(pObject,eMaterialRenderType_Light,lLightIndex,pLight,lPass);
							}
						}
					}
				}

				//If it has a diffuse pass, add to the diffuse queue.
				if(pObject->GetMaterial()->UsesType(eMaterialRenderType_Diffuse))
				{
					for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Diffuse,nullptr);lPass++)
					{
						AddDraw(pObject,eMaterialRenderType_Diffuse,0,nullptr,lPass);
					}
				}
			}
		}
		// A stable sort keeps first-submission order for equal keys.
		std::stable_sort(mvTransparentDraws.begin(), mvTransparentDraws.end(), cTransparentDraw_Compare);

		mDepthQueue.Sort();
		mDiffuseQueue.Sort();
		for(int i = 0; i < mlLightNum; ++i) mvLights[i].mQueue.Sort();

	}

	//-----------------------------------------------------------------------

	bool cRenderList::Add(iRenderable* apObject)
	{
		if(apObject->IsVisible()==false) return false;

		//Check if the object is culled by fog.
		cRenderer3D *pRenderer = mpGraphics->GetRenderer3D();
		if(pRenderer && pRenderer->GetFogActive() && pRenderer->GetFogCulling())
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
			if(apObject->GetMaterial()->IsTransparent())
			{
				//Calculate the Z for the trans object
				cVector3f vCameraPos = cMath::MatrixMul(cRenderList::GetCamera()->GetViewMatrix(),
					apObject->GetBoundingVolume()->GetWorldCenter());
				apObject->SetZ(vCameraPos.z);

				m_setObjects.insert(apObject);

				// Record in submission order, before any pointer-ordered set
				// is traversed; Compile() only stable-sorts this list.
				cTransparentDraw draw;
				draw.mpObject = apObject;
				draw.mfZ = apObject->GetZ();
				draw.mbDepthTest = apObject->GetMaterial()->GetDepthTest();
				draw.mAlphaMode = apObject->GetMaterial()->GetAlphaMode(eMaterialRenderType_Diffuse, 0, nullptr);
				draw.mBlendMode = apObject->GetMaterial()->GetBlendMode(eMaterialRenderType_Diffuse, 0, nullptr);
				draw.mpProgram = apObject->GetMaterial()->GetProgram(eMaterialRenderType_Diffuse, 0, nullptr);
				draw.mpProgramSetup = apObject->GetMaterial()->GetProgramSetup(eMaterialRenderType_Diffuse, 0, nullptr);
				for(int i=0; i<MAX_TEXTUREUNITS; ++i)
					draw.mpTexture[i] = apObject->GetMaterial()->GetTexture(i, eMaterialRenderType_Diffuse, 0, nullptr);
				draw.mpVtxBuffer = apObject->GetVertexBuffer();

				// Read the model matrix after GetInvModelMatrix() as the old
				// draw path did: some objects return a scratch matrix that the
				// inverse call replaces, and they drew the replaced value.
				cMatrixf *pModelMatrix = apObject->GetModelMatrix(mpCamera);
				cMatrixf *pInvModelMatrix = apObject->GetInvModelMatrix();
				if(pModelMatrix)
				{
					draw.mbHasModelMatrix = true;
					draw.mtxModelMatrix = *pModelMatrix;
				}
				if(pInvModelMatrix)
				{
					draw.mbHasInvModelMatrix = true;
					draw.mtxInvModelMatrix = *pInvModelMatrix;
				}

				draw.mpRefraction = apObject->GetMaterial()->GetTexture(eMaterialTexture_Refraction);
				draw.mpRefractionProgram = apObject->GetMaterial()->GetRefractionProgram();
				draw.mRefractionDiffuseTexture = apObject->GetMaterial()->GetRefractionDiffuseTexture();
				draw.mpSpecular = apObject->GetMaterial()->GetTexture(eMaterialTexture_Specular);
				draw.mpDiffuse = apObject->GetMaterial()->GetTexture(eMaterialTexture_Diffuse);
				// Water names the specular texture as its refraction source.
				draw.mpRefractionDiffuse = apObject->GetMaterial()->GetTexture(
					apObject->GetMaterial()->GetRefractionDiffuseTexture());
				draw.mfValue = apObject->GetMaterial()->GetValue();
				draw.mbRefractionUsesEye = apObject->GetMaterial()->GetRefractionUsesEye();
				draw.mbRefractionUsesTime = apObject->GetMaterial()->GetRefractionUsesTime();
				draw.mbRefractionUsesDiffuse = apObject->GetMaterial()->GetRefractionUsesDiffuse();
				draw.mbRefractionSkipsStandardTrans = apObject->GetMaterial()->GetRefractionSkipsStandardTrans();

				mvTransparentDraws.push_back(draw);
			}
			else
			{
				m_setObjects.insert(apObject);

				//Drawn again for the fog
				if(pRenderer && pRenderer->GetRenderSettings()->mbFogActive)
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

	void cRenderList::AddDraw(iRenderable* apObject, eMaterialRenderType aPassType,
								int alLightNum,iLight3D* apLight, int alPass)
	{

		cRenderQueue* queue = GetQueue(aPassType, alLightNum);
		queue->mvDraws.emplace_back();
		cRenderDraw& draw = queue->mvDraws.back();
		iRenderState* pTempState = &draw.mState;
		iMaterial* material = apObject->GetMaterial();

		pTempState->mpSector = aPassType == eMaterialRenderType_Z ? apObject->GetCurrentSector() : nullptr;
		pTempState->mlPass = alPass;
		pTempState->mbDepthTest = material->GetDepthTest();
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
