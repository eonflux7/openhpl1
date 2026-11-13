// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "enemies/GameEnemy_Dog.h"


#include "player/Player.h"
#include "systems/AttackHandler.h"
#include "systems/EffectHandler.h"
#include "systems/GameMusicHandler.h"

// BASE STATE

iGameEnemyState_Dog_Base::iGameEnemyState_Dog_Base(int alId, cInit *apInit, iGameEnemy *apEnemy)
							: iGameEnemyState_DogBase(alId,apInit,apEnemy)
{	
	mpEnemyDog = static_cast<cGameEnemy_Dog*>(mpEnemy);
}

// ATTENTION STATE

void cGameEnemyState_Dog_Attention::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Angry",true,0.2f);

	cGameEnemyState_DogBase_Attention::OnEnterState(apPrevState);
}

// HUNT STATE

void cGameEnemyState_Dog_Hunt::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	float fMul = 1.0f;

	if(mpInit->mDifficulty == eGameDifficulty_Easy)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyDog->mfHuntSpeed*0.7f*fMul);
	else if(mpInit->mDifficulty == eGameDifficulty_Normal)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyDog->mfHuntSpeed*fMul);
	else 
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyDog->mfHuntSpeed*1.25f*fMul);


	//Setup enemy
	mpEnemy->SetFOV(mpEnemyDog->mfHuntFOV);

	mfPathUpdateDelaySeconds =0;
	mfPathUpdateIntervalSeconds = 1.0f;
	mbFreePlayerPath = false;
	
	if(mbBreakingDoor && mpEnemy->CanSeePlayer()==false)
	{
		mlBreakDoorCount++;
		if(mlBreakDoorCount>=3)
		{
			mpEnemy->ChangeState(STATE_IDLE);
			return;
		}
	}
	else
	{
		mlBreakDoorCount =0;
	}
	
	mbBreakingDoor = false;
	mbFoundNoPath = false;
	mbLostPlayer = false;
	mbCheckDynamicPath = false;
	mlSearchPhase = -1;
	mfLostPlayerRemainingSeconds =0;
	mfLostPlayerDurationSeconds = mpEnemyDog->mfHuntForLostPlayerTime;

	mlStuckAtMaxCount =0;

    mpInit->mpMusicHandler->AddAttacker(mpEnemy);
}

void cGameEnemyState_Dog_Hunt::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->SetMaxDoorToughness(-1);
}

// Original pursuit/search decisions, retained for in-game comparison.
void cGameEnemyState_Dog_Hunt::UpdateOriginal(float afTimeStep)
{
	if(mpPlayer->GetHealth() <=0){
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}

	if(mpMover->GetStuckCounter() > 1.1f)
	{
		if(mpEnemy->CheckForDoor())
		{
			mbBreakingDoor = true;
			mpEnemy->ChangeState(STATE_BREAKDOOR);
			mpMover->ResetStuckCounter();
			return;
		}
		else
		{
			mlStuckAtMaxCount++;
			if(mlStuckAtMaxCount >= 6)
			{
				mpEnemy->ChangeState(STATE_IDLE);
				return;
			}
		}
		mpMover->ResetStuckCounter();
	}

	if(mfPathUpdateDelaySeconds <=0)
	{
		mbFoundNoPath = false;
		mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds;

		if(mbLostPlayer == false && mpMover->FreeDirectPathToChar(mpPlayer->GetCharacterBody()))
		{
			mbFreePlayerPath = true;
			mpMover->Stop();
			mpMover->SetMaxDoorToughness(-1);
		}
		else
		{
			mbFreePlayerPath = false;
		}
		if(mbFreePlayerPath==false && mbLostPlayer == false)
		{
			if(mpEnemy->GetDoorBreakCount()> 6.0f)
			{
				mpMover->SetMaxDoorToughness(0);
			}

			if(mpMover->MoveToPos(mpEnemy->GetLastPlayerPos())==false)
			{
				mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds*5.0f;
				mpMover->Stop();
				mbFoundNoPath = true;
			}
		}
	}
	else
	{
		mfPathUpdateDelaySeconds -= afTimeStep;
	}
	if(mbFreePlayerPath || (mbFoundNoPath && mpMover->IsMoving()==false))
	{
		mpMover->MoveDirectToPos(mpPlayer->GetCharacterBody()->GetFeetPosition(),afTimeStep);
		// A nearby player behind a door must not trigger repeated lunges that
		// reset the stuck timer before the existing door-breaking check runs.
		if(mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody()) < mpEnemyDog->mfAttackDistance &&
			mpInit->mpAttackHandler->HasFreePathToPlayer(mpMover->GetCharBody()->GetPosition()))
		{
			float fHeight = mpMover->GetCharBody()->GetPosition().y -
							mpPlayer->GetCharacterBody()->GetPosition().y;
			if(fHeight < 0)
			{
				fHeight += mpMover->GetCharBody()->GetSize().y/2.0f;
				float fMax = mpEnemyDog->mvAttackDamageSize.y;
				if(fHeight > -fMax)
				{
					mpEnemy->ChangeState(STATE_ATTACK);
				}
				else
				{
					if(cMath::RandRectf(0,1)<0.2f)
						mpEnemy->ChangeState(STATE_ATTACK);
					else
						mpEnemy->ChangeState(STATE_FLEE);
				}
			}
			else
			{
				mpEnemy->ChangeState(STATE_ATTACK);
			}

		}
	}
	else if(mbFreePlayerPath==false)
	{
		if( mbLostPlayer==false && mpMover->IsMoving()==false && mpEnemy->CanSeePlayer()==false)
		{
			mbLostPlayer = true;
			mfLostPlayerRemainingSeconds = mfLostPlayerDurationSeconds;
		}

		if(mbLostPlayer)
		{
			mpMover->GetCharBody()->Move(eCharDir_Forward,1.0f,afTimeStep);

			mfLostPlayerRemainingSeconds -= afTimeStep;
			if(mfLostPlayerRemainingSeconds <= 0 || mpMover->GetStuckCounter()>0.5f)
			{
				mpEnemy->ChangeState(STATE_IDLE);
				return;
			}
		}
	}
}


