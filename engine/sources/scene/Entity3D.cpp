// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/Entity3D.h"
#include "scene/Node3D.h"
#include "math/Math.h"
#include "graphics/RenderList.h"

#include "system/LowLevelSystem.h"

#include "scene/PortalContainer.h"
#include "scene/RenderInterpolation.h"

namespace hpl {

	iEntity3D::iEntity3D(tString asName) :	iEntity(asName)
	{
		m_mtxLocalTransform = cMatrixf::Identity;
		m_mtxWorldTransform = cMatrixf::Identity;

		mbTransformUpdated = true;

		mlCount = 0;

		mlGlobalRenderCount = cRenderList::GetGlobalRenderCount();

		msSourceFile = "";

		mbApplyTransformToBV = true;
		mbUpdateBoundingVolume = true;

		mpParent = nullptr;

		mlIteratorCount =-1;

		mpCurrentSector = nullptr;

		mbRenderInterpolated = true;
		mlRenderInterpolateCreateStep = cRenderInterpolation::GetStep();
		mlRenderInterpolateStep = 0;
		mbRenderInterpolateTransform = false;
		mbRenderInterpolateApplied = false;
	}

	iEntity3D::~iEntity3D()
	{
		if(IsRenderInterpolationRegistered()) cRenderInterpolation::RemoveEntity(this);

		if(mpParent) mpParent->RemoveChild(this);

		for(tEntity3DListIt it = mlstChildren.begin(); it != mlstChildren.end();++it)
		{
			iEntity3D *pChild = *it;
			pChild->mpParent = nullptr;
		}
	}

	cVector3f iEntity3D::GetLocalPosition()
	{
		return m_mtxLocalTransform.GetTranslation();
	}

	cMatrixf& iEntity3D::GetLocalMatrix()
	{
		return m_mtxLocalTransform;
	}

	cVector3f iEntity3D::GetWorldPosition()
	{
		UpdateWorldTransform();

		return m_mtxWorldTransform.GetTranslation();
	}

	cMatrixf& iEntity3D::GetWorldMatrix()
	{
		UpdateWorldTransform();

		return m_mtxWorldTransform;
	}

	void iEntity3D::SetPosition(const cVector3f& avPos)
	{
		KeepRenderInterpolationTransform();

		m_mtxLocalTransform.m[0][3] = avPos.x;
		m_mtxLocalTransform.m[1][3] = avPos.y;
		m_mtxLocalTransform.m[2][3] = avPos.z;

		SetTransformUpdated();
	}

	void iEntity3D::SetMatrix(const cMatrixf& a_mtxTransform)
	{
		KeepRenderInterpolationTransform();

		m_mtxLocalTransform = a_mtxTransform;

		SetTransformUpdated();
	}

	void iEntity3D::SetWorldPosition(const cVector3f& avWorldPos)
	{
		if(mpParent)
		{
			SetPosition(avWorldPos - mpParent->GetWorldPosition());
		}
		else
		{
			SetPosition(avWorldPos);
		}
	}

	void iEntity3D::SetWorldMatrix(const cMatrixf& a_mtxWorldTransform)
	{
		if(mpParent)
		{
			SetMatrix(cMath::MatrixMul(cMath::MatrixInverse(mpParent->GetWorldMatrix()),
				a_mtxWorldTransform));
		}
		else
		{
			SetMatrix(a_mtxWorldTransform);
		}
	}

	void iEntity3D::SetTransformUpdated(bool abUpdateCallbacks)
	{
		mbTransformUpdated = true;
		mlCount++;

		//Perhaps not update this yet? This is baaaad!
		if(mbApplyTransformToBV)
			mBoundingVolume.SetTransform(GetWorldMatrix());

		mbUpdateBoundingVolume = true;

		//Update children
		for(tEntity3DListIt EntIt = mlstChildren.begin(); EntIt != mlstChildren.end();++EntIt)
		{
			iEntity3D *pChild = *EntIt;
			pChild->SetTransformUpdated(true);
		}

		//Update callbacks
		if(mlstCallbacks.empty() || abUpdateCallbacks==false) return;

		tEntityCallbackListIt it = mlstCallbacks.begin();
		for(; it!= mlstCallbacks.end(); ++it)
		{
			iEntityCallback* pCallback = *it;
			pCallback->OnTransformUpdate(this);
		}

	}

