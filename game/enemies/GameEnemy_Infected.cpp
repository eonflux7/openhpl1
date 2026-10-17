/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * The Infected of Penumbra: Black Plague. Frictional Games never released
 * Black Plague's game code; this is the Dog of Overture changed after what
 * the game's binary does.
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
#include "enemies/GameEnemy_Infected.h"


#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/AttackHandler.h"
#include "systems/EffectHandler.h"
#include "systems/GameMusicHandler.h"
#include "entities/GameSwingDoor.h"
#include "core/MapHandler.h"

//////////////////////////////////////////////////////////////////////////
// BASE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

iGameEnemyState_Infected_Base::iGameEnemyState_Infected_Base(int alId, cInit *apInit, iGameEnemy *apEnemy)
							: iGameEnemyState(alId,apInit,apEnemy)
{	
	mpEnemyInfected = static_cast<cGameEnemy_Infected*>(mpEnemy);
}

//-----------------------------------------------------------------------

void iGameEnemyState_Infected_Base::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	//return;
	if(mpPlayer->GetHealth() <=0) return;

	if(afChance >= mpEnemyInfected->mfIdleMinSeeChance)
	{
		/*if( (mlId == STATE_IDLE || mlId == STATE_INVESTIGATE || mlId == STATE_PATROL) &&
			cMath::RandRectf(0,1) < mpEnemyInfected->mfIdleCallBackupChance && 
			mpEnemy->CheckForTeamMate(8,true)==false)
		{
			mpEnemy->ChangeState(STATE_CALLBACKUP);
		}
		else
		{
			//mpEnemy->ChangeState(STATE_HUNT);
			//mpEnemyInfected->PlaySound(mpEnemyInfected->msIdleFoundPlayerSound);	
		}*/

		float fDist = cMath::Vector3Dist(mpMover->GetCharBody()->GetFeetPosition(),
										mpPlayer->GetCharacterBody()->GetFeetPosition());
		if(fDist >= mpEnemyInfected->mfAttentionMinDist)
		{
			mpEnemy->ChangeState(STATE_ATTENTION);
		}
		else
		{
			mpEnemy->ChangeState(STATE_HUNT);
			mpEnemyInfected->PlaySound(mpEnemyInfected->msIdleFoundPlayerSound);	
		}

	}
}

bool iGameEnemyState_Infected_Base::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	//return false;
	float afDistance = (mpMover->GetCharBody()->GetPosition() - avPosition).Length();

	if(afVolume >= mpEnemyInfected->mfIdleMinHearVolume && afDistance > 0.4f)
	{
		mpEnemy->SetTempPosition(avPosition);
		mpEnemy->ChangeState(STATE_INVESTIGATE);
		return true;
	}

	return false;
}

void iGameEnemyState_Infected_Base::OnTakeHit(float afDamage)
{
	if(afDamage >= mpEnemyInfected->mfMinKnockDamage)
	{
		if(mpInit->mbWeaponAttacking)
		{
			float fChance = afDamage / mpEnemyInfected->mfCertainKnockDamage;//(mpEnemyInfected->mfCertainKnockDamage*4);
			if(fChance > cMath::RandRectf(0,1))
			{
				mpEnemy->ChangeState(STATE_KNOCKDOWN);
			}
		}
		else
		{
			if(afDamage >= mpEnemyInfected->mfCertainKnockDamage)
			{
				mpEnemy->ChangeState(STATE_KNOCKDOWN);
			}
			else
			{
				float fChance = afDamage / mpEnemyInfected->mfCertainKnockDamage;
				if(fChance > cMath::RandRectf(0,1))
				{
					mpEnemy->ChangeState(STATE_KNOCKDOWN);
				}
			}
		}
	}
}

void iGameEnemyState_Infected_Base::OnFlashlight(const cVector3f &avPosition)
{
	//mpInit->mpEffectHandler->GetSubTitle()->Add("Flashlight!",0.5f,true);
	//OnSeePlayer(mpPlayer->GetCharacterBody()->GetFeetPosition(),1.0f);
	
	//mpEnemy->SetLastPlayerPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
	//mpEnemy->ChangeState(STATE_HUNT);

	mpEnemy->SetTempPosition(avPosition);
	mpEnemy->ChangeState(STATE_INVESTIGATE);
}