void cGameEnemyState_Dog_Hunt::BeginSearch()
{
	mbLostPlayer = true;
	mfLostPlayerRemainingSeconds = mfLostPlayerDurationSeconds;
	mfSearchAngle = mpMover->GetCharBody()->GetYaw();
	mlSearchPhase = -1;
	mbFreePlayerPath = false;
	mpMover->Stop();
	mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward,0);
}

void cGameEnemyState_Dog_Hunt::UpdateSearch(float afTimeStep)
{
	mfLostPlayerRemainingSeconds -= afTimeStep;
	if(mfLostPlayerRemainingSeconds <= 0)
	{
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}

	// Spend the existing lost-player budget listening, turning left,
	// listening again, then turning right. All poses come from the original
	// Idle/Walk clips; there is no new awareness or omniscient target.
	const float fElapsed = mfLostPlayerDurationSeconds - mfLostPlayerRemainingSeconds;
	const int lPhase = (int)(4 * fElapsed / mfLostPlayerDurationSeconds);
	if(lPhase != mlSearchPhase)
	{
		mlSearchPhase = lPhase;
		if(lPhase == 1 || lPhase == 3)
		{
			mpEnemy->UseMoveStateAnimations();
			mpMover->TurnToAngle(mfSearchAngle + cMath::ToRad(lPhase == 1 ? 35.0f : -35.0f));
		}
		else
		{
			mpMover->TurnToAngle(mpMover->GetCharBody()->GetYaw());
			mpEnemy->PlayAnim("Idle",true,0.2f);
		}
	}
}

