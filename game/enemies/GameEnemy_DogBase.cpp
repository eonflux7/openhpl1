// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "enemies/GameEnemy_DogBase.h"


#include "player/Player.h"
#include "systems/AttackHandler.h"
#include "systems/GameMusicHandler.h"
#include "entities/GameSwingDoor.h"
#include "core/MapHandler.h"

// BASE STATE

iGameEnemyState_DogBase::iGameEnemyState_DogBase(int alId, cInit *apInit, iGameEnemy *apEnemy)
							: iGameEnemyState(alId,apInit,apEnemy)
{	
	mpDogBase = static_cast<iGameEnemy_DogBase*>(mpEnemy);
}

void iGameEnemyState_DogBase::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	if(mpPlayer->GetHealth() <=0) return;

	if(afChance >= mpDogBase->mfIdleMinSeeChance)
	{

		float fDist = cMath::Vector3Dist(mpMover->GetCharBody()->GetFeetPosition(),
										mpPlayer->GetCharacterBody()->GetFeetPosition());
		if(fDist >= mpDogBase->mfAttentionMinDist)
		{
			mpEnemy->ChangeState(STATE_ATTENTION);
		}
		else
		{
			mpEnemy->ChangeState(STATE_HUNT);
			mpDogBase->PlaySound(mpDogBase->msIdleFoundPlayerSound);	
		}

	}
}

bool iGameEnemyState_DogBase::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	float afDistance = (mpMover->GetCharBody()->GetPosition() - avPosition).Length();

	if(afVolume >= mpDogBase->mfIdleMinHearVolume && afDistance > 0.4f)
	{
		mpEnemy->SetTempPosition(avPosition);
		mpEnemy->ChangeState(STATE_INVESTIGATE);
		return true;
	}

	return false;
}

void iGameEnemyState_DogBase::OnTakeHit(float afDamage)
{
	if(afDamage >= mpDogBase->mfMinKnockDamage)
	{
		if(mpInit->mbWeaponAttacking)
		{
			float fChance = afDamage / mpDogBase->mfCertainKnockDamage;//(mpDogBase->mfCertainKnockDamage*4);
			if(fChance > cMath::RandRectf(0,1))
			{
				mpEnemy->ChangeState(STATE_KNOCKDOWN);
			}
		}
		else
		{
			if(afDamage >= mpDogBase->mfCertainKnockDamage)
			{
				mpEnemy->ChangeState(STATE_KNOCKDOWN);
			}
			else
			{
				float fChance = afDamage / mpDogBase->mfCertainKnockDamage;
				if(fChance > cMath::RandRectf(0,1))
				{
					mpEnemy->ChangeState(STATE_KNOCKDOWN);
				}
			}
		}
	}
}

void iGameEnemyState_DogBase::OnFlashlight(const cVector3f &avPosition)
{
	
	mpEnemy->SetTempPosition(avPosition);
	mpEnemy->ChangeState(STATE_INVESTIGATE);
}

void iGameEnemyState_DogBase::OnDeath(float afDamage)
{
	mpEnemy->ChangeState(STATE_DEAD);
}

// IDLE STATE

void cGameEnemyState_DogBase_Idle::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpDogBase->mfIdleFOV);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

void cGameEnemyState_DogBase_Idle::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_DogBase_Idle::OnUpdate(float afTimeStep)
{
	if(mpEnemy->GetPatrolNodeNum()>0)
	{
		mpEnemy->ChangeState(STATE_PATROL);
	}
}

// PATROL STATE

void cGameEnemyState_DogBase_Patrol::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpDogBase->mfIdleFOV);

	//Setup patrol
	cEnemyPatrolNode *pPatrolNode = mpEnemy->CurrentPatrolNode();
	cAINode *pNode = mpMover->GetNodeContainer()->GetNodeFromName(pPatrolNode->msNodeName);

	if(mpEnemy->GetDoorBreakCount()> 3.0f)
	{
		mpEnemy->SetDoorBreakCount(0);
		mpMover->SetMaxDoorToughness(0);
	}

	mbWaiting = false;
	mbAnimation = false;
	mlStuckAtMaxCount =0;


	mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
										mpDogBase->mfIdleSoundMaxInteraval);

	mpMover->SetMaxDoorToughness(-1);

	if(mpMover->MoveToPos(pNode->GetPosition())==false)
	{
		
		mpEnemy->IncCurrentPatrolNode();
		mbWaiting = true;
		mpEnemy->SetWaitTime(1.0f);
	}
	else
	{
	}
	
	
	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

