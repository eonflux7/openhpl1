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
#include "entities/GameCamera.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEntityLoader_GameCamera::cEntityLoader_GameCamera(const tString &asName, cInit *apInit)
: cEntityLoader_Object(asName)
{
	mpInit = apInit;
}

cEntityLoader_GameCamera::~cEntityLoader_GameCamera()
{
}

//-----------------------------------------------------------------------

void cEntityLoader_GameCamera::BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
										  cWorld3D *apWorld)
{
}

//-----------------------------------------------------------------------

void cEntityLoader_GameCamera::AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
										 cWorld3D *apWorld)
{
	cGameCamera *pObject = hplNew( cGameCamera, (mpInit,mpEntity->GetName()) );

	pObject->msFileName = msFileName;
	pObject->m_mtxOnLoadTransform = a_mtxTransform;

	// Set the engine objects to the object
	pObject->SetBodies(mvBodies);

	// The object as the bodies' user data, so that the player can pick it
	for(size_t i=0; i<mvBodies.size(); ++i)
	{
		if(mvBodies[i]) mvBodies[i]->SetUserData((void*)pObject);
	}
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
		pObject->mfSpottedPlayerTime = cString::ToFloat(pGameElem->Attribute("SpottedPlayerTime"),3.0f);
		pObject->mAlertColor = cString::ToColor(pGameElem->Attribute("AlertColor"),cColor(1,1));
		pObject->msAlertSound = cString::ToString(pGameElem->Attribute("AlertSound"),"");
		pObject->msTurnSound = cString::ToString(pGameElem->Attribute("TurnSound"),"");
		pObject->msStillSound = cString::ToString(pGameElem->Attribute("StillSound"),"");
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

cGameCamera::cGameCamera(cInit *apInit,const tString& asName) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Camera;
	mbHasInteraction = false;

	mfSpottedPlayerTime = 3.0f;
	mfLightRadius = 0;

	mfMinAngle = cMath::ToRad(-30);
	mfMaxAngle = cMath::ToRad(30);
	mfMinPause = 2.0f;
	mfMaxPause = 2.0f;
	mfSpeed = 0.5f;

	mfAngle = 0;
	mfDir = 1;
	mfPauseCount = 0;

	mfCheckCount = 0;
	mbAlerted = false;
	mlSpottedCount = 0;
	mbRayHit = false;
	mfAlertCount = 0;

	mpTurnSound = nullptr;
}

//-----------------------------------------------------------------------

cGameCamera::~cGameCamera(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameCamera::Init()
{
	if(mvBodies.empty() || mvLights.empty())
	{
		Error("Security camera '%s' needs a body and a light\n",msName.c_str());
		return;
	}

	m_mtxStart = mvBodies[0]->GetWorldMatrix();

	mfLightRadius = mvLights[0]->GetFarAttenuation();
	mNormalColor = mvLights[0]->GetDiffuseColor();
}

//-----------------------------------------------------------------------

void cGameCamera::Setup(float afMinAngle, float afMaxAngle, float afMinPause, float afMaxPause, float afSpeed,
						const tString &asSpottedCallback, const tString &asAlertOverCallback)
{
	mfMinAngle = afMinAngle;
	mfMaxAngle = afMaxAngle;
	mfMinPause = afMinPause;
	mfMaxPause = afMaxPause;
	mfSpeed = afSpeed;
	msSpottedCallback = asSpottedCallback;
	msAlertOverCallback = asAlertOverCallback;
}

//-----------------------------------------------------------------------

void cGameCamera::Update(float afTimeStep)
{
	if(mvBodies.empty() || mvLights.empty()) return;

	//////////////////////////////////////
	// In alert: still, until the time is up
	if(mfAlertCount > 0)
	{
		StopTurnSound();

		mfAlertCount -= afTimeStep;
		if(mfAlertCount <= 0)
		{
			SetAlerted(false);

			if(msAlertOverCallback != "")
			{
				tString sCommand = msAlertOverCallback + "(\"" + msName + "\")";
				mpInit->RunScriptCommand(sCommand);
			}
		}
		return;
	}

	//////////////////////////////////////
	// Turning, with a pause at each end
	if(mfPauseCount > 0)
	{
		StopTurnSound();
		mfPauseCount -= afTimeStep;
	}
	else
	{
		PlayTurnSound();

		mfAngle += afTimeStep * mfDir * mfSpeed;
		if(mfDir > 0)
		{
			if(mfAngle >= mfMaxAngle)
			{
				mfDir = -1;
				mfPauseCount = mfMaxPause;
			}
		}
		else if(mfAngle <= mfMinAngle)
		{
			mfDir = 1;
			mfPauseCount = mfMinPause;
		}

		cMatrixf mtxTransform = cMath::MatrixMul(m_mtxStart.GetRotation(),cMath::MatrixRotateY(mfAngle));
		mtxTransform.SetTranslation(m_mtxStart.GetTranslation());
		mvBodies[0]->SetMatrix(mtxTransform);
	}

	//////////////////////////////////////
	// Look for the player, four times a second
	if(mfCheckCount > 0)
	{
		mfCheckCount -= afTimeStep;
		return;
	}
	mfCheckCount = 0.25f;

	iCharacterBody *pPlayerBody = mpInit->mpPlayer->GetCharacterBody();
	iLight3D *pLight = mvLights[0];

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld==nullptr) return;
	iPhysicsWorld *pPhysicsWorld = pWorld->GetPhysicsWorld();

	if(	cMath::CheckCollisionBV(*pLight->GetBoundingVolume(), *pPlayerBody->GetBody()->GetBV()) &&
		HasLineOfSight(pLight,pPhysicsWorld))
	{
		//Seen at four checks before the alert
		mlSpottedCount++;
		if(mlSpottedCount >= 4)
		{
			mlSpottedCount = 4;
			PlayerIsSpotted();
		}
	}
	else
	{
		mlSpottedCount--;
		if(mlSpottedCount < 0) mlSpottedCount = 0;
	}
}