void cGameEnemyState_Dog_Hunt::OnUpdate(float afTimeStep)
{
	if(!mpEnemyDog->UseImprovedAI())
	{
		UpdateOriginal(afTimeStep);
		return;
	}
	if(mpPlayer->GetHealth() <=0)
	{
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}
	if(mbLostPlayer)
	{
		UpdateSearch(afTimeStep);
		return;
	}

	if(mpMover->GetStuckCounter() > 1.1f)
	{
		mpMover->ResetStuckCounter();
		if(mpEnemy->CheckForDoor())
		{
			mbBreakingDoor = true;
			mpEnemy->ChangeState(STATE_BREAKDOOR);
			return;
		}
		if(++mlStuckAtMaxCount >= 6)
		{
			mpMover->Stop();
			mpEnemy->ChangeState(STATE_IDLE);
			return;
		}
		// Retry with current heavy obstacles considered by A*, rather than
		// repeatedly pushing along the same blocked route.
		mbCheckDynamicPath = true;
		mfPathUpdateDelaySeconds = 0;
	}

	const cVector3f vTarget = mpEnemyDog->GetPursuitPosition();
	if(mfPathUpdateDelaySeconds <= 0)
	{
		mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds;
		mbFoundNoPath = false;
		mbFreePlayerPath = mpMover->FreeDirectPathToPos(vTarget);
		if(mbFreePlayerPath)
		{
			mpMover->Stop();
			mpMover->SetMaxDoorToughness(-1);
			mbCheckDynamicPath = false;
		}
		else
		{
			if(mpEnemy->GetDoorBreakCount() > 6.0f) mpMover->SetMaxDoorToughness(0);
			if(!mpMover->MoveToPos(vTarget,mbCheckDynamicPath))
			{
				mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds * 5.0f;
				mpMover->Stop();
				mbFoundNoPath = true;
			}
		}
	}
	else mfPathUpdateDelaySeconds -= afTimeStep;

	if(mbFreePlayerPath)
	{
		cVector3f vDistance = vTarget - mpMover->GetCharBody()->GetFeetPosition();
		vDistance.y = 0;
		if(!mpEnemy->CanSeePlayer() && vDistance.Length() < 0.6f)
		{
			BeginSearch();
			return;
		}
		mpMover->MoveDirectToPos(vTarget,afTimeStep);

		// Keep the original attack selection/range, but do not use a hidden
		// player's current position to aim or decide when to lunge.
		if(mpEnemy->CanSeePlayer() &&
			mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody()) < mpEnemyDog->mfAttackDistance &&
			mpInit->mpAttackHandler->HasFreePathToPlayer(mpMover->GetCharBody()->GetPosition()))
		{
			float fHeight = mpMover->GetCharBody()->GetPosition().y -
				mpPlayer->GetCharacterBody()->GetPosition().y;
			if(fHeight < 0)
			{
				fHeight += mpMover->GetCharBody()->GetSize().y/2.0f;
				if(fHeight > -mpEnemyDog->mvAttackDamageSize.y || cMath::RandRectf(0,1)<0.2f)
					mpEnemy->ChangeState(STATE_ATTACK);
				else mpEnemy->ChangeState(STATE_FLEE);
			}
			else mpEnemy->ChangeState(STATE_ATTACK);
		}
		return;
	}

	if(!mpEnemy->CanSeePlayer() && (mbFoundNoPath || !mpMover->IsMoving()))
		BeginSearch();
	else if(mbFoundNoPath)
	{
		// A visible target behind a blocked route is still known. Hold and
		// retry on the existing cadence instead of charging the obstacle or
		// treating the target as lost.
		mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward,0);
		mpMover->TurnToPos(vTarget);
	}
}

void cGameEnemyState_Dog_Hunt::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	if(mbLostPlayer && afChance >= mpEnemyDog->GetHuntMinSeeChance())
	{
		mbLostPlayer = false;
		mfPathUpdateDelaySeconds =0;
		mpEnemy->UseMoveStateAnimations();
	}
}

bool cGameEnemyState_Dog_Hunt::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	if(!mpEnemyDog->UseImprovedAI())
	{
		if(mbLostPlayer && afVolume >= mpEnemyDog->GetHuntMinHearVolume())
		{
			cAINode *pNode = mpMover->GetAINodeAtPosInRange(avPosition,0,5,true,0.1f);
			if(pNode)
			{
				mbLostPlayer = false;
				mfPathUpdateDelaySeconds = 0;
				mpEnemy->SetLastPlayerPos(pNode->GetPosition());
				return true;
			}
		}
		return false;
	}
	if(!mpEnemy->CanSeePlayer() && afVolume >= mpEnemyDog->GetHuntMinHearVolume())
	{
		// Remember the actual sound source, not a randomly selected nearby
		// navigation node. Only a fresh observation restarts pursuit.
		mpEnemy->SetLastPlayerPos(avPosition);
		mbLostPlayer = false;
		mbCheckDynamicPath = false;
		mfPathUpdateDelaySeconds = 0;
		mpEnemy->UseMoveStateAnimations();
		return true;
	}
	return false;
}

