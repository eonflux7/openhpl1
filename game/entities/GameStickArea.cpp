// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "entities/GameStickArea.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerState.h"

#include "core/GlobalInit.h"

#include <algorithm>
#include <cmath>

// LOADER

cAreaLoader_GameStickArea::cAreaLoader_GameStickArea(const tString &asName, cInit *apInit)
: iArea3DLoader(asName)
{
	mpInit = apInit;
}

cAreaLoader_GameStickArea::~cAreaLoader_GameStickArea()
{

}

iEntity3D* cAreaLoader_GameStickArea::Load(const tString &asName, const cVector3f &avSize, 
									  const cMatrixf &a_mtxTransform,cWorld3D *apWorld)
{
	cGameStickArea *pArea = hplNew( cGameStickArea, (mpInit,asName) ); 

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
	mpInit->mpMapHandler->AddStickArea(pArea);

	// The map handler owns the game entity; no scene entity is returned.
	return nullptr;
}

bool cGameStickArea::mbAllowAttachment = false;

cGameStickArea::cGameStickArea(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_StickArea;

	mpAttachedBody = nullptr;
	mpLastAttachedBody = nullptr;
	mfBodyMass = 0;
	mbBodyGravity = true;

	msAttachFunction = "";
	msDetachFunction = "";

	msAttachSound = "";
	msDetachFunction = "";

	msAttachSound = "";
	msDetachSound = "";

	mbMoveBody = true;
	mbRotateBody = true;

	mbCheckCenterInArea = true;

	mfPoseTime = 0.2f;

	mbCanDeatch = true;

	mfSetMtxTime = 1.0f;
}

cGameStickArea::~cGameStickArea(void)
{
	mpInit->mpMapHandler->RemoveStickArea(this);
}

void cGameStickArea::OnPlayerPick()
{
}

void cGameStickArea::Update(float afTimeStep)
{
	iPhysicsBody *pAreaBody = mvBodies[0];
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	
	if(mpAttachedBody && mfSetMtxTime < 1)
	{	
		// The pose time stops at 1. At 60 steps a second, 0.3 s took 19 steps to
		// pass 1 (1.056), and the slerp then went past the area's pose: a card
		// turned into level 8's server came to rest about 6% beyond it.
		if(mbMoveBody && mbRotateBody)
		{
			if(mfPoseTime==0)	mfSetMtxTime = 1.0f;
			else				mfSetMtxTime = std::min(mfSetMtxTime + afTimeStep / mfPoseTime, 1.0f);

			cMatrixf mtxGoal = pAreaBody->GetWorldMatrix();
			mtxGoal.SetTranslation(pAreaBody->GetWorldPosition() - mpAttachedBody->GetMassCentre());
		
			cMatrixf mtxNew = cMath::MatrixSlerp(mfSetMtxTime,m_mtxAttachedStart,mtxGoal,true);
        
			mpAttachedBody->SetMatrix(mtxNew);
		}
		else if(mbMoveBody && !mbRotateBody)
		{
			if(mfPoseTime==0)	mfSetMtxTime = 1.0f;
			else				mfSetMtxTime = std::min(mfSetMtxTime + afTimeStep / mfPoseTime, 1.0f);

			cVector3f vGoal = pAreaBody->GetWorldPosition() - mpAttachedBody->GetMassCentre();

			cVector3f vNew =	m_mtxAttachedStart.GetTranslation()* (1-mfSetMtxTime) +
								vGoal *mfSetMtxTime;

			mpAttachedBody->SetPosition(vNew);
		}
		else if(!mbMoveBody && mbRotateBody)
		{
			if(mfPoseTime==0)	mfSetMtxTime = 1.0f;
			else				mfSetMtxTime = std::min(mfSetMtxTime + afTimeStep / mfPoseTime, 1.0f);
			
			cMatrixf mtxGoal = pAreaBody->GetWorldMatrix();
			
			cMatrixf mtxNew = cMath::MatrixSlerp(mfSetMtxTime,m_mtxAttachedStart,mtxGoal,true);

			mtxNew.SetTranslation(mpAttachedBody->GetWorldPosition());
			mpAttachedBody->SetMatrix(mtxNew);
		}
		else
		{
			mfSetMtxTime = 1.0f;
		}


		if(mfSetMtxTime >= 1)
		{
			//Sound
			if(msAttachSound!="")
			{
				cSoundEntity *pSound = pWorld->CreateSoundEntity("AttachSound",msAttachSound,true);
				if(pSound) pSound->SetPosition(pAreaBody->GetWorldPosition());
			}

			//Particle System
			if(msAttachPS!="")
			{
				pWorld->CreateParticleSystem("AttachPS",msAttachPS,1,pAreaBody->GetWorldMatrix());
			}
		}
	}

	
	if(mpAttachedBody) return;

	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	iPhysicsBody *pAttachBody = nullptr;
	
	//Iterate all bodies in world and check for intersection
    cPhysicsBodyIterator bodyIt = pPhysicsWorld->GetBodyIterator();
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = bodyIt.Next();

		if(	pBody->IsActive() && pBody->GetCollide() && pBody->GetMass()>0 &&
			pBody->IsCharacter()==false)
		{
			//Bounding volume check

            bool bPassesBoundsCheck = false;
			if(mbCheckCenterInArea)
			{
				cVector3f vPos =	pBody->GetLocalPosition() + 
									cMath::MatrixMul(	pBody->GetLocalMatrix().GetRotation(),
														pBody->GetMassCentre());
				bPassesBoundsCheck = cMath::PointBVCollision(vPos,*pAreaBody->GetBV());
			}
			else
			{
				bPassesBoundsCheck = cMath::CheckCollisionBV(*pBody->GetBV(),*pAreaBody->GetBV());
			}

			if(bPassesBoundsCheck==false)
			{
				if(pBody == mpLastAttachedBody)
				{
					mpLastAttachedBody = nullptr;
				}
				continue;
			}

			if(pBody == mpLastAttachedBody) continue;

			//Shape collision check.
			cCollideData collideData;
			collideData.SetMaxSize(1);
			bool bCollide = pPhysicsWorld->CheckShapeCollision(pAreaBody->GetShape(), 
														pAreaBody->GetLocalMatrix(),
														pBody->GetShape(), 
														pBody->GetLocalMatrix(),
														collideData,1);
			if(bCollide)
			{
				if(msAttachFunction!="")
				{
					mbAllowAttachment = false;
					tString sCommand = BuildBodyCallbackCommand(msAttachFunction,pBody);
					mpInit->RunScriptCommand(sCommand);

					if(mbAllowAttachment==false) continue;
				}

				pAttachBody = pBody;
			}
		}
	}
	
	if(pAttachBody) AttachBody(pAttachBody);
}