//-----------------------------------------------------------------------

void cGameCamera::OnSetActive(bool abX)
{
	if(abX==false) StopTurnSound();
}

//-----------------------------------------------------------------------

void cGameCamera::SetAlerted(bool abX)
{
	if(mbAlerted == abX) return;
	mbAlerted = abX;

	if(mvLights.empty()) return;

	if(mbAlerted)
	{
		mvLights[0]->SetDiffuseColor(cColor(0,0));
		mvLights[0]->FadeTo(mAlertColor,mfLightRadius,0.3f);

		mfAlertCount = mfSpottedPlayerTime;

		if(msAlertSound != "")
		{
			cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
			cSoundEntity *pSound = pWorld->CreateSoundEntity("CameraAlert",msAlertSound,true);
			if(pSound) pSound->SetPosition(mpMeshEntity->GetBoundingVolume()->GetWorldCenter());
		}
	}
	else
	{
		mvLights[0]->FadeTo(mNormalColor,mfLightRadius,1.0f);
		mfAlertCount = 0;
	}
}

//-----------------------------------------------------------------------

bool cGameCamera::OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams)
{
	if(pBody->IsCharacter() || pBody->GetCollide()==false || pBody->GetBlocksLight()==false)
	{
		return true;
	}

	//Things that can be seen through do not hide the player
	iGameEntity* pEntity = (iGameEntity*)pBody->GetUserData();
	if(pEntity && pEntity->GetMeshEntity())
	{
		cMeshEntity *pMeshEntity = pEntity->GetMeshEntity();
		if(pMeshEntity->IsShadowCaster()==false) return true;
		
		bool bFoundSolid = false;
		for(int i=0; i< pMeshEntity->GetSubMeshEntityNum(); ++i)
		{
			iMaterial *pMaterial = pMeshEntity->GetSubMeshEntity(i)->GetMaterial(); 
			if(pMaterial && 
				(pMaterial->IsTransperant()==false && pMaterial->HasAlpha()==false))
			{
				bFoundSolid = true;
				break;
			}
		}
		if(bFoundSolid==false) return true;
	}

	mbRayHit = true;
	return false;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

bool cGameCamera::HasLineOfSight(iLight3D *apLight, iPhysicsWorld *apPhysicsWorld)
{
	iCharacterBody *pCharBody = mpInit->mpPlayer->GetCharacterBody();
	cVector3f vPlayerPos = pCharBody->GetPosition();

	//Inside the cone of the light
	if(apLight->GetLightType() == eLight3DType_Spot)
	{
		cLight3DSpot *pSpotLight = static_cast<cLight3DSpot*>(apLight);
		
		if(pSpotLight->GetFrustum()->CollideBoundingVolume(pCharBody->GetBody()->GetBV()) == 
			eFrustumCollision_Outside)
		{
			return false;
		}
	}

	//Not at the far end of the light
	float fDist = cMath::Vector3Dist(apLight->GetWorldPosition(),vPlayerPos);
	if(fDist > apLight->GetFarAttenuation() * 0.8f) return false;

	if(apLight->GetCastShadows()==false) return true;

	//Rays from the middle, head, feet and both sides: seen when no more
	//than two of them are blocked.
	cVector3f vPosAdd[5];
	const int lPosAddNum = 5;

	vPosAdd[0] = cVector3f(0,0,0); 
	vPosAdd[1] = cVector3f(0,pCharBody->GetSize().y/2 -0.1f,0); 
	vPosAdd[2] = cVector3f(0,-(pCharBody->GetSize().y/2 + 0.1f),0); 

	cVector3f vRight = cMath::Vector3Cross(	cMath::Vector3Normalize(apLight->GetWorldPosition()-vPlayerPos),
											cVector3f(0,1,0));
	vPosAdd[3] = vRight * (pCharBody->GetSize().x *0.4f);
	vPosAdd[4] = vRight * (pCharBody->GetSize().x *-0.4f);
	
	int lCount =0;
	for(int i=0; i<lPosAddNum; ++i)
	{
		mbRayHit = false;
		apPhysicsWorld->CastRay(this,vPlayerPos + vPosAdd[i],apLight->GetWorldPosition(),
								false,false,false,false);
		if(mbRayHit) ++lCount;
	}

	return lCount <= 2;
}

//-----------------------------------------------------------------------

void cGameCamera::PlayerIsSpotted()
{
	SetAlerted(true);

	if(msSpottedCallback != "")
	{
		tString sCommand = msSpottedCallback + "(\"" + msName + "\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

//-----------------------------------------------------------------------

void cGameCamera::PlayTurnSound()
{
	if(mpTurnSound) return;
	if(msTurnSound == "") return;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

	mpTurnSound = pWorld->CreateSoundEntity("CameraTurn",msTurnSound,false);
	if(mpTurnSound)
	{
		mpTurnSound->FadeIn(4.0f);
		mpTurnSound->SetPosition(mpMeshEntity->GetBoundingVolume()->GetWorldCenter());
	}
}

//-----------------------------------------------------------------------

void cGameCamera::StopTurnSound()
{
	if(mpTurnSound==nullptr) return;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld->SoundEntityExists(mpTurnSound)) mpTurnSound->FadeOut(5.0f);

	mpTurnSound = nullptr;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SAVE OBJECT STUFF
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

kBeginSerialize(cGameCamera_SaveData,iGameEntity_SaveData)
kSerializeVar(mfMinAngle, eSerializeType_Float32)
kSerializeVar(mfMaxAngle, eSerializeType_Float32)
kSerializeVar(mfMinPause, eSerializeType_Float32)
kSerializeVar(mfMaxPause, eSerializeType_Float32)
kSerializeVar(mfPauseCount, eSerializeType_Float32)
kSerializeVar(mfAngle, eSerializeType_Float32)
kSerializeVar(mfDir, eSerializeType_Float32)
kSerializeVar(mfSpeed, eSerializeType_Float32)
kSerializeVar(mbAlerted, eSerializeType_Bool)
kSerializeVar(msSpottedCallback, eSerializeType_String)
kSerializeVar(msAlertOverCallback, eSerializeType_String)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameCamera_SaveData::CreateEntity()
{
	return nullptr;
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameCamera::CreateSaveData()
{
	return hplNew( cGameCamera_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameCamera::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	cGameCamera_SaveData *pData = static_cast<cGameCamera_SaveData*>(apSaveData);

	kCopyToVar(pData,mfMinAngle);
	kCopyToVar(pData,mfMaxAngle);
	kCopyToVar(pData,mfMinPause);
	kCopyToVar(pData,mfMaxPause);
	kCopyToVar(pData,mfPauseCount);
	kCopyToVar(pData,mfAngle);
	kCopyToVar(pData,mfDir);
	kCopyToVar(pData,mfSpeed);
	kCopyToVar(pData,mbAlerted);
	kCopyToVar(pData,msSpottedCallback);
	kCopyToVar(pData,msAlertOverCallback);
}

//-----------------------------------------------------------------------

void cGameCamera::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameCamera_SaveData *pData = static_cast<cGameCamera_SaveData*>(apSaveData);

	kCopyFromVar(pData,mfMinAngle);
	kCopyFromVar(pData,mfMaxAngle);
	//The original leaves the two pauses at what the map's script set
	kCopyFromVar(pData,mfMinPause);
	kCopyFromVar(pData,mfMaxPause);
	kCopyFromVar(pData,mfPauseCount);
	kCopyFromVar(pData,mfAngle);
	kCopyFromVar(pData,mfDir);
	kCopyFromVar(pData,mfSpeed);
	kCopyFromVar(pData,msSpottedCallback);
	kCopyFromVar(pData,msAlertOverCallback);

	SetAlerted(pData->mbAlerted);
}

//-----------------------------------------------------------------------