void iGameEnemyState_Infected_Base::OnDeath(float afDamage)
{
	mpEnemy->ChangeState(STATE_DEAD);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// IDLE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Idle::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfIdleFOV);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Idle::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Idle::OnUpdate(float afTimeStep)
{
	if(mpEnemy->GetPatrolNodeNum()>0)
	{
		mpEnemy->ChangeState(STATE_PATROL);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PATROL STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Patrol::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfIdleFOV);

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


	mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
										mpEnemyInfected->mfIdleSoundMaxInteraval);

	mpMover->SetMaxDoorToughness(-1);

	if(mpMover->MoveToPos(pNode->GetPosition())==false)
	{
		//tString sStr = "Could not get to path node "+pPatrolNode->msNodeName;
		//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,true);
		
		mpEnemy->IncCurrentPatrolNode();
		mbWaiting = true;
		mpEnemy->SetWaitTime(1.0f);
	}
	else
	{
		//tString sStr = "Moving to path node "+pPatrolNode->msNodeName;
		//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,true);
	}
	
	
	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Patrol::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Patrol::OnUpdate(float afTimeStep)
{	
	/////////////////////////////////
	//Waiting for timer or animation to end
	if(mbWaiting)
	{
		//////////////////////////////
		//Play idle sound
		if(mfIdleSoundTime<=0)
		{
			mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
				mpEnemyInfected->mfIdleSoundMaxInteraval);

			mpEnemy->PlaySound(mpEnemyInfected->msIdleSound);
		}
		else
		{
			mfIdleSoundTime -= afTimeStep;
		}

		//////////////////////////////
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
					//tString sStr = "Could not get to path node "+pPatrolNode->msNodeName;
					//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,false);
					mpEnemy->IncCurrentPatrolNode();
					mbWaiting = true;
					mpEnemy->SetWaitTime(1.0f);
				}
				else
				{
					//tString sStr = "Moving to path node "+pPatrolNode->msNodeName;
					//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,false);
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
	/////////////////////////////////
	//Check if path is over
	else
	{
		//////////////////////////////
		//Play idle sound
		if(mfIdleSoundTime<=0)
		{
			mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
												mpEnemyInfected->mfIdleSoundMaxInteraval);

			mpEnemy->PlaySound(mpEnemyInfected->msIdleSound);
		}
		else
		{
			mfIdleSoundTime -= afTimeStep;
		}

		//////////////////////////////
		//Stuck counter
		if(mpMover->GetStuckCounter() > 1.7f)
		{
			if(mpEnemy->CheckForDoor())
			{
				mpEnemy->ChangeState(STATE_BREAKDOOR);
			}
			else
			{
				mlStuckAtMaxCount++;
				if(mlStuckAtMaxCount >= 6)
				{
					mpEnemy->ChangeState(STATE_IDLE);
					mpEnemy->SetWaitTime(1.0f);
					mpEnemy->IncCurrentPatrolNode();
				}
			}
			mpMover->ResetStuckCounter();
		}

		//////////////////////////////
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

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Patrol::OnAnimationOver(const tString &asName)
{
	mbAnimation = false;
}

//////////////////////////////////////////////////////////////////////////
// ATTENTION STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attention::OnEnterState(iGameEnemyState *apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfIdleFOV);

	mpMover->Stop();
	mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());

	mpEnemy->PlaySound(mpEnemyInfected->msAttentionSound);	
	mfTime = mpEnemyInfected->mfAttentionTime;
	
	if(mpInit->mDifficulty == eGameDifficulty_Easy) mfTime *=1.7f;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attention::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attention::OnUpdate(float afTimeStep)
{
	mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
	mfTime -= afTimeStep;

	if(mfTime <=0)
	{
		if(mpEnemy->CanSeePlayer())
		{
			mpEnemy->ChangeState(STATE_HUNT);
			mpEnemy->PlaySound(mpEnemyInfected->msIdleFoundPlayerSound);	
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

//-----------------------------------------------------------------------


void cGameEnemyState_Infected_Attention::OnSeePlayer(const cVector3f &avPosition, float afChance)
{

}
bool cGameEnemyState_Infected_Attention::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	return false;
}
void cGameEnemyState_Infected_Attention::OnFlashlight(const cVector3f &avPosition)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attention::OnDraw()
{

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// EAT STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Eat::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Eating",true,0.2f);

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfEatFOV);

	mfTime = mpEnemy->GetTempFloat();

	mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward,0);
	mpMover->Stop();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Eat::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Eat::OnUpdate(float afTimeStep)
{	
	mfTime -= afTimeStep;
	if(mfTime <=0)
	{
		mpEnemy->ChangeState(mlPreviousState);//STATE_IDLE);
	}
}

//-----------------------------------------------------------------------

bool cGameEnemyState_Infected_Eat::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	//return false;
	if(	afVolume >= mpEnemyInfected->mfEatMinHearVolume)
	{
		mpEnemy->SetTempPosition(avPosition);
		mpEnemy->ChangeState(STATE_INVESTIGATE);
		return true;
	}

	return false;
}

void cGameEnemyState_Infected_Eat::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	//return;
	if(mpPlayer->GetHealth() <=0) return;

	if(afChance >= mpEnemyInfected->mfEatMinSeeChance)
	{
		mpEnemy->ChangeState(STATE_HUNT);
		mpEnemyInfected->PlaySound(mpEnemyInfected->msIdleFoundPlayerSound);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// INVESTIGATE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Investigate::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfIdleFOV);

	cAINode *pNode = mpMover->GetAINodeAtPosInRange(mpEnemy->GetTempPosition(),0.0f,5.0f,true, 0.1f);

	if(mpEnemy->GetDoorBreakCount()> 6.0f)
	{
		mpEnemy->SetDoorBreakCount(0);
		mpMover->SetMaxDoorToughness(0);
	}
	
	if(pNode==nullptr || mpMover->MoveToPos(pNode->GetPosition())==false)
	{
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}

	mpMover->SetMaxDoorToughness(-1);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
										mpEnemyInfected->mfIdleSoundMaxInteraval);

	if(apPrevState->GetId() != STATE_INVESTIGATE)
	{
		mfHighestVolume = 0.0f;

		//Play sound
		mpEnemy->PlaySound(mpEnemyInfected->msInvestigateSound);
	}

	mfNoiseRetargetDelaySeconds = 5.0f;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Investigate::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Investigate::OnUpdate(float afTimeStep)
{	
	if(mfNoiseRetargetDelaySeconds >0){
		mfNoiseRetargetDelaySeconds-=afTimeStep;
		if(mfNoiseRetargetDelaySeconds<=0)mfNoiseRetargetDelaySeconds=0;
	}

	////////////////////////////////
	//Play idle sound
	if(mfIdleSoundTime<=0)
	{
		mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
			mpEnemyInfected->mfIdleSoundMaxInteraval);

		mpEnemy->PlaySound(mpEnemyInfected->msIdleSound);
	}
	else
	{
		mfIdleSoundTime -= afTimeStep;
	}

	////////////////////////////////
	//Stuck counter
	if(mpMover->GetStuckCounter() > 1.5f)
	{
		if(mlKnockCount == 1){
			mpEnemy->ChangeState(STATE_IDLE);
			mpEnemy->PlaySound(mpEnemyInfected->msInvestigateOverSound);
			mlKnockCount =0;
		}
		else
		{
			if(mpEnemy->CheckForDoor())
			{
				mpEnemy->ChangeState(STATE_BREAKDOOR);
			}
			mpMover->ResetStuckCounter();
			mlKnockCount++;
		}
	}

	if(mpMover->IsMoving()==false)
	{
		mlKnockCount =0;
		mpEnemy->ChangeState(STATE_IDLE);
		mpEnemy->PlaySound(mpEnemyInfected->msInvestigateOverSound);
	}
}

//-----------------------------------------------------------------------

