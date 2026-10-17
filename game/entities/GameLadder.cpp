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
#include "entities/GameLadder.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerState_Misc.h"

#include "core/GlobalInit.h"

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cAreaLoader_GameLadder::cAreaLoader_GameLadder(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameLadder::~cAreaLoader_GameLadder()
{

}

//-----------------------------------------------------------------------

iEntity3D* cAreaLoader_GameLadder::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameLadder *pLadder = hplNew( cGameLadder, (mpInit,asName) );

	pLadder->m_mtxOnLoadTransform = a_mtxTransform;

	//Create physics data
	iPhysicsWorld *pPhysicsWorld = apWorld->GetPhysicsWorld();
	iCollideShape* pShape = pPhysicsWorld->CreateBoxShape(avSize,nullptr);
	std::vector<iPhysicsBody*> vBodies;
	vBodies.push_back(pPhysicsWorld->CreateBody(asName,pShape));

	vBodies[0]->SetCollide(false);
	vBodies[0]->SetCollideCharacter(false);
	vBodies[0]->SetMatrix(a_mtxTransform);

	vBodies[0]->SetUserData(pLadder);
	pLadder->SetBodies(vBodies);

	mpInit->mpMapHandler->AddGameEntity(pLadder);

	pLadder->Setup();

	return nullptr;
}


//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameLadder::cGameLadder(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Ladder;

	msAttachSound = mpInit->mpGameConfig->GetString("Movement_Climb","DefaultAttachSound","");
	msClimbUpSound = mpInit->mpGameConfig->GetString("Movement_Climb","DefaultClimbUpSound","");
	msClimbDownSound = mpInit->mpGameConfig->GetString("Movement_Climb","DefaultClimbDownSound","");
}

//-----------------------------------------------------------------------

cGameLadder::~cGameLadder(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameLadder::OnPlayerPick()
{
	float fPickedHeightOffset = GetPickedHeightOffset();
	if(GetPlayerDistanceXZ() <= 1.4 && fabs(fPickedHeightOffset)<1.1f)
	{
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Ladder);
	}
}

void cGameLadder::OnPlayerInteract()
{
	float fPickedHeightOffset = GetPickedHeightOffset();
	if(GetPlayerDistanceXZ() <= 1.4 && fabs(fPickedHeightOffset)<1.1f)
	{
		//Black Plague: out of the water onto the ladder
		if(cInit::IsOverture()==false) mpInit->mpPlayer->SetIsUnderWater(false);

		mpInit->mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);

		///////////////////////////////
		// Search upward for a clear entry position in front of the ladder.
		iCharacterBody *pCharBody = mpInit->mpPlayer->GetCharacterBody();
		iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
		cMatrixf mtxEntryPosition = cMath::MatrixTranslate(GetStartPosition());
		cVector3f vResolvedPosition;
		bool bEntryClear = false;

		do{
			pWorld->CheckShapeWorldCollision(&vResolvedPosition,pCharBody->GetShape(),mtxEntryPosition,
												pCharBody->GetBody(),false,true,nullptr,true);
			if(vResolvedPosition == mtxEntryPosition.GetTranslation())
			{
					bEntryClear = true;
					break;
			}
			
			mtxEntryPosition.SetTranslation(mtxEntryPosition.GetTranslation() + cVector3f(0,0.1f,0));
		}
		while(mtxEntryPosition.GetTranslation().y <= mfMaxY);
		
		if(bEntryClear==false) {
			// TODO: Give feedback when no clear ladder entry position is found.
			return;
		}

		///////////////////////////////
		//Set state
		cPlayerState_Climb* pState = static_cast<cPlayerState_Climb*>(mpInit->mpPlayer->GetStateData(ePlayerState_Climb));
		pState->mpLadder = this;
		pState->mvStartPosition = mtxEntryPosition.GetTranslation();

		mpInit->mpPlayer->ChangeState(ePlayerState_Climb);
	}
}