void cGameEnemyState_DogBase_Patrol::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_DogBase_Patrol::OnUpdate(float afTimeStep)
{	
	//Waiting for timer or animation to end
	if(mbWaiting)
	{
		//Play idle sound
		if(mfIdleSoundTime<=0)
		{
			mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
				mpDogBase->mfIdleSoundMaxInteraval);

			mpEnemy->PlaySound(mpDogBase->msIdleSound);
		}
		else
		{
			mfIdleSoundTime -= afTimeStep;
		}

		// Timer is up
		if(mpEnemy->GetWaitTimeCount() >= mpEnemy->GetWaitTime())
		{
			if(mbAnimation==false)
			{
				mpEnemy->SetWaitTimeCount(0);
				cEnemyPatrolNode *pPatrolNode = mpEnemy->CurrentPatrolNode();
				cAINode *pNode = mpMover->GetNodeContainer()->GetNodeFromName(pPatrolNode->msNodeName);
				
				mpEnemy->UseMoveStateAnimations();
				mbWaiting = false;
				
				if(mpMover->MoveToPos(pNode->GetPosition())==false)
				{
					mpEnemy->IncCurrentPatrolNode();
					mbWaiting = true;
					mpEnemy->SetWaitTime(1.0f);
				}
				else
				{
				}
				
			}
			else
			{
				mpEnemy->GetCurrentAnimation()->SetLoop(false);
			}
		}
		else
		{
			mpEnemy->AddWaitTimeCount(afTimeStep);
		}
	}
	//Check if path is over
	else
	{
		//Play idle sound
		if(mfIdleSoundTime<=0)
		{
			mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
												mpDogBase->mfIdleSoundMaxInteraval);

			mpEnemy->PlaySound(mpDogBase->msIdleSound);
		}
		else
		{
			mfIdleSoundTime -= afTimeStep;
		}

		//Stuck counter
		if(mpMover->GetStuckCounter() > 1.7f)
		{
			if(mpEnemy->CheckForDoor())
			{
				mpMover->ResetStuckCounter();
				mpEnemy->ChangeState(STATE_BREAKDOOR);
				return;
			}
			else
			{
				mlStuckAtMaxCount++;
				if(mlStuckAtMaxCount >= 6)
				{
					mpEnemy->ChangeState(STATE_IDLE);
					mpEnemy->SetWaitTime(1.0f);
					mpEnemy->IncCurrentPatrolNode();
					return;
				}
			}
			mpMover->ResetStuckCounter();
		}

		//Got to ned of path
		if(mpMover->IsMoving()==false)
		{
			cEnemyPatrolNode *pPatrolNode = mpEnemy->CurrentPatrolNode();

			mpEnemy->SetWaitTime(pPatrolNode->mfWaitTime);
            
			if(pPatrolNode->msAnimation != "")
			{
				mpEnemy->PlayAnim(pPatrolNode->msAnimation,true,0.2f);
				mbAnimation = true;
			}

			mbWaiting = true;

			//Last, as in Black Plague: the callback at the end of the patrol
			//may clear the nodes.
			mpEnemy->IncCurrentPatrolNode();
		}
	}
}

void cGameEnemyState_DogBase_Patrol::OnAnimationOver(const tString &asName)
{
	mbAnimation = false;
}

// ATTENTION STATE

void cGameEnemyState_DogBase_Attention::OnEnterState(iGameEnemyState *apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpDogBase->mfIdleFOV);

	mpMover->Stop();
	mpMover->TurnToPos(mpDogBase->GetPursuitPosition());

	mpEnemy->PlaySound(mpDogBase->msAttentionSound);	
	mfTime = mpDogBase->mfAttentionTime;
	
	if(mpInit->mDifficulty == eGameDifficulty_Easy) mfTime *=1.7f;
}

void cGameEnemyState_DogBase_Attention::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_DogBase_Attention::OnUpdate(float afTimeStep)
{
	mpMover->TurnToPos(mpDogBase->GetPursuitPosition());
	mfTime -= afTimeStep;

	if(mfTime <=0)
	{
		if(mpEnemy->CanSeePlayer())
		{
			mpEnemy->ChangeState(STATE_HUNT);
			mpEnemy->PlaySound(mpDogBase->msIdleFoundPlayerSound);	
		}
		else
		{
			if(mlPreviousState == STATE_ATTENTION)
				mpEnemy->ChangeState(STATE_IDLE);
			else
				mpEnemy->ChangeState(mlPreviousState);
		}
	}

}

