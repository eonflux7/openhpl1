// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later

// What Overture's Dog and Black Plague's Infected share. Black Plague's
// Infected began as a copy of the Dog; the states that both still run the
// same way, and the properties they read, live here. Each enemy keeps its own
// Hunt, Attack, KnockDown and Dead states.
#ifndef GAME_GAME_ENEMY_DOG_BASE_H
#define GAME_GAME_ENEMY_DOG_BASE_H

#include "core/StdAfx.h"
#include "enemies/GameEnemy.h"

using namespace hpl;

class iGameEnemy_DogBase;

// BASE STATE
class iGameEnemyState_DogBase : public iGameEnemyState
{
public:
	iGameEnemyState_DogBase(int alId, cInit *apInit, iGameEnemy *apEnemy);

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	void OnTakeHit(float afDamage) override;
	void OnDeath(float afDamage) override;
	void OnFlashlight(const cVector3f &avPosition) override;

	void OnAnimationOver(const tString& asAnimName) override{}

	void OnDraw() override{}
	void OnPostSceneDraw() override{}

protected:
	iGameEnemy_DogBase *mpDogBase;
};

// IDLE STATE
class cGameEnemyState_DogBase_Idle : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_Idle(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_DogBase(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;
};

// ATTENTION STATE
class cGameEnemyState_DogBase_Attention : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_Attention(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_DogBase(alId, apInit,apEnemy){}

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

// BREAK DOOR STATE
class cGameEnemyState_DogBase_BreakDoor : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_BreakDoor(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_DogBase(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnAnimationOver(const tString &asName) override;

	  void OnPostSceneDraw() override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnFlashlight(const cVector3f &avPosition) override{}
protected:
	float mfDamageTimer;
	float mfStopMoveTimer;
	bool mbAttacked;
	bool mbStopped;
	int mlCount;
};

// FLEE STATE
class cGameEnemyState_DogBase_Flee : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_Flee(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_DogBase(alId, apInit,apEnemy){}

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

// CALL BACKUP STATE
class cGameEnemyState_DogBase_CallBackup : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_CallBackup(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_DogBase(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnFlashlight(const cVector3f &avPosition) override{}

	  void OnAnimationOver(const tString &asName) override;
};

// PATROL STATE
class cGameEnemyState_DogBase_Patrol : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_Patrol(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_DogBase(alId, apInit,apEnemy){}

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

// INVESTIGATE STATE
class cGameEnemyState_DogBase_Investigate : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_Investigate(int alId, cInit *apInit, iGameEnemy *apEnemy) :
									iGameEnemyState_DogBase(alId, apInit,apEnemy){
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

// MOVETO STATE
class cGameEnemyState_DogBase_MoveTo : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_MoveTo(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_DogBase(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  float mfIdleSoundTime;
	  int mlBreakCount;
};

// EAT STATE
class cGameEnemyState_DogBase_Eat : public iGameEnemyState_DogBase
{
public:
	cGameEnemyState_DogBase_Eat(int alId, cInit *apInit, iGameEnemy *apEnemy) :
							iGameEnemyState_DogBase(alId, apInit,apEnemy){}

	 void OnEnterState(iGameEnemyState *apPrevState) override;
	 void OnLeaveState(iGameEnemyState *apNextState) override;

	 void OnUpdate(float afTimeStep) override;

	 bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	 void OnSeePlayer(const cVector3f &avPosition, float afChance) override;

	 float mfTime;
};

class iGameEnemy_DogBase : public iGameEnemy
{
public:
	iGameEnemy_DogBase(cInit *apInit,const tString& asName,TiXmlElement *apGameElem);

	// Where the enemy turns to and measures the player from outside Hunt.
	virtual cVector3f GetPursuitPosition();

	void OnLoad() override;

	void ShowPlayer(const cVector3f& avPlayerFeetPos) override;

	bool MoveToPos(const cVector3f& avFeetPos) override;

	bool IsFighting() override;

	iCollideShape *GetAttackShape(){ return mpAttackShape;}
	iCollideShape *GetBreakDoorShape(){ return mpBreakDoorShape;}

	// Black Plague's Investigate gives up as soon as it finds no path, and
	// plays its sound only when it starts, not when a louder noise moves it.
	bool mbBlackPlagueInvestigate = false;

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
	tString msInvestigateOverSound;

	tString msAttentionSound;
	float mfAttentionTime;
	float mfAttentionMinDist;

	float mfHuntFOV;
	float mfHuntSpeed;
	float mfHuntForLostPlayerTime;
	float mfHuntMinSeeChance;
	float mfHuntMinHearVolume;

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
	iCollideShape *mpAttackShape;
	iCollideShape *mpBreakDoorShape;
};

#endif // GAME_GAME_ENEMY_DOG_BASE_H
