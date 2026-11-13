// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/Entity.h"

#include "scene/Node.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	iEntity::~iEntity()
	{
		if(mpParentNode)
		{
			mpParentNode->RemoveEntity(this);
		}
	}

	kBeginSerializeVirtual(cSaveData_iEntity, iSaveData)
	kSerializeVar(mlParentNodeId, eSerializeType_Int32)
	kSerializeVar(msName, eSerializeType_String)
	kSerializeVar(mbIsVisible, eSerializeType_Bool)
	kSerializeVar(mbIsActive, eSerializeType_Bool)
	kEndSerialize()

	iSaveData* iEntity::CreateSaveData()
	{
		return nullptr;
	}

	void iEntity::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iEntity);

		kSaveData_SaveTo(mbIsActive);
		kSaveData_SaveTo(mbIsVisible);
		kSaveData_SaveTo(msName);

		kSaveData_SaveObject(mpParentNode,mlParentNodeId);
	}

	void iEntity::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iEntity);

		kSaveData_LoadFrom(mbIsActive);
		kSaveData_LoadFrom(mbIsVisible);
		kSaveData_LoadFrom(msName);
	}

	void iEntity::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iEntity);

		//kSaveData_LoadObject(mpParentNode,mlParentNodeId,iNode*);
	}
}
