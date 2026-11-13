// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_ENEMY_DOG_H
#define GAME_GAME_ENEMY_DOG_H

#include "core/StdAfx.h"
#include "enemies/GameEnemy_DogBase.h"

using namespace hpl;

class cGameEnemy_Dog;

// BASE STATE
class iGameEnemyState_Dog_Base : public iGameEnemyState_DogBase
{
public:
	iGameEnemyState_Dog_Base(int alId, cInit *apInit, iGameEnemy *apEnemy);

protected:
	cGameEnemy_Dog *mpEnemyDog;
};

// ATTENTION STATE
class cGameEnemyState_Dog_Attention : public cGameEnemyState_DogBase_Attention
{
public:
	cGameEnemyState_Dog_Attention(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  cGameEnemyState_DogBase_Attention(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
};

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

class cGameEnemy_Dog : public iGameEnemy_DogBase
{
public:
	cGameEnemy_Dog(cInit *apInit,const tString& asName,TiXmlElement *apGameElem);
	~cGameEnemy_Dog() override;
	bool UseImprovedAI() const;
	bool TrackPlayerAfterSightLoss() override { return !UseImprovedAI(); }
	float GetHuntMinSeeChance() const;
	float GetHuntMinHearVolume() const;
	cVector3f GetPursuitPosition() override;
	
	void OnLoad() override;

	void OnUpdate(float afTimeStep) override;

	bool mbAppliedImprovedAI = true;

private:
    int mlRearCheckCount;
};

#endif // GAME_GAME_ENEMY_DOG_H
