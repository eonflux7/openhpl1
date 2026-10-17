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
#ifndef GAME_GAME_ENEMY_DOG_H
#define GAME_GAME_ENEMY_DOG_H

#include "core/StdAfx.h"
#include "enemies/GameEnemy.h"

using namespace hpl;

//-----------------------------------------

class cGameEnemy_Dog;

// BASE STATE
class iGameEnemyState_Dog_Base : public iGameEnemyState
{
public:
	iGameEnemyState_Dog_Base(int alId, cInit *apInit, iGameEnemy *apEnemy);

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	void OnTakeHit(float afDamage) override;
	void OnDeath(float afDamage) override;
	void OnFlashlight(const cVector3f &avPosition) override;

	void OnAnimationOver(const tString& asAnimName) override{}

	void OnDraw() override{}
	void OnPostSceneDraw() override{}

protected:
	cGameEnemy_Dog *mpEnemyDog;
};

//-----------------------------------------

// IDLE STATE
class cGameEnemyState_Dog_Idle : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Idle(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;
};

//-----------------------------------------

// ATTENTION STATE
class cGameEnemyState_Dog_Attention : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Attention(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	  void OnFlashlight(const cVector3f &avPosition) override;

	  void OnDraw() override;
private:
	float mfTime;

};

//-----------------------------------------

// HUNT STATE
class cGameEnemyState_Dog_Hunt : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Hunt(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Dog_Base(alId, apInit,apEnemy){
		mbBreakingDoor = false;
	}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	void OnFlashlight(const cVector3f &avPosition) override{}

	void OnDraw() override;
private:
	float mfPathUpdateDelaySeconds;
	float mfPathUpdateIntervalSeconds;
	bool mbFreePlayerPath;
	bool mbLostPlayer;
	float mfLostPlayerRemainingSeconds;
	float mfLostPlayerDurationSeconds;
	bool mbFoundNoPath;
	float mfSearchAngle;
	int mlSearchPhase;
	bool mbCheckDynamicPath;

	void BeginSearch();
	void UpdateSearch(float afTimeStep);
	void UpdateOriginal(float afTimeStep);

	int mlStuckAtMaxCount;

	int mlBreakDoorCount;
	bool mbBreakingDoor;
};

//-----------------------------------------

// ATTACK STATE
class cGameEnemyState_Dog_Attack : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Attack(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnAnimationOver(const tString &asName) override;

	  void OnPostSceneDraw() override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnFlashlight(const cVector3f &avPosition) override{}
private:
	float mfDamageTimer;
	float mfJumpTimer;
	bool mbAttacked;
	cVector3f mvTargetPosition;
};

//-----------------------------------------

// BREAK DOOR STATE
class cGameEnemyState_Dog_BreakDoor : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_BreakDoor(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnAnimationOver(const tString &asName) override;

	  void OnPostSceneDraw() override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnFlashlight(const cVector3f &avPosition) override{}
private:
	float mfDamageTimer;
	float mfStopMoveTimer;
	bool mbAttacked;
	bool mbStopped;
	int mlCount;
};

//-----------------------------------------

// FLEE STATE
class cGameEnemyState_Dog_Flee : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Flee(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  void OnFlashlight(const cVector3f &avPosition) override{}
private:
	float mfTimer;
	float mfBackAngle;
	bool mbBackwards;
	float mfCheckBehindTime;
	bool mbBackingFromBreakDoor;
};

//-----------------------------------------

// CALL BACKUP STATE
class cGameEnemyState_Dog_CallBackup : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_CallBackup(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  //void OnTakeHit(float afDamage){}
	  void OnFlashlight(const cVector3f &avPosition) override{}

	  void OnAnimationOver(const tString &asName) override;

private:
	bool mbCheckAnim;
};

//-----------------------------------------

// KNOCKDOWN STATE
class cGameEnemyState_Dog_KnockDown : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_KnockDown(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnTakeHit(float afDamage) override{}
	  void OnFlashlight(const cVector3f &avPosition) override{}

	  void OnAnimationOver(const tString &asName) override;

private:
	float mfTimer;
	bool mbCheckAnim;
	int mlPrevToughness;

	int mlStuckAtMaxCount;
};

//-----------------------------------------


// DEAD STATE
class cGameEnemyState_Dog_Dead : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Dead(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	void OnTakeHit(float afDamage) override{}
	void OnFlashlight(const cVector3f &avPosition) override{}
};

//-----------------------------------------

// PATROL STATE
class cGameEnemyState_Dog_Patrol : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Patrol(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnAnimationOver(const tString &asName) override;
private:
	bool mbWaiting;
	bool mbAnimation;

	float mfIdleSoundTime;

	int mlStuckAtMaxCount;
};

//-----------------------------------------