void cGameEnemyState_Dog_Hunt::OnDraw()
{
	float fWantedSpeed = mpMover->GetCharBody()->GetMoveSpeed(eCharDir_Forward);
	float fRealSpeed = cMath::Vector3Dist(	mpMover->GetCharBody()->GetPosition(), 
											mpMover->GetCharBody()->GetLastPosition());
	fRealSpeed = fRealSpeed / (1.0f/60.0f);

	float fDist = mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody());

	mpInit->mpDefaultFont->Draw(cVector3f(0,110,100),14,cColor(1,1,1,1),eFontAlign_Left,
		_W("LostPlayerCount: %f FreePath: %d NoPath: %d MaxStuck: %d Dist: %f / %f"),
															mfLostPlayerRemainingSeconds,mbFreePlayerPath,
															mbFoundNoPath,
															mlStuckAtMaxCount,
															fDist,
															mpEnemyDog->mfAttackDistance);
}

// ATTACK STATE

void cGameEnemyState_Dog_Attack::OnEnterState(iGameEnemyState *apPrevState)
{
	mvTargetPosition = mpEnemyDog->GetPursuitPosition();
	//Setup body
	mpEnemy->SetupBody();
	if(mpEnemyDog->mfAttackSpeed>0)
	{
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyDog->mfAttackSpeed);
		mpMover->SetMaxTurnSpeed(10000.0f);

	}
	
	//Animation
	float fHeight = mvTargetPosition.y + mpPlayer->GetCharacterBody()->GetSize().y / 2.0f -
					mpMover->GetCharBody()->GetPosition().y;
	
	//Player is above
	if(fHeight > 0.1f)
		mpEnemy->PlayAnim("Attack",false,0.2f);
	else
		mpEnemy->PlayAnim("AttackLow",false,0.2f);

	//Other
	mpEnemyDog->PlaySound(mpEnemyDog->msAttackStartSound);

	mfDamageTimer = mpEnemyDog->mfAttackDamageTime;
	mfJumpTimer = mpEnemyDog->mfAttackJumpTime;
	mbAttacked = false;

	if(mpEnemy->GetOnAttackCallback() != "")
	{
		tString sCommand = mpEnemy->GetOnAttackCallback() + "(\""+mpEnemy->GetName()+"\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

void cGameEnemyState_Dog_Attack::OnLeaveState(iGameEnemyState *apNextState)
{
	mpEnemyDog->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();

	
}

void cGameEnemyState_Dog_Attack::OnUpdate(float afTimeStep)
{
	if(!mpEnemyDog->UseImprovedAI() || mpEnemy->CanSeePlayer())
		mvTargetPosition = mpEnemyDog->GetPursuitPosition();
	//Move forward
	if(mpEnemyDog->mfAttackSpeed>0)
	{
		if(mfJumpTimer <=0)
		{
			mpMover->MoveDirectToPos(mvTargetPosition,afTimeStep);
		}
		else
		{
			mpMover->TurnToPos(mvTargetPosition);
			mfJumpTimer-= afTimeStep;
		}
	}

	if(mbAttacked) return;

	//Get the 2D distance to the player
	cVector3f vStart = mpPlayer->GetCharacterBody()->GetPosition();
	vStart.y =0;
	cVector3f vEnd = mpMover->GetCharBody()->GetPosition();
	vEnd.y = 0;
	float fDist2D = cMath::Vector3DistSqr(vStart,vEnd);
	float fMinRange = mpEnemyDog->mfAttackDamageRange;

	// Check if dog is in range of player
	if(fDist2D <=  fMinRange*fMinRange && mfDamageTimer <=0 )
	{
		if(mbAttacked == false)
		{
			cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
								mpMover->GetCharBody()->GetForward() * 
								mpEnemyDog->mfAttackDamageRange;

			cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
			cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
			mtxOffset.SetTranslation(vPos);
			
			eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies;
			
			if(mpInit->mpAttackHandler->CreateShapeAttack(mpEnemyDog->GetAttackShape(),
										mtxOffset,
										mpMover->GetCharBody()->GetPosition(),
										cMath::RandRectf(	mpEnemyDog->mfAttackMinDamage,
															mpEnemyDog->mfAttackMaxDamage),
										
										mpEnemyDog->mfAttackMinMass, mpEnemyDog->mfAttackMaxMass,
										mpEnemyDog->mfAttackMinImpulse, mpEnemyDog->mfAttackMaxImpulse,
										
										mpEnemyDog->mlAttackStrength,

										target,nullptr))
			{
				mpEnemyDog->PlaySound(mpEnemyDog->msAttackHitSound);
			}
			mbAttacked = true;
		}
	}
	else
	{
		mfDamageTimer -= afTimeStep;
	}

}

void cGameEnemyState_Dog_Attack::OnAnimationOver(const tString &asName)
{
	if(mpPlayer->GetHealth() <=0)
	{
		float fDist = mpMover->DistanceToChar2D(mpInit->mpPlayer->GetCharacterBody());
		if(fDist < 2.3f)
		{
			mpEnemy->SetTempFloat(60.0f);
			mpEnemy->ChangeState(STATE_EAT);
		}
		else
		{
			mpEnemy->ChangeState(STATE_FLEE);
		}
	}
	else
	{
		mpEnemy->ChangeState(STATE_FLEE);
	}
}

void cGameEnemyState_Dog_Attack::OnPostSceneDraw()
{
	cCamera3D *pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());
	
	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpEnemyDog->mfAttackDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDrawMatrices(mtxOffset, pCamera->GetViewMatrix(), pCamera->GetProjectionMatrix());

	cVector3f vSize = mpEnemyDog->GetAttackShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize*0.5f,vSize*-0.5f,
		cColor(1,0,1,1));
}

