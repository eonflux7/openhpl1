// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/Node.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	iNode::~iNode()
	{
		for(tEntityListIt it = mlstEntity.begin();it != mlstEntity.end();it++)
		{
			iEntity *pEntity = *it;
			pEntity->SetParent(nullptr);
		}
		mlstEntity.clear();
	}

	int iNode::SetVisible(bool abX, bool abCascade)
	{
		int lNum=0;
		for(tEntityListIt it = mlstEntity.begin();it != mlstEntity.end();it++)
		{
			(*it)->SetVisible(abX);
			lNum++;
		}

		if(abCascade)
		{
			for(tNodeListIt NIt = mlstNode.begin(); NIt != mlstNode.end(); NIt++)
			{
				(*NIt)->SetVisible(abX,abCascade);
			}
		}
		return lNum;
	}

	bool iNode::AddEntity(iEntity* apEntity)
	{
		if(apEntity->HasParent())return false;

		mlstEntity.push_back(apEntity);
		apEntity->SetParent(this);

		return true;
	}

	bool iNode::RemoveEntity(iEntity* apEntity)
	{
		for(tEntityListIt it = mlstEntity.begin();it != mlstEntity.end();it++)
		{
			if(*it == apEntity){
				apEntity->SetParent(nullptr);
				mlstEntity.erase(it);
				return true;
			}
		}

		return false;
	}

	void iNode::ClearEntities()
	{
		mlstEntity.clear();
	}

	cNodeIterator iNode::GetChildIterator()
	{
		return cNodeIterator(&mlstNode);
	}

	cEntityIterator iNode::GetEntityIterator()
	{
		return cEntityIterator(&mlstEntity);
	}

	kBeginSerializeVirtual(cSaveData_iNode, iSaveData)
	kSerializeVarContainer(mlstEntities, eSerializeType_Int32)
	kSerializeVarContainer(mlstNodes, eSerializeType_Int32)
	kEndSerialize()

	void iNode::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iNode);

		kSaveData_SaveIdList(mlstEntity,tEntityListIt,mlstEntities);
		kSaveData_SaveIdList(mlstNode,tNodeListIt,mlstNodes);
	}

	void iNode::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iNode);
	}

	void iNode::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iNode);

		kSaveData_LoadIdList(mlstEntity,mlstEntities, iEntity*);
		kSaveData_LoadIdList(mlstNode,mlstNodes, iNode*);
	}
}