void cGameEnemyState_DogBase_Attention::OnSeePlayer(const cVector3f &avPosition, float afChance)
{

}
bool cGameEnemyState_DogBase_Attention::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	return false;
}
void cGameEnemyState_DogBase_Attention::OnFlashlight(const cVector3f &avPosition)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

void cGameEnemyState_DogBase_Attention::OnDraw()
{

}

// EAT STATE

void cGameEnemyState_DogBase_Eat::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Eating",true,0.2f);

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpDogBase->mfEatFOV);

	mfTime = mpEnemy->GetTempFloat();

	mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward,0);
	mpMover->Stop();
}

void cGameEnemyState_DogBase_Eat::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_DogBase_Eat::OnUpdate(float afTimeStep)
{	
	mfTime -= afTimeStep;
	if(mfTime <=0)
	{
		mpEnemy->ChangeState(mlPreviousState);//STATE_IDLE);
	}
}

bool cGameEnemyState_DogBase_Eat::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	if(	afVolume >= mpDogBase->mfEatMinHearVolume)
	{
		mpEnemy->SetTempPosition(avPosition);
		mpEnemy->ChangeState(STATE_INVESTIGATE);
		return true;
	}

	return false;
}

void cGameEnemyState_DogBase_Eat::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	if(mpPlayer->GetHealth() <=0) return;

	if(afChance >= mpDogBase->mfEatMinSeeChance)
	{
		mpEnemy->ChangeState(STATE_HUNT);
		mpDogBase->PlaySound(mpDogBase->msIdleFoundPlayerSound);
	}
}

// INVESTIGATE STATE

void cGameEnemyState_DogBase_Investigate::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpDogBase->mfIdleFOV);

	//Play sound
	if(mpDogBase->mbBlackPlagueInvestigate==false)
		mpEnemy->PlaySound(mpDogBase->msInvestigateSound);

	cAINode *pNode = mpMover->GetAINodeAtPosInRange(mpEnemy->GetTempPosition(),0.0f,5.0f,true, 0.1f);

	if(mpEnemy->GetDoorBreakCount()> 6.0f)
	{
		mpEnemy->SetDoorBreakCount(0);
		mpMover->SetMaxDoorToughness(0);
	}
	
	if(pNode==nullptr || mpMover->MoveToPos(pNode->GetPosition())==false)
	{
		mpEnemy->ChangeState(STATE_IDLE);
		if(mpDogBase->mbBlackPlagueInvestigate) return;
	}

	mpMover->SetMaxDoorToughness(-1);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
										mpDogBase->mfIdleSoundMaxInteraval);

	if(apPrevState->GetId() != STATE_INVESTIGATE)
	{
		mfHighestVolume = 0.0f;

		if(mpDogBase->mbBlackPlagueInvestigate)
			mpEnemy->PlaySound(mpDogBase->msInvestigateSound);
	}

	mfNoiseRetargetDelaySeconds = 5.0f;
}

void cGameEnemyState_DogBase_Investigate::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_DogBase_Investigate::OnUpdate(float afTimeStep)
{	
	if(mfNoiseRetargetDelaySeconds >0){
		mfNoiseRetargetDelaySeconds-=afTimeStep;
		if(mfNoiseRetargetDelaySeconds<=0)mfNoiseRetargetDelaySeconds=0;
	}

	//Play idle sound
	if(mfIdleSoundTime<=0)
	{
		mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
			mpDogBase->mfIdleSoundMaxInteraval);

		mpEnemy->PlaySound(mpDogBase->msIdleSound);
	}
	else
	{
		mfIdleSoundTime -= afTimeStep;
	}

	//Stuck counter
	if(mpMover->GetStuckCounter() > 1.5f)
	{
		if(mlKnockCount == 1){
			mlKnockCount =0;
			mpEnemy->ChangeState(STATE_IDLE);
			mpEnemy->PlaySound(mpDogBase->msInvestigateOverSound);
			return;
		}
		else
		{
			if(mpEnemy->CheckForDoor())
			{
				mpMover->ResetStuckCounter();
				mlKnockCount++;
				mpEnemy->ChangeState(STATE_BREAKDOOR);
				return;
			}
			mpMover->ResetStuckCounter();
			mlKnockCount++;
		}
	}

	if(mpMover->IsMoving()==false)
	{
		mlKnockCount =0;
		mpEnemy->ChangeState(STATE_IDLE);
		mpEnemy->PlaySound(mpDogBase->msInvestigateOverSound);
	}
}