// KNOCK DOWN STATE

void cGameEnemyState_Dog_KnockDown::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Idle",true,0.7f);

	//Sound
	mpEnemy->PlaySound(mpEnemyDog->msKnockDownSound);

	//Setup body
	mpEnemy->SetupBody();

	//Go to rag doll
	mpEnemy->GetMeshEntity()->AlignBodiesToSkeleton(false);
	mpEnemy->GetMeshEntity()->SetSkeletonPhysicsActive(true);
	mpEnemy->GetMeshEntity()->Stop();

	mpEnemy->GetMover()->GetCharBody()->SetEntity(nullptr);
	mpEnemy->GetMover()->GetCharBody()->SetActive(false);

	mpEnemy->GetMover()->Stop();

	mfTimer = 2.0f;
	mbCheckAnim = false;

	mlPrevToughness = mpEnemy->GetToughness();
	mpEnemy->SetToughness(12);
}

void cGameEnemyState_Dog_KnockDown::OnLeaveState(iGameEnemyState *apNextState)
{
	mpEnemy->SetToughness(mlPrevToughness);

	mpEnemy->GetMover()->GetCharBody()->SetEntity(mpEnemy->GetMeshEntity());
	mpEnemy->GetMover()->GetCharBody()->SetActive(true);
	mpMover->ResetStuckCounter();
}

