/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "core/StdAfx.h"
#include "entities/GameLink.h"

#include "core/Init.h"
#include "systems/GameMessageHandler.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "systems/RadioHandler.h"

#include "core/GlobalInit.h"

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cAreaLoader_GameLink::cAreaLoader_GameLink(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameLink::~cAreaLoader_GameLink()
{

}

//-----------------------------------------------------------------------

iEntity3D* cAreaLoader_GameLink::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameLink *pLink = hplNew( cGameLink, (mpInit,asName) );

	pLink->m_mtxOnLoadTransform = a_mtxTransform;

	//Create physics data
	iPhysicsWorld *pPhysicsWorld = apWorld->GetPhysicsWorld();
	iCollideShape* pShape = pPhysicsWorld->CreateBoxShape(avSize,nullptr);
	std::vector<iPhysicsBody*> vBodies;
	vBodies.push_back(pPhysicsWorld->CreateBody(asName,pShape));

	vBodies[0]->SetCollide(false);
	vBodies[0]->SetCollideCharacter(false);
	vBodies[0]->SetMatrix(a_mtxTransform);

	vBodies[0]->SetUserData(pLink);
	pLink->SetBodies(vBodies);

	mpInit->mpMapHandler->AddGameEntity(pLink);

	return nullptr;
}


//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameLink::cGameLink(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Link;

	mbLocked = false;
}

//-----------------------------------------------------------------------

cGameLink::~cGameLink(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameLink::OnPlayerPick()
{
	if(mpInit->mpPlayer->GetPickedDist() <= 1.8)
	{
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_DoorLink);
	}
}

void cGameLink::OnPlayerInteract()
{
	//if(mpInit->mpRadioHandler->IsActive()) return;

	if(mbLocked)
	{
		if(mpInit->mpPlayer->GetPickedDist() <= 1.8)
		{
			if(msLockedSound != "")
				mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui(msLockedSound,false,1);

			if(msLockedDesc != _W(""))
				mpInit->mpGameMessageHandler->Add(msLockedDesc);
		}
		return;
	}

	if(mpInit->mpPlayer->GetPickedDist() <= 1.8 && msMapFile != "")
	{
		mpInit->mpMapHandler->ChangeMap(msMapFile, msMapPos, msStartSound, msStopSound, mfFadeOutTime,
										mfFadeInTime,msLoadTextCat,msLoadTextEntry,msLoadImage);
	}
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

kBeginSerialize(cGameLink_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)
kSerializeVar(msMapFile, eSerializeType_String)
kSerializeVar(msMapPos, eSerializeType_String)
kSerializeVar(msStartSound, eSerializeType_String)
kSerializeVar(msStopSound, eSerializeType_String)
kSerializeVar(mfFadeOutTime, eSerializeType_Float32)
kSerializeVar(mfFadeInTime, eSerializeType_Float32)
kSerializeVar(msLoadTextCat, eSerializeType_String)
kSerializeVar(msLoadTextEntry, eSerializeType_String)
kSerializeVar(msLoadImage, eSerializeType_String)
kSerializeVar(mbLocked, eSerializeType_Bool)
kSerializeVar(msLockedSound, eSerializeType_String)
kSerializeVar(msLockedDesc, eSerializeType_WString)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameLink_SaveData::CreateEntity()
{
	return nullptr;	
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameLink::CreateSaveData()
{
	return hplNew( cGameLink_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameLink::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameLink_SaveData *pData = static_cast<cGameLink_SaveData*>(apSaveData);

	kCopyToVar(pData,msMapFile);
	kCopyToVar(pData,msMapPos);
	kCopyToVar(pData,msStartSound);
	kCopyToVar(pData,msStopSound);
	kCopyToVar(pData,mfFadeOutTime);
	kCopyToVar(pData,mfFadeInTime);
	kCopyToVar(pData,msLoadTextCat);
	kCopyToVar(pData,msLoadTextEntry);
	kCopyToVar(pData,msLoadImage);
	kCopyToVar(pData,mbLocked);
	kCopyToVar(pData,msLockedSound);
	kCopyToVar(pData,msLockedDesc);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();
}

//-----------------------------------------------------------------------

void cGameLink::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameLink_SaveData *pData = static_cast<cGameLink_SaveData*>(apSaveData);

	kCopyFromVar(pData,msMapFile);
	kCopyFromVar(pData,msMapPos);
	kCopyFromVar(pData,msStartSound);
	kCopyFromVar(pData,msStopSound);
	kCopyFromVar(pData,mfFadeOutTime);
	kCopyFromVar(pData,mfFadeInTime);
	kCopyFromVar(pData,msLoadTextCat);
	kCopyFromVar(pData,msLoadTextEntry);
	kCopyFromVar(pData,msLoadImage);
	kCopyFromVar(pData,mbLocked);
	kCopyFromVar(pData,msLockedSound);
	kCopyFromVar(pData,msLockedDesc);

}
//-----------------------------------------------------------------------
