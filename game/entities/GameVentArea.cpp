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
#include "entities/GameVentArea.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cAreaLoader_GameVentArea::cAreaLoader_GameVentArea(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameVentArea::~cAreaLoader_GameVentArea()
{

}

//-----------------------------------------------------------------------

iEntity3D* cAreaLoader_GameVentArea::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameVentArea *pArea = hplNew( cGameVentArea, (mpInit,asName) );

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

	return nullptr;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameVentArea::cGameVentArea(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_VentArea;

	mbHasInteraction = false;

	mbMovingPlayer = false;
	mvEntryVelocity = 0;
	mfEntryRemainingSeconds = 0;

	mbPlayerInside = false;
}

//-----------------------------------------------------------------------

cGameVentArea::~cGameVentArea(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameVentArea::OnPlayerPick()
{
}

//-----------------------------------------------------------------------

void cGameVentArea::Update(float afTimeStep)
{
	cPlayer *pPlayer = mpInit->mpPlayer;
	iCharacterBody *pCharBody = pPlayer->GetCharacterBody();

	////////////////////////////////////////////
	// The player is being carried in
	if(mbMovingPlayer)
	{
		pCharBody->SetFeetPosition(pCharBody->GetFeetPosition() + mvEntryVelocity * afTimeStep, false);

		mfEntryRemainingSeconds -= afTimeStep;
		if(mfEntryRemainingSeconds <= 0)
		{
			mbMovingPlayer = false;
			pCharBody->SetGravityActive(true);
			pCharBody->SetTestCollision(true);
		}
		return;
	}

	////////////////////////////////////////////
	// Is the player in the area, and not below its floor?
	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	iPhysicsBody *pAreaBody = mvBodies[0];
	iPhysicsBody *pPlayerBody = pCharBody->GetBody();

	cVector3f vFloorCentre = pAreaBody->GetLocalPosition();
	vFloorCentre.y -= pAreaBody->GetShape()->GetSize().y * 0.5f;

	if(	cMath::CheckCollisionBV(*pPlayerBody->GetBV(), *pAreaBody->GetBV())==false ||
		pPlayerBody->GetLocalPosition().y < vFloorCentre.y)
	{
		mbPlayerInside = false;
		return;
	}

	////////////////////////////////////////////
	// Attempt entry once per visit, while the player is jumping.
	if(mbPlayerInside || pPlayer->GetMoveState() != ePlayerMoveState_Jump) return;

	cCollideData collideData;
	collideData.SetMaxSize(1);
	if(pPhysicsWorld->CheckShapeCollision(pPlayerBody->GetShape(), pPlayerBody->GetLocalMatrix(),
										pAreaBody->GetShape(), pAreaBody->GetLocalMatrix(),
										collideData,1)==false)
	{
		return;
	}

	mbPlayerInside = true;

	pPlayer->ChangeMoveState(ePlayerMoveState_Crouch);

	////////////////////////////////////////////
	// Is there room for the crouching player on the middle of the floor?
	cVector3f vCrouchedCentre = vFloorCentre + cVector3f(0, pCharBody->GetSize().y * 0.5f, 0);
	cVector3f vResolvedCentre(0);
	pPhysicsWorld->CheckShapeWorldCollision(&vResolvedCentre, pCharBody->GetShape(), cMath::MatrixTranslate(vCrouchedCentre),
											pPlayerBody, false, true, nullptr, true);
	if(vResolvedCentre != vCrouchedCentre)
	{
		pPlayer->ChangeMoveState(ePlayerMoveState_Walk);
		return;
	}

	////////////////////////////////////////////
	// Carry the player there in a second
	mbMovingPlayer = true;
	mfEntryRemainingSeconds = 1.0f;
	mvEntryVelocity = (vFloorCentre - pCharBody->GetFeetPosition()) / mfEntryRemainingSeconds;

	pCharBody->SetGravityActive(false);
	pCharBody->SetTestCollision(false);
	pCharBody->SetForceVelocity(0);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SAVE OBJECT STUFF
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

kBeginSerialize(cGameVentArea_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)
kSerializeVar(mbPlayerInside, eSerializeType_Bool)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameVentArea_SaveData::CreateEntity()
{
	return nullptr;	
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameVentArea::CreateSaveData()
{
	return hplNew( cGameVentArea_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameVentArea::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameVentArea_SaveData *pData = static_cast<cGameVentArea_SaveData*>(apSaveData);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();

	kCopyToVar(pData,mbPlayerInside);
}

//-----------------------------------------------------------------------

void cGameVentArea::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameVentArea_SaveData *pData = static_cast<cGameVentArea_SaveData*>(apSaveData);

	kCopyFromVar(pData,mbPlayerInside);
}

//-----------------------------------------------------------------------