void cGameEnemyState_Dog_KnockDown::OnUpdate(float afTimeStep)
{
	if(mbCheckAnim)
	{
		iCharacterBody *pCharBody = mpEnemy->GetMover()->GetCharBody();
		cBoundingVolume *pBV = pCharBody->GetBody()->GetBoundingVolume();
		
		//Add a force to all objects around dog.
		iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

		std::list<iPhysicsBody*> lstBodies;
		cPhysicsBodyIterator bodyIt = pWorld->GetBodyIterator();
		while(bodyIt.HasNext())
		{
			lstBodies.push_back(bodyIt.Next());
		}

		//Force Iteration
		std::list<iPhysicsBody*>::iterator it = lstBodies.begin();
		for(; it != lstBodies.end(); ++it)
		{
			iPhysicsBody *pBody = *it;

			if(pBody->GetCollideCharacter()==false) continue;
			if(pBody->IsActive()==false) continue;
			if(pBody == pCharBody->GetBody()) continue;

			if(cMath::CheckCollisionBV(*pBody->GetBoundingVolume(), *pBV))
			{
				cVector3f vDir = pBody->GetWorldPosition() - pCharBody->GetPosition();
				float fLength = vDir.Length();
				vDir.Normalise();
				
				if(fLength==0) fLength = 0.001f;
				float fForce = (1 / fLength) * 2;
				if(fForce > 300) fForce = 300;
				if(	mpInit->mpPlayer->GetState() == ePlayerState_Grab &&
					mpInit->mpPlayer->GetPushBody() == pBody)
				{
					fForce *= 40;
				}
				
				if(pBody->IsCharacter())
				{
					pBody->GetCharacterBody()->AddForce(vDir * fForce *10 *
											pBody->GetCharacterBody()->GetMass());
				}
				else
				{
					pBody->AddForce(vDir * fForce * pBody->GetMass());
				}
			}
		}
	}
	else
	{
		mfTimer -= afTimeStep;

		if(mfTimer <=0)
		{
			//Get the forward vector from root bone (the right vector)
			cNodeIterator StateIt = mpEnemy->GetMeshEntity()->GetRootNode()->GetChildIterator();
			cBoneState *pBoneState = static_cast<cBoneState*>(StateIt.Next());
			
			cVector3f vRight = cMath::MatrixInverse(pBoneState->GetWorldMatrix()).GetForward();

			//Play animation and fade physics
			float fFadeTime = 1.0f;
			mbCheckAnim = true;
			
			mpEnemy->GetMeshEntity()->Stop();
			if(cMath::Vector3Dot(vRight,cVector3f(0,1,0))<0)
				mpEnemy->PlayAnim("RiseRight",false,fFadeTime);
			else
				mpEnemy->PlayAnim("RiseLeft",false,fFadeTime);

			mpEnemy->GetMeshEntity()->FadeSkeletonPhysicsWeight(fFadeTime);

			//Calculate values
			cVector3f vPosition;
			cVector3f vAngles;
			cMatrixf mtxTransform = mpEnemy->GetMeshEntity()->CalculateTransformFromSkeleton(&vPosition,&vAngles);
			
			//Seems to work better...
			vPosition = mpEnemy->GetMeshEntity()->GetBoundingVolume()->GetWorldCenter();
			cVector3f vGroundPos = vPosition;
			
			bool bFoundGround = mpEnemy->GetGroundFinder()->GetGround(vPosition,cVector3f(0,-1,0),&vGroundPos,nullptr);

			//Set body
			iCharacterBody *pCharBody = mpEnemy->GetMover()->GetCharBody();
			
			float fYAngle = vAngles.y - mpEnemy->GetModelOffsetAngles().y;
			pCharBody->SetFeetPosition(vGroundPos);
			pCharBody->SetYaw(fYAngle);
			pCharBody->SetEntity(mpEnemy->GetMeshEntity());
			pCharBody->SetActive(true);
			
			for(int i=0; i<3; ++i)
			{
				pCharBody->Update(1.0f/60.0f);

				mpEnemy->GetMeshEntity()->UpdateLogic(1.0f/60.0f);
				mpEnemy->GetMeshEntity()->UpdateGraphics(nullptr,1.0f/60.0f,nullptr);
			}
		}
	}
}

void cGameEnemyState_Dog_KnockDown::OnAnimationOver(const tString &asName)
{
	iCharacterBody *pCharBody = mpEnemy->GetMover()->GetCharBody();

	if(	mpEnemy->CheckForTeamMate(mpEnemyDog->mfCallBackupRange*1.5f,false) && 
		mpEnemy->CheckForTeamMate(14,true)==false)
	{
		pCharBody->SetActive(true);
		mpEnemy->ChangeState(STATE_FLEE);
	}
	else
	{
		pCharBody->SetActive(true);
		mpEnemy->ChangeState(STATE_FLEE);
	}
}

// DEAD STATE

void cGameEnemyState_Dog_Dead::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Idle",true,0.7f);

	//Sound
	if(mpEnemy->IsLoading()==false)
		mpEnemy->PlaySound(mpEnemyDog->msDeathSound);

	//Setup body
	mpEnemy->SetupBody();

	//Go to ragdoll
	if(mpEnemy->IsLoading()==false)
		mpEnemy->GetMeshEntity()->AlignBodiesToSkeleton(false);
	
	mpEnemy->GetMeshEntity()->SetSkeletonPhysicsActive(true);
	mpEnemy->GetMeshEntity()->Stop();

	mpEnemy->GetMover()->GetCharBody()->SetEntity(nullptr);
	mpEnemy->GetMover()->GetCharBody()->SetActive(false);

	mpEnemy->GetMover()->Stop();

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

void cGameEnemyState_Dog_Dead::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_Dog_Dead::OnUpdate(float afTimeStep)
{
}