void cGameStickArea::AttachBody(iPhysicsBody *apBody)
{
	if(mpAttachedBody) return;

	//If in an interact state, set the previous state		
	if(	mpInit->mpPlayer->GetPushBody() == apBody &&
	(	mpInit->mpPlayer->GetState() == ePlayerState_Grab ||
		mpInit->mpPlayer->GetState() == ePlayerState_Move ||
		mpInit->mpPlayer->GetState() == ePlayerState_Push)
	)
	{

		ePlayerState state =  mpInit->mpPlayer->GetState();
		ePlayerState prev = mpInit->mpPlayer->GetStateData(state)->mPreviousState;
		if(prev == ePlayerState_InteractMode)
			mpInit->mpPlayer->ChangeState(ePlayerState_InteractMode);
		else
			mpInit->mpPlayer->ChangeState(ePlayerState_Normal);
	}
	
	// Capture the starting pose before interpolation into the area.
	m_mtxAttachedStart = apBody->GetLocalMatrix();

	mfSetMtxTime = 0;

	apBody->SetLinearVelocity(0);
	apBody->SetAngularVelocity(0);
	
	mbBodyGravity = apBody->GetGravity();
	apBody->SetGravity(false);
	
	mfBodyMass = apBody->GetMass();
	apBody->SetMass(0);

	mpAttachedBody = apBody;
	mpLastAttachedBody = mpAttachedBody;
}

void cGameStickArea::DetachBody()
{
	if(mpAttachedBody)
	{


		cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
		iPhysicsBody *pAreaBody = mvBodies[0];

		//Callback function
		if(msDetachFunction!="")
		{
			tString sCommand = BuildBodyCallbackCommand(msDetachFunction,mpAttachedBody);
			mpInit->RunScriptCommand(sCommand);
		}

		//Sound
		if(msDetachSound!="")
		{
			cSoundEntity *pSound = pWorld->CreateSoundEntity("DetachSound",msDetachSound,true);
			if(pSound) pSound->SetPosition(pAreaBody->GetWorldPosition());
		}
		
		//Particle System
		if(msDetachPS!="")
		{
			pWorld->CreateParticleSystem("DetachPS",msDetachPS,1,pAreaBody->GetWorldMatrix());
		}

        mpAttachedBody->SetGravity(true);
		mpAttachedBody->SetMass(mfBodyMass);
		mpAttachedBody->SetEnabled(true);

		mpAttachedBody = nullptr;
	}
}

