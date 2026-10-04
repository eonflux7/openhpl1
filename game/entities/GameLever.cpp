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
#include "entities/GameLever.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "entities/GameStickArea.h"

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEntityLoader_GameLever::cEntityLoader_GameLever(const tString &asName, cInit *apInit)
: cEntityLoader_Object(asName)
{
	mpInit = apInit;
}

cEntityLoader_GameLever::~cEntityLoader_GameLever()
{
}

//-----------------------------------------------------------------------

void cEntityLoader_GameLever::BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
										  cWorld3D *apWorld)
{
}

//-----------------------------------------------------------------------

static eGameLeverMovement ToMovementType(const char *apString)
{
	if(apString==nullptr) return eGameLeverMovement_None;

	tString sType = cString::ToLowerCase(apString);

	if(sType == "none") return eGameLeverMovement_None;
	if(sType == "min") return eGameLeverMovement_Min;
	if(sType == "max") return eGameLeverMovement_Max;
	if(sType == "value") return eGameLeverMovement_Value;

	return eGameLeverMovement_None;
}

//-----------------------------------------------------------------------

void cEntityLoader_GameLever::AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
										 cWorld3D *apWorld)
{
	cGameLever *pObject = hplNew( cGameLever, (mpInit,mpEntity->GetName()) );

	pObject->msFileName = msFileName;
	pObject->m_mtxOnLoadTransform = a_mtxTransform;

	// Set the engine objects to the object
	pObject->SetBodies(mvBodies);
	pObject->SetJoints(mvJoints);
	pObject->SetMeshEntity(mpEntity);
	pObject->SetLights(mvLights);
	pObject->SetParticleSystems(mvParticleSystems);
	pObject->SetBillboards(mvBillboards);
	pObject->SetSoundEntities(mvSoundEntities);
	pObject->SetHapticShapes(mvHapticShapes);

	///////////////////////////////////
	// Load game properties
	TiXmlElement *pGameElem = apRootElem->FirstChildElement("GAME");
	if(pGameElem)
	{
		pObject->mfMaxInteractDist = cString::ToFloat(pGameElem->Attribute("InteractDist"),1.8f);

		pObject->mfMinLimit = cMath::ToRad(cString::ToFloat(pGameElem->Attribute("MinLimit"),0));
		pObject->mfMaxLimit = cMath::ToRad(cString::ToFloat(pGameElem->Attribute("MaxLimit"),0));

		pObject->msMinSound = cString::ToString(pGameElem->Attribute("MinSound"),"");
		pObject->msMaxSound = cString::ToString(pGameElem->Attribute("MaxSound"),"");

		pObject->mbStuckAtMin = cString::ToBool(pGameElem->Attribute("StuckAtMin"),false);
		pObject->mbStuckAtMax = cString::ToBool(pGameElem->Attribute("StuckAtMax"),false);

		pObject->mMovementType = ToMovementType(pGameElem->Attribute("MovementType"));
		pObject->mfMovementValue = cMath::ToRad(cString::ToFloat(pGameElem->Attribute("MovementValue"),0));
	}
	else
	{
		Error("Couldn't find game element for entity '%s'\n",mpEntity->GetName().c_str());
	}

	pObject->Init();

	/////////////////////////////////
	// Add to map handler
	mpInit->mpMapHandler->AddGameEntity(pObject);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameLever::cGameLever(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Lever;
	mbHasInteraction = true;

	mState = eGameLeverState_Middle;

	mfMaxInteractDist = 2.1f;

	mpJoint = nullptr;
	mpBody = nullptr;
	mfJointMaxLimit = 0;
	mfJointMinLimit = 0;

	mfMinLimit = 0;
	mfMaxLimit = 0;

	mbStuckAtMin = false;
	mbStuckAtMax = false;

	mMovementType = eGameLeverMovement_None;
	mfMovementValue = 0;

	mbPauseGravity = true;
	mbPauseControllers = true;

	mMovePid.SetErrorNum(10);
	mMovePid.p = 3.0f;
	mMovePid.i = 0.0f;
	mMovePid.d = 1.0f;

	mbStuck = false;
}

//-----------------------------------------------------------------------

cGameLever::~cGameLever(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameLever::Init()
{
	if(mvJoints.empty() || mvJoints[0]->GetType() != ePhysicsJointType_Hinge) return;

	mpJoint = static_cast<iPhysicsJointHinge*>(mvJoints[0]);
	mfJointMaxLimit = mpJoint->GetMaxAngle();
	mfJointMinLimit = mpJoint->GetMinAngle();

	// The body that moves is the first with a mass
	mpBody = nullptr;
	for(size_t i=0; i<mvBodies.size(); ++i)
	{
		if(mvBodies[i]->GetMass() != 0)
		{
			mpBody = mvBodies[i];
			break;
		}
	}
}

//-----------------------------------------------------------------------

void cGameLever::OnPlayerPick()
{
	float fDist = mpInit->mpPlayer->GetPickedDist();

	if(mfMaxInteractDist > fDist) return;

	if(fDist > mfMaxExamineDist || msDescription == _W(""))
	{
		if(mpInit->mpPlayer->GetState() == ePlayerState_InteractMode)
			mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Inactive);
		else
			mpInit->mpPlayer->SetCrossHairState(eCrossHairState_None);
	}
}

//-----------------------------------------------------------------------

void cGameLever::OnPlayerInteract()
{
	iPhysicsBody *pBody = mpInit->mpPlayer->GetPickedBody();

	cGameStickArea *pStickArea = mpInit->mpMapHandler->GetBodyStickArea(pBody);
	if(	(pBody->GetMass()==0 && pStickArea==nullptr) ||
		(pStickArea && pStickArea->GetCanDeatch()==false))
	{
		return;
	}

	if(mpInit->mpPlayer->GetPickedDist() >= mfMaxInteractDist) return;

	//Set some properties
	mpInit->mpPlayer->mfForwardUpMul = 1.0f;
	mpInit->mpPlayer->mfForwardRightMul = 1.0f;

	mpInit->mpPlayer->mfUpMul = 1.0f;
	mpInit->mpPlayer->mfRightMul = 1.0f;

	mpInit->mpPlayer->mbCanBeThrown = true;

	mpInit->mpPlayer->mfCurrentMaxInteractDist = mfMaxInteractDist;

	mpInit->mpPlayer->SetPushBody(mpInit->mpPlayer->GetPickedBody());
	mpInit->mpPlayer->ChangeState(ePlayerState_Move);
}

//-----------------------------------------------------------------------

void cGameLever::Update(float afTimeStep)
{
	if(mvJoints.empty()) return;

	eGameLeverState prevState = mState;
	float fAngle = mvJoints[0]->GetAngle();

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

	////////////////////////////////////
	// The state by the angle
	if(mbStuck==false)
	{
		if(fAngle >= mfMaxLimit)		mState = eGameLeverState_Max;
		else if(fAngle <= mfMinLimit)	mState = eGameLeverState_Min;
		else							mState = eGameLeverState_Middle;
	}

	////////////////////////////////////
	// A new state
	if(mState != prevState)
	{
		RunCallback(mState, eGameLeverAction_Enter);
		RunCallback(prevState, eGameLeverAction_Leave);

		if(mState == eGameLeverState_Min || mState == eGameLeverState_Max)
		{
			bool bMin = mState == eGameLeverState_Min;
			const tString &sSound = bMin ? msMinSound : msMaxSound;
			if(sSound != "")
			{
				cSoundEntity *pSound = pWorld->CreateSoundEntity(bMin ? "LeverMin" : "LeverMax", sSound, true);
				if(pSound) pSound->SetPosition(mpMeshEntity->GetBoundingVolume()->GetWorldCenter());
			}
		}

		if(mbStuck==false)
		{
			if(mState == eGameLeverState_Max && mbStuckAtMax) SetStuck(eGameLeverState_Max);
			if(mbStuck==false && mState == eGameLeverState_Min && mbStuckAtMin) SetStuck(eGameLeverState_Min);

			// At an end with the player's hand on it: stop it there
			if(	mbStuck==false && mState != eGameLeverState_Middle && mpBody &&
				mpInit->mpPlayer->GetState() == ePlayerState_Move &&
				mpInit->mpPlayer->GetPushBody() == mpBody)
			{
				mpBody->SetLinearVelocity(0);
				mpBody->SetAngularVelocity(0);
			}
		}
	}

	RunCallback(mState, eGameLeverAction_During);

	////////////////////////////////////
	// Let go, it turns back to where it rests
	if(mbStuck) return;
	if(mMovementType == eGameLeverMovement_None) return;
	if(mpJoint==nullptr || mpBody==nullptr) return;

	if(	mpInit->mpPlayer->GetState() == ePlayerState_Move &&
		mpInit->mpPlayer->GetPushBody() == mpBody)
	{
		return;
	}

	float fGoalAngle = 0;
	switch(mMovementType)
	{
	case eGameLeverMovement_Min:	fGoalAngle = (mfJointMinLimit + mfMinLimit) * 0.5f; break;
	case eGameLeverMovement_Max:	fGoalAngle = (mfJointMaxLimit + mfMaxLimit) * 0.5f; break;
	case eGameLeverMovement_Value:	fGoalAngle = mfMovementValue; break;
	default: break;
	}

	float fTorque = mMovePid.Output(fGoalAngle - fAngle, afTimeStep);

	mpBody->AddTorque(mpJoint->GetPinDir() * fTorque);
}

//-----------------------------------------------------------------------

void cGameLever::SetStuck(eGameLeverState aState)
{
	if(mpJoint==nullptr || mpBody==nullptr) return;

	// The hinge is closed to a degree on each side of the middle between
	// its own limit and the angle the end is reached at.
	if(aState == eGameLeverState_Max)
	{
		float fAngle = (mfJointMaxLimit + mfMaxLimit) * 0.5f;

		mpJoint->SetMaxAngle(fAngle + cMath::ToRad(1));
		mpJoint->SetMinAngle(fAngle - cMath::ToRad(1));
		mbStuck = true;
	}
	else if(aState == eGameLeverState_Min)
	{
		float fAngle = (mfJointMinLimit + mfMinLimit) * 0.5f;

		// As in the game: both a degree above, which holds it just as well
		mpJoint->SetMaxAngle(fAngle + cMath::ToRad(1));
		mpJoint->SetMinAngle(fAngle + cMath::ToRad(1));
		mbStuck = true;
	}

	mpBody->SetEnabled(true);
}

//-----------------------------------------------------------------------

void cGameLever::SetIsStuck(bool abX)
{
	if(mbStuck == abX) return;

	mbStuck = abX;

	if(mbStuck)
	{
		SetStuck(mState);
	}
	else if(mpJoint)
	{
		mpJoint->SetMaxAngle(mfJointMaxLimit);
		mpJoint->SetMinAngle(mfJointMinLimit);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

// Callback("name", action, state): action 0 enter, 1 leave, 2 during;
// state -1 min, 0 middle, 1 max
void cGameLever::RunCallback(eGameLeverState aState, eGameLeverAction aAction)
{
	if(mvCallbacks[aAction] == "") return;

	tString sAction = "";
	switch(aAction)
	{
	case eGameLeverAction_Enter:	sAction = "0"; break;
	case eGameLeverAction_Leave:	sAction = "1"; break;
	case eGameLeverAction_During:	sAction = "2"; break;
	default: break;
	}

	tString sState = "";
	switch(aState)
	{
	case eGameLeverState_Middle:	sState = "0"; break;
	case eGameLeverState_Min:		sState = "-1"; break;
	case eGameLeverState_Max:		sState = "1"; break;
	default: break;
	}

	tString sCommand = mvCallbacks[aAction] + "(\"" + msName + "\"," + sAction + "," + sState + ")";
	mpInit->RunScriptCommand(sCommand);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SAVE OBJECT STUFF
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

kBeginSerialize(cGameLever_SaveData,iGameEntity_SaveData)
kSerializeVar(mlState, eSerializeType_Int32)
kSerializeVar(mbStuck, eSerializeType_Bool)
kSerializeVar(msEnterCallback, eSerializeType_String)
kSerializeVar(msLeaveCallback, eSerializeType_String)
kSerializeVar(msDuringCallback, eSerializeType_String)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameLever_SaveData::CreateEntity()
{
	return nullptr;
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameLever::CreateSaveData()
{
	return hplNew( cGameLever_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameLever::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameLever_SaveData *pData = static_cast<cGameLever_SaveData*>(apSaveData);

	pData->mlState = mState;
	kCopyToVar(pData,mbStuck);

	pData->msEnterCallback = mvCallbacks[eGameLeverAction_Enter];
	pData->msLeaveCallback = mvCallbacks[eGameLeverAction_Leave];
	pData->msDuringCallback = mvCallbacks[eGameLeverAction_During];
}

//-----------------------------------------------------------------------

void cGameLever::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameLever_SaveData *pData = static_cast<cGameLever_SaveData*>(apSaveData);

	mState = (eGameLeverState)pData->mlState;
	kCopyFromVar(pData,mbStuck);
	if(mbStuck) SetStuck(mState);

	mvCallbacks[eGameLeverAction_Enter] = pData->msEnterCallback;
	mvCallbacks[eGameLeverAction_Leave] = pData->msLeaveCallback;
	mvCallbacks[eGameLeverAction_During] = pData->msDuringCallback;
}

//-----------------------------------------------------------------------