	bool iEntity3D::GetTransformUpdated()
	{
		return mbTransformUpdated;
	}

	int iEntity3D::GetTransformUpdateCount()
	{
		return mlCount;
	}

	void iEntity3D::AddCallback(iEntityCallback *apCallback)
	{
		mlstCallbacks.push_back(apCallback);
	}

	void iEntity3D::RemoveCallback(iEntityCallback *apCallback)
	{
		STLFindAndDelete(mlstCallbacks, apCallback);
	}

	void iEntity3D::AddChild(iEntity3D *apEntity)
	{
		if(apEntity==nullptr)return;
		if(apEntity->mpParent != nullptr) return;

		mlstChildren.push_back(apEntity);
		apEntity->mpParent = this;

		apEntity->SetTransformUpdated(true);
	}

	void iEntity3D::RemoveChild(iEntity3D *apEntity)
	{
		for(tEntity3DListIt it = mlstChildren.begin(); it != mlstChildren.end();)
		{
			iEntity3D *pChild = *it;
			if(pChild == apEntity)
			{
				pChild->mpParent = nullptr;
				it = mlstChildren.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	bool iEntity3D::IsChild(iEntity3D *apEntity)
	{
		for(tEntity3DListIt it = mlstChildren.begin(); it != mlstChildren.end();++it)
		{
			iEntity3D *pChild = *it;
			if(pChild == apEntity) return true;
		}
		return false;
	}

	iEntity3D * iEntity3D::GetEntityParent()
	{
		return mpParent;
	}

	bool iEntity3D::IsInSector(cSector *apSector)
	{

		if(apSector == GetCurrentSector())
		{
			return true;
		}

		tRenderContainerDataList *pDataList = GetRenderContainerDataList();
		tRenderContainerDataListIt it = pDataList->begin();
		for(; it != pDataList->end(); ++it)
		{
			iRenderContainerData *pRenderContainerData = *it;
			cSector *pSector = static_cast<cSector*>(pRenderContainerData);

			if(pSector == apSector)
			{
				return true;
			}
		}

		return false;
	}

	bool iEntity3D::IsRenderInterpolationRegistered()
	{
		return mlRenderInterpolateStep != 0 && mlRenderInterpolateStep == cRenderInterpolation::GetStep();
	}

	bool iEntity3D::RegisterRenderInterpolation()
	{
		if(cRenderInterpolation::IsRecording()==false) return false;
		// Placed where it was made: there is nothing to blend from.
		if(mlRenderInterpolateCreateStep == cRenderInterpolation::GetStep()) return false;

		if(IsRenderInterpolationRegistered()==false)
		{
			mlRenderInterpolateStep = cRenderInterpolation::GetStep();
			mbRenderInterpolateTransform = false;
			mbRenderInterpolateApplied = false;
			cRenderInterpolation::AddEntity(this);
		}
		return true;
	}

	// The first time in a step that the entity's own matrix is set, keep
	// the one it had before the step.
	void iEntity3D::KeepRenderInterpolationTransform()
	{
		if(mbRenderInterpolated==false) return;
		if(IsRenderInterpolationRegistered() && mbRenderInterpolateTransform) return;
		if(RegisterRenderInterpolation()==false) return;

		m_mtxRenderInterpolatePrev = m_mtxLocalTransform;
		mbRenderInterpolateTransform = true;
	}

	// Further than this in one step is a teleport, not a movement.
	static const float kMaxInterpolateEntityMove = 2.0f;

	void iEntity3D::ApplyRenderInterpolation(float afT)
	{
		if(mbRenderInterpolateTransform==false || mbRenderInterpolated==false || mbRenderInterpolateApplied) return;
		if(m_mtxRenderInterpolatePrev == m_mtxLocalTransform) return;

		cVector3f vMove = m_mtxLocalTransform.GetTranslation() - m_mtxRenderInterpolatePrev.GetTranslation();
		if(vMove.SqrLength() > kMaxInterpolateEntityMove*kMaxInterpolateEntityMove) return;

		m_mtxRenderInterpolateCurrent = m_mtxLocalTransform;
		m_mtxRenderInterpolateBlend = cRenderInterpolation::BlendMatrix(afT, m_mtxRenderInterpolatePrev, m_mtxLocalTransform);
		m_mtxLocalTransform = m_mtxRenderInterpolateBlend;
		mbRenderInterpolateApplied = true;
		SetTransformUpdated();
	}

	void iEntity3D::RestoreRenderInterpolation()
	{
		if(mbRenderInterpolateApplied==false) return;
		mbRenderInterpolateApplied = false;

		// Something placed it again while the frame was drawn: keep that.
		if(m_mtxLocalTransform != m_mtxRenderInterpolateBlend) return;

		m_mtxLocalTransform = m_mtxRenderInterpolateCurrent;
		SetTransformUpdated();
	}

	void iEntity3D::UpdateWorldTransform()
	{
		if(mbTransformUpdated)
		{
			mbTransformUpdated = false;

			//first check if there is a node parent
			if(mpParentNode)
			{
				cNode3D* pNode3D = static_cast<cNode3D*>(mpParentNode);

				m_mtxWorldTransform = cMath::MatrixMul(pNode3D->GetWorldMatrix(), m_mtxLocalTransform);
			}
			//If there is no node parent check for entity parent
			else if(mpParent)
			{
				m_mtxWorldTransform = cMath::MatrixMul(mpParent->GetWorldMatrix(), m_mtxLocalTransform);
			}
			else
			{
				m_mtxWorldTransform = m_mtxLocalTransform;
			}
		}
	}

	kBeginSerializeVirtual(cSaveData_iEntity3D,cSaveData_iEntity)
	kSerializeVar(m_mtxLocalTransform,eSerializeType_Matrixf)
	kSerializeVar(mBoundingVolume,eSerializeType_Class)
	kSerializeVar(msSourceFile,eSerializeType_String)

	kSerializeVar(mlParentId,eSerializeType_Int32)
	kSerializeVarContainer(mlstChildren,eSerializeType_Int32)
	kEndSerialize()

	iSaveData* iEntity3D::CreateSaveData()
	{
		return nullptr;
	}

	void iEntity3D::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iEntity3D);

		kSaveData_SaveTo(m_mtxLocalTransform);
		kSaveData_SaveTo(mBoundingVolume);
		kSaveData_SaveTo(msSourceFile);

		kSaveData_SaveObject(mpParent,mlParentId);
		kSaveData_SaveIdList(mlstChildren,tEntity3DListIt,mlstChildren);

	}

	void iEntity3D::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iEntity3D);

		SetMatrix(pData->m_mtxLocalTransform);

		//Not sure of this is needed:
		kSaveData_LoadFrom(mBoundingVolume);

		kSaveData_LoadFrom(msSourceFile);
	}

	void iEntity3D::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iEntity3D);

		if(pData->mlParentId!=-1 && mpParent == nullptr)
		{
			iEntity3D *pParentEntity = static_cast<iEntity3D*>(apSaveObjectHandler->Get(pData->mlParentId));
			if(pParentEntity)
				pParentEntity->AddChild(this);
			else
				Error("Couldn't find parent entity id %d for '%s'\n",pData->mlParentId,GetName().c_str());
		}

		for(int mlId : pData->mlstChildren.GetIterator())
		{
			if(mlId != -1)
			{
				iEntity3D *pChildEntity = static_cast<iEntity3D*>(apSaveObjectHandler->Get(mlId));
				if(pChildEntity)
					AddChild(pChildEntity);
				else
					Error("Couldn't find child entity id %d for '%s'\n",mlId,GetName().c_str());
			}
		}


		SetTransformUpdated(true);
	}
}