// INVESTIGATE STATE
class cGameEnemyState_Dog_Investigate : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Investigate(int alId, cInit *apInit, iGameEnemy *apEnemy) :
									iGameEnemyState_Dog_Base(alId, apInit,apEnemy){
										mlKnockCount =0;
									}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;

	  float mfIdleSoundTime;
	  float mfNoiseRetargetDelaySeconds;
	  float mfHighestVolume;

	  int mlKnockCount;
};

//-----------------------------------------

// MOVETO STATE
class cGameEnemyState_Dog_MoveTo : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_MoveTo(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  float mfIdleSoundTime;
	  int mlBreakCount;
};

//-----------------------------------------

// EAT STATE
class cGameEnemyState_Dog_Eat : public iGameEnemyState_Dog_Base
{
public:
	cGameEnemyState_Dog_Eat(int alId, cInit *apInit, iGameEnemy *apEnemy) :
							iGameEnemyState_Dog_Base(alId, apInit,apEnemy){}

	 void OnEnterState(iGameEnemyState *apPrevState) override;
	 void OnLeaveState(iGameEnemyState *apNextState) override;

	 void OnUpdate(float afTimeStep) override;
	
	 bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	 void OnSeePlayer(const cVector3f &avPosition, float afChance) override;

	 float mfTime;
};

//-----------------------------------------

class cGameEnemy_Dog : public iGameEnemy
{
public:
	cGameEnemy_Dog(cInit *apInit,const tString& asName,TiXmlElement *apGameElem);
	~cGameEnemy_Dog() override;
	bool UseImprovedAI() const;
	bool TrackPlayerAfterSightLoss() override { return !UseImprovedAI(); }
	float GetHuntMinSeeChance() const;
	float GetHuntMinHearVolume() const;
	cVector3f GetPursuitPosition();
	
	void OnLoad() override;

	void OnUpdate(float afTimeStep) override;

	void ShowPlayer(const cVector3f& avPlayerFeetPos) override;

	bool MoveToPos(const cVector3f& avFeetPos) override;

	bool IsFighting() override;
	
	iCollideShape *GetAttackShape(){ return mpAttackShape;}
	iCollideShape *GetBreakDoorShape(){ return mpBreakDoorShape;}
	
	float mfMinKnockDamage;
	float mfCertainKnockDamage;

	float mfLengthBodyToAss;

//State properties
	float mfIdleFOV;
	tString msIdleFoundPlayerSound;
	float mfIdleMinSeeChance;
	float mfIdleMinHearVolume;
	tString msIdleSound;
	float mfIdleSoundMinInteraval;
	float mfIdleSoundMaxInteraval;
	float mfIdleCallBackupChance;

	tString msInvestigateSound;

	tString msAttentionSound;
	float mfAttentionTime;
	float mfAttentionMinDist;
	
	float mfHuntFOV;
	float mfHuntSpeed;
	float mfHuntForLostPlayerTime;
	float mfHuntMinSeeChance;
	float mfHuntMinHearVolume;
	bool mbAppliedImprovedAI = true;

	float mfAttackDistance;
	float mfAttackSpeed;
	float mfAttackJumpTime;
	float mfAttackDamageTime;
	cVector3f mvAttackDamageSize;
	float mfAttackDamageRange;
	float mfAttackMinDamage;
	float mfAttackMaxDamage;
	tString msAttackStartSound;
	tString msAttackHitSound;
	float mfAttackMinMass;
	float mfAttackMaxMass;
	float mfAttackMinImpulse;
	float mfAttackMaxImpulse;
	int mlAttackStrength;

	tString msBreakDoorAnimation;
	float mfBreakDoorSpeed;
	float mfBreakDoorDamageTime;
	cVector3f mvBreakDoorDamageSize;
	float mfBreakDoorDamageRange;
	float mfBreakDoorMinDamage;
	float mfBreakDoorMaxDamage;
	tString msBreakDoorStartSound;
	tString msBreakDoorHitSound;
	float mfBreakDoorMinMass;
	float mfBreakDoorMaxMass;
	float mfBreakDoorMinImpulse;
	float mfBreakDoorMaxImpulse;
	int mlBreakDoorStrength;
	bool mbBreakDoorRiseAtEnd;

	tString msKnockDownSound;

	tString msDeathSound;

	float mfFleePositionChance;
	float mfFleePositionMaxTime;
	float mfFleePositionMinDistance;
	float mfFleePositionMaxDistance;
	float mfFleeBackChance;
	float mfFleeBackTime;
	float mfFleeBackSpeed;

	tString msCallBackupAnimation;
	tString msCallBackupSound;
	float mfCallBackupRange;

	float mfEatFOV;
	float mfEatMinSeeChance;
	float mfEatMinHearVolume;

private:
    int mlRearCheckCount;
    iCollideShape *mpAttackShape;
	iCollideShape *mpBreakDoorShape;
};

//-----------------------------------------

#endif // GAME_GAME_ENEMY_DOG_H