bool cGameEnemyState_DogBase_Investigate::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	if(	mfNoiseRetargetDelaySeconds<=0 && mfHighestVolume < afVolume &&
		afVolume >= mpDogBase->mfIdleMinHearVolume)
	{
		mfHighestVolume = afVolume;
		mpEnemy->SetTempPosition(avPosition);
		OnEnterState(this);

		return true;
	}

	return false;
}

// MOVE TO STATE

void cGameEnemyState_DogBase_MoveTo::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpDogBase->mfIdleFOV);

	//Play sound
	mpEnemy->PlaySound(mpDogBase->msInvestigateSound);
	
	if(mpMover->MoveToPos(mpEnemy->GetTempPosition())==false)
	{
		mpEnemy->ChangeState(apPrevState->GetId());
		return;
	}
	else
	{
	}
	
	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
										mpDogBase->mfIdleSoundMaxInteraval);

	mlBreakCount = 0;
}

void cGameEnemyState_DogBase_MoveTo::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_DogBase_MoveTo::OnUpdate(float afTimeStep)
{	
	//Play idle sound
	if(mfIdleSoundTime<=0)
	{
		mfIdleSoundTime = cMath::RandRectf(	mpDogBase->mfIdleSoundMinInteraval,
			mpDogBase->mfIdleSoundMaxInteraval);

		mpEnemy->PlaySound(mpDogBase->msIdleSound);
	}
	else
	{
		mfIdleSoundTime -= afTimeStep;
	}

	//Stuck counter
	if(mpMover->GetStuckCounter() > 1.5f)
	{
		if(mlBreakCount ==1)
		{
			mpEnemy->ChangeState(STATE_IDLE);
			return;
		}
		else
		{
			if(mpEnemy->CheckForDoor())
			{
				mpMover->ResetStuckCounter();
				mlBreakCount++;
				mpEnemy->ChangeState(STATE_BREAKDOOR);
				return;
			}
			mpMover->ResetStuckCounter();
			mlBreakCount++;
		}
	}

	if(mpMover->IsMoving()==false)
	{
		mpEnemy->ChangeState(STATE_IDLE);
	}
}

// FLEE STATE

void cGameEnemyState_DogBase_Flee::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();
	mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpDogBase->mfHuntSpeed);
	mpMover->GetCharBody()->SetMaxNegativeMoveSpeed(eCharDir_Forward,-mpDogBase->mfFleeBackSpeed);
	
	float fPosMul = 1;
	if(apPrevState->GetId() == STATE_KNOCKDOWN) fPosMul = 4.0f;
	
	mbBackingFromBreakDoor = false;
	if(apPrevState->GetId() == STATE_BREAKDOOR)
		mbBackingFromBreakDoor = true;

	// The dog has just broken a door
	if(mbBackingFromBreakDoor)
	{
		mfBackAngle = mpMover->GetCharBody()->GetYaw();
		mbBackwards = true;

		mfTimer = mpDogBase->mfFleeBackTime;
		mfCheckBehindTime = 1.0f / 10.0f;
	}
	// Normal flee
	else
	{
		if(	(	apPrevState->GetId() == STATE_KNOCKDOWN || apPrevState->GetId() == STATE_HUNT ||
				cMath::RandRectf(0,1) < 0)//mpDogBase->mfFleePositionChance) 
			)
		{
			cAINode *pNode = mpMover->GetAINodeInRange(	mpDogBase->mfFleePositionMinDistance * fPosMul,
														mpDogBase->mfFleePositionMaxDistance * fPosMul);
			if(pNode) 
			{
				mpMover->MoveToPos(pNode->GetPosition());
			}
			else
			{
				mpEnemy->ChangeState(STATE_HUNT);
				return;
			}
			mfTimer = mpDogBase->mfFleePositionMaxTime;
			mbBackwards = false;
		}
		else if(cMath::RandRectf(0,1) < mpDogBase->mfFleeBackChance)
		{
			mfBackAngle = mpMover->GetCharBody()->GetYaw();
			mbBackwards = true;

			mfTimer = mpDogBase->mfFleeBackTime;
			mfCheckBehindTime = 1.0f / 10.0f;
		}
		else
		{
			mpEnemy->ChangeState(STATE_HUNT);
		}
	}
}

void cGameEnemyState_DogBase_Flee::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->ResetStuckCounter();
}

