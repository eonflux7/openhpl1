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
#include "enemies/GameEnemy.h"

#include <SDL3/SDL.h>

#include "enemies/GameEnemy_Dog.h"
#include "enemies/GameEnemy_Infected.h"
#include "enemies/GameEnemy_Spider.h"
#include "enemies/GameEnemy_Worm.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/Triggers.h"
#include "systems/AttackHandler.h"
#include "systems/EffectHandler.h"
#include "systems/GameMusicHandler.h"
#include "entities/GameObject.h"

#include "enemies/CharacterMove.h"

#include "core/GlobalInit.h"

tString gvStateName[STATE_NUM] = {
	"IDLE",
		"HUNT",
		"ATTACK",
		"FLEE",
		"KNOCKDOWN",
		"DEAD",
		"PATROL",
		"INVESTIGATE",
		"BREAKDOOR",
		"CALLBACKUP",
		"MOVETO",
		"EAT",
		"ATTENTION",
};

//////////////////////////////////////////////////////////////////////////
// GAME ENEMY STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

iGameEnemyState::iGameEnemyState(int alId, cInit *apInit, iGameEnemy *apEnemy)
{
	mlId = alId;
	mpInit = apInit;
	mpPlayer = mpInit->mpPlayer;
	mpEnemy = apEnemy;
	mpMover = mpEnemy->GetMover();
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// RAY INTERSECT
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cLineOfSightRayCallback::Reset()
{
	mbIntersected = false;

	if(gpInit->mpPlayer->GetState() == ePlayerState_Grab)
		mpGrabBody = gpInit->mpPlayer->GetPushBody();
	else
		mpGrabBody = nullptr;
}

//-----------------------------------------------------------------------

bool cLineOfSightRayCallback::Intersected()
{
	return mbIntersected;
}

//-----------------------------------------------------------------------

static bool BodyIsTransparent(iPhysicsBody *apBody)
{
	iGameEntity* pEntity = (iGameEntity*)apBody->GetUserData();
	if(pEntity && pEntity->GetMeshEntity())
	{
		cMeshEntity *pMeshEntity = pEntity->GetMeshEntity();
		
		bool bFoundSolid = false;
		for(int i=0; i< pMeshEntity->GetSubMeshEntityNum(); ++i)
		{
			iMaterial *pMaterial = pMeshEntity->GetSubMeshEntity(i)->GetMaterial();
			if(pMaterial &&
				(pMaterial->IsTransparent() == false && pMaterial->HasAlpha() == false))
			{
				bFoundSolid = true;
				break;
			}
		}
		if(bFoundSolid==false) return true;
	}
	
	return false;
}

bool cLineOfSightRayCallback::OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams)
{
	if(pBody->GetCollide()==false) return true;
	if(pBody->IsCharacter()) return true;
	if(mpGrabBody == pBody) return true;

	if(BodyIsTransparent(pBody)) return true;


	mbIntersected = true;
	return false;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// GROUND FINDER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

bool cEnemyFindGround::GetGround(	const cVector3f &avStartPos,const cVector3f &avDir,
									cVector3f *apDestPosition, cVector3f *apDestNormal,
									float afMaxDistance)
{
	mbIntersected = false;
	mfMinDist = afMaxDistance;
	mfMaxDistance = afMaxDistance;

    iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	pPhysicsWorld->CastRay(this,avStartPos,avStartPos + avDir * mfMaxDistance,true,true,true);

	if(mbIntersected)
	{
		if(apDestPosition)	*apDestPosition = mvPos;
		if(apDestNormal)	*apDestNormal = mvNormal;
		return true;
	}

	return false;
}

bool cEnemyFindGround::OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams)
{
	if(apParams->mfT < 0) return true;
	if(pBody->GetCollideCharacter() == false || pBody->IsCharacter()) return true;
    
	if(mbIntersected == false || mfMinDist > apParams->mfDist)
	{
		mbIntersected = true;
		mfMinDist = apParams->mfDist;
		mvPos = apParams->mvPoint;
		mvNormal = apParams->mvNormal;
	}

	return true;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// DOOR CHECKER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

bool cEnemyCheckForDoor::CheckDoor(const cVector3f &avStart, const cVector3f &avEnd)
{
	mbIntersected = false;

	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	pPhysicsWorld->CastRay(this,avStart,avEnd,false,false,false);

	return mbIntersected;
}

static bool BodyCanBeBroken(iPhysicsBody *pBody)
{
	if(pBody->GetUserData()== nullptr) return false;

	//Black Plague: anything of an entity that can be moved is in the way too
	if(cInit::IsOverture()==false && pBody->GetMass() != 0) return true;

	iGameEntity *pEntity = (iGameEntity*)pBody->GetUserData();
	if(pEntity->GetType() == eGameEntityType_SwingDoor)
	{
		return true;
	}
	if(	pEntity->GetType() == eGameEntityType_Object)
	{
		cGameObject *pObject = static_cast<cGameObject*>(pEntity);
		if(pObject->IsBreakable()) return true;
	}
	return false;
}

bool cEnemyCheckForDoor::BeforeIntersect(iPhysicsBody *pBody)
{
	if(BodyCanBeBroken(pBody)) return true;	

	return false;
}

bool cEnemyCheckForDoor::OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams)
{
	if(apParams->mfT<0 || apParams->mfT >1) return true;

	if(BodyCanBeBroken(pBody))
	{
		mbIntersected = true;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

iGameEnemy::iGameEnemy(cInit *apInit,const tString& asName,TiXmlElement *apGameElem) : iGameEntity(apInit,asName)
{
	mType = eGameEntityType_Enemy;

	mTriggerTypes = eGameTriggerType_Sound;

	mpMover = hplNew( cCharacterMove, (mpInit) );

	mlCurrentState = -1;

	mbHasBeenActivated = false;

	mbSetFeetAtGroundOnStart = true;
	mbAttachMeshToBody = true;
	mbRemoveAttackerOnDisable = true;

	//States
	mvStates.resize(100);
	for(size_t i=0; i < mvStates.size(); ++i) mvStates[i] = nullptr;
	
	//Player find init
	mvLastPlayerPos = cVector3f(0,0,0);
	mbCanSeePlayer = false;
	mfCanSeePlayerCount = 0;
	mlPlayerInLOSCount = 0;
	mlMaxPlayerInLOSCount = 3;

	mfPlayerCheckIntervalSeconds  = 0.55f;
	mfPlayerCheckElapsedSeconds = cMath::RandRectf(0, mfPlayerCheckIntervalSeconds);

	mfDamageSoundTimer =0;

	msOnDeathCallback = "";
	msOnAttackCallback = "";

	m_mtxStartPose = cMatrixf::Identity;
	m_mtxGoalPose = cMatrixf::Identity;
	mfGroundPoseProgress = 0;

	//TODO: Should not be here
	mpAStarAir = nullptr;
	mpAStarGround = nullptr;
	mpNodeContainerAir = nullptr;
	mpNodeContainerGround = nullptr;

	//Patroling
	mlCurrentPatrolNode =0;
	mfWaitTime =0;
	mfWaitTimeCount =0;

	mfDoorBreakCount =0;

    //Default body settings
	mfDisappearTime =0;
	mbDisappearActive = false;
	mbHasDisappeared = false;
	
	msCloseMusic ="";
	mlCloseMusicPrio =0;

	msAttackMusic ="";
	mlAttackMusicPrio =0;
	
	mbShowDebug = false;

	msGroundNodeType = "ground";
	
	mvBodySize = cVector3f(0.5f,1.4f,0.5f);
	mfBodyMass = 10;
		
	mfMaxForwardSpeed = 1.0f;
	mfMaxBackwardSpeed = 1.0f;
	mfAcceleration = 1;
	mfDeacceleration = 1;

	mfMaxTurnSpeed = 8.5f;
	mfAngleDistTurnMul =2.3f;

	mfMinBreakAngle = cMath::ToRad(16);
	mfBreakAngleMul = 1.5f;

	mfSpeedMoveAnimMul = 4.7f;
	mfTurnSpeedMoveAnimMul = 4.0f;
	
	mfMaxPushMass = 10.0f;
	mfPushForce = 19.0f;

	mfMaxSeeDist = 10.0f;

	mfMinAttackDist = 1.6f;

	mfStoppedToWalkSpeed = 0.05f;
	mfWalkToStoppedSpeed = 0.02f;
	mfWalkToRunSpeed = 1.2f;
	mfRunToWalkSpeed = 1.0f;
	mfMoveAnimSpeedMul = 1.0f;
	msBackwardAnim = "Backward";
	msStoppedAnim = "Idle";
	msWalkAnim = "Walk";
	msRunAnim = "Run";

	m_mtxModelOffset = cMatrixf::Identity;
	mvModelOffsetAngles =0;

	mfFOV = cMath::ToRad(90.0f);
	mfFOVXMul = 0.7f;

	//trigger init
	mfTriggerUpdateCount =0;
	mfTriggerUpdateRate = 1.0f/ 60.0f;

	mfSkipSoundTriggerCount =0;
		
	mpCurrentAnimation = nullptr;
	mbAnimationCompletionSent = false;

	mbAnimationIsSpeedDependant = false;
	mfAnimationSpeedMul = 1.0f;

	msHitPS = "";
	
	mbOverrideMoveState = false;
	mMoveState = eEnemyMoveState_LastEnum;

	mbLoading = false;

	mbIsAttracted = false;

	mbUsesTriggers = true;

	mfCalcPlayerHiddenPosCount = 0;

	mbAccurateClimbing = false;
	mpCurrentSound = nullptr;
	mbAlignToGroundSkipNormal = false;
	mbStopAtLastPatrolNode = false;
}

//-----------------------------------------------------------------------

iGameEnemy::~iGameEnemy(void)
{
	hplDelete( mpMover );

	for(size_t i=0; i < mvStates.size(); ++i)
	{
		if(mvStates[i]) hplDelete( mvStates[i] );
	}

	mvStates.clear();
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

void iGameEnemy::LoadBaseProperties(TiXmlElement *apGameElem)
{
	////////////////////////////////////////
	// Load settings from XML
	mbShowDebug = cString::ToBool(apGameElem->Attribute("ShowDebug"),false);

	mbDisappear = cString::ToBool(apGameElem->Attribute("Disappear"),false);
	mfDisappearMinTime = cString::ToFloat(apGameElem->Attribute("DisappearMinTime"),0);
	mfDisappearMaxTime = cString::ToFloat(apGameElem->Attribute("DisappearMaxTime"),0);
	mfDisappearMinDistance = cString::ToFloat(apGameElem->Attribute("DisappearMinDistance"),0);
	msDisappearPS = cString::ToString(apGameElem->Attribute("DisappearPS"),"");
	msDisappearSound = cString::ToString(apGameElem->Attribute("DisappearSound"),"");
	mbDisappearFreezesRagdoll = cString::ToBool(apGameElem->Attribute("DisappearFreezesRagdoll"),false);

	msCloseMusic = cString::ToString(apGameElem->Attribute("CloseMusic"),"");
	mlCloseMusicPrio = cString::ToInt(apGameElem->Attribute("CloseMusicPrio"),0);
	mfCloseMusicStartDist = cString::ToFloat(apGameElem->Attribute("CloseMusicStartDist"),0);
	mfCloseMusicStopDist = cString::ToFloat(apGameElem->Attribute("CloseMusicStopDist"),0);

	msAttackMusic = cString::ToString(apGameElem->Attribute("AttackMusic"),"");
	mlAttackMusicPrio = cString::ToInt(apGameElem->Attribute("AttackMusicPrio"),0);

	mfFOV = cMath::ToRad(90);

	mfMaxPushMass = cString::ToFloat(apGameElem->Attribute("MaxPushMass"),0);
	mfPushForce = cString::ToFloat(apGameElem->Attribute("PushForce"),0);

	mfMaxHealth = cString::ToFloat(apGameElem->Attribute("MaxHealth"),0);
	mfHealth = mfMaxHealth;

	mfMaxSeeDist = cString::ToFloat(apGameElem->Attribute("MaxSeeDist"),0);

	mfMaxForwardSpeed =  cString::ToFloat(apGameElem->Attribute("MaxForwardSpeed"),0);
	mfAcceleration =  cString::ToFloat(apGameElem->Attribute("Acceleration"),0);
	mfDeacceleration =  cString::ToFloat(apGameElem->Attribute("Deacceleration"),0);

	mfMaxTurnSpeed =  cString::ToFloat(apGameElem->Attribute("MaxTurnSpeed"),mfMaxTurnSpeed);
	mfAngleDistTurnMul =  cString::ToFloat(apGameElem->Attribute("AngleDistTurnMul"),mfAngleDistTurnMul);

	mfMinBreakAngle =  cMath::ToRad(cString::ToFloat(apGameElem->Attribute("MinBreakAngle"),mfMinBreakAngle));
	mfBreakAngleMul =  cString::ToFloat(apGameElem->Attribute("BreakAngleMul"),mfBreakAngleMul);

	mfStoppedToWalkSpeed =  cString::ToFloat(apGameElem->Attribute("StoppedToWalkSpeed"),0);
	mfWalkToStoppedSpeed =  cString::ToFloat(apGameElem->Attribute("WalkToStoppedSpeed"),0);
	mfWalkToRunSpeed =  cString::ToFloat(apGameElem->Attribute("WalkToRunSpeed"),0);
	mfRunToWalkSpeed =  cString::ToFloat(apGameElem->Attribute("RunToWalkSpeed"),0);
	mfMoveAnimSpeedMul =  cString::ToFloat(apGameElem->Attribute("MoveAnimSpeedMul"),0);

	mvBodySize =  cString::ToVector3f(apGameElem->Attribute("BodySize"),0);
	mfBodyMass =  cString::ToFloat(apGameElem->Attribute("BodyMass"),10);

	cVector3f vRot = cString::ToVector3f(apGameElem->Attribute("ModelOffset_Rot"),0);
	vRot = cVector3f(cMath::ToRad(vRot.x),cMath::ToRad(vRot.y),cMath::ToRad(vRot.z));
	cVector3f vPos = cString::ToVector3f(apGameElem->Attribute("ModelOffset_Pos"),0);

	mvModelOffsetAngles = vRot;
	m_mtxModelOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	m_mtxModelOffset.SetTranslation(vPos);

	mbAlignToGroundNormal = cString::ToBool(apGameElem->Attribute("AlignToGroundNormal"),false);
	mbAlignToGroundSkipNormal = cString::ToBool(apGameElem->Attribute("AlignToGroundSkipNormal"),false);

	msHitPS = cString::ToString(apGameElem->Attribute("HitPS"),"");
}

//-----------------------------------------------------------------------

void iGameEnemy::OnPlayerInteract()
{
	
}

//-----------------------------------------------------------------------

void iGameEnemy::OnPlayerPick()
{
	//SKIp this for now since this means that enemies in ragdoll can take damage.
	/*mpInit->mpPlayer->mbPickAtPoint = false;
	mpInit->mpPlayer->mbRotateWithPlayer = true;
	mpInit->mpPlayer->mbUseNormalMass = false;
	mpInit->mpPlayer->mfGrabMassMul = (float)mvBodies.size();

	mpInit->mpPlayer->SetPushBody(mpInit->mpPlayer->GetPickedBody());
	mpInit->mpPlayer->ChangeState(ePlayerState_Grab);*/
}

//-----------------------------------------------------------------------

void iGameEnemy::Setup(cWorld3D *apWorld)
{
	/////////////////////////////////////////////
	//Create body
	iCharacterBody *pBody = apWorld->GetPhysicsWorld()->CreateCharacterBody("Enemy",
															mvBodySize);

	pBody->SetEntityOffset(m_mtxModelOffset);
	pBody->SetMass(mfBodyMass);
	pBody->SetMaxStepSize(0.35f);
	pBody->SetStepClimbSpeed(3.35f);
	pBody->SetCustomGravity(cVector3f(0,-13.0f,0));
	pBody->SetEntitySmoothPosNum(10);
	pBody->SetGroundFriction(10);
	pBody->SetAccurateClimbing(mbAccurateClimbing);

	mpMover->SetCharBody(pBody);
	
	SetCharBody(pBody);

	SetupBody();
}

//-----------------------------------------------------------------------

void iGameEnemy::OnWorldLoad()
{	
	//////////////////////////////////
	// Setup EnemyMove
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	
	mpNodeContainerGround = pWorld->CreateAINodeContainer(msEnemyType,msGroundNodeType,
													mvBodySize,
													false, 2 , 6 , 5.0f,0.41f);
	

	if(mpNodeContainerGround)
		mpAStarGround = pWorld->CreateAStarHandler(mpNodeContainerGround);
	else
		mpAStarGround = nullptr;

	//////////////////////////////////
	//Set up the body
	if(mbAttachMeshToBody && mfHealth > 0)
		mpMover->GetCharBody()->SetEntity(mpMeshEntity);
	
	mpMover->GetCharBody()->GetBody()->SetUserData(this);

	mpMover->GetCharBody()->Update(0.001f);

	mpMover->SetAStar(mpAStarGround);
	mpMover->SetNodeContainer(mpNodeContainerGround);
	
	//////////////////////////////////
	//Stop all animations
    mpMeshEntity->Stop();
	mpMeshEntity->UpdateLogic(0.005f);

	//////////////////////////////////
	//Preload data

	//Sounds
	for(size_t i=0; i< mvPreloadSounds.size(); ++i)
	{
		mpInit->PreloadSoundEntityData(mvPreloadSounds[i]);
	}

    //Particle system
	mpInit->PreloadParticleSystem(msHitPS);
	
	//////////////////////////////////
	//Implemented load
	OnLoad();

	//////////////////////////////////
	//Check if dead
	mbLoading = true;
	if(mfHealth <= 0)
	{
		ChangeState(STATE_DEAD);
		mpMeshEntity->SetSkeletonPhysicsCanSleep(false);
		mpMeshEntity->UpdateLogic(1.0f/60.0f);
		mpMeshEntity->SetSkeletonPhysicsCanSleep(true);
	}
	mbLoading = false;

	mpMeshEntity->ResetGraphicsUpdated();
}

//-----------------------------------------------------------------------

void iGameEnemy::OnPostLoadScripts()
{
	//Randomize start pos
	if(IsActive() && mvPatrolNodes.size()>0 && mbHasBeenActivated)
	{
		int lStartNode = cMath::RandRectl(0,(int)mvPatrolNodes.size()-1);
		tString sNode = mvPatrolNodes[lStartNode].msNodeName;
        cAINode *pNode = mpMover->GetNodeContainer()->GetNodeFromName(sNode);

		mpMover->GetCharBody()->SetFeetPosition(pNode->GetPosition());
	}
	else
	{
		mbHasBeenActivated = true;
	}
}

//-----------------------------------------------------------------------

void iGameEnemy::OnWorldExit()
{
	if(mfHealth <=0)
	{
		SetActive(false);
	}

	mpInit->mpMusicHandler->RemoveAttacker(this);
}

//-----------------------------------------------------------------------

float gfAngle=0;
float gfCurrentViewDist=0;
float gfCurrentMaxViewDist=0;

void iGameEnemy::OnDraw()
{
	return;
	if(mbActive==false) return;
	if(mbCanSeePlayer){
		mpInit->mpDefaultFont->Draw(cVector3f(5,15,100),14,cColor(1,1,1,1),eFontAlign_Left,
									_W("Player is seen!"));
	}
	else {
		mpInit->mpDefaultFont->Draw(cVector3f(5,15,100),14,cColor(1,1,1,1),eFontAlign_Left,
			_W("Can NOT see player..."));
	}
	//mpInit->mpDefaultFont->Draw(cVector3f(5,29,100),14,cColor(1,1,1,1),eFontAlign_Left,
	//	"State: %s",mStateMachine.CurrentState()->GetName().c_str());
	
	tWString sStateName = _W("NONE");
	if(mlCurrentState >=0) sStateName = cString::To16Char(gvStateName[mlCurrentState]);
	mpInit->mpDefaultFont->Draw(cVector3f(5,48,100),14,cColor(1,1,1,1),eFontAlign_Left,
		_W("Health: %f State: %ls Moving: %d Stuck: %f MaxViewDist: %f"),mfHealth,
											sStateName.c_str(),
											mpMover->IsMoving(),
											mpMover->GetStuckCounter(),
											gfCurrentMaxViewDist);
	mpInit->mpDefaultFont->Draw(cVector3f(5,64,100),14,cColor(1,1,1,1),eFontAlign_Left,
		_W("Speed: %f"), mpMover->GetCharBody()->GetMoveSpeed(eCharDir_Forward));

											

	mpMover->OnDraw(mpInit);

	mvStates[mlCurrentState]->OnDraw();

	/*mpInit->mpDefaultFont->Draw(cVector3f(5,15,100),14,cColor(1,1,1,1),eFontAlign_Left,
								"Active: %d",mbActive);
	mpInit->mpDefaultFont->Draw(cVector3f(5,30,100),14,cColor(1,1,1,1),eFontAlign_Left,
								"Yaw: %f",cMath::ToDeg(mpMover->GetCharBody()->GetYaw()));
	mpInit->mpDefaultFont->Draw(cVector3f(5,45,100),14,cColor(1,1,1,1),eFontAlign_Left,
								"Pos: %s",mpMover->GetCharBody()->GetPosition().ToString().c_str());*/
}

//-----------------------------------------------------------------------

void iGameEnemy::OnPostSceneDraw()
{
	if(IsActive()==false) return;
	if(mbShowDebug == false) return;
	
	iLowLevelGraphics *pLowLevelGfx = mpInit->mpGame->GetGraphics()->GetLowLevel();
	mpMover->OnPostSceneDraw(pLowLevelGfx);

	ExtraPostSceneDraw();

	/////////////////////////////////////
	// Begin debug pos

	/*pLowLevelGfx->SetDepthTestActive(false);
	pLowLevelGfx->SetDepthWriteActive(false);
	
	cVector3f vNormal(0,1,0);
	cVector3f vUp(0,1,0);
	cVector3f vStartPos = mpMover->GetCharBody()->GetFeetPosition() + cVector3f(0,0.05f,0);
	cVector3f vPosition = vStartPos;

	mFindGround.GetGround(vStartPos,cVector3f(0,-1,0),NULL,&vNormal);
	
	vNormal.Normalise();
	float fAngle = cMath::Vector3Angle(vUp,vNormal);
	cVector3f vRotateAxis = cMath::Vector3Cross(vUp,vNormal);
	//cVector3f vRotateAxis2 = cMath::Vector3Cross(vUp,vRotateAxis);

	vRotateAxis.Normalise();
	cQuaternion qRotation = cQuaternion(fAngle, vRotateAxis);
	cMatrixf mtxPoseRotation = cMath::MatrixQuaternion(qRotation);

	cMatrixf mtxFinalOffset = cMath::MatrixMul(mtxPoseRotation,m_mtxModelOffset);

    cVector3f vCenter = mpMover->GetCharBody()->GetPosition();

	cVector3f vRot = cMath::MatrixMul(mtxPoseRotation,cVector3f(0,1,0));

	pLowLevelGfx->DrawLine(vStartPos, vStartPos + vNormal,cColor(1,0,1,1));
	pLowLevelGfx->DrawLine(vCenter, vCenter + vRotateAxis,cColor(1,0.5,0.5,1));
//	pLowLevelGfx->DrawLine(vCenter, vCenter + vRotateAxis2,cColor(1,0.5,0.5,1));
	pLowLevelGfx->DrawLine(vCenter, vCenter + vRot,cColor(0,1,1,1));

	gfAngle =cMath::ToDeg(fAngle);
			
	pLowLevelGfx->SetDepthTestActive(true);
	pLowLevelGfx->SetDepthWriteActive(true);*/

	// End debug pose
	//////////////////////////////

	/*pLowLevelGfx->SetDepthTestActive(false);
	pLowLevelGfx->SetDepthWriteActive(false);

	pLowLevelGfx->DrawSphere(mpMover->GetCharBody()->GetPosition(),0.1f,cColor(1,0.5,0));
	pLowLevelGfx->DrawSphere(GetMeshEntity()->GetWorldPosition(),0.1f,cColor(0,0.5,1));

	pLowLevelGfx->SetDepthTestActive(true);
	pLowLevelGfx->SetDepthWriteActive(true);*/


	/*for(size_t i=0; i< mvRayStartPoses.size(); ++i)
	{
		pLowLevelGfx->DrawLine(mvRayStartPoses[i], mvRayEndPoses[i],cColor(1,0,1,1));
		pLowLevelGfx->DrawSphere(mvRayStartPoses[i],0.2f,cColor(0,1,1,1));
		pLowLevelGfx->DrawSphere(mvRayEndPoses[i],0.2f,cColor(0,1,1,1));
	}*/

	//mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->RenderDebugGeometry(pLowLevelGfx,cColor(1,1));

	mvStates[mlCurrentState]->OnPostSceneDraw();
}

//-----------------------------------------------------------------------

void iGameEnemy::Update(float afTimeStep)
{
	if(mbActive==false) return;
	
	START_TIMING_EX(GetName().c_str(),enemy);

	// Test hook: with OVERTURE_ENEMY_LOG, where the enemy is, once a second
	static const bool bLogStates = SDL_getenv("OVERTURE_ENEMY_LOG") != nullptr;
	if(bLogStates && mpInit->mpPlayer->GetCharacterBody())
	{
		mfLogCount -= afTimeStep;
		if(mfLogCount <= 0)
		{
			mfLogCount = 1.0f;
			cVector3f vPos = mpMover->GetCharBody()->GetPosition();
			cVector3f vPlayer = mpInit->mpPlayer->GetCharacterBody()->GetPosition();
			Log("ENEMY %s: state %d at (%.1f %.1f %.1f) player (%.1f %.1f %.1f) moving %d stuck %.2f sees %d anim %s\n",
				msName.c_str(),mlCurrentState,vPos.x,vPos.y,vPos.z,vPlayer.x,vPlayer.y,vPlayer.z,
				mpMover->IsMoving(),mpMover->GetStuckCounter(),mbCanSeePlayer,
				mpCurrentAnimation ? mpCurrentAnimation->GetName() : "-");
		}
	}

	if(mpMeshEntity->GetSkeletonPhysicsActive() && mpCharBody->IsActive()==false &&
		mfHealth <=0)
	{
		mbHasInteraction = true;
	}
	else
	{
		mbHasInteraction = false;
	}
	
	START_TIMING_TAB(pose);
	UpdateEnemyPose(afTimeStep);
	STOP_TIMING_TAB(pose);

	START_TIMING_TAB(checkforplayer);
	UpdateCheckForPlayer(afTimeStep);
	STOP_TIMING_TAB(checkforplayer);
	
	START_TIMING_TAB(MoverUpdate);
	mpMover->Update(afTimeStep);
	STOP_TIMING_TAB(MoverUpdate);

	START_TIMING_TAB(Animations);
	UpdateAnimations(afTimeStep);
	STOP_TIMING_TAB(Animations);

	OnUpdate(afTimeStep);
	
#ifdef UPDATE_TIMING_ENABLED
	LogUpdate("\tState: %d\n",mlCurrentState);
#endif
	START_TIMING_TAB(State);
	mvStates[mlCurrentState]->OnUpdate(afTimeStep);
	STOP_TIMING_TAB(State);
	

	if(mfDamageSoundTimer>0) mfDamageSoundTimer -= afTimeStep;

	if(mfSkipSoundTriggerCount>0) mfSkipSoundTriggerCount -= afTimeStep;

	if(mfDoorBreakCount>0) mfDoorBreakCount -= afTimeStep;

	//////////////////////////////////////////////
	//Disappear
	if(mbDisappear && GetHealth()<=0 && mbHasDisappeared==false)
	{
		if(mbDisappearActive)
		{
			if(mfDisappearTime <= 0)
			{
				mbHasDisappeared = true;

				cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
				cVector3f vPostion = mpMeshEntity->GetBoundingVolume()->GetWorldCenter();

				if(msDisappearSound!="")
				{
					cSoundEntity *pSound = pWorld->CreateSoundEntity("Disappear",msDisappearSound,true);
					if(pSound) pSound->SetPosition(vPostion);
				}
				if(msDisappearPS!="")
				{
					pWorld->CreateParticleSystem("Disappear",msDisappearPS,cVector3f(1,1,1),
												cMath::MatrixTranslate(vPostion));
				}
				
				if(mbDisappearFreezesRagdoll)
				{
					mpMeshEntity->ResetGraphicsUpdated();

					for(int i=0; i< mpMeshEntity->GetBoneStateNum(); ++i)
					{
						cBoneState *pBone = mpMeshEntity->GetBoneState(i);
						iPhysicsBody *pBody = pBone->GetBody();

						if(pBody)
						{
							pBody->SetMass(0);
						}
					}
				}
				else
				{
					SetActive(false);
				}
			}
			else
			{
				mfDisappearTime	-= afTimeStep;
			}
		}
		else
		{
			mbDisappearActive = true;
			mfDisappearTime = cMath::RandRectf(mfDisappearMinTime,mfDisappearMaxTime);
		}
	}

	//////////////////////////////////////////////
	//Outside of map
	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	cBoundingVolume worldBV;
	worldBV.SetLocalMinMax(pPhysicsWorld->GetWorldSizeMin(),pPhysicsWorld->GetWorldSizeMax());

	if(cMath::CheckCollisionBV(worldBV, *mpMover->GetCharBody()->GetBody()->GetBV())==false)
	{
		SetHealth(0);
		SetActive(false);
	}
	
	STOP_TIMING(enemy);
}

//-----------------------------------------------------------------------

void iGameEnemy::AddState(iGameEnemyState *apState)
{
	mvStates[apState->GetId()] = apState;
}

void iGameEnemy::ChangeState(int alId)
{
	if(mlCurrentState == alId) return;

	/*char sStr[512];
	tString sStateName1 = "NONE";
	if(mlCurrentState >=0) sStateName1 = gvStateName[mlCurrentState];
	tString sStateName2 = "NONE";
	if(mlCurrentState >=0) sStateName2 = gvStateName[alId];

	sprintf(sStr,"%s State %s -> %s",msName.c_str(),sStateName1.c_str(),sStateName2.c_str());
	//mpInit->mpEffectHandler->GetSubTitle()->Add(cString::To16Char(sStr),1.2f,false);
	Log("%s\n",sStr);*/

	// Test hook: OVERTURE_ENEMY_LOG=1 writes each change of state to the log
	static const bool bLogStates = SDL_getenv("OVERTURE_ENEMY_LOG") != nullptr;
	if(bLogStates)
	{
		Log("ENEMY %s: state %d -> %d (health %.1f, player health %.1f, dist %.2f)\n",msName.c_str(),
			mlCurrentState,alId,mfHealth,mpInit->mpPlayer->GetHealth(),
			(mpMover->GetCharBody() && mpInit->mpPlayer->GetCharacterBody()) ?
				mpMover->DistanceToChar2D(mpInit->mpPlayer->GetCharacterBody()) : -1.0f);
	}

	//Log("Leave old...");
	if(mlCurrentState>=0) 
		mvStates[mlCurrentState]->OnLeaveState(mvStates[alId]);
	
	int lPrevState = mlCurrentState;
	iGameEnemyState *pPrevState = nullptr;
	if(mlCurrentState>=0) pPrevState = mvStates[mlCurrentState];
	
	mlCurrentState = alId;
	mbCanSeePlayer = false;
	
	//Log("enter newer\n");
	mvStates[mlCurrentState]->SetPreviousState(lPrevState);
	mvStates[mlCurrentState]->OnEnterState(pPrevState);
}

iGameEnemyState *iGameEnemy::GetState(int alId)
{
	if(alId < 0 || alId >= (int)mvStates.size()) return nullptr;
	return mvStates[alId];
}


//-----------------------------------------------------------------------

bool iGameEnemy::HandleTrigger(cGameTrigger *apTrigger)
{
	switch(apTrigger->GetType())
	{
	case eGameTriggerType_Sound: return HandleSoundTrigger(apTrigger);
	}


	return true;
}

//-----------------------------------------------------------------------

bool iGameEnemy::HandleSoundTrigger(cGameTrigger *apTrigger)
{
	if(mfSkipSoundTriggerCount>0) return false;

	cGameTrigger_Sound *pSoundTrigger = static_cast<cGameTrigger_Sound*>(apTrigger);

	//////////////////////////////////////
	//Calculate volume of sound
	float fDistance = cMath::Vector3Dist(GetPosition(), pSoundTrigger->GetWorldPosition());
	float fMin = pSoundTrigger->mpSound->GetMinDistance();
	float fMax = pSoundTrigger->mpSound->GetMaxDistance();

	float fHearVolume = 1.0f - cMath::Clamp( (fDistance - fMin)/(fMax - fMin), 0.0f ,1.0f);

	fHearVolume *= pSoundTrigger->mpSound->GetVolume();
	
	//If not audible return
	if(fHearVolume <=0) return false;
	
    return mvStates[mlCurrentState]->OnHearNoise(pSoundTrigger->GetWorldPosition(),fHearVolume);

	return true;
}

//-----------------------------------------------------------------------

cVector3f iGameEnemy::GetPosition()
{
	return mpMover->GetCharBody()->GetPosition();	
}

//-----------------------------------------------------------------------

void iGameEnemy::PlayAnim(	const tString &asName, bool abLoop, float afFadeTime,
							bool abDependsOnSpeed, float afSpeedMul,
							bool abSyncWithPrevFrame,
							bool abOverrideMoveState)
{
	//Check if the animation is already playing.
	if(	mpCurrentAnimation != nullptr && 
		mpCurrentAnimation->GetName() == asName && 
		mpCurrentAnimation->IsActive() &&
		mpCurrentAnimation->IsOver()== false)
	{
		// A patrol/script may reuse the movement clip with different options.
		mpCurrentAnimation->SetLoop(abLoop);
		mbAnimationIsSpeedDependant = abDependsOnSpeed;
		mfAnimationSpeedMul = afSpeedMul;
		mbOverrideMoveState = abOverrideMoveState;
		if(!abDependsOnSpeed) mpCurrentAnimation->SetSpeed(afSpeedMul);
		return;
	}

	cAnimationState *pNewAnim = mpMeshEntity->GetAnimationStateFromName(asName);
	if(pNewAnim==nullptr){
		//Warning("Animation '%s' does not exist!\n",asName.c_str());
		return;
	}
	
	pNewAnim->SetActive(true);
	if(mpCurrentAnimation && mpCurrentAnimation != pNewAnim) 
	{
		mpCurrentAnimation->FadeOut(afFadeTime);
		
		if(pNewAnim->IsFading()==false) pNewAnim->SetWeight(0);
		pNewAnim->FadeIn(afFadeTime);
	}
	else
	{
		pNewAnim->SetWeight(1.0f);
	}
	pNewAnim->SetLoop(abLoop);
	
	/////////////////////////////////////////
	//Check if this animation should start at the same place as the previous
	if(abSyncWithPrevFrame && mpCurrentAnimation)
	{
		pNewAnim->SetRelativeTimePosition(mpCurrentAnimation->GetRelativeTimePosition());
	}
	else
	{
		pNewAnim->SetTimePosition(0);
	}
	

	mpCurrentAnimation  = pNewAnim;
	mbAnimationCompletionSent = false;
	// Movement can leave a shared clip at speed zero. Authored base speed
	// still applies; reset only the runtime multiplier for a fixed clip.
	if(!abDependsOnSpeed) pNewAnim->SetSpeed(afSpeedMul);

	mbAnimationIsSpeedDependant = abDependsOnSpeed;
	mfAnimationSpeedMul = afSpeedMul;

	mbOverrideMoveState = abOverrideMoveState;
}

//-----------------------------------------------------------------------

void iGameEnemy::UseMoveStateAnimations()
{
	if(mbOverrideMoveState)
	{
		mbOverrideMoveState = false;
		mMoveState = eEnemyMoveState_LastEnum;
	}
}

//-----------------------------------------------------------------------

void iGameEnemy::PlaySound(const tString &asName)
{
	if(asName == "") return;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

    cSoundEntity *pSound = pWorld->CreateSoundEntity("Enemy",asName,true);
	if(pSound)
	{
		if(cInit::IsOverture())
		{
			pSound->SetPosition(mpMover->GetCharBody()->GetPosition());
			//TODO: Attach instead...
		}
		//Black Plague: one sound at a time, and it follows the enemy
		else
		{
			if(mpCurrentSound)
			{
				if(pWorld->SoundEntityExists(mpCurrentSound)) mpCurrentSound->FadeOut(0.1f);
				mpCurrentSound = nullptr;
			}
			mpCurrentSound = pSound;

			mpMover->GetCharBody()->GetBody()->AddChild(pSound);
		}
	}
	else
	{
		Warning("Couldn't play sound '%s'\n",asName.c_str());
	}
}

//-----------------------------------------------------------------------

void iGameEnemy::AddPatrolNode(const tString& asNode, float afWaitSeconds, const tString& asAnimation)
{
	mvPatrolNodes.push_back(cEnemyPatrolNode(asNode,afWaitSeconds,asAnimation));
}

//-----------------------------------------------------------------------

void iGameEnemy::IncCurrentPatrolNode()
{ 
	mlCurrentPatrolNode++;
	if(mlCurrentPatrolNode < (int)mvPatrolNodes.size()) return;
	
	mlCurrentPatrolNode =0;

	//Black Plague: the patrol can end at its last node, and tell the script.
	if(mbStopAtLastPatrolNode)
	{
		ClearPatrolNodes();
	}

	if(msOnPatrolOverCallback != "")
	{
		tString sCommand = msOnPatrolOverCallback + "(\"" + msName + "\")";
		msOnPatrolOverCallback = "";
		mpInit->RunScriptCommand(sCommand);
	}
}

//-----------------------------------------------------------------------

void iGameEnemy::ClearPatrolNodes()
{
	mvPatrolNodes.clear();
	mlCurrentPatrolNode =0;

	if(mbActive && mfHealth >0) ChangeState(STATE_IDLE);
}

//-----------------------------------------------------------------------

void iGameEnemy::OnDeath(float afDamage)
{
	//PlaySound("temp_roach_death");

	if(msOnDeathCallback != "")
	{
		tString sCommand = msOnDeathCallback + "(\""+msName+"\")";
		msOnDeathCallback ="";

		mpInit->RunScriptCommand(sCommand);
	}

	mvStates[mlCurrentState]->OnDeath(afDamage);
}

//-----------------------------------------------------------------------

void iGameEnemy::OnDamage(float afDamage)
{
	if(mfDamageSoundTimer<=0)
	{
		//PlaySound("temp_roach_damage");

		mfDamageSoundTimer =0.8f;
	}

	mvStates[mlCurrentState]->OnTakeHit(afDamage);
}

//-----------------------------------------------------------------------

void iGameEnemy::OnFlashlight(const cVector3f &avPos)
{
	mvStates[mlCurrentState]->OnFlashlight(avPos);
}

//-----------------------------------------------------------------------

void iGameEnemy::OnSetActive(bool abActive)
{
	//This will do for now:
	for(size_t i=0; i<mvBodies.size(); ++i)
	{
		mvBodies[i]->SetActive(false);
	}
	
	//Make sure it is on the ground
	if(mfHealth >0 && mbSetFeetAtGroundOnStart)
	{
		cVector3f vGroundPosition = mpMover->GetCharBody()->GetFeetPosition();
		mFindGround.GetGround(mpMover->GetCharBody()->GetPosition(),cVector3f(0,-1,0),
								&vGroundPosition,nullptr);
		mpMover->GetCharBody()->SetFeetPosition(vGroundPosition);
	}

	
	if(mbActive==false)
	{
		if(mbRemoveAttackerOnDisable)
			mpInit->mpMusicHandler->RemoveAttacker(this);
		
		if(mfHealth >0) ChangeState(STATE_IDLE);
	}
	else
	{
		mbHasBeenActivated = true;
	}
}

//-----------------------------------------------------------------------


bool iGameEnemy::CanSeePlayer()
{
	if(mpInit->mpMapHandler->IsPreUpdating() || mpInit->mpPlayer->IsDead()) return false;

	return mbCanSeePlayer;
}

//-----------------------------------------------------------------------

bool iGameEnemy::CheckForDoor()
{
	iCharacterBody *pBody = mpMover->GetCharBody();
	float fRadius = pBody->GetSize().x/2.0f - 0.1f;
	cVector3f vStart = pBody->GetPosition() + pBody->GetForward() * fRadius;
	cVector3f vEnd = vStart + pBody->GetForward() * 0.4f;

	bool bRet = mDoorCheck.CheckDoor(vStart,vEnd);
	//Black Plague leaves this line out of the log
	if(cInit::IsOverture()) Log("CheckDoor: %d\n",bRet);

	return bRet;
}

bool iGameEnemy::CheckForTeamMate(float afMaxDist, bool abCheckIfFighting)
{
	cVector3f vPosition = mpMover->GetCharBody()->GetFeetPosition();
	tGameEnemyIterator it = mpInit->mpMapHandler->GetGameEnemyIterator();
	while(it.HasNext())
	{
		iGameEnemy *pEnemy = it.Next();
		if(GetEnemyType() != pEnemy->GetEnemyType()) continue;
		if(pEnemy == this || pEnemy->IsActive()==false || pEnemy->GetHealth()<=0) continue;

		if(abCheckIfFighting && pEnemy->IsFighting()==false) continue;

		float fDist = cMath::Vector3Dist(	pEnemy->GetMover()->GetCharBody()->GetPosition(),
											vPosition);

		if(fDist <= afMaxDist)
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PROTECTED METHODS
//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------

void iGameEnemy::UpdateEnemyPose(float afTimeStep)
{
	if(mbAlignToGroundNormal==false) return;

	if(mfGroundPoseProgress ==0)
	{
		m_mtxStartPose = m_mtxGoalPose;

		cVector3f vNormal(0,1,0);
			
		cVector3f vStartPos = mpMover->GetCharBody()->GetFeetPosition() + cVector3f(0,0.05f,0);
		cVector3f vPosition = vStartPos;
		
		mFindGround.GetGround(vStartPos,cVector3f(0,-1,0),&vPosition,&vNormal);

		cVector3f vUp(0,1,0);
		
		float fDist = vStartPos.y - vPosition.y;
        
        vNormal.Normalise();
		float fAngle = cMath::Vector3Angle(vUp,vNormal);
		
		cVector3f vRotateAxis = cMath::Vector3Cross(vUp,vNormal);
		//cVector3f vRotateAxis = cMath::Vector3Cross(vUp,mpMover->GetCharBody()->GetForward());

		//cVector3f vRotateAxis2 = cMath::Vector3Cross(vUp,vRotateAxis);
		
		vRotateAxis.Normalise();
		cQuaternion qRotation = cQuaternion(fAngle, vRotateAxis);
		cMatrixf mtxPoseRotation = cMath::MatrixQuaternion(qRotation);

		//Black Plague: only moved down to the ground, not leant after it
		if(mbAlignToGroundSkipNormal) mtxPoseRotation = cMatrixf::Identity;
		
			//cVector3f vDelta = vPosition - mpMover->GetCharBody()->GetFeetPosition();
			
			//mtxPoseRotation.SetTranslation(cVector3f(0,-fabs(vDelta.y),0));
			//mtxPoseRotation.SetTranslation(vNormal * -fabs(vDelta.y));
		
		if(vNormal != cVector3f(0,1,0))
		{
			mFindGround.GetGround(mpMover->GetCharBody()->GetPosition(),vNormal *-1.0f,&vPosition,nullptr);
			fDist = cMath::Vector3Dist(mpMover->GetCharBody()->GetPosition(),vPosition);
			
			//So there is no warp to the ground.
			float fLimit = mpMover->GetCharBody()->GetSize().y*0.82f;
			if(fDist > fLimit) vNormal = cVector3f(0,1,0);
		}

		///////////////////////////
		//Get the offest
		if(vNormal != cVector3f(0,1,0))
		{
			fDist -= mpMover->GetCharBody()->GetSize().y/2.0f;
			
			mtxPoseRotation.SetTranslation(vNormal * -fDist);
		}
		else
		{
			mtxPoseRotation.SetTranslation(0);
		}
	
		m_mtxGoalPose = mtxPoseRotation;
	}
	else
	{
		cMatrixf mtxPoseRotation =cMath::MatrixSlerp(mfGroundPoseProgress,m_mtxStartPose,m_mtxGoalPose,true);
        
		mpMover->GetCharBody()->SetEntityPostOffset(mtxPoseRotation);
	}

	mfGroundPoseProgress+= 6.5f *afTimeStep;
	if(mfGroundPoseProgress >1.0f) mfGroundPoseProgress =0;
}

//-----------------------------------------------------------------------

void iGameEnemy::UpdateCheckForPlayer(float afTimeStep)
{
	

	//Do not check for player at pre update.
	if(mpInit->mpMapHandler->IsPreUpdating() ||
		mpInit->mpPlayer->IsDead() ||
		mbUsesTriggers == false ||
		mfHealth <= 0) 
	{
		mbCanSeePlayer = false;
		return;
	}

	/*if(mfCanSeePlayerCount>0)
	{
		mfCanSeePlayerCount -= afTimeStep;
		if(mfCanSeePlayerCount<=0) mbCanSeePlayer = false;
	}*/

	if(mfCalcPlayerHiddenPosCount >0)
		mfCalcPlayerHiddenPosCount -= afTimeStep;


	//Check if it is time to check for player.
	if(mfPlayerCheckElapsedSeconds < mfPlayerCheckIntervalSeconds)
	{
		mfPlayerCheckElapsedSeconds += afTimeStep;
		return;
	}
	
	mfPlayerCheckElapsedSeconds =0;

	iCharacterBody *pPlayerBody = mpInit->mpPlayer->GetCharacterBody();

	float fDist = cMath::Vector3Dist(mpMover->GetCharBody()->GetPosition(),pPlayerBody->GetPosition());
	float fMinLength = mpMover->GetCharBody()->GetBody()->GetBV()->GetRadius() + 
						pPlayerBody->GetBody()->GetBV()->GetRadius();

	//Lower some stuff if player is hidden
	float fViewFOV = mfFOV;
	float fMaxViewDist = mfMaxSeeDist;
	if(mbCanSeePlayer==false && fDist >1.3f) //1.3 = really close, remove all handicap.
	{
		if(mpInit->mDifficulty == eGameDifficulty_Easy){
			fViewFOV *= 0.6f;
			fMaxViewDist *= 0.6f;
		}

		if(mpInit->mpPlayer->GetHidden()->IsHidden())
		{
			fViewFOV *= 0.36f;
			fMaxViewDist *= 0.25f;
		}
		else if(mpInit->mpPlayer->GetHidden()->InShadows())
		{
			if(mpInit->mpPlayer->GetMoveState() == ePlayerMoveState_Crouch)
			{
				fViewFOV *= 0.6f;
				fMaxViewDist *= 0.65f;
			}
			else
			{
				fViewFOV *= 0.8f;
				fMaxViewDist *= 0.85f;
			}
		}
	}

	gfCurrentViewDist = fDist;
	gfCurrentMaxViewDist = fMaxViewDist;

	//Black Plague: a player who touches the enemy is no longer seen through
	//a wall, but is seen from any side.
	bool bSeen;
	if(cInit::IsOverture())
		bSeen = (fDist <= fMaxViewDist && LineOfSight(pPlayerBody->GetPosition(), pPlayerBody->GetSize(), fViewFOV)) ||
				fDist <=  fMinLength;
	else
		bSeen = fDist <= fMaxViewDist &&
				LineOfSight(pPlayerBody->GetPosition(), pPlayerBody->GetSize(), fViewFOV, fDist <= fMinLength);

	if(bSeen)
	{
		//Increase LOS counter,
		mlPlayerInLOSCount++;

		//Player must have been in LOS mlMaxPlayerInLOSCount times before it is considered seen.
		if(mlPlayerInLOSCount>= mlMaxPlayerInLOSCount)
		{
			mlPlayerInLOSCount = mlMaxPlayerInLOSCount;

			float fChance=0;

			if(fDist > fMaxViewDist || fMaxViewDist <= 0)
				fChance =0;
			else 
				fChance = 1 - (fDist / fMaxViewDist);

			// The state entered by this callback must receive the observation
			// that triggered it, not the previous observation's position.
			mvLastPlayerPos = pPlayerBody->GetFeetPosition();

			if(mbCanSeePlayer==false)
			{
				mvStates[mlCurrentState]->OnSeePlayer(pPlayerBody->GetPosition(),fChance);
				mpInit->mpPlayer->GetHidden()->UnHide();
			}

			mbCanSeePlayer = true;
			mfCanSeePlayerCount = 1.0f/ 3.0f;
			mfCalcPlayerHiddenPosCount = 1.5f;
		}
	}
	else
	{
        //Reset LOS counter,
		mlPlayerInLOSCount--;
		if(mlPlayerInLOSCount<0)mlPlayerInLOSCount=0;

		//this is so that the enemy get a little better last pos 
		//and thus improving path finding.
		if(mfCalcPlayerHiddenPosCount >0 && TrackPlayerAfterSightLoss())
		{
			mvLastPlayerPos = pPlayerBody->GetFeetPosition();
		}

		mbCanSeePlayer = false;
	}

}

//-----------------------------------------------------------------------

void iGameEnemy::UpdateAnimations(float afTimeStep)
{
	iCharacterBody *pBody = mpMover->GetCharBody();
	
	float fMoveSpeed = pBody->GetMoveSpeed(eCharDir_Forward);
	
	cVector3f vVelocity = pBody->GetVelocity(afTimeStep);
	vVelocity.y = 0; // Falling/step climbing is not stride distance.
	float fSpeed = vVelocity.Length();
	if(fMoveSpeed <0) fSpeed = -fSpeed;
	
	float fTurnSpeed = mpMover->GetTurnSpeed();
	
	////////////////////////////////
	// Override animation
	if(mbOverrideMoveState && mpCurrentAnimation!=nullptr)
	{
		if(mpCurrentAnimation->IsOver() && !mbAnimationCompletionSent)
		{
			mbAnimationCompletionSent = true;
			const tString sName = mpCurrentAnimation->GetName();
			mvStates[mlCurrentState]->OnAnimationOver(sName);

			//Black Plague: the script's callback for an animation played by script
			if(msOnAnimOverCallback != "")
			{
				tString sCommand = msOnAnimOverCallback + "(\"" + msName + "\")";
				msOnAnimOverCallback = "";
				mpInit->RunScriptCommand(sCommand);
			}
			// Completion can replace the clip or enter another state. Let
			// that state control its own playback on the following update.
			return;
		}
		
		if(mbAnimationIsSpeedDependant)
		{
			if(std::abs(fSpeed) > 0.05f)
				mpCurrentAnimation->SetSpeed(std::abs(fSpeed) * mfAnimationSpeedMul);
			else
				mpCurrentAnimation->SetSpeed(std::abs(fTurnSpeed) * mfAnimationSpeedMul * 2);
		}
	}
	////////////////////////////////
	// Move state animation
	else
	{
		eEnemyMoveState prevMoveState = mMoveState;

		switch(mMoveState)
		{
		// Backward
		case eEnemyMoveState_Backward:
			if(fSpeed >= 0)
				mMoveState = eEnemyMoveState_Stopped;
			
			break;

		// Stopped State
		case eEnemyMoveState_Stopped:
				if(fSpeed < -0.05f)
					mMoveState = eEnemyMoveState_Backward;
				else if(fSpeed >= mfStoppedToWalkSpeed)
					mMoveState = eEnemyMoveState_Walking;
				else if(std::abs(fTurnSpeed) > 0.07f)
					mMoveState = eEnemyMoveState_Walking;
				
				break;
		
		// Walking State
		case eEnemyMoveState_Walking:
			if(fSpeed >= mfWalkToRunSpeed)
				mMoveState = eEnemyMoveState_Running;
			else if(fSpeed <= mfWalkToStoppedSpeed)
			{
				if(std::abs(fTurnSpeed) < 0.03f) mMoveState = eEnemyMoveState_Stopped;
			}
			
			break;
		
		// Running State
		case eEnemyMoveState_Running:
			if(fSpeed <= mfRunToWalkSpeed)
				mMoveState = eEnemyMoveState_Walking;
			
			break;

		// NULL
		case eEnemyMoveState_LastEnum:
			mMoveState = eEnemyMoveState_Stopped;
			break;
		}
		
		//////////////////////////////////////////////
		//If move state has changed, change animation
        if(prevMoveState != mMoveState)
		{
			//Backward
			if(mMoveState == eEnemyMoveState_Backward)
			{
				PlayAnim(msBackwardAnim,true,0.4f,true,mfMoveAnimSpeedMul,false,false);
			}
			//Stopped
			else if(mMoveState == eEnemyMoveState_Stopped)
			{
				PlayAnim(msStoppedAnim,true,0.7f,false,1.0f,false,false);
			}
			//Walking
			else if(mMoveState == eEnemyMoveState_Walking)
			{
				bool bSync = prevMoveState == eEnemyMoveState_Running ? true : false;

				PlayAnim(msWalkAnim,true,0.2f,true,mfMoveAnimSpeedMul,bSync,false);
			}
			//Running
			else if(mMoveState == eEnemyMoveState_Running)
			{
				bool bSync = prevMoveState == eEnemyMoveState_Walking ? true : false;

				PlayAnim(msRunAnim,true,0.2f,true,mfMoveAnimSpeedMul,bSync,false);
			}
		}
		
		/////////////////////////////////
		//Update animation speed
		if(mbAnimationIsSpeedDependant && mpCurrentAnimation)
		{
			if(std::abs(fSpeed) > 0.05f)
				mpCurrentAnimation->SetSpeed(std::abs(fSpeed) * mfMoveAnimSpeedMul);
			else
				mpCurrentAnimation->SetSpeed(std::abs(fTurnSpeed) * mfMoveAnimSpeedMul * 2);
		}
	}
}


//-----------------------------------------------------------------------

void iGameEnemy::SetupBody()
{
	mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mfMaxForwardSpeed);
	mpMover->GetCharBody()->SetMaxNegativeMoveSpeed(eCharDir_Forward,-mfMaxBackwardSpeed);
	mpMover->GetCharBody()->SetMoveAcc(eCharDir_Forward, mfAcceleration);
	//Black Plague
	if(cInit::IsOverture()==false)
		mpMover->GetCharBody()->SetMoveDeacc(eCharDir_Forward, mfDeacceleration);

	mpMover->GetCharBody()->SetMaxPushMass(mfMaxPushMass);
	mpMover->GetCharBody()->SetPushForce(mfPushForce);

	mpMover->SetMaxTurnSpeed(mfMaxTurnSpeed);
	mpMover->SetAngleDistTurnMul(mfAngleDistTurnMul);

	mpMover->SetMinBreakAngle(mfMinBreakAngle);
	mpMover->SetBreakAngleMul(mfBreakAngleMul);

	mpMover->SetMaxPushMass(mfMaxPushMass);
}

//-----------------------------------------------------------------------

static const cVector2f gvPosAdds[] = {cVector2f(0,0),
										cVector2f(1,0),
										cVector2f(-1,0),
										cVector2f(0,1),
										cVector2f(0,-1)
};	

//Black Plague looks from the top of the body, with more rays and closer together.
static const cVector2f gvPosAddsBP[] = {cVector2f(0,0),
										cVector2f(0.5f,0),
										cVector2f(-0.5f,0),
										cVector2f(0,1),
										cVector2f(0,-1),
										cVector2f(0.25f,0.25f),
										cVector2f(-0.25f,0.25f),
										cVector2f(0.25f,0),
										cVector2f(-0.25f,0)
};	

bool iGameEnemy::LineOfSight(const cVector3f &avPos, const cVector3f &avSize, float afFOV, bool abSkipFOV)
{
	const float fFOV = afFOV >= 0 ? afFOV : mfFOV;
	const bool bOverture = cInit::IsOverture();
	const cVector2f *pPosAdds = bOverture ? gvPosAdds : gvPosAddsBP;
	const int lRayNum = bOverture ? 5 : 9;

	//Setup debug
	//if(mvRayStartPoses.size()<5) mvRayStartPoses.resize(5);
	//if(mvRayEndPoses.size()<5) mvRayEndPoses.resize(5);

	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	
	cVector3f vStartCenter = mpMover->GetCharBody()->GetPosition();
	if(bOverture==false) vStartCenter.y += mpMover->GetCharBody()->GetSize().y*0.5f - 0.1f;
	cVector3f vEndCenter = avPos;

	/////////////////////////////
	//Calculate the right vector
	const cVector3f vForward = cMath::Vector3Normalize(vEndCenter - vStartCenter);
	const cVector3f vUp = cVector3f(0,1.0f,0);
	const cVector3f vRight = cMath::Vector3Cross(vForward, vUp);

	////////////////////////////////////
	//Check if the pos is within FOV
	if(fFOV < k2Pif && abSkipFOV==false)
	{
		cVector3f vEnemyForward = mpMover->GetCharBody()->GetForward();
		
		//float fAngle = cMath::Vector3Angle(vEnemyForward, vForward);
		//if(fAngle > mfFOV*0.5f) return false;

		cVector3f vToPlayerAngle = cMath::GetAngleFromPoints3D(0,vForward);
		cVector3f vEnemyAngle = cMath::GetAngleFromPoints3D(0,vEnemyForward);

		float fAngleX = cMath::Abs(cMath::GetAngleDistanceRad(vToPlayerAngle.x,vEnemyAngle.x));
		float fAngleY = cMath::Abs(cMath::GetAngleDistanceRad(vToPlayerAngle.y,vEnemyAngle.y));

		//Log("X:%f Y:%f\n",cMath::ToDeg(fAngleX), cMath::ToDeg(fAngleY));

		if(fAngleY > fFOV*0.5f) return false;
		if(fAngleX > fFOV*mfFOVXMul*0.5f) return false;
	}

	//Get the half with and height. Make them a little smaller so that player can slide over funk on floor.
	const float fHalfWidth = avSize.x * 0.4f;
	const float fHalfHeight = avSize.y * 0.4f;
	
	//Count of 2 is need for a line of sight sucess.
	int lCount=0;
	//Iterate through all the rays.
	for(int i=0; i< lRayNum; ++i)
	{
		cVector3f vAdd = vRight * (pPosAdds[i].x*fHalfWidth) + vUp * (pPosAdds[i].y*fHalfHeight);
		cVector3f vStart = vStartCenter + vAdd;
		cVector3f vEnd = vEndCenter + vAdd;
		
		//mvRayStartPoses[i] = vStart;
		//mvRayEndPoses[i] =vEnd;
		
		mRayCallback.Reset(); 
		pPhysicsWorld->CastRay(&mRayCallback,vStart,vEnd,false,false,false);
		if(mRayCallback.Intersected()==false) lCount++;

		if(lCount==2) return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// LOADER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEntityLoader_GameEnemy::cEntityLoader_GameEnemy(const tString &asName, cInit *apInit)
: cEntityLoader_Object(asName)
{
	mpInit = apInit;
}

cEntityLoader_GameEnemy::~cEntityLoader_GameEnemy()
{

}

//-----------------------------------------------------------------------

void cEntityLoader_GameEnemy::BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
											   cWorld3D *apWorld)
{
	
}

//-----------------------------------------------------------------------

void cEntityLoader_GameEnemy::AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
											  cWorld3D *apWorld)
{
	iGameEnemy *pEnemy = nullptr;

	tString sSubtype="";
	tString sName="";

	///////////////////////////////////
	// Load game properties
	TiXmlElement *pMainElem = apRootElem->FirstChildElement("MAIN");
	if(pMainElem)
	{
		sSubtype = cString::ToString(pMainElem->Attribute("Subtype"),"");
		sName = cString::ToString(pMainElem->Attribute("Name"),"");
	}
	else
	{
		Error("Couldn't find main element for entity '%s'\n",mpEntity->GetName().c_str());
	}
	
	///////////////////////////////////
	// Load the enemy type

	TiXmlElement *pGameElem = apRootElem->FirstChildElement("GAME");

    
	if(sSubtype == "Dog")
	{
		pEnemy = hplNew( cGameEnemy_Dog, (mpInit,mpEntity->GetName(),pGameElem) );
	}
	else if(sSubtype == "Infected")
	{
		pEnemy = hplNew( cGameEnemy_Infected, (mpInit,mpEntity->GetName(),pGameElem) );
	}
#ifndef DEMO_VERSION
	else if(sSubtype == "Spider")
	{
		pEnemy = hplNew( cGameEnemy_Spider, (mpInit,mpEntity->GetName(),pGameElem) );
	}
	else if(sSubtype == "Worm")
	{
		pEnemy = hplNew( cGameEnemy_Worm, (mpInit,mpEntity->GetName(),pGameElem) );
	}
#endif
	if(pEnemy==nullptr)
	{
		Warning("Enemy '%s' has subtype '%s', which is not known; it is left out\n",
				mpEntity->GetName().c_str(), sSubtype.c_str());
		mpEntity->SetActive(false);
		mpEntity->SetVisible(false);
		for(size_t i=0; i<mvBodies.size(); ++i) mvBodies[i]->SetActive(false);
		return;
	}
	
	pEnemy->msSubType = sSubtype;
	pEnemy->msEnemyType = msName;
	pEnemy->msFileName = msFileName;
	pEnemy->m_mtxOnLoadTransform = a_mtxTransform;
	
	//Do stuff that is not done when loading from savegame.
	pEnemy->SetMeshEntity(mpEntity);
	pEnemy->SetBodies(mvBodies);
	pEnemy->Setup(apWorld);

	/////////////////////////////////
	// Add to map handler
	mpInit->mpMapHandler->AddGameEntity(pEnemy);
	mpInit->mpMapHandler->AddGameEnemy(pEnemy);
	
	iCharacterBody *pBody = pEnemy->mpMover->GetCharBody();
	pBody->SetPosition(mpEntity->GetWorldPosition() + cVector3f(0,pBody->GetSize().y/2,0));

	//Set the correct heading
	cMatrixf mtxInv = cMath::MatrixInverse(mpEntity->GetWorldMatrix());
	cVector3f vBodyRotation = cMath::GetAngleFromPoints3D(cVector3f(0,0,0), mtxInv.GetForward()*-1);

	pBody->SetYaw(vBodyRotation.y);
	
	//Log("Loaded enemy!\n");
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SAVE OBJECT STUFF
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

kBeginSerializeBase(cEnemyPatrolNode)
kSerializeVar(msNodeName, eSerializeType_String)
kSerializeVar(mfWaitTime, eSerializeType_Float32)
kSerializeVar(msAnimation, eSerializeType_String)
kEndSerialize()

kBeginSerialize(iGameEnemy_SaveData,iGameEntity_SaveData)
kSerializeVar(mbHasBeenActivated, eSerializeType_Bool)
kSerializeVar(mvCharBodyPosition, eSerializeType_Vector3f)
kSerializeVar(mvCharBodyRotation, eSerializeType_Vector3f)
kSerializeVar(mlCurrentPatrolNode, eSerializeType_Int32)

kSerializeVar(mfDisappearTime, eSerializeType_Float32)
kSerializeVar(mbDisappearActive, eSerializeType_Bool)
kSerializeVar(mbHasDisappeared, eSerializeType_Bool)

kSerializeVar(mbUsesTriggers, eSerializeType_Bool)

kSerializeVar(mvLastPlayerPos, eSerializeType_Vector3f)
kSerializeVar(msOnDeathCallback, eSerializeType_String)
kSerializeVar(msOnAttackCallback, eSerializeType_String)
kSerializeVar(msOnHuntCallback, eSerializeType_String)
kSerializeVar(msOnAnimOverCallback, eSerializeType_String)
kSerializeVar(msOnPatrolOverCallback, eSerializeType_String)
kSerializeVar(mbStopAtLastPatrolNode, eSerializeType_Bool)
kSerializeClassContainer(mvPatrolNodes,cEnemyPatrolNode,eSerializeType_Class)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* iGameEnemy_SaveData::CreateEntity()
{
	return nullptr;	
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* iGameEnemy::CreateSaveData()
{
	return hplNew( iGameEnemy_SaveData, () );
}

//-----------------------------------------------------------------------

void iGameEnemy::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::SaveToSaveData(apSaveData);
	iGameEnemy_SaveData *pData = static_cast<iGameEnemy_SaveData*>(apSaveData);

	kCopyToVar(pData,mbHasBeenActivated);

	pData->mvCharBodyPosition = mpMover->GetCharBody()->GetPosition();
	
	pData->mvCharBodyRotation.x = mpMover->GetCharBody()->GetPitch();
	pData->mvCharBodyRotation.y = mpMover->GetCharBody()->GetYaw();

	kCopyToVar(pData,mlCurrentPatrolNode);
	kCopyToVar(pData,mvLastPlayerPos);
	kCopyToVar(pData,msOnDeathCallback);
	kCopyToVar(pData,msOnAttackCallback);
	kCopyToVar(pData,msOnHuntCallback);
	kCopyToVar(pData,msOnAnimOverCallback);
	kCopyToVar(pData,msOnPatrolOverCallback);
	kCopyToVar(pData,mbStopAtLastPatrolNode);

	kCopyToVar(pData,mfDisappearTime);
	kCopyToVar(pData,mbDisappearActive);
	kCopyToVar(pData,mbHasDisappeared);

	kCopyToVar(pData,mbUsesTriggers);
	
	pData->mvPatrolNodes.Resize(mvPatrolNodes.size());
	for(size_t i=0; i< mvPatrolNodes.size(); ++i)
	{
		pData->mvPatrolNodes[i].msNodeName = mvPatrolNodes[i].msNodeName;
		pData->mvPatrolNodes[i].mfWaitTime = mvPatrolNodes[i].mfWaitTime;
		pData->mvPatrolNodes[i].msAnimation = mvPatrolNodes[i].msAnimation;
	}
}

//-----------------------------------------------------------------------

void iGameEnemy::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	iGameEnemy_SaveData *pData = static_cast<iGameEnemy_SaveData*>(apSaveData);

	kCopyFromVar(pData,mbHasBeenActivated);

	mpMover->GetCharBody()->SetPosition(pData->mvCharBodyPosition);

	mpMover->GetCharBody()->SetPitch(pData->mvCharBodyRotation.x);
	mpMover->GetCharBody()->SetYaw(pData->mvCharBodyRotation.y);
	mpMover->GetCharBody()->UpdateMoveMatrix();

	kCopyFromVar(pData,mlCurrentPatrolNode);
	kCopyFromVar(pData,mvLastPlayerPos);
	kCopyFromVar(pData,msOnDeathCallback);
	kCopyFromVar(pData,msOnAttackCallback);
	kCopyFromVar(pData,msOnHuntCallback);
	kCopyFromVar(pData,msOnAnimOverCallback);
	kCopyFromVar(pData,msOnPatrolOverCallback);
	kCopyFromVar(pData,mbStopAtLastPatrolNode);

	kCopyFromVar(pData,mfDisappearTime);
	kCopyFromVar(pData,mbDisappearActive);
	kCopyFromVar(pData,mbHasDisappeared);

	kCopyFromVar(pData,mbUsesTriggers);

	mvPatrolNodes.resize(pData->mvPatrolNodes.Size());
	for(size_t i=0; i< mvPatrolNodes.size(); ++i)
	{
		mvPatrolNodes[i].msNodeName = pData->mvPatrolNodes[i].msNodeName;
		mvPatrolNodes[i].mfWaitTime = pData->mvPatrolNodes[i].mfWaitTime;
		mvPatrolNodes[i].msAnimation = pData->mvPatrolNodes[i].msAnimation;
	}

	//Log("Load Save Data!\n");
}
//-----------------------------------------------------------------------
