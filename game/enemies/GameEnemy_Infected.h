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
#ifndef GAME_GAME_ENEMY_INFECTED_H
#define GAME_GAME_ENEMY_INFECTED_H

#include "core/StdAfx.h"
#include "enemies/GameEnemy.h"

using namespace hpl;

//-----------------------------------------

class cGameEnemy_Infected;

// BASE STATE
class iGameEnemyState_Infected_Base : public iGameEnemyState
{
public:
	iGameEnemyState_Infected_Base(int alId, cInit *apInit, iGameEnemy *apEnemy);

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override;
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	void OnTakeHit(float afDamage) override;
	void OnDeath(float afDamage) override;
	void OnFlashlight(const cVector3f &avPosition) override;

	void OnAnimationOver(const tString& asAnimName) override{}

	void OnDraw() override{}
	void OnPostSceneDraw() override{}

protected:
	cGameEnemy_Infected *mpEnemyInfected;
};

//-----------------------------------------

// IDLE STATE
class cGameEnemyState_Infected_Idle : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Idle(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;
};

//-----------------------------------------

// ATTENTION STATE
class cGameEnemyState_Infected_Attention : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Attention(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

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
class cGameEnemyState_Infected_Hunt : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Hunt(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Infected_Base(alId, apInit,apEnemy){
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
	float mfUpdatePathCount;
	float mfUpdateFreq;
	bool mbFreePlayerPath;
	bool mbLostPlayer;
	float mfLostPlayerCount;
	float mfMaxLostPlayerCount;
	bool mbFoundNoPath;

	//The wait after a swing made while moving in, and the time to the next hunt sound
	float mfAttackPauseCount = 0;
	float mfHuntSoundCount = 0;

	int mlStuckAtMaxCount;

	int mlBreakDoorCount;
	bool mbBreakingDoor;
};

//-----------------------------------------

// ATTACK STATE
class cGameEnemyState_Infected_Attack : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Attack(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  void OnAnimationOver(const tString &asName) override;

	  void OnPostSceneDraw() override;

	  void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	  void OnFlashlight(const cVector3f &avPosition) override{}
private:
	float mfDamageTimer = 0;
	float mfJumpTimer = 0;
	bool mbAttacked = false;

	//The grab for the throat, the time to the next shake of the player's
	//view and the swing's animation (the blow lands at its special event)
	bool mbGrabThroat = false;
	float mfLookAtTimer = 0;
	cAnimationState *mpAttackAnim = nullptr;
};

//-----------------------------------------

// BREAK DOOR STATE
class cGameEnemyState_Infected_BreakDoor : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_BreakDoor(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

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
class cGameEnemyState_Infected_Flee : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Flee(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

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
class cGameEnemyState_Infected_CallBackup : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_CallBackup(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

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
class cGameEnemyState_Infected_KnockDown : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_KnockDown(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

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
class cGameEnemyState_Infected_Dead : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Dead(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

	void OnEnterState(iGameEnemyState *apPrevState) override;
	void OnLeaveState(iGameEnemyState *apNextState) override;

	void OnUpdate(float afTimeStep) override;

	void OnSeePlayer(const cVector3f &avPosition, float afChance) override{}
	bool OnHearNoise(const cVector3f &avPosition, float afVolume) override{return false;}
	void OnTakeHit(float afDamage) override{}
	void OnFlashlight(const cVector3f &avPosition) override{}

	void OnAnimationOver(const tString &asName) override;

private:
	int mlAnimCount = 0;
};

//-----------------------------------------

// PATROL STATE
class cGameEnemyState_Infected_Patrol : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Patrol(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

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
class cGameEnemyState_Infected_Investigate : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Investigate(int alId, cInit *apInit, iGameEnemy *apEnemy) :
									iGameEnemyState_Infected_Base(alId, apInit,apEnemy){
										mlKnockCount =0;
									}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;

	  float mfIdleSoundTime;
	  float mfHearSoundCount;
	  float mfHighestVolume;

	  int mlKnockCount;
};

//-----------------------------------------

// MOVETO STATE
class cGameEnemyState_Infected_MoveTo : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_MoveTo(int alId, cInit *apInit, iGameEnemy *apEnemy) :
								iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

	  void OnEnterState(iGameEnemyState *apPrevState) override;
	  void OnLeaveState(iGameEnemyState *apNextState) override;

	  void OnUpdate(float afTimeStep) override;

	  float mfIdleSoundTime;
	  int mlBreakCount;
};

//-----------------------------------------

// EAT STATE
class cGameEnemyState_Infected_Eat : public iGameEnemyState_Infected_Base
{
public:
	cGameEnemyState_Infected_Eat(int alId, cInit *apInit, iGameEnemy *apEnemy) :
							iGameEnemyState_Infected_Base(alId, apInit,apEnemy){}

	 void OnEnterState(iGameEnemyState *apPrevState) override;
	 void OnLeaveState(iGameEnemyState *apNextState) override;

	 void OnUpdate(float afTimeStep) override;
	
	 bool OnHearNoise(const cVector3f &avPosition, float afVolume) override;
	 void OnSeePlayer(const cVector3f &avPosition, float afChance) override;

	 float mfTime;
};

//-----------------------------------------

class cGameEnemy_Infected_SaveData : public iGameEnemy_SaveData
{
	kSerializableClassInit(cGameEnemy_Infected_SaveData);
public:
	bool mbHasWeapon = false;
	bool mbHasFlashLight = false;
	tString msWeaponFile;
	float mfWeaponDamage = 0;
	cMatrixf m_mtxWeaponMatrix;

	iGameEntity* CreateEntity() override;
};

//-----------------------------------------

class cGameEnemy_Infected : public iGameEnemy
{
#ifdef __GNUC__
	typedef iGameEnemy __super;
#endif
public:
	cGameEnemy_Infected(cInit *apInit,const tString& asName,TiXmlElement *apGameElem);
	~cGameEnemy_Infected() override;
	
	void OnLoad() override;

	void OnUpdate(float afTimeStep) override;

	void ShowPlayer(const cVector3f& avPlayerFeetPos) override;

	bool MoveToPos(const cVector3f& avFeetPos) override;

	bool IsFighting() override;

	//A weapon in the hand (bone "WeaponJoint"): its blows do that much more damage.
	void SetWeapon(const tString &asFile, const cMatrixf &a_mtxOffset, float afDamage, bool abVisible);
	bool HasWeapon(){ return mbHasWeapon;}
	float GetWeaponDamage(){ return mfWeaponDamage;}

	//A lit flashlight in the other hand (bone "FlashlightJoint"), and the
	//animations of walking with it.
	void AddFlashLight(bool abVisible);

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
	
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
	tString msInvestigateOverSound;

	tString msAttentionSound;
	float mfAttentionTime;
	float mfAttentionMinDist;
	
	float mfHuntFOV;
	float mfHuntSpeed;
	float mfHuntForLostPlayerTime;
	float mfHuntMinSeeChance;
	float mfHuntMinHearVolume;
	tString msHuntSound;
	float mfHuntSoundMinInteraval;
	float mfHuntSoundMaxInteraval;

	float mfAttackDistance;
	float mfAttackStillDistance;
	float mfAttackSpeed;
	float mfAttackJumpTime;
	float mfAttackDamageTime;
	float mfAttackStillDamageTime;
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
	bool mbAttackGrabThroat;
	float mfAttackGrabThroatChance;
	float mfAttackGrabThroatDist;
	float mfAttackGrabThroatDamage;
	float mfAttackGrabThroatLength;
	float mfAttackGrabThroatDamageTime;

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
	bool mbHasWeapon;
	bool mbHasFlashLight;
	tString msWeaponFile;
	float mfWeaponDamage;
	cMatrixf m_mtxWeaponMatrix;
	tString msIdleAnim;
	cMeshEntity *mpWeaponEntity;
	cMeshEntity *mpFlashLightEntity;

    iCollideShape *mpAttackShape;
	iCollideShape *mpBreakDoorShape;
};

//-----------------------------------------

#endif // GAME_GAME_ENEMY_INFECTED_H