void cGameEnemyState_DogBase_Flee::OnUpdate(float afTimeStep)
{
	mfTimer -= afTimeStep;

	if(mbBackwards)
	{
		if(mfCheckBehindTime <= 0)
		{
			mfCheckBehindTime = 1.0f / 20.0f;

			bool bHit = mpEnemy->GetGroundFinder()->GetGround(mpMover->GetCharBody()->GetPosition(),
												mpMover->GetCharBody()->GetForward() *-1,
												nullptr, nullptr, 1.9f);
			if(bHit)
			{
                if(mbBackingFromBreakDoor)
				{
					if(mlPreviousState == STATE_FLEE)
						mpEnemy->ChangeState(STATE_HUNT);
					else
						mpEnemy->ChangeState(mlPreviousState);
				}
				else
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
				return;
			}
		}
		else
		{
			mfCheckBehindTime -= afTimeStep;
		}

		if(mfTimer <=0)
		{
			if(mbBackingFromBreakDoor)
				mpEnemy->ChangeState(mlPreviousState);
			else
				mpEnemy->ChangeState(STATE_HUNT);
			return;
		}
		mpMover->GetCharBody()->Move(eCharDir_Forward,-1.0f,afTimeStep);
		mpMover->TurnToPos(mpDogBase->GetPursuitPosition());
	}
	else
	{
		//Move forward
		if(mpMover->IsMoving()==false || mpMover->GetStuckCounter() > 0.3f	|| mfTimer <=0)
		{
			//Check if there is any enemies nearaby and if anyone is allready fighting
			if(	mpEnemy->CheckForTeamMate(mpDogBase->mfCallBackupRange*1.5f,false) && 
				mpEnemy->CheckForTeamMate(8,true)==false)
			{
				float fPlayerDist = cMath::Vector3Dist(mpMover->GetCharBody()->GetFeetPosition(),
					mpDogBase->GetPursuitPosition());
				if(fPlayerDist > 8)
				{
					mpEnemy->ChangeState(STATE_CALLBACKUP);
				}
				else
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
			}
			else
			{
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
			}
		}
	}
}

// CALL BACKUP STATE

void cGameEnemyState_DogBase_CallBackup::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim(mpDogBase->msCallBackupAnimation,false,0.2f);
	
	//Sound
	mpDogBase->PlaySound(mpDogBase->msCallBackupSound);
	
	//Iterate enemies and show them the player
	cVector3f vPostion = mpMover->GetCharBody()->GetFeetPosition();

	for(iGameEnemy *pEnemy : mpInit->mpMapHandler->GetGameEnemyIterator())
	{
		if(pEnemy->GetEnemyType() != mpEnemy->GetEnemyType()) continue;
		if(pEnemy == mpEnemy || pEnemy->IsActive()==false || pEnemy->GetHealth()<=0) continue;

		float fDist = cMath::Vector3Dist(pEnemy->GetMover()->GetCharBody()->GetPosition(),
											vPostion);

		if(fDist <= mpDogBase->mfCallBackupRange)
		{
			pEnemy->ShowPlayer(mpDogBase->GetLastPlayerPos());
			break; // Call only for one dog backup!
		}
	}

	mpMover->Stop();
	mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward,0);
}

void cGameEnemyState_DogBase_CallBackup::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->ResetStuckCounter();
}

void cGameEnemyState_DogBase_CallBackup::OnUpdate(float afTimeStep)
{
}

void cGameEnemyState_DogBase_CallBackup::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

// BREAK DOOR STATE

void cGameEnemyState_DogBase_BreakDoor::OnEnterState(iGameEnemyState *apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();
	if(mpDogBase->mfBreakDoorSpeed>0)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpDogBase->mfBreakDoorSpeed);


	//Animation
	mpEnemy->PlayAnim(mpDogBase->msBreakDoorAnimation,false,0.2f);

	mpDogBase->PlaySound(mpDogBase->msBreakDoorStartSound);

	mfDamageTimer = mpDogBase->mfBreakDoorDamageTime;
	mfStopMoveTimer = mpDogBase->mfBreakDoorDamageTime + 1.1f;
	mbAttacked = false;
	mbStopped = false;

	mlCount =0;
}

void cGameEnemyState_DogBase_BreakDoor::OnLeaveState(iGameEnemyState *apNextState)
{
	mpDogBase->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();
}