cGameEnemy_Dog::cGameEnemy_Dog(cInit *apInit,const tString& asName,TiXmlElement *apGameElem) : iGameEnemy_DogBase(apInit,asName,apGameElem)
{
	mlRearCheckCount = 0;

	//Set up states
	AddState(new cGameEnemyState_DogBase_Idle(STATE_IDLE,mpInit,this) );
	AddState(new cGameEnemyState_Dog_Hunt(STATE_HUNT,mpInit,this) );
	AddState(new cGameEnemyState_Dog_Attack(STATE_ATTACK,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Flee(STATE_FLEE,mpInit,this) );
	AddState(new cGameEnemyState_Dog_KnockDown(STATE_KNOCKDOWN,mpInit,this) );
	AddState(new cGameEnemyState_Dog_Dead(STATE_DEAD,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Patrol(STATE_PATROL,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Investigate(STATE_INVESTIGATE,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_BreakDoor(STATE_BREAKDOOR,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_CallBackup(STATE_CALLBACKUP,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_MoveTo(STATE_MOVETO,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Eat(STATE_EAT,mpInit,this) );
	AddState(new cGameEnemyState_Dog_Attention(STATE_ATTENTION,mpInit,this) );
}

cGameEnemy_Dog::~cGameEnemy_Dog()
{

}

void cGameEnemy_Dog::OnLoad()
{
	mbAppliedImprovedAI = UseImprovedAI();
	mpMover->SetSmoothNavigation(mbAppliedImprovedAI);

	iGameEnemy_DogBase::OnLoad();
}

cVector3f cGameEnemy_Dog::GetPursuitPosition()
{
	return !UseImprovedAI() || CanSeePlayer() ? mpInit->mpPlayer->GetCharacterBody()->GetFeetPosition()
		: GetLastPlayerPos();
}

bool cGameEnemy_Dog::UseImprovedAI() const { return mpInit->GetImprovedDogAI(); }
float cGameEnemy_Dog::GetHuntMinSeeChance() const
{
	return UseImprovedAI() ? mfHuntMinSeeChance : mfIdleMinSeeChance;
}
float cGameEnemy_Dog::GetHuntMinHearVolume() const
{
	return UseImprovedAI() ? mfHuntMinHearVolume : mfIdleMinHearVolume;
}

void cGameEnemy_Dog::OnUpdate(float afTimeStep)
{
	if(IsActive()==false) return;
	if(mbAppliedImprovedAI != UseImprovedAI())
	{
		mbAppliedImprovedAI = UseImprovedAI();
		mpMover->SetSmoothNavigation(mbAppliedImprovedAI);
		// Reset a running search when its decision rules change. Attacks and
		// recovery finish normally, so switching cannot interrupt a lunge.
		if(mlCurrentState == STATE_HUNT)
		{
			ChangeState(STATE_IDLE);
			ChangeState(STATE_HUNT);
		}
	}
	
	//Regenerate health:
	if(mfHealth > 0)
	{
		if(	mpInit->mDifficulty != eGameDifficulty_Easy &&
			mfHealth <= mfMaxHealth *0.5f)
		{
			mfHealth += afTimeStep * (10.0f / 60.0f); //10 heal units / min
		}
	}

	//Check for ass in wall
	if(mfHealth >0 && mpMover->GetCharBody()->IsActive())
	{
		float fMaxMove = afTimeStep * 2;
		mlRearCheckCount = (mlRearCheckCount + 1) % 2;
		if(mlRearCheckCount == 0)
		{
			iCharacterBody *pCharBody = mpMover->GetCharBody();
			
			cVector3f vPos, vNormal;
			if(!mFindGround.GetGround(pCharBody->GetPosition(), pCharBody->GetForward()*-1,
									&vPos,&vNormal,mfLengthBodyToAss)) return;
			float fDist = cMath::Vector3Dist(pCharBody->GetPosition(), vPos);
			if(fDist < mfLengthBodyToAss)
			{
				float fAdd = mfLengthBodyToAss - fDist;
				if(fAdd > fMaxMove) fAdd = fMaxMove;
				
				cVector3f vAdd = pCharBody->GetForward() * fAdd;
				pCharBody->SetPosition(pCharBody->GetPosition() + vAdd);
			}
		}
	}
}
