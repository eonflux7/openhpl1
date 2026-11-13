// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later

// The Infected of Penumbra: Black Plague. Frictional Games never released
// Black Plague's game code; this is the Dog of Overture changed after what
// the game's binary does.
#ifndef GAME_GAME_ENEMY_INFECTED_H
#define GAME_GAME_ENEMY_INFECTED_H

#include "core/StdAfx.h"
#include "enemies/GameEnemy_DogBase.h"

using namespace hpl;

class cGameEnemy_Infected;

// BASE STATE
class iGameEnemyState_Infected_Base : public iGameEnemyState_DogBase
{
public:
	iGameEnemyState_Infected_Base(int alId, cInit *apInit, iGameEnemy *apEnemy);

protected:
	cGameEnemy_Infected *mpEnemyInfected;
};

// BREAK DOOR STATE
class cGameEnemyState_Infected_BreakDoor : public cGameEnemyState_DogBase_BreakDoor
{
public:
	cGameEnemyState_Infected_BreakDoor(int alId, cInit *apInit, iGameEnemy *apEnemy) :
	  cGameEnemyState_DogBase_BreakDoor(alId, apInit,apEnemy){}

	  void OnUpdate(float afTimeStep) override;

	  void OnAnimationOver(const tString &asName) override;
};

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
	float mfPathUpdateDelaySeconds;
	float mfPathUpdateIntervalSeconds;
	bool mbFreePlayerPath;
	bool mbLostPlayer;
	float mfLostPlayerRemainingSeconds;
	float mfLostPlayerDurationSeconds;
	bool mbFoundNoPath;

	//The wait after a swing made while moving in, and the time to the next hunt sound
	float mfAttackPauseRemainingSeconds = 0;
	float mfHuntSoundDelaySeconds = 0;

	int mlStuckAtMaxCount;

	int mlBreakDoorCount;
	bool mbBreakingDoor;
};

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

class cGameEnemy_Infected : public iGameEnemy_DogBase
{
#ifdef __GNUC__
	typedef iGameEnemy __super;
#endif
public:
	cGameEnemy_Infected(cInit *apInit,const tString& asName,TiXmlElement *apGameElem);
	~cGameEnemy_Infected() override;

	void OnUpdate(float afTimeStep) override;

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

	tString msHuntSound;
	float mfHuntSoundMinInteraval;
	float mfHuntSoundMaxInteraval;

	float mfAttackStillDistance;
	float mfAttackStillDamageTime;
	bool mbAttackGrabThroat;
	float mfAttackGrabThroatChance;
	float mfAttackGrabThroatDist;
	float mfAttackGrabThroatDamage;
	float mfAttackGrabThroatLength;
	float mfAttackGrabThroatDamageTime;

private:
	bool mbHasWeapon;
	bool mbHasFlashLight;
	tString msWeaponFile;
	float mfWeaponDamage;
	cMatrixf m_mtxWeaponMatrix;
	tString msIdleAnim;
	cMeshEntity *mpWeaponEntity;
	cMeshEntity *mpFlashLightEntity;
};

#endif // GAME_GAME_ENEMY_INFECTED_H