void cGameEnemyState_DogBase_BreakDoor::OnUpdate(float afTimeStep)
{
	//Move forward
	if(mpDogBase->mfBreakDoorSpeed>0 && mlCount==0 && mbAttacked == false)
	{
		//Skip this for now
	}

	if(mfDamageTimer <=0 )
	{
		if(!mbStopped)
		{
			mpMover->Stop();
			mbStopped = true;
		}
	}
	else
	{
		mfDamageTimer -= afTimeStep;
	}

	if(mfDamageTimer <=0 )
	{
		if(mbAttacked == false)
		{
			cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
								mpMover->GetCharBody()->GetForward() * 
								mpDogBase->mfBreakDoorDamageRange;

			cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
			cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
			mtxOffset.SetTranslation(vPos);

			eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies;

			if(mpInit->mpAttackHandler->CreateShapeAttack(mpDogBase->GetBreakDoorShape(),
											mtxOffset,
											mpMover->GetCharBody()->GetPosition(),
											cMath::RandRectf(	mpDogBase->mfBreakDoorMinDamage,
											mpDogBase->mfBreakDoorMaxDamage),

											mpDogBase->mfBreakDoorMinMass, mpDogBase->mfBreakDoorMaxMass,
											mpDogBase->mfBreakDoorMinImpulse, mpDogBase->mfBreakDoorMaxImpulse,

											mpDogBase->mlBreakDoorStrength,

											target,nullptr))
			{
				mpDogBase->PlaySound(mpDogBase->msBreakDoorHitSound);

				cGameSwingDoor *pDoor = mpInit->mpAttackHandler->GetLastSwingDoor();
				if(pDoor)
				{
					//The door is unbreakable
					if(pDoor->GetToughness() - mpDogBase->mlBreakDoorStrength >= 4)
					{
						cMatrixf mtxDoor = pDoor->GetBody(0)->GetWorldMatrix();
						cMatrixf mtxInvDoor = cMath::MatrixInverse(mtxDoor);

						cVector3f vDoorForward = mtxInvDoor.GetForward();
						cVector3f vEnemyForward = mpMover->GetCharBody()->GetForward();

						if(cMath::Vector3Dot(vDoorForward,vEnemyForward) < 0)
						{
							mpEnemy->AddDoorBreakCount(2);//8);
						}
						else
						{
							mpEnemy->AddDoorBreakCount(2);
							//Just continue hitting it since it is just barricaded.
						}
					}
					//The door is breakable
					else
					{
						mpEnemy->AddDoorBreakCount(2);
					}

				}

			}
			mbAttacked = true;
		}
	}
}

void cGameEnemyState_DogBase_BreakDoor::OnAnimationOver(const tString &asName)
{
	if(mlCount ==0)
	{
		if(mpDogBase->mbBreakDoorRiseAtEnd)
		{
			mpEnemy->PlayAnim("RiseRight",false,0.2f);
			mlCount++;
		}
		else
		{
			mpEnemy->ChangeState(STATE_FLEE);
			mpEnemy->GetState(STATE_FLEE)->SetPreviousState(mlPreviousState);
		}
	}
	else
	{
		mpEnemy->ChangeState(STATE_FLEE);
		mpEnemy->GetState(STATE_FLEE)->SetPreviousState(mlPreviousState);
	}
}

void cGameEnemyState_DogBase_BreakDoor::OnPostSceneDraw()
{
	cCamera3D *pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());

	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpDogBase->mfBreakDoorDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDrawMatrices(mtxOffset, pCamera->GetViewMatrix(), pCamera->GetProjectionMatrix());

	cVector3f vSize = mpDogBase->GetBreakDoorShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize*0.5f,vSize*-0.5f,
		cColor(1,0,1,1));
}

