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
	// RENDER NODE
	//////////////////////////////////////////////////////////////////////////

	cMemoryPool<iRenderState>* g_poolRenderState = nullptr;
	cMemoryPool<cRenderNode>* g_poolRenderNode = nullptr;

	//-----------------------------------------------------------------------

	bool cRenderNodeCompare::operator()(cRenderNode* apNodeA,cRenderNode* apNodeB) const
	{
		int  val = apNodeA->mpState->Compare(apNodeB->mpState);
		bool ret =  val>0 ? true : false;
		return ret;
	}

	//-----------------------------------------------------------------------

	void cRenderNode::DeleteChildren()
	{
		tRenderNodeSetIt it = m_setNodes.begin();
		for(;it != m_setNodes.end(); ++it)
		{
			cRenderNode* pNode = *it;

			pNode->DeleteChildren();
			g_poolRenderNode->Release(pNode);
		}

		if(mpState) g_poolRenderState->Release(mpState);
		mpState = nullptr;
		m_setNodes.clear();
	}

	//-----------------------------------------------------------------------

	void cRenderNode::Render(cRenderSettings* apSettings)
	{
		tRenderNodeSetIt it = m_setNodes.begin();
		for(;it != m_setNodes.end(); ++it)
		{
			cRenderNode* pNode = *it;

			pNode->mpState->SetMode(apSettings);
			pNode->Render(apSettings);
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cRenderList::cRenderList(cGraphics *apGraphics)
	{
		mfFrameTime = 0;
		mTempNode.mpState = hplNew( iRenderState, () );

		mlRenderCount=0;
		mlLastRenderCount =0;

		mpGraphics = apGraphics;

		m_poolRenderState = hplNew( cMemoryPool<iRenderState>, (3000, nullptr));
		m_poolRenderNode = hplNew( cMemoryPool<cRenderNode>, (3000, nullptr));

		g_poolRenderState = m_poolRenderState;
		g_poolRenderNode = m_poolRenderNode;
	}

	//-----------------------------------------------------------------------

	cRenderList::~cRenderList()
	{
		Clear();
		hplDelete(mTempNode.mpState);

		hplDelete(m_poolRenderState);
		hplDelete(m_poolRenderNode);

		g_poolRenderState = nullptr;
		g_poolRenderNode = nullptr;
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

		mRootNodeDepth.DeleteChildren();
		mRootNodeDiffuse.DeleteChildren();
		mRootNodeTrans.DeleteChildren();

		for(int i=0;i<MAX_NUM_OF_LIGHTS;i++)
		{
			mRootNodeLight[i].DeleteChildren();
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

	cRenderNode* cRenderList::GetRootNode(eRenderListDrawType aObjectType, eMaterialRenderType aPassType, int alLightNum)
	{
		if(aObjectType== eRenderListDrawType_Normal)
		{
			if(aPassType == eMaterialRenderType_Z) return &mRootNodeDepth;
			if(aPassType == eMaterialRenderType_Diffuse) return &mRootNodeDiffuse;
			if(aPassType == eMaterialRenderType_Light) return &mRootNodeLight[alLightNum];
			return nullptr;
		}
		else
		{
			return &mRootNodeTrans;
		}
	}

	//-----------------------------------------------------------------------

	void cRenderList::Compile()
	{
		int lLightNum = (int)m_setLights.size();
		if(lLightNum > MAX_NUM_OF_LIGHTS) lLightNum = MAX_NUM_OF_LIGHTS;

		for(int i=0; i<lLightNum; ++i) mvObjectsPerLight[i] =0;

		// Iterate the objects to be rendered and build trees with render states.
		tRenderableSetIt it = m_setObjects.begin();
		for(;it != m_setObjects.end();++it)
		{
			iRenderable* pObject = *it;
			iMaterial* pMat = pObject->GetMaterial();

			//Skip meshes...
			if(pObject->GetRenderType() == eRenderableType_Mesh) continue;

			//If the object is transparent add tot eh trans tree
			if(pMat->IsTransperant())
			{
				//Use the set cotainer instead for now:
				m_setTransperantObjects.insert(pObject);
			}
			//If object is not trans add to another tree.
			else
			{
				//If the object uses z pass add to z tree.
				if(pMat->UsesType(eMaterialRenderType_Z))
				{
					for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Z, nullptr);lPass++)
					{
						AddToTree(pObject,eRenderListDrawType_Normal,eMaterialRenderType_Z,
									0,nullptr, false,lPass);
					}
				}

				//If object uses light add the object to each light's tree.
				if(pMat->UsesType(eMaterialRenderType_Light))
				{
					//Light trees that the object will belong to.
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
								AddToTree(pObject,eRenderListDrawType_Normal,
											eMaterialRenderType_Light,lLightCount,pLight, false,lPass);
							}
						}
						++lLightCount;
					}
				}

				//If it has a diffuse pass, add to the diffuse tree.
				if(pObject->GetMaterial()->UsesType(eMaterialRenderType_Diffuse))
				{
					for(int lPass=0; lPass< pMat->GetNumOfPasses(eMaterialRenderType_Diffuse,nullptr);lPass++)
					{
						AddToTree(pObject,eRenderListDrawType_Normal,
									eMaterialRenderType_Diffuse,0,nullptr, false,lPass);
					}
				}
			}
		}
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

	cRenderNode* cRenderList::InsertNode(cRenderNode* apListNode, cRenderNode* apTempNode)
	{
		//Create a node with the state.

		tRenderNodeSetIt it = apListNode->m_setNodes.find(apTempNode);

		if(it != apListNode->m_setNodes.end())
		{
			//node found, return it.
			return *it;
		}
		else
		{
			//no node found, create one and return it.
			cRenderNode *pNode = m_poolRenderNode->Create();
			pNode->mpState = m_poolRenderState->Create();

			//Copy the relevant values to the state.
			pNode->mpState->Set(apTempNode->mpState);

			apListNode->m_setNodes.insert(pNode);

			return pNode;
		}
	}

	//-----------------------------------------------------------------------


	//-----------------------------------------------------------------------

	void cRenderList::AddToTree(iRenderable* apObject,eRenderListDrawType aObjectType,
								eMaterialRenderType aPassType, int alLightNum,iLight3D* apLight,
								bool abUseDepth, int alPass)
	{

		cRenderNode *pNode = GetRootNode(aObjectType,aPassType, alLightNum);
		iMaterial *pMaterial = apObject->GetMaterial();
		iRenderState *pTempState = mTempNode.mpState;
		cRenderNode *pTempNode = &mTempNode;

		//-------------- EXPLAINATION ----------------------------------
		// Go through each render state type and set the appropriate
		// variables for each type. The most important states are set first.
		// The state is then inserted to a tree structure, where each state type is a level.
		//                       Example:
		//                 |----------root--------|
		//           |---pass              |-----pass----|
		//       depth-test            depth-test     depth-test
		//              ............etc......................
		// After each insertion a pointer to the inserted or existing (if variables already existed)
		// node is returned
		// This is then passed on the next state, the state inserted (or exist found) there and so on.
		// At each insertion, it is first checked if a state with variables exist, and if so, use that.
		// if not, a new node is created and inserted.
		// The temp node state is used as storage for the data to skip the overhead of deletion
		// and creation when testing if state exist. Instead new data is only created if the state
		// is new.
		// Each Render state has the same class, but uses different var depending on type, this to
		// skip the overhead of using inheritance.
		//-------------------------------------------------------------

		/////// SECTOR //////////////
		if(aPassType == eMaterialRenderType_Z)
		{

			pTempState->mType = eRenderStateType_Sector;
			pTempState->mpSector = apObject->GetCurrentSector();

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// PASS //////////////
		{


			pTempState->mType = eRenderStateType_Pass;
			pTempState->mlPass = alPass;

			pNode = InsertNode(pNode, pTempNode);
		}
		/////// DEPTH TEST //////////////
		{


			pTempState->mType = eRenderStateType_DepthTest;
			pTempState->mbDepthTest = pMaterial->GetDepthTest();

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// DEPTH //////////////
		if(abUseDepth)
		{


			pTempState->mType = eRenderStateType_Depth;
			pTempState->mfZ = apObject->GetZ();

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// ALPHA MODE //////////////
		{


			pTempState->mType = eRenderStateType_AlphaMode;
			pTempState->mAlphaMode = pMaterial->GetAlphaMode(aPassType,alPass, apLight);

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// BLEND MODE //////////////
		{


			pTempState->mType = eRenderStateType_BlendMode;

			pTempState->mBlendMode = pMaterial->GetBlendMode(aPassType,alPass,apLight);
			pTempState->mChannelMode = pMaterial->GetChannelMode(aPassType,alPass,apLight);

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// VERTEX PROGRAM //////////////
		{

			pTempState->mType = eRenderStateType_VertexProgram;

			pTempState->mpVtxProgram = pMaterial->GetVertexProgram(aPassType,alPass,apLight);
			pTempState->mpVtxProgramSetup = pMaterial->GetVertexProgramSetup(aPassType,alPass,apLight);
			pTempState->mbUsesLight = pMaterial->VertexProgramUsesLight(aPassType, alPass,apLight);
			pTempState->mbUsesEye = pMaterial->VertexProgramUsesEye(aPassType, alPass,apLight);
			pTempState->mpLight = apLight;

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// FRAGMENT PROGRAM //////////////
		{


			pTempState->mType = eRenderStateType_FragmentProgram;

			pTempState->mpFragProgram = pMaterial->GetFragmentProgram(aPassType,alPass,apLight);
			pTempState->mpFragProgramSetup = pMaterial->GetFragmentProgramSetup(aPassType,alPass,apLight);

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// TEXTURE //////////////
		{


			pTempState->mType = eRenderStateType_Texture;

			for(int i=0; i<MAX_TEXTUREUNITS;i++)
			{
				pTempState->mpTexture[i] = pMaterial->GetTexture(i,aPassType, alPass,apLight);
			}

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// VERTEX BUFFER //////////////
		{


			pTempState->mType = eRenderStateType_VertexBuffer;

			pTempState->mpVtxBuffer = apObject->GetVertexBuffer();

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// MATRIX //////////////
		{


			pTempState->mType = eRenderStateType_Matrix;

			pTempState->mpModelMatrix = apObject->GetModelMatrix(mpCamera);
			pTempState->mpInvModelMatrix = apObject->GetInvModelMatrix();
			pTempState->mvScale = apObject->GetCalcScale();

			pNode = InsertNode(pNode, pTempNode);
		}

		/////// RENDER //////////////
		{


			pTempState->mType = eRenderStateType_Render;

			pTempState->mpObject = apObject;

			InsertNode(pNode, pTempNode);
		}

	}

	//----------------------------------------------------------------------------------------------


	//-----------------------------------------------------------------------

}
