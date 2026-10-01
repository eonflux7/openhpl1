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
#ifndef GAME_GAME_ENEMY_SPIDER_H
#define GAME_GAME_ENEMY_SPIDER_H

#include "core/StdAfx.h"
#include "enemies/GameEnemy.h"

using namespace hpl;

//-----------------------------------------

class cGameEnemy_Spider;

// BASE STATE
class iGameEnemyState_Spider_Base : public iGameEnemyState
{
public:
	iGameEnemyState_Spider_Base(int alId, cInit *apInit, iGameEnemy *apEnemy);

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	void OnTakeHit(float afDamage) override;
	void OnDeath(float afDamage) override;
	void OnFlashlight(const cVector3f &avPosition) override;

	void OnAnimationOver(const tString& asAnimName) override{}

	void OnDraw() override{}
	void OnPostSceneDraw() override{}

protected:
	cGameEnemy_Spider *mpEnemySpider;
};

//-----------------------------------------

// IDLE STATE
class cGameEnemyState_Spider_Idle : public iGameEnemyState_Spider_Base
{
public:
	cGameEnemyState_Spider_Idle(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_Spider_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	//void OnSeePlayer(const cVector3f &avPosition, float afChance){}


private:
	bool mbStopped;
	float mfNextWalkTime;
};

//-----------------------------------------

// HUNT STATE
class cGameEnemyState_Spider_Hunt : public iGameEnemyState_Spider_Base
{
public:
	cGameEnemyState_Spider_Hunt(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Spider_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;

	void OnDraw() override;
private:
	float mfUpdatePathCount;
	float mfUpdateFreq;
	bool mbFreePlayerPath;
	bool mbLostPlayer;
	float mfLostPlayerCount;
	float mfMaxLostPlayerCount;
};

//-----------------------------------------

// ATTACK STATE
class cGameEnemyState_Spider_Attack : public iGameEnemyState_Spider_Base
{
public:
	cGameEnemyState_Spider_Attack(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Spider_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	void OnAnimationOver(const tString &asName) override;
	
	void OnPostSceneDraw() override;

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	void OnFlashlight(const cVector3f &avPosition) override;

private:
	float mfDamageTimer;
	float mfJumpTimer;
	bool mbAttacked;
};

//-----------------------------------------

// FLEE STATE
class cGameEnemyState_Spider_Flee : public iGameEnemyState_Spider_Base
{
public:
	cGameEnemyState_Spider_Flee(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Spider_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  void OnFlashlight(const cVector3f &avPosition) override{}
private:
	float mfTimer;
	float mfBackAngle;
	bool mbBackwards;
};

//-----------------------------------------

// KNOCKDOWN STATE
class cGameEnemyState_Spider_KnockDown : public iGameEnemyState_Spider_Base
{
public:
	cGameEnemyState_Spider_KnockDown(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Spider_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnTakeHit(float afDamage) override{}

	  void OnAnimationOver(const tString &asName) override;

private:
	float mfTimer;
	bool mbCheckAnim;
};

//-----------------------------------------

// DEAD STATE
class cGameEnemyState_Spider_Dead : public iGameEnemyState_Spider_Base
{
public:
	cGameEnemyState_Spider_Dead(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Spider_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	void OnFlashlight(const cVector3f &avPosition) override{}
	void OnTakeHit(float afDamage) override{}
	void OnDeath(float afDamage) override{}
};

//-----------------------------------------

class cGameEnemy_Spider : public iGameEnemy
{
public:
	cGameEnemy_Spider(cInit *apInit,const tString& asName,TiXmlElement *apGameElem);
	~cGameEnemy_Spider() override;
	
	void OnLoad() override;

	void OnUpdate(float afTimeStep) override;

	void ShowPlayer(const cVector3f& avPlayerFeetPos) override;

	bool MoveToPos(const cVector3f& avFeetPos) override;

	bool IsFighting() override;
	
	iCollideShape *GetAttackShape(){ return mpAttackShape;}
	
//State properties
	bool mbPathFind;

	float mfIdleFOV;
	tString msIdleFoundPlayerSound;
	float mfIdleMinSeeChance;
	float mfIdleMinHearVolume;
	float mfIdleMinWaitLength;
	float mfIdleMaxWaitLength;
	
	float mfHuntFOV;
	float mfHuntSpeed;
	float mfHuntForLostPlayerTime;
	float mfHuntMinSeeChance;
	float mfHuntMinHearVolume;

	float mfAttackDistance;
	float mfAttackForce;
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

	float mfFleeMinDistance;
	float mfFleeMaxDistance;
	bool mbFleeFromFlashlight;

	tString msKnockDownSound;

	tString msDeathSound;

private:
    iCollideShape *mpAttackShape;
};

//-----------------------------------------

#endif // GAME_GAME_ENEMY_SPIDER_H