iGameEnemy_DogBase::iGameEnemy_DogBase(cInit *apInit,const tString& asName,TiXmlElement *apGameElem) : iGameEnemy(apInit,asName,apGameElem)
{
	LoadBaseProperties(apGameElem);
	
	//Special properties 

	mfLengthBodyToAss = cString::ToFloat(apGameElem->Attribute("LengthBodyToAss"),1.5f);
	
	mfMinKnockDamage = cString::ToFloat(apGameElem->Attribute("MinKnockDamage"),0);
	mfCertainKnockDamage = cString::ToFloat(apGameElem->Attribute("CertainKnockDamage"),0);

	//State properties
	mfIdleFOV = cMath::ToRad(cString::ToFloat(apGameElem->Attribute("IdleFOV"),0));
	msIdleFoundPlayerSound = cString::ToString(apGameElem->Attribute("IdleFoundPlayerSound"),"");
	mfIdleMinSeeChance = cString::ToFloat(apGameElem->Attribute("IdleMinSeeChance"),0);
	mfIdleMinHearVolume = cString::ToFloat(apGameElem->Attribute("IdleMinHearVolume"),0);
	msIdleSound = cString::ToString(apGameElem->Attribute("IdleSound"),"");
	mfIdleSoundMinInteraval = cString::ToFloat(apGameElem->Attribute("IdleSoundMinInteraval"),0);
	mfIdleSoundMaxInteraval = cString::ToFloat(apGameElem->Attribute("IdleSoundMaxInteraval"),0);
	mfIdleCallBackupChance = cString::ToFloat(apGameElem->Attribute("IdleCallBackupChance"),0);
	mvPreloadSounds.push_back(msIdleSound);

	msInvestigateSound = cString::ToString(apGameElem->Attribute("InvestigateSound"),"");
	msInvestigateOverSound = cString::ToString(apGameElem->Attribute("InvestigateOverSound"),"");
	mvPreloadSounds.push_back(msInvestigateSound);
	mvPreloadSounds.push_back(msInvestigateOverSound);

	msAttentionSound = cString::ToString(apGameElem->Attribute("AttentionSound"),"");
	mfAttentionTime = cString::ToFloat(apGameElem->Attribute("AttentionTime"),0);
	mfAttentionMinDist = cString::ToFloat(apGameElem->Attribute("AttentionMinDist"),0);
	mvPreloadSounds.push_back(msAttentionSound);

	mfHuntFOV = cMath::ToRad(cString::ToFloat(apGameElem->Attribute("HuntFOV"),0));
	mfHuntSpeed = cString::ToFloat(apGameElem->Attribute("HuntSpeed"),0);
	mfHuntForLostPlayerTime = cString::ToFloat(apGameElem->Attribute("HuntForLostPlayerTime"),0);
	mfHuntMinSeeChance = cString::ToFloat(apGameElem->Attribute("HuntMinSeeChance"),mfIdleMinSeeChance);
	mfHuntMinHearVolume = cString::ToFloat(apGameElem->Attribute("HuntMinHearVolume"),mfIdleMinHearVolume);

	mfAttackDistance = cString::ToFloat(apGameElem->Attribute("AttackDistance"),0);
	mfAttackSpeed = cString::ToFloat(apGameElem->Attribute("AttackSpeed"),0);
	mfAttackJumpTime = cString::ToFloat(apGameElem->Attribute("AttackJumpTime"),0);
	mfAttackDamageTime = cString::ToFloat(apGameElem->Attribute("AttackDamageTime"),0);
	mvAttackDamageSize = cString::ToVector3f(apGameElem->Attribute("AttackDamageSize"),0);
	mfAttackDamageRange = cString::ToFloat(apGameElem->Attribute("AttackDamageRange"),0);
	mfAttackMinDamage = cString::ToFloat(apGameElem->Attribute("AttackMinDamage"),0);
	mfAttackMaxDamage = cString::ToFloat(apGameElem->Attribute("AttackMaxDamage"),0);
	msAttackStartSound = cString::ToString(apGameElem->Attribute("AttackStartSound"),"");
	msAttackHitSound = cString::ToString(apGameElem->Attribute("AttackHitSound"),"");
	mfAttackMinMass = cString::ToFloat(apGameElem->Attribute("AttackMinMass"),0);
	mfAttackMaxMass = cString::ToFloat(apGameElem->Attribute("AttackMaxMass"),0);
	mfAttackMinImpulse = cString::ToFloat(apGameElem->Attribute("AttackMinImpulse"),0);
	mfAttackMaxImpulse = cString::ToFloat(apGameElem->Attribute("AttackMaxImpulse"),0);
	mlAttackStrength = cString::ToInt(apGameElem->Attribute("AttackStrength"),0);
	mvPreloadSounds.push_back(msAttackStartSound);
	mvPreloadSounds.push_back(msAttackHitSound);

	msBreakDoorAnimation = cString::ToString(apGameElem->Attribute("BreakDoorAnimation"),"");
	mfBreakDoorSpeed = cString::ToFloat(apGameElem->Attribute("BreakDoorSpeed"),0);
	mfBreakDoorDamageTime = cString::ToFloat(apGameElem->Attribute("BreakDoorDamageTime"),0);
	mvBreakDoorDamageSize = cString::ToVector3f(apGameElem->Attribute("BreakDoorDamageSize"),0);
	mfBreakDoorDamageRange = cString::ToFloat(apGameElem->Attribute("BreakDoorDamageRange"),0);
	mfBreakDoorMinDamage = cString::ToFloat(apGameElem->Attribute("BreakDoorMinDamage"),0);
	mfBreakDoorMaxDamage = cString::ToFloat(apGameElem->Attribute("BreakDoorMaxDamage"),0);
	msBreakDoorStartSound = cString::ToString(apGameElem->Attribute("BreakDoorStartSound"),"");
	msBreakDoorHitSound = cString::ToString(apGameElem->Attribute("BreakDoorHitSound"),"");
	mfBreakDoorMinMass = cString::ToFloat(apGameElem->Attribute("BreakDoorMinMass"),0);
	mfBreakDoorMaxMass = cString::ToFloat(apGameElem->Attribute("BreakDoorMaxMass"),0);
	mfBreakDoorMinImpulse = cString::ToFloat(apGameElem->Attribute("BreakDoorMinImpulse"),0);
	mfBreakDoorMaxImpulse = cString::ToFloat(apGameElem->Attribute("BreakDoorMaxImpulse"),0);
	mlBreakDoorStrength = cString::ToInt(apGameElem->Attribute("BreakDoorStrength"),0);
	mbBreakDoorRiseAtEnd = cString::ToBool(apGameElem->Attribute("BreakDoorRiseAtEnd"),false);
	mvPreloadSounds.push_back(msBreakDoorStartSound);
	mvPreloadSounds.push_back(msBreakDoorHitSound);

	msKnockDownSound = cString::ToString(apGameElem->Attribute("KnockDownSound"),"");
	mvPreloadSounds.push_back(msKnockDownSound);

	msDeathSound = cString::ToString(apGameElem->Attribute("DeathSound"),"");
	mvPreloadSounds.push_back(msDeathSound);

	mfFleePositionChance = cString::ToFloat(apGameElem->Attribute("FleePositionChance"),0);
	mfFleePositionMaxTime = cString::ToFloat(apGameElem->Attribute("FleePositionMaxTime"),0);
	mfFleePositionMinDistance = cString::ToFloat(apGameElem->Attribute("FleePositionMinDistance"),0);
	mfFleePositionMaxDistance = cString::ToFloat(apGameElem->Attribute("FleePositionMaxDistance"),0);
	mfFleeBackChance = cString::ToFloat(apGameElem->Attribute("FleeBackChance"),0);
	mfFleeBackTime = cString::ToFloat(apGameElem->Attribute("FleeBackTime"),0);
	mfFleeBackSpeed = cString::ToFloat(apGameElem->Attribute("FleeBackSpeed"),0);

	msCallBackupAnimation = cString::ToString(apGameElem->Attribute("CallBackupAnimation"),"");
	msCallBackupSound = cString::ToString(apGameElem->Attribute("CallBackupSound"),"");
	mfCallBackupRange = cString::ToFloat(apGameElem->Attribute("CallBackupRange"),0);
	mvPreloadSounds.push_back(msCallBackupSound);

	mfEatFOV = cMath::ToRad(cString::ToFloat(apGameElem->Attribute("EatFOV"),0));
	mfEatMinSeeChance = cString::ToFloat(apGameElem->Attribute("EatMinSeeChance"),0);
	mfEatMinHearVolume = cString::ToFloat(apGameElem->Attribute("EatMinHearVolume"),0);
}

