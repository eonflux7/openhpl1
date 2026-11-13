// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "entities/GameArea.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"

#include "core/GlobalInit.h"

// LOADER

cAreaLoader_GameArea::cAreaLoader_GameArea(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameArea::~cAreaLoader_GameArea()
{

}

iEntity3D* cAreaLoader_GameArea::Load(const tString &asName, const cVector3f &avSize, 
									const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameArea *pArea = new cGameArea(mpInit,asName);

	pArea->m_mtxOnLoadTransform = a_mtxTransform;

	//Create physics data
    iPhysicsWorld *pPhysicsWorld = apWorld->GetPhysicsWorld();
	iCollideShape* pShape = pPhysicsWorld->CreateBoxShape(avSize,nullptr);
	std::vector<iPhysicsBody*> vBodies;
	vBodies.push_back(pPhysicsWorld->CreateBody(asName,pShape));

	vBodies[0]->SetCollide(false);
	vBodies[0]->SetCollideCharacter(false);
	vBodies[0]->SetMatrix(a_mtxTransform);

	vBodies[0]->SetUserData(pArea);
	pArea->SetBodies(vBodies);

	mpInit->mpMapHandler->AddGameEntity(pArea);

	// The map handler owns the game entity; no scene entity is returned.
	return nullptr;
}

cGameArea::cGameArea(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Area;

	mbUseCustomIcon = false;
	mCustomIcon = eCrossHairState_Inactive;
}

cGameArea::~cGameArea(void)
{
}

void cGameArea::OnPlayerPick()
{
	if(mbUseCustomIcon && mpInit->mpPlayer->GetPickedDist() <= mfMaxInteractDist)
	{
		mpInit->mpPlayer->SetCrossHairState(mCustomIcon);
	}
}

void cGameArea::SetCustomIcon(eCrossHairState aIcon)
{
	mbUseCustomIcon = true;
	mCustomIcon = aIcon;
}

kBeginSerialize(cGameArea_SaveData,iGameEntity_SaveData)
kSerializeVar(mbUseCustomIcon,eSerializeType_Bool)
kSerializeVar(mCustomIcon,eSerializeType_Int32)
kSerializeVar(mvSize,eSerializeType_Vector3f)
kEndSerialize()

iGameEntity* cGameArea_SaveData::CreateEntity()
{
	return nullptr;	
}

iGameEntity_SaveData* cGameArea::CreateSaveData()
{
	return new cGameArea_SaveData();
}

void cGameArea::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameArea_SaveData *pData = static_cast<cGameArea_SaveData*>(apSaveData);

	kCopyToVar(pData,mbUseCustomIcon);
	kCopyToVar(pData,mCustomIcon);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();
}

void cGameArea::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameArea_SaveData *pData = static_cast<cGameArea_SaveData*>(apSaveData);

	kCopyFromVar(pData,mbUseCustomIcon);
	kCopyFromVar(pData,mCustomIcon);
}