//-----------------------------------------------------------------------

void cGameLadder::OnPostSceneDraw()
{
	//mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawLine(mvBodies[0]->GetWorldPosition(),
	//											mvBodies[0]->GetWorldPosition()+mvForward,
	//											cColor(0,1,0,1));
}

//-----------------------------------------------------------------------

void cGameLadder::Setup()
{
	cMatrixf mtxInv = cMath::MatrixInverse(mvBodies[0]->GetWorldMatrix());

	mvForward = mtxInv.GetForward();

	mfMaxY = mvBodies[0]->GetWorldPosition().y + mvBodies[0]->GetShape()->GetSize().y/2.0f;
	mfMinY = mvBodies[0]->GetWorldPosition().y - mvBodies[0]->GetShape()->GetSize().y/2.0f;
}

//-----------------------------------------------------------------------

float cGameLadder::GetPlayerDistanceXZ()
{
	iCharacterBody *pCharBody = mpInit->mpPlayer->GetCharacterBody();
	cVector3f vPlayerPosition = pCharBody->GetPosition();
	vPlayerPosition.y =0;
	cVector3f vLadderPosition = mvBodies[0]->GetWorldPosition();
	vLadderPosition.y =0;
	return cMath::Vector3Dist(vPlayerPosition,vLadderPosition);
}

//-----------------------------------------------------------------------

float cGameLadder::GetPickedHeightOffset()
{
	iCharacterBody *pCharBody = mpInit->mpPlayer->GetCharacterBody();
	cVector3f vPlayerPosition = pCharBody->GetPosition();
	cVector3f vPickedPosition = mpInit->mpPlayer->GetPickedPos();
	
	return vPickedPosition.y - vPlayerPosition.y;
}

//-----------------------------------------------------------------------

cVector3f cGameLadder::GetStartRotation()
{
	iCharacterBody *pCharBody = mpInit->mpPlayer->GetCharacterBody();

	return cMath::GetAngleFromPoints3D(cVector3f(0,0,0), GetForward()*-1);
}
cVector3f cGameLadder::GetStartPosition()
{
	iCharacterBody *pCharBody = mpInit->mpPlayer->GetCharacterBody();

	cVector3f vPos = pCharBody->GetPosition();
	cVector3f vLadderPos =	GetBody(0)->GetWorldPosition() + 
							GetForward()*pCharBody->GetSize().x *0.6f;
	vLadderPos.y = vPos.y+0.05f;

	if(vLadderPos.y > mfMaxY - pCharBody->GetSize().y*0.3f)
		vLadderPos.y = mfMaxY - pCharBody->GetSize().y*0.3f;
	
	if(vLadderPos.y - pCharBody->GetSize().y/2 < mfMinY)
		vLadderPos.y = mfMinY + pCharBody->GetSize().y/2+0.1f;
	
	return vLadderPos;
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

kBeginSerialize(cGameLadder_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)

kSerializeVar(msAttachSound, eSerializeType_String)
kSerializeVar(msClimbUpSound, eSerializeType_String)
kSerializeVar(msClimbDownSound, eSerializeType_String)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameLadder_SaveData::CreateEntity()
{
	return nullptr;	
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameLadder::CreateSaveData()
{
	return hplNew( cGameLadder_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameLadder::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameLadder_SaveData *pData = static_cast<cGameLadder_SaveData*>(apSaveData);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();

	kCopyToVar(pData,msAttachSound);
	kCopyToVar(pData,msClimbUpSound);
	kCopyToVar(pData,msClimbDownSound);
}

//-----------------------------------------------------------------------

void cGameLadder::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameLadder_SaveData *pData = static_cast<cGameLadder_SaveData*>(apSaveData);

	kCopyFromVar(pData,msAttachSound);
	kCopyFromVar(pData,msClimbUpSound);
	kCopyFromVar(pData,msClimbDownSound);

}
//-----------------------------------------------------------------------