bool cGameEnemyState_Infected_Investigate::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	if(	mfNoiseRetargetDelaySeconds<=0 && mfHighestVolume < afVolume &&
		afVolume >= mpEnemyInfected->mfIdleMinHearVolume)
	{
		mfHighestVolume = afVolume;
		mpEnemy->SetTempPosition(avPosition);
		OnEnterState(this);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// MOVE TO STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_MoveTo::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfIdleFOV);

	//Play sound
	mpEnemy->PlaySound(mpEnemyInfected->msInvestigateSound);
	
	if(mpMover->MoveToPos(mpEnemy->GetTempPosition())==false)
	{
		//mpInit->mpEffectHandler->GetSubTitle()->Add("Could not move to pos!\n",3,true);
		mpEnemy->ChangeState(apPrevState->GetId());
		return;
	}
	else
	{
		//mpInit->mpEffectHandler->GetSubTitle()->Add("Moving to pos!\n",3,true);
	}
	
	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
										mpEnemyInfected->mfIdleSoundMaxInteraval);

	mlBreakCount = 0;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_MoveTo::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_MoveTo::OnUpdate(float afTimeStep)
{	
	////////////////////////////////
	//Play idle sound
	if(mfIdleSoundTime<=0)
	{
		mfIdleSoundTime = cMath::RandRectf(	mpEnemyInfected->mfIdleSoundMinInteraval,
			mpEnemyInfected->mfIdleSoundMaxInteraval);

		mpEnemy->PlaySound(mpEnemyInfected->msIdleSound);
	}
	else
	{
		mfIdleSoundTime -= afTimeStep;
	}

	////////////////////////////////
	//Stuck counter
	if(mpMover->GetStuckCounter() > 1.5f)
	{
		if(mlBreakCount ==1)
		{
			mpEnemy->ChangeState(STATE_IDLE);
		}
		else
		{
			if(mpEnemy->CheckForDoor())
			{
				mpEnemy->ChangeState(STATE_BREAKDOOR);
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

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// HUNT STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Hunt::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	if(mpInit->mDifficulty == eGameDifficulty_Easy)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed*0.7f);
	else if(mpInit->mDifficulty == eGameDifficulty_Normal)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed);
	else 
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed*1.25f);

	if(mfHuntSoundDelaySeconds <= 0)
	{
		mfHuntSoundDelaySeconds = cMath::RandRectf(mpEnemyInfected->mfHuntSoundMinInteraval,
											mpEnemyInfected->mfHuntSoundMaxInteraval);
	}

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfHuntFOV);

	mfPathUpdateDelaySeconds =0;
	mfPathUpdateIntervalSeconds = 1.0f;
	mbFreePlayerPath = false;
	
	mbBreakingDoor = false;
	mbFoundNoPath = false;
	mbLostPlayer = false;
	mfLostPlayerRemainingSeconds =0;
	mfLostPlayerDurationSeconds = mpEnemyInfected->mfHuntForLostPlayerTime;

	mlStuckAtMaxCount =0;

    mpInit->mpMusicHandler->AddAttacker(mpEnemy);

	if(mpEnemy->GetOnHuntCallback() != "")
	{
		tString sCommand = mpEnemy->GetOnHuntCallback() + "(\""+mpEnemy->GetName()+"\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Hunt::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->SetMaxDoorToughness(-1);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Hunt::OnUpdate(float afTimeStep)
{
	if(mpPlayer->GetHealth() <=0){
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}

	//The sound made now and then while hunting
	if(mfHuntSoundDelaySeconds <= 0)
	{
		mfHuntSoundDelaySeconds = cMath::RandRectf(mpEnemyInfected->mfHuntSoundMinInteraval,
											mpEnemyInfected->mfHuntSoundMaxInteraval);
		mpEnemy->PlaySound(mpEnemyInfected->msHuntSound);
	}
	else
	{
		mfHuntSoundDelaySeconds -= afTimeStep;
	}

	if(mpMover->GetStuckCounter() > 0.6f)
	{
		if(mpEnemy->CheckForDoor())
		{
			mbBreakingDoor = true;
			mpEnemy->ChangeState(STATE_BREAKDOOR);
			mpMover->ResetStuckCounter();
			return;
		}

		mlStuckAtMaxCount++;
		if(mlStuckAtMaxCount >= 6)
		{
			mpEnemy->ChangeState(STATE_IDLE);
			mpMover->ResetStuckCounter();
			mlStuckAtMaxCount =0;
			return;
		}
		mpMover->ResetStuckCounter();
	}

	//The wait after an attack made from afar
	if(mfAttackPauseRemainingSeconds > 0) mfAttackPauseRemainingSeconds -= afTimeStep;

	if(mfPathUpdateDelaySeconds <=0)
	{
		mbFoundNoPath = false;
		//mpInit->mpEffectHandler->GetSubTitle()->Add("Update Path!",1.0f,true);
		mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds;
		
		cAINodeContainer *pNodeCont = mpEnemy->GetMover()->GetNodeContainer();

		//Log("%s: Checking free path\n",mpEnemy->GetName().c_str());
		
		//Check if there is a free path to the player
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
		
		//Get path to player
		if(mbFreePlayerPath==false && mbLostPlayer == false)
		{
			if(mpEnemy->GetDoorBreakCount()> 6.0f)
			{
				mpMover->SetMaxDoorToughness(0);
			}

			//Log("%s: Move to pos\n",mpEnemy->GetName().c_str());

			if(mpMover->MoveToPos(mpEnemy->GetLastPlayerPos())==false)
			{
				bool bFoundAnotherWay = false;
				/*float fHeight = mpMover->GetCharBody()->GetPosition().y -
								mpPlayer->GetCharacterBody()->GetPosition().y;
				if(cMath::Abs(fHeight) > mpMover->GetNodeContainer()->GetMaxHeight())
				{
					cVector3f vPos = mpEnemy->GetLastPlayerPos();
					vPos.y = mpMover->GetCharBody()->GetFeetPosition().y+0.1f;

					if(mpMover->MoveToPos(vPos))
					{
						bFoundAnotherWay = true;
					}
				}*/
				
				if(bFoundAnotherWay == false)
				{
					mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds*5.0f;
					mpMover->Stop();
					//Set this so the enemey at least runs toward the player.
					mbFoundNoPath = true;
				}
			}

			//Log("%s: Done with that.\n",mpEnemy->GetName().c_str());
		}
	}
	else
	{
		mfPathUpdateDelaySeconds -= afTimeStep;
	}
	
	////////////////////////////////
	//Go directly towards the player
	if(mbFreePlayerPath || (mbFoundNoPath && mpMover->IsMoving()==false))
	{
		//Go towards player
		mpMover->MoveDirectToPos(mpPlayer->GetCharacterBody()->GetFeetPosition(),afTimeStep);
		
        //Check if he should attack.
		float fDist = mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody());

		//Just after a swing made while moving in, only a player who is near is attacked
		if(mfAttackPauseRemainingSeconds > 0 && fDist > mpEnemyInfected->mfAttackStillDistance) return;

		if(fDist < mpEnemyInfected->mfAttackDistance)
		{
			float fHeight = mpMover->GetCharBody()->GetPosition().y -
							mpPlayer->GetCharacterBody()->GetPosition().y;
			
			//Player is above
			if(fHeight < 0)
			{
				fHeight += mpMover->GetCharBody()->GetSize().y/2.0f;
				float fMax = mpEnemyInfected->mvAttackDamageSize.y;
				if(fHeight <= -fMax && cMath::RandRectf(0,1) >= 0.2f)
				{
					//too far up, most of the time
					mpEnemy->ChangeState(STATE_FLEE);
					return;
				}
			}

			mpEnemy->ChangeState(STATE_ATTACK);
			if(fDist > mpEnemyInfected->mfAttackStillDistance) mfAttackPauseRemainingSeconds = 1.0f;
		}
	}
	////////////////////////////////
	//Update path search
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
			}
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Hunt::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	if(mbLostPlayer && afChance >= mpEnemyInfected->mfHuntMinSeeChance)
	{
		mbLostPlayer = false;
		mfPathUpdateDelaySeconds =0;
	}
}

