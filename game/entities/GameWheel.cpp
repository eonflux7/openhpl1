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
#include "entities/GameWheel.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "entities/GameStickArea.h"

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEntityLoader_GameWheel::cEntityLoader_GameWheel(const tString &asName, cInit *apInit)
: cEntityLoader_Object(asName)
{
	mpInit = apInit;
}

cEntityLoader_GameWheel::~cEntityLoader_GameWheel()
{
}

//-----------------------------------------------------------------------

void cEntityLoader_GameWheel::BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
										  cWorld3D *apWorld)
{
}

//-----------------------------------------------------------------------

void cEntityLoader_GameWheel::AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
										 cWorld3D *apWorld)
{
	cGameWheel *pObject = hplNew( cGameWheel, (mpInit,mpEntity->GetName()) );

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

		pObject->mlSpinDir = cString::ToInt(pGameElem->Attribute("SpinDir"),0);

		pObject->mbPauseControllers = cString::ToBool(pGameElem->Attribute("PauseControllers"),true);
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

cGameWheel::cGameWheel(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Wheel;
	mbHasInteraction = true;

	mState = eGameWheelState_Middle;

	mfMaxInteractDist = 2.1f;

	mpJoint = nullptr;
	mpBody = nullptr;
	mfJointMaxLimit = 0;
	mfJointMinLimit = 0;

	mfMinLimit = 0;
	mfMaxLimit = 0;

	mbStuckAtMin = false;
	mbStuckAtMax = false;

	mbPauseGravity = true;

	mbStuck = false;

	mlSpinDir = 0;
	mfSpinLimitJointAngle = 0;

	mfAngle = 0;
	mfPrevAngle = 0;
	mfJointAngle = 0;
	mfPrevJointAngle = 0;

	mfSpinMaxAngle = 0;
	mfSpinMinAngle = 0;

	mpControllerEntity = nullptr;
	mpController = nullptr;
	mfControllerScale = 0;
	mfControllerOffset = 0;

	mpConnectedJoint = nullptr;
	mfConnectedScale = 0;
	mfConnectedOffset = 0;
	mfConnectedRange = 0;
}

//-----------------------------------------------------------------------

cGameWheel::~cGameWheel(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameWheel::Init()
{
	if(mvJoints.empty() || mvJoints[0]->GetType() != ePhysicsJointType_Hinge) return;

	mpJoint = static_cast<iPhysicsJointHinge*>(mvJoints[0]);
	mfJointMaxLimit = mpJoint->GetMaxAngle();
	mfJointMinLimit = mpJoint->GetMinAngle();

	// The hinge turns freely; the wheel's own limits are set as it turns
	mpJoint->SetMinAngle(-k2Pif);
	mpJoint->SetMaxAngle(k2Pif);

	mfJointAngle = mpJoint->GetAngle();

	// The body that turns is the first with a mass
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

void cGameWheel::OnPlayerPick()
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

void cGameWheel::OnPlayerInteract()
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

void cGameWheel::Update(float afTimeStep)
{
	if(mvJoints.empty() || mpJoint==nullptr) return;

	UpdateController(afTimeStep);
	UpdateConnectedJoint(afTimeStep);

	////////////////////////////////////
	// Add what the hinge turned to the angle
	mfJointAngle = mpJoint->GetAngle();
	float fTurned = cMath::GetAngleDistanceRad(mfPrevJointAngle, mfJointAngle);

	mfPrevAngle = mfAngle;
	mfPrevJointAngle = mfJointAngle;
	mfAngle += fTurned;

	////////////////////////////////////
	// A wheel that turns one way only: remember how far it got
	if(mlSpinDir == 1)
	{
		if(mfAngle > mfSpinMaxAngle)
		{
			mfSpinMaxAngle = mfAngle;
			mfSpinLimitJointAngle = mpJoint->GetAngle();
		}
	}
	else if(mlSpinDir == -1)
	{
		if(mfAngle < mfSpinMinAngle)
		{
			mfSpinMinAngle = mfAngle;
			mfSpinLimitJointAngle = mpJoint->GetAngle();
		}
	}

	////////////////////////////////////
	// The hinge's limits for the next step
	if(mbStuck==false)
	{
		if(mlSpinDir == 1)
		{
			SetMaxJointAngle(mfMaxLimit);
			mpJoint->SetMinAngle(mfSpinLimitJointAngle - cMath::ToRad(0.5f));
		}
		else if(mlSpinDir == -1)
		{
			SetMinJointAngle(mfMinLimit);
			mpJoint->SetMaxAngle(mfSpinLimitJointAngle + cMath::ToRad(0.5f));
		}
		else
		{
			SetMaxJointAngle(mfMaxLimit);
			SetMinJointAngle(mfMinLimit);
		}
	}

	////////////////////////////////////
	// The state by the angle
	eGameWheelState prevState = mState;
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

	if(mbStuck==false)
	{
		float fMargin = cMath::ToRad(1);

		if(mfAngle >= mfMaxLimit - fMargin)			mState = eGameWheelState_Max;
		else if(mfAngle <= mfMinLimit + fMargin)	mState = eGameWheelState_Min;
		else										mState = eGameWheelState_Middle;
	}

	////////////////////////////////////
	// A new state
	if(mState != prevState)
	{
		RunCallback(mState, eGameWheelAction_Enter);
		RunCallback(prevState, eGameWheelAction_Leave);

		if(mState == eGameWheelState_Min || mState == eGameWheelState_Max)
		{
			bool bMin = mState == eGameWheelState_Min;
			const tString &sSound = bMin ? msMinSound : msMaxSound;
			if(sSound != "")
			{
				cSoundEntity *pSound = pWorld->CreateSoundEntity(bMin ? "WheelMin" : "WheelMax", sSound, true);
				if(pSound) pSound->SetPosition(mpMeshEntity->GetBoundingVolume()->GetWorldCenter());
			}
		}

		if(mbStuck==false)
		{
			if(mState == eGameWheelState_Max && mbStuckAtMax) SetStuck(eGameWheelState_Max);
			if(mbStuck==false && mState == eGameWheelState_Min && mbStuckAtMin) SetStuck(eGameWheelState_Min);

			// At an end with the player's hand on it: stop it there
			if(	mbStuck==false && mState != eGameWheelState_Middle && mpBody &&
				mpInit->mpPlayer->GetState() == ePlayerState_Move &&
				mpInit->mpPlayer->GetPushBody() == mpBody)
			{
				mpBody->SetLinearVelocity(0);
				mpBody->SetAngularVelocity(0);
			}
		}
	}

	RunCallback(mState, eGameWheelAction_During);
}

//-----------------------------------------------------------------------

float cGameWheel::GetSpeed()
{
	return (mfAngle - mfPrevAngle) / mpInit->mpGame->GetStepSize();
}

//-----------------------------------------------------------------------

void cGameWheel::SetStuck(eGameWheelState aState)
{
	if(mpJoint==nullptr || mpBody==nullptr) return;

	if(aState == eGameWheelState_Min || aState == eGameWheelState_Max)
	{
		mfAngle = aState == eGameWheelState_Max ? mfMaxLimit : mfMinLimit;
		mfPrevAngle = mfAngle;

		// The hinge is closed to a degree on each side of where it is
		float fJointAngle = mpJoint->GetAngle();

		float fMax = cMath::Min(cMath::ToRad(180), fJointAngle + cMath::ToRad(1));
		float fMin = cMath::Max(cMath::ToRad(-180), fJointAngle - cMath::ToRad(1));

		mpJoint->SetMaxAngle(fMax);
		mpJoint->SetMinAngle(fMin);

		mbStuck = true;
	}

	mpBody->SetEnabled(true);
}

//-----------------------------------------------------------------------

void cGameWheel::SetIsStuck(bool abX)
{
	if(mbStuck == abX) return;

	mbStuck = abX;

	if(mbStuck)
	{
		SetStuck(mState);
	}
	else if(mpJoint)
	{
		mfJointAngle = mpJoint->GetAngle();
		mfPrevJointAngle = mfJointAngle;

		mpJoint->SetMinAngle(-k2Pif);
		mpJoint->SetMaxAngle(k2Pif);
	}
}

//-----------------------------------------------------------------------

void cGameWheel::ConnectToJointLimit(iPhysicsJoint *apJoint, float afMin, float afMax, float afRange)
{
	mpConnectedJoint = apJoint;

	mfConnectedScale = (afMax - afMin) / (mfMaxLimit - mfMinLimit);
	// As in the game: the controller's scale, where the joint's own was
	// surely meant. Without a controller that is zero, and the offset afMin.
	mfConnectedOffset = afMin - mfControllerScale * mfMinLimit;

	mfConnectedRange = afRange;
}

//-----------------------------------------------------------------------

void cGameWheel::ConnectToController(iGameEntity *apEntity, const tString& asController, float afMin, float afMax)
{
	iPhysicsController *pController = nullptr;

	for(int i=0; i<apEntity->GetBodyNum() && pController==nullptr; ++i)
	{
		iPhysicsBody *pBody = apEntity->GetBody(i);

		for(int j=0; j<pBody->GetJointNum() && pController==nullptr; ++j)
		{
			pController = pBody->GetJoint(j)->GetController(asController);
		}
	}

	if(pController==nullptr)
	{
		Warning("Could not find controller '%s' in '%s'\n",asController.c_str(),apEntity->GetName().c_str());
		return;
	}

	mpController = pController;
	mpControllerEntity = apEntity;

	mfControllerScale = (afMax - afMin) / (mfMaxLimit - mfMinLimit);
	mfControllerOffset = afMin - mfControllerScale * mfMinLimit;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

// The hinge stops within a turn, the wheel's limit may be turns away: the
// hinge gets the limit only when it is less than half a turn off, and on
// the side of the hinge's zero where its angle does not wrap.
void cGameWheel::SetMaxJointAngle(float afAngle)
{
	float fLeft = afAngle - mfAngle;

	if(fLeft <= kPif)
	{
		float fLimit = mfJointAngle + fLeft;

		if(	(mfJointAngle >= 0 && fLimit <= kPif) ||
			(mfJointAngle < 0 && fLimit < 0))
		{
			mpJoint->SetMaxAngle(fLimit);
			return;
		}
	}

	mpJoint->SetMaxAngle(k2Pif);
}

void cGameWheel::SetMinJointAngle(float afAngle)
{
	float fLeft = mfAngle - afAngle;

	if(fLeft <= kPif)
	{
		float fLimit = mfJointAngle - fLeft;

		if(	(mfJointAngle >= 0 && fLimit >= 0) ||
			(mfJointAngle < 0 && fLimit > -kPif))
		{
			mpJoint->SetMinAngle(fLimit);
			return;
		}
	}

	mpJoint->SetMinAngle(-k2Pif);
}

//-----------------------------------------------------------------------

void cGameWheel::UpdateController(float afTimeStep)
{
	if(mpController==nullptr) return;

	mpController->SetDestValue(mfAngle * mfControllerScale + mfControllerOffset);
}

//-----------------------------------------------------------------------

void cGameWheel::UpdateConnectedJoint(float afTimeStep)
{
	if(mpConnectedJoint==nullptr) return;

	float fValue = mfAngle * mfConnectedScale + mfConnectedOffset;
	float fMin = fValue - mfConnectedRange * 0.5f;
	float fMax = fValue + mfConnectedRange * 0.5f;

	if(mpConnectedJoint->GetChildBody()) mpConnectedJoint->GetChildBody()->SetEnabled(true);
	if(mpConnectedJoint->GetParentBody()) mpConnectedJoint->GetParentBody()->SetEnabled(true);

	switch(mpConnectedJoint->GetType())
	{
	case ePhysicsJointType_Slider:
		{
			iPhysicsJointSlider *pSlider = static_cast<iPhysicsJointSlider*>(mpConnectedJoint);
			pSlider->SetMinDistance(fMin);
			pSlider->SetMaxDistance(fMax);
			break;
		}
	case ePhysicsJointType_Screw:
		{
			iPhysicsJointScrew *pScrew = static_cast<iPhysicsJointScrew*>(mpConnectedJoint);
			pScrew->SetMinDistance(fMin);
			pScrew->SetMaxDistance(fMax);
			break;
		}
	case ePhysicsJointType_Hinge:
		{
			iPhysicsJointHinge *pHinge = static_cast<iPhysicsJointHinge*>(mpConnectedJoint);
			pHinge->SetMinAngle(cMath::ToRad(fMin));
			pHinge->SetMaxAngle(cMath::ToRad(fMax));
			break;
		}
	default:
		break;
	}
}

//-----------------------------------------------------------------------

// Callback("name", action, state): action 0 enter, 1 leave, 2 during;
// state -1 min, 0 middle, 1 max
void cGameWheel::RunCallback(eGameWheelState aState, eGameWheelAction aAction)
{
	if(mvCallbacks[aAction] == "") return;

	tString sAction = "";
	switch(aAction)
	{
	case eGameWheelAction_Enter:	sAction = "0"; break;
	case eGameWheelAction_Leave:	sAction = "1"; break;
	case eGameWheelAction_During:	sAction = "2"; break;
	default: break;
	}

	tString sState = "";
	switch(aState)
	{
	case eGameWheelState_Middle:	sState = "0"; break;
	case eGameWheelState_Min:		sState = "-1"; break;
	case eGameWheelState_Max:		sState = "1"; break;
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

kBeginSerialize(cGameWheel_SaveData,iGameEntity_SaveData)
kSerializeVar(mlState, eSerializeType_Int32)
kSerializeVar(mbStuck, eSerializeType_Bool)
kSerializeVar(mfSpinLimitJointAngle, eSerializeType_Float32)
kSerializeVar(mfAngle, eSerializeType_Float32)
kSerializeVar(mfPrevAngle, eSerializeType_Float32)
kSerializeVar(mfPrevJointAngle, eSerializeType_Float32)
kSerializeVar(mfJointAngle, eSerializeType_Float32)
kSerializeVar(msControllerEntity, eSerializeType_String)
kSerializeVar(msController, eSerializeType_String)
kSerializeVar(mfControllerScale, eSerializeType_Float32)
kSerializeVar(mfControllerOffset, eSerializeType_Float32)
kSerializeVar(msConnectedJoint, eSerializeType_String)
kSerializeVar(mfConnectedScale, eSerializeType_Float32)
kSerializeVar(mfConnectedOffset, eSerializeType_Float32)
kSerializeVar(mfConnectedRange, eSerializeType_Float32)
kSerializeVar(msEnterCallback, eSerializeType_String)
kSerializeVar(msLeaveCallback, eSerializeType_String)
kSerializeVar(msDuringCallback, eSerializeType_String)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameWheel_SaveData::CreateEntity()
{
	return nullptr;
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameWheel::CreateSaveData()
{
	return hplNew( cGameWheel_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameWheel::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameWheel_SaveData *pData = static_cast<cGameWheel_SaveData*>(apSaveData);

	pData->mlState = mState;
	kCopyToVar(pData,mbStuck);

	kCopyToVar(pData,mfSpinLimitJointAngle);
	kCopyToVar(pData,mfAngle);
	kCopyToVar(pData,mfPrevAngle);
	kCopyToVar(pData,mfPrevJointAngle);
	kCopyToVar(pData,mfJointAngle);

	pData->msControllerEntity = mpControllerEntity ? mpControllerEntity->GetName() : "";
	pData->msController = mpController ? mpController->GetName() : "";
	kCopyToVar(pData,mfControllerScale);
	kCopyToVar(pData,mfControllerOffset);

	pData->msConnectedJoint = mpConnectedJoint ? mpConnectedJoint->GetName() : "";
	kCopyToVar(pData,mfConnectedScale);
	kCopyToVar(pData,mfConnectedOffset);
	kCopyToVar(pData,mfConnectedRange);

	pData->msEnterCallback = mvCallbacks[eGameWheelAction_Enter];
	pData->msLeaveCallback = mvCallbacks[eGameWheelAction_Leave];
	pData->msDuringCallback = mvCallbacks[eGameWheelAction_During];
}

//-----------------------------------------------------------------------

void cGameWheel::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameWheel_SaveData *pData = static_cast<cGameWheel_SaveData*>(apSaveData);

	mState = (eGameWheelState)pData->mlState;
	kCopyFromVar(pData,mbStuck);
	if(mbStuck) SetStuck(mState);

	kCopyFromVar(pData,mfSpinLimitJointAngle);
	kCopyFromVar(pData,mfAngle);
	kCopyFromVar(pData,mfPrevAngle);
	kCopyFromVar(pData,mfPrevJointAngle);
	kCopyFromVar(pData,mfJointAngle);

	if(pData->msControllerEntity != "")
	{
		iGameEntity *pEntity = mpInit->mpMapHandler->GetGameEntity(pData->msControllerEntity);
		// The scale and offset saved are put back below
		if(pEntity) ConnectToController(pEntity, pData->msController, 0, 1);
	}
	kCopyFromVar(pData,mfControllerScale);
	kCopyFromVar(pData,mfControllerOffset);

	if(pData->msConnectedJoint != "")
	{
		iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
		mpConnectedJoint = pPhysicsWorld->GetJoint(pData->msConnectedJoint);
	}
	kCopyFromVar(pData,mfConnectedScale);
	kCopyFromVar(pData,mfConnectedOffset);
	kCopyFromVar(pData,mfConnectedRange);

	mvCallbacks[eGameWheelAction_Enter] = pData->msEnterCallback;
	mvCallbacks[eGameWheelAction_Leave] = pData->msLeaveCallback;
	mvCallbacks[eGameWheelAction_During] = pData->msDuringCallback;
}

//-----------------------------------------------------------------------