bool cGameStickArea::CanDetachBody()
{ 
	if(mfSetMtxTime < 1) return false;
    
	return mbCanDeatch;
}

tString cGameStickArea::BuildBodyCallbackCommand(const tString &asFunc,iPhysicsBody *apBody)
{
	return asFunc + "(\"" + msName + "\",\"" + apBody->GetName() + "\")"; 
}

void cGameStickArea::OnPostSceneDraw()
{
}

kBeginSerialize(cGameStickArea_SaveData,iGameEntity_SaveData)
kSerializeVar(mvSize,eSerializeType_Vector3f)

kSerializeVar(msAttachFunction,eSerializeType_String)
kSerializeVar(msDetachFunction,eSerializeType_String)

kSerializeVar(msAttachSound,eSerializeType_String)
kSerializeVar(msDetachSound,eSerializeType_String)

kSerializeVar(msAttachPS,eSerializeType_String)
kSerializeVar(msDetachPS,eSerializeType_String)

kSerializeVar(mbCanDeatch,eSerializeType_Bool)

kSerializeVar(msAttachedBody,eSerializeType_String)

kSerializeVar(mfSetMtxTime,eSerializeType_Float32)

kSerializeVar(mbCheckCenterInArea,eSerializeType_Bool)

kSerializeVar(mfPoseTime,eSerializeType_Float32)

kSerializeVar(mfBodyMass,eSerializeType_Float32)
kEndSerialize()

iGameEntity* cGameStickArea_SaveData::CreateEntity()
{
	return nullptr;	
}

iGameEntity_SaveData* cGameStickArea::CreateSaveData()
{
	return hplNew( cGameStickArea_SaveData, () );
}

void cGameStickArea::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameStickArea_SaveData *pData = static_cast<cGameStickArea_SaveData*>(apSaveData);

	kCopyToVar(pData,msAttachFunction);
	kCopyToVar(pData,msDetachFunction);

	kCopyToVar(pData,msAttachSound);
	kCopyToVar(pData,msDetachSound);

	kCopyToVar(pData,msAttachPS);
	kCopyToVar(pData,msDetachPS);

	kCopyToVar(pData,mbCanDeatch);

	kCopyToVar(pData,mbCheckCenterInArea);

	kCopyToVar(pData,mfPoseTime);

	kCopyToVar(pData,mfSetMtxTime);

	kCopyToVar(pData,mfBodyMass);

	pData->mvSize = mvBodies[0]->GetShape()->GetSize();

	if(mpAttachedBody)
	{
		pData->msAttachedBody = mpAttachedBody->GetName();
	}
}

void cGameStickArea::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameStickArea_SaveData *pData = static_cast<cGameStickArea_SaveData*>(apSaveData);

	kCopyFromVar(pData,msAttachFunction);
	kCopyFromVar(pData,msDetachFunction);

	kCopyFromVar(pData,msAttachSound);
	kCopyFromVar(pData,msDetachSound);

	kCopyFromVar(pData,msAttachPS);
	kCopyFromVar(pData,msDetachPS);

	kCopyFromVar(pData,mbCanDeatch);

	kCopyFromVar(pData,mbCheckCenterInArea);

	kCopyFromVar(pData,mfPoseTime);

	kCopyFromVar(pData,mfSetMtxTime);

	kCopyFromVar(pData,mfBodyMass);
}

void cGameStickArea::SetupSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SetupSaveData(apSaveData);
	cGameStickArea_SaveData *pData = static_cast<cGameStickArea_SaveData*>(apSaveData);

	if(pData->msAttachedBody!="")
	{
		iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
		mpAttachedBody = pWorld->GetBody(pData->msAttachedBody);
		mpLastAttachedBody = mpAttachedBody;

		// The original game did not save the body's mass, so detaching it
		// after a load (or on returning to the map) gave it whatever was in
		// memory: Black Plague's level 20 painting crashed Newton with a NaN
		// mass, or stayed fixed to the wall with 0. Saves without it get 1 kg.
		if(mpAttachedBody && !(mfBodyMass >= 0 && std::isfinite(mfBodyMass)))
		{
			Warning("Stick area '%s': mass of attached body '%s' not saved, using 1\n",
					msName.c_str(), mpAttachedBody->GetName().c_str());
			mfBodyMass = 1;
		}
	}
}