//-----------------------------------------------------------------------

bool cGameEnemyState_Infected_Hunt::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	//////////////////////////////////
	//If player is lost the sound might be of help
	if(mbLostPlayer)
	{
		//Check if sound can be heard
		if(afVolume >= mpEnemyInfected->mfHuntMinHearVolume)
		{
			//Check if a node is found near the sound.
			cAINode *pNode = mpMover->GetAINodeAtPosInRange(avPosition,0.0f,5.0f,true, 0.1f);
			if(pNode)
			{
				//Update last player postion.
				mbLostPlayer = false;
				mfPathUpdateDelaySeconds =0;
				mpEnemy->SetLastPlayerPos(pNode->GetPosition());

				return true;
			}
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Hunt::OnDraw()
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
															mpEnemyInfected->mfAttackDistance);
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// ATTACK STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attack::OnEnterState(iGameEnemyState *apPrevState)
{
	///////////////
	//The kind of attack: a grab for the throat when close (never twice in a
	//row), a swing standing still when near, else a swing while moving in.
	float fDist = mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody());
	float fRand = cMath::RandRectf(0,1);
	bool bStill;

	if(	mbGrabThroat==false && mpEnemyInfected->mbAttackGrabThroat &&
		fDist < mpEnemyInfected->mfAttackGrabThroatDist &&
		fRand < mpEnemyInfected->mfAttackGrabThroatChance)
	{
		mbGrabThroat = true;
		bStill = true;
		mfJumpTimer = 0;
		//Not in the original, where the count is never set back and so a
		//second grab did no damage and let go at once.
		mfDamageTimer = 0;
	}
	else if(fDist < mpEnemyInfected->mfAttackStillDistance)
	{
		mbGrabThroat = false;
		bStill = true;
		mfJumpTimer = 0;
	}
	else
	{
		mbGrabThroat = false;
		bStill = false;
		mfJumpTimer = mpEnemyInfected->mfAttackJumpTime;
	}

	///////////////
	//Setup body
	mpEnemy->SetupBody();
	if(mpEnemyInfected->mfAttackSpeed>0)
	{
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfAttackSpeed);
		mpMover->SetMaxTurnSpeed(10000.0f);
	}
	
	///////////////
	//Animation
	if(mbGrabThroat)
	{
		mpEnemy->PlayAnim("GrabThroat",false,0.2f);

		//The player is held and made to look at the enemy
		mpPlayer->SetActive(false);

		cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
						cVector3f(0,mpMover->GetCharBody()->GetSize().y*0.5f,0);

		mpInit->mpPlayer->GetLookAt()->SetTarget(vPos,12.0f,30.0f);
		mpInit->mpPlayer->GetLookAt()->SetActive(true);
		mpInit->mpPlayer->GetLookAt()->SetZoomTarget(0.61f,0.075f,1.0f);

		if(mpInit->mpPlayer->GetMoveState() == ePlayerMoveState_Crouch)
			mpInit->mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);

		mfLookAtTimer = 3.0f;
	}
	else
	{
		tString sAnim = "";
		if(mpEnemyInfected->HasWeapon())
		{
			if(bStill)	sAnim = "SwingStill_0" + cString::ToString(cMath::RandRectl(1,3));
			else		sAnim = "SwingLaunch";
		}
		else
		{
			if(bStill)	sAnim = "SwingClawsStill_0" + cString::ToString(cMath::RandRectl(1,3));
			else		sAnim = "SwingClawsLaunch";
		}

		mpEnemy->PlayAnim(sAnim,false,0.2f);
		mpAttackAnim = mpEnemy->GetMeshEntity()->GetAnimationStateFromName(sAnim);
	}

	///////////////
	//Other
	if(mbGrabThroat==false)
		mpEnemyInfected->PlaySound(mpEnemyInfected->msAttackStartSound);

	mbAttacked = false;

	if(mpEnemy->GetOnAttackCallback() != "")
	{
		tString sCommand = mpEnemy->GetOnAttackCallback() + "(\""+mpEnemy->GetName()+"\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attack::OnLeaveState(iGameEnemyState *apNextState)
{
	mpEnemyInfected->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();

	if(mbGrabThroat)
	{
		mpPlayer->SetActive(true);
		mpInit->mpPlayer->GetLookAt()->SetActive(false);
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attack::OnUpdate(float afTimeStep)
{
	//////////////////////////////////////
	//The grab for the throat: the time held is counted in the damage timer.
	if(mbGrabThroat)
	{
		if(mfDamageTimer >= mpEnemyInfected->mfAttackGrabThroatLength) return;

		mfDamageTimer += afTimeStep;

		//The view shakes: a new point near the enemy's middle to look at
		mfLookAtTimer -= afTimeStep * 2.5f;
		if(mfLookAtTimer <= 0)
		{
			cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
							cVector3f(0,mpMover->GetCharBody()->GetSize().y*0.5f,0);
			vPos += cMath::RandRectVector3f(cVector3f(-0.1f),cVector3f(0.1f));

			mpInit->mpPlayer->GetLookAt()->SetTarget(vPos,4.0f,30.0f);
			mfLookAtTimer = 1.0f;
		}

		if(mbAttacked==false && mfDamageTimer >= mpEnemyInfected->mfAttackGrabThroatDamageTime)
		{
			mpPlayer->Damage(mpEnemyInfected->mfAttackGrabThroatDamage,ePlayerDamageType_BloodSplash);
			mbAttacked = true;
		}

		if(mfDamageTimer >= mpEnemyInfected->mfAttackGrabThroatLength)
		{
			mpPlayer->SetActive(true);
			mpInit->mpPlayer->GetLookAt()->SetActive(false);
		}
		return;
	}

	//Move forward
	if(mpEnemyInfected->mfAttackSpeed>0)
	{
		if(mfJumpTimer > 0)
		{
			mpMover->MoveDirectToPos(mpPlayer->GetCharacterBody()->GetFeetPosition(),afTimeStep);
			mfJumpTimer-= afTimeStep;
		}
		else
		{
			mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
		}
	}

	if(mbAttacked) return;

	////////////////////////////////////////
	// The blow lands at the animation's special event
	if(mpAttackAnim==nullptr || mpAttackAnim->IsAfterSpecialEvent()==false) return;

	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpEnemyInfected->mfAttackDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);
	
	eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies;

	float fDamage = cMath::RandRectf(	mpEnemyInfected->mfAttackMinDamage,
										mpEnemyInfected->mfAttackMaxDamage);
	if(mpEnemyInfected->HasWeapon()) fDamage += mpEnemyInfected->GetWeaponDamage();
	
	if(mpInit->mpAttackHandler->CreateShapeAttack(mpEnemyInfected->GetAttackShape(),
								mtxOffset,
								mpMover->GetCharBody()->GetPosition(),
								fDamage,
								
								mpEnemyInfected->mfAttackMinMass, mpEnemyInfected->mfAttackMaxMass,
								mpEnemyInfected->mfAttackMinImpulse, mpEnemyInfected->mfAttackMaxImpulse,
								
								mpEnemyInfected->mlAttackStrength,

								target,nullptr))
	{
		mpEnemyInfected->PlaySound(mpEnemyInfected->msAttackHitSound);
	}
	mbAttacked = true;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attack::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Attack::OnPostSceneDraw()
{
	cCamera3D *pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());
	
	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpEnemyInfected->mfAttackDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDrawMatrices(mtxOffset, pCamera->GetViewMatrix(), pCamera->GetProjectionMatrix());

	cVector3f vSize = mpEnemyInfected->GetAttackShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize*0.5f,vSize*-0.5f,
		cColor(1,0,1,1));
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// FLEE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Flee::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();
	mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed);
	mpMover->GetCharBody()->SetMaxNegativeMoveSpeed(eCharDir_Forward,-mpEnemyInfected->mfFleeBackSpeed);
	
	float fPosMul = 1;
	if(apPrevState->GetId() == STATE_KNOCKDOWN) fPosMul = 4.0f;
	
	mbBackingFromBreakDoor = false;
	if(apPrevState->GetId() == STATE_BREAKDOOR)
		mbBackingFromBreakDoor = true;

	///////////////////////////////////////
	// The dog has just broken a door
	if(mbBackingFromBreakDoor)
	{
		mfBackAngle = mpMover->GetCharBody()->GetYaw();
		mbBackwards = true;

		mfTimer = mpEnemyInfected->mfFleeBackTime;
		mfCheckBehindTime = 1.0f / 10.0f;
	}
	///////////////////////////////////////
	// Normal flee
	else
	{
		if(	(	apPrevState->GetId() == STATE_KNOCKDOWN || apPrevState->GetId() == STATE_HUNT ||
				cMath::RandRectf(0,1) < 0)//mpEnemyInfected->mfFleePositionChance) 
			)
		{
			cAINode *pNode = mpMover->GetAINodeInRange(	mpEnemyInfected->mfFleePositionMinDistance * fPosMul,
														mpEnemyInfected->mfFleePositionMaxDistance * fPosMul);
			if(pNode) 
			{
				mpMover->MoveToPos(pNode->GetPosition());
			}
			else
			{
				mpEnemy->ChangeState(STATE_HUNT);
			}
			mfTimer = mpEnemyInfected->mfFleePositionMaxTime;
			mbBackwards = false;
		}
		else if(cMath::RandRectf(0,1) < mpEnemyInfected->mfFleeBackChance)
		{
			mfBackAngle = mpMover->GetCharBody()->GetYaw();
			mbBackwards = true;

			mfTimer = mpEnemyInfected->mfFleeBackTime;
			mfCheckBehindTime = 1.0f / 10.0f;
		}
		else
		{
			mpEnemy->ChangeState(STATE_HUNT);
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Flee::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Flee::OnUpdate(float afTimeStep)
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
		}
		mpMover->GetCharBody()->Move(eCharDir_Forward,-1.0f,afTimeStep);
		mpMover->TurnToPos(mpInit->mpPlayer->GetCharacterBody()->GetFeetPosition());
	}
	else
	{
		//Move forward
		if(mpMover->IsMoving()==false || mpMover->GetStuckCounter() > 0.3f	|| mfTimer <=0)
		{
			//Check if there is any enemies nearaby and if anyone is allready fighting
			if(	mpEnemy->CheckForTeamMate(mpEnemyInfected->mfCallBackupRange*1.5f,false) && 
				mpEnemy->CheckForTeamMate(8,true)==false)
			{
				float fPlayerDist = mpMover->DistanceToChar(mpInit->mpPlayer->GetCharacterBody());
				//Log("Dist: %f\n",fPlayerDist);
				if(fPlayerDist > 8)
				{
					mpEnemy->ChangeState(STATE_CALLBACKUP);
				}
				else
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
				//Log("Back from flee!!\n");
			}
			else
			{
				//if(mpInit->mpMusicHandler->AttackerExist(mpEnemy))
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
				//else
				//{
				//	mpEnemy->ChangeState(STATE_IDLE);
				//}
			}
		}
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CALL BACKUP STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_CallBackup::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->PlayAnim(mpEnemyInfected->msCallBackupAnimation,false,0.2f);
	
	//Sound
	mpEnemyInfected->PlaySound(mpEnemyInfected->msCallBackupSound);
	
	//Iterate enemies and show them the player
	cVector3f vPostion = mpMover->GetCharBody()->GetFeetPosition();

	tGameEnemyIterator it = mpInit->mpMapHandler->GetGameEnemyIterator();
	while(it.HasNext())
	{
		iGameEnemy *pEnemy = it.Next();
		if(pEnemy->GetEnemyType() != mpEnemy->GetEnemyType()) continue;
		if(pEnemy == mpEnemy || pEnemy->IsActive()==false || pEnemy->GetHealth()<=0) continue;

		cGameEnemy_Infected *pInfected = static_cast<cGameEnemy_Infected*>(pEnemy);

		float fDist = cMath::Vector3Dist(pInfected->GetMover()->GetCharBody()->GetPosition(),
											vPostion);

		if(fDist <= mpEnemyInfected->mfCallBackupRange)
		{
			pInfected->ShowPlayer(mpEnemyInfected->GetLastPlayerPos());
			break; // Call only for one dog backup!
		}
	}

	mpMover->Stop();
	mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward,0);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_CallBackup::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_CallBackup::OnUpdate(float afTimeStep)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_CallBackup::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// BREAK DOOR STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_BreakDoor::OnEnterState(iGameEnemyState *apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();
	if(mpEnemyInfected->mfBreakDoorSpeed>0)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfBreakDoorSpeed);


	//Animation
	mpEnemy->PlayAnim(mpEnemyInfected->msBreakDoorAnimation,false,0.2f);

	mpEnemyInfected->PlaySound(mpEnemyInfected->msBreakDoorStartSound);

	mfDamageTimer = mpEnemyInfected->mfBreakDoorDamageTime;
	mfStopMoveTimer = mpEnemyInfected->mfBreakDoorDamageTime + 1.1f;
	mbAttacked = false;
	mbStopped = false;

	mlCount =0;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_BreakDoor::OnLeaveState(iGameEnemyState *apNextState)
{
	mpEnemyInfected->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_BreakDoor::OnUpdate(float afTimeStep)
{
	//Move forward
	if(mpEnemyInfected->mfBreakDoorSpeed>0 && mlCount==0 && mbAttacked == false)
	{
		//Skip this for now
		//mpMover->GetCharBody()->Move(eCharDir_Forward,1.0f,afTimeStep);
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
								mpEnemyInfected->mfBreakDoorDamageRange;

			cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
			cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
			mtxOffset.SetTranslation(vPos);

			eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies;

			if(mpInit->mpAttackHandler->CreateShapeAttack(mpEnemyInfected->GetBreakDoorShape(),
											mtxOffset,
											mpMover->GetCharBody()->GetPosition(),
											cMath::RandRectf(	mpEnemyInfected->mfBreakDoorMinDamage,
											mpEnemyInfected->mfBreakDoorMaxDamage),

											mpEnemyInfected->mfBreakDoorMinMass, mpEnemyInfected->mfBreakDoorMaxMass,
											mpEnemyInfected->mfBreakDoorMinImpulse, mpEnemyInfected->mfBreakDoorMaxImpulse,

											mpEnemyInfected->mlBreakDoorStrength,

											target,nullptr))
			{
				mpEnemyInfected->PlaySound(mpEnemyInfected->msBreakDoorHitSound);

				cGameSwingDoor *pDoor = mpInit->mpAttackHandler->GetLastSwingDoor();
				if(pDoor)
				{
					/////////////////////////////
					//The door is unbreakable
					if(pDoor->GetToughness() - mpEnemyInfected->mlBreakDoorStrength >= 4)
					{
						cMatrixf mtxDoor = pDoor->GetBody(0)->GetWorldMatrix();
						cMatrixf mtxInvDoor = cMath::MatrixInverse(mtxDoor);

						cVector3f vDoorForward = mtxInvDoor.GetForward();
						cVector3f vEnemyForward = mpMover->GetCharBody()->GetForward();

						if(cMath::Vector3Dot(vDoorForward,vEnemyForward) < 0)
						{
							mpEnemy->AddDoorBreakCount(2);//8);
							//mpInit->mpEffectHandler->GetSubTitle()->Add("Cannot break this door! On BAD side",4);
						}
						else
						{
							mpEnemy->AddDoorBreakCount(2);
							//Just continue hitting it since it is just barricaded.
							//mpInit->mpEffectHandler->GetSubTitle()->Add("Cannot break this door! On GOOD side",4);
						}
					}
					/////////////////////////////
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
	else
	{
		mfDamageTimer -= afTimeStep;
	}

}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_BreakDoor::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(mlPreviousState);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_BreakDoor::OnPostSceneDraw()
{
	cCamera3D *pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());

	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpEnemyInfected->mfBreakDoorDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDrawMatrices(mtxOffset, pCamera->GetViewMatrix(), pCamera->GetProjectionMatrix());

	cVector3f vSize = mpEnemyInfected->GetBreakDoorShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize*0.5f,vSize*-0.5f,
		cColor(1,0,1,1));
}

//-----------------------------------------------------------------------



//////////////////////////////////////////////////////////////////////////
// KNOCK DOWN STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_KnockDown::OnEnterState(iGameEnemyState *apPrevState)
{
	//A blow only makes the Infected flinch, and it is whole again after it.
	mpEnemy->SetHealth(mpEnemyInfected->GetMaxHealth());

	//Animation
	mpEnemy->PlayAnim("Flinch",false,0.2f);

	//Sound
	mpEnemy->PlaySound(mpEnemyInfected->msKnockDownSound);

	//Setup body
	mpEnemy->SetupBody();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_KnockDown::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_KnockDown::OnUpdate(float afTimeStep)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_KnockDown::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// DEAD STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Dead::OnEnterState(iGameEnemyState *apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();
	mpEnemy->GetMover()->Stop();

	//Animation: no rag doll, the Infected falls by animation
	mpEnemy->PlayAnim("DeathStart",false,0.7f);

	//Sound
	if(mpEnemy->IsLoading()==false)
		mpEnemy->PlaySound(mpEnemyInfected->msDeathSound);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mlAnimCount =0;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Dead::OnLeaveState(iGameEnemyState *apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Dead::OnUpdate(float afTimeStep)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Infected_Dead::OnAnimationOver(const tString &asName)
{
	if(mlAnimCount==0)
	{
		mpEnemy->PlayAnim("DeathMid",false,0.7f);

		mpEnemy->GetMover()->GetCharBody()->SetEntity(nullptr);
		mpEnemy->GetMover()->GetCharBody()->SetActive(false);
	}
	mlAnimCount++;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameEnemy_Infected::cGameEnemy_Infected(cInit *apInit,const tString& asName,TiXmlElement *apGameElem) : iGameEnemy(apInit,asName,apGameElem)
{
	mbHasWeapon = false;
	mbHasFlashLight = false;
	mfWeaponDamage = 0;
	mpWeaponEntity = nullptr;
	mpFlashLightEntity = nullptr;

	mbAccurateClimbing = true;

	LoadBaseProperties(apGameElem);
	
	//////////////////////////////
	//Special properties 

	mfLengthBodyToAss = cString::ToFloat(apGameElem->Attribute("LengthBodyToAss"),1.5f);
	
	mfMinKnockDamage = cString::ToFloat(apGameElem->Attribute("MinKnockDamage"),0);
	mfCertainKnockDamage = cString::ToFloat(apGameElem->Attribute("CertainKnockDamage"),0);

	msIdleAnim = "Idle";

		
	//////////////////////////////
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
	mfHuntMinSeeChance = cString::ToFloat(apGameElem->Attribute("HuntMinSeeChance"),0);
	mfHuntMinHearVolume = cString::ToFloat(apGameElem->Attribute("HuntMinHearVolume"),0);
	msHuntSound = cString::ToString(apGameElem->Attribute("HuntSound"),"");
	mfHuntSoundMinInteraval = cString::ToFloat(apGameElem->Attribute("HuntSoundMinInteraval"),0);
	mfHuntSoundMaxInteraval = cString::ToFloat(apGameElem->Attribute("HuntSoundMaxInteraval"),0);
	mvPreloadSounds.push_back(msHuntSound);

	mfAttackDistance = cString::ToFloat(apGameElem->Attribute("AttackDistance"),0);
	mfAttackStillDistance = cString::ToFloat(apGameElem->Attribute("AttackStillDistance"),0);
	mfAttackSpeed = cString::ToFloat(apGameElem->Attribute("AttackSpeed"),0);
	mfAttackJumpTime = cString::ToFloat(apGameElem->Attribute("AttackJumpTime"),0);
	mfAttackDamageTime = cString::ToFloat(apGameElem->Attribute("AttackDamageTime"),0);
	mfAttackStillDamageTime = cString::ToFloat(apGameElem->Attribute("AttackStillDamageTime"),0);
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
	mbAttackGrabThroat = cString::ToBool(apGameElem->Attribute("AttackGrabThroat"),false);
	mfAttackGrabThroatChance = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatChance"),0);
	mfAttackGrabThroatDist = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatDist"),0);
	mfAttackGrabThroatDamage = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatDamage"),0);
	mfAttackGrabThroatLength = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatLength"),0);
	mfAttackGrabThroatDamageTime = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatDamageTime"),0);
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


	//////////////////////////////
	//Set up states
	AddState(hplNew( cGameEnemyState_Infected_Idle,(STATE_IDLE,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Hunt,(STATE_HUNT,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Attack,(STATE_ATTACK,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Flee,(STATE_FLEE,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_KnockDown,(STATE_KNOCKDOWN,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Dead,(STATE_DEAD,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Patrol,(STATE_PATROL,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Investigate,(STATE_INVESTIGATE,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_BreakDoor,(STATE_BREAKDOOR,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_CallBackup,(STATE_CALLBACKUP,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_MoveTo,(STATE_MOVETO,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Eat,(STATE_EAT,mpInit,this)) );
	AddState(hplNew( cGameEnemyState_Infected_Attention,(STATE_ATTENTION,mpInit,this)) );
}

//-----------------------------------------------------------------------

cGameEnemy_Infected::~cGameEnemy_Infected()
{

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemy_Infected::OnLoad()
{
	//Create attack shape
	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	mpAttackShape = pPhysicsWorld->CreateBoxShape(mvAttackDamageSize,nullptr);
	mpBreakDoorShape = pPhysicsWorld->CreateBoxShape(mvBreakDoorDamageSize,nullptr);
	
	//Set up shape
	ChangeState(STATE_IDLE);
}

//-----------------------------------------------------------------------

void cGameEnemy_Infected::OnUpdate(float afTimeStep)
{
	if(IsActive()==false) return;
	
	///////////////////////////////////
	//Regenerate health: fast, an Infected is not worn down
	if(mfHealth > 0 && mfHealth <= mfMaxHealth *0.7f)
	{
		mfHealth += afTimeStep * (500.0f / 60.0f); //500 heal units / min
	}
}

//-----------------------------------------------------------------------

void cGameEnemy_Infected::SetWeapon(const tString &asFile, const cMatrixf &a_mtxOffset, float afDamage, bool abVisible)
{
	if(mbHasWeapon) return;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	tString sJoint = "WeaponJoint";

	cMesh *pMesh = mpInit->mpGame->GetResources()->GetMeshManager()->CreateMesh(asFile);
	if(pMesh==nullptr)
	{
		Warning("Could not load mesh '%s'\n",asFile.c_str());
		return;
	}

	cMeshEntity *pEntity = pWorld->CreateMeshEntity("WeaponObject",pMesh,true);
	pEntity->SetMatrix(a_mtxOffset);

	AttachMeshEntityToBone(pEntity,sJoint);

	pEntity->SetIsSaved(false);
	pEntity->SetActive(abVisible);
	pEntity->SetVisible(abVisible);

	mpWeaponEntity = pEntity;
	mbHasWeapon = true;
	msWeaponFile = asFile;
	mfWeaponDamage = afDamage;
	m_mtxWeaponMatrix = a_mtxOffset;
}

//-----------------------------------------------------------------------

void cGameEnemy_Infected::AddFlashLight(bool abVisible)
{
	if(mbHasFlashLight) return;

	tString sFile = "weapon_flashlight_no_ray.dae";
	tString sJoint = "FlashlightJoint";

	cMesh *pMesh = mpInit->mpGame->GetResources()->GetMeshManager()->CreateMesh(sFile);
	if(pMesh==nullptr)
	{
		Warning("Could not load mesh '%s'\n",sFile.c_str());
		return;
	}

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

	cMeshEntity *pEntity = pWorld->CreateMeshEntity("WeaponObject",pMesh,true);
	cMatrixf mtxOffset = cMath::MatrixRotate(cVector3f(cMath::ToRad(35),cMath::ToRad(0),cMath::ToRad(60)),
											eEulerRotationOrder_XYZ);
	pEntity->SetMatrix(mtxOffset);

	//The light and the glow of the lamp
	for(int i=0; i< pMesh->GetLightNum(); ++i)
	{
		iLight3D *pLight = pMesh->CreateLightInWorld("Flashlight_light",pMesh->GetLight(i),pEntity,pWorld);
		pLight->SetActive(abVisible);
		pLight->SetVisible(abVisible);
		pLight->SetIsSaved(false);
		mvLights.push_back(pLight);
	}
	for(int i=0; i< pMesh->GetBillboardNum(); ++i)
	{
		cBillboard *pBillboard = pMesh->CreateBillboardInWorld("Flashlight_bb",pMesh->GetBillboard(i),pEntity,pWorld);
		pBillboard->SetActive(abVisible);
		pBillboard->SetVisible(abVisible);
		mvBillboards.push_back(pBillboard);
	}

	AttachMeshEntityToBone(pEntity,sJoint);

	pEntity->SetIsSaved(false);
	pEntity->SetActive(abVisible);
	pEntity->SetVisible(abVisible);

	mbHasFlashLight = true;
	mpFlashLightEntity = pEntity;

	//Walk about holding it up
	msStoppedAnim = "IdleFlashlight";
	msWalkAnim = "WalkFlashlight";
	msRunAnim = "RunFlashlight";
	msIdleAnim = "IdleFlashlight";
}

//-----------------------------------------------------------------------

void cGameEnemy_Infected::ShowPlayer(const cVector3f& avPlayerFeetPos)
{
	if(	mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL || 
		mlCurrentState == STATE_INVESTIGATE)
	{
		mvLastPlayerPos = avPlayerFeetPos;
		ChangeState(STATE_HUNT);
	}
}

//-----------------------------------------------------------------------

bool cGameEnemy_Infected::MoveToPos(const cVector3f& avFeetPos)
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

//-----------------------------------------------------------------------

bool cGameEnemy_Infected::IsFighting()
{
	if( mfHealth <= 0 || IsActive()==false) return false;
	if(	mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL || 
		mlCurrentState == STATE_INVESTIGATE) return false;

	return true;
}

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SAVE OBJECT STUFF
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

kBeginSerialize(cGameEnemy_Infected_SaveData,iGameEnemy_SaveData)
kSerializeVar(mbHasWeapon, eSerializeType_Bool)
kSerializeVar(mbHasFlashLight, eSerializeType_Bool)
kSerializeVar(msWeaponFile, eSerializeType_String)
kSerializeVar(mfWeaponDamage, eSerializeType_Float32)
kSerializeVar(m_mtxWeaponMatrix, eSerializeType_Matrixf)
kEndSerialize()

//-----------------------------------------------------------------------

iGameEntity* cGameEnemy_Infected_SaveData::CreateEntity()
{
	return nullptr;
}

//-----------------------------------------------------------------------

iGameEntity_SaveData* cGameEnemy_Infected::CreateSaveData()
{
	return hplNew( cGameEnemy_Infected_SaveData, () );
}

//-----------------------------------------------------------------------

void cGameEnemy_Infected::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	//The original calls iGameEntity's here, passing the enemy's own data by.
	__super::SaveToSaveData(apSaveData);
	cGameEnemy_Infected_SaveData *pData = static_cast<cGameEnemy_Infected_SaveData*>(apSaveData);

	kCopyToVar(pData,mbHasWeapon);
	kCopyToVar(pData,mbHasFlashLight);
	kCopyToVar(pData,msWeaponFile);
	kCopyToVar(pData,mfWeaponDamage);
	kCopyToVar(pData,m_mtxWeaponMatrix);
}

//-----------------------------------------------------------------------

void cGameEnemy_Infected::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameEnemy_Infected_SaveData *pData = static_cast<cGameEnemy_Infected_SaveData*>(apSaveData);

	if(pData->mbHasWeapon)
		SetWeapon(pData->msWeaponFile,pData->m_mtxWeaponMatrix,pData->mfWeaponDamage,mbActive);
	if(pData->mbHasFlashLight)
		AddFlashLight(mbActive);
}

//-----------------------------------------------------------------------