void iGameEnemy_DogBase::OnLoad()
{
	//Create attack shape
	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	mpAttackShape = pPhysicsWorld->CreateBoxShape(mvAttackDamageSize,nullptr);
	mpBreakDoorShape = pPhysicsWorld->CreateBoxShape(mvBreakDoorDamageSize,nullptr);
	
	//Set up shape
	ChangeState(STATE_IDLE);
}

cVector3f iGameEnemy_DogBase::GetPursuitPosition()
{
	return mpInit->mpPlayer->GetCharacterBody()->GetFeetPosition();
}

void iGameEnemy_DogBase::ShowPlayer(const cVector3f& avPlayerFeetPos)
{
	if(	mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL || 
		mlCurrentState == STATE_INVESTIGATE)
	{
		mvLastPlayerPos = avPlayerFeetPos;
		ChangeState(STATE_HUNT);
	}
}

bool iGameEnemy_DogBase::MoveToPos(const cVector3f& avFeetPos)
{
	if(	mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL)
	{
		SetTempPosition(avFeetPos);
		ChangeState(STATE_MOVETO);
		return true;
	}
	else
	{
		return false;
	}
}

bool iGameEnemy_DogBase::IsFighting()
{
	if( mfHealth <= 0 || IsActive()==false) return false;
	if(	mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL || 
		mlCurrentState == STATE_INVESTIGATE) return false;

	return true;
}
