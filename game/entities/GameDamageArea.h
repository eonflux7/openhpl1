// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_DAMAGE_AREA_H
#define GAME_GAME_DAMAGE_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameDamageArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameDamageArea_SaveData);
public:
	cVector3f mvSize;

	float mfDamage;
	float mfUpdatesPerSec;
	int mlStrength;
	bool mbDisableObjects;
	bool mbDisableEnemies;
	// Black Plague: how the player is shown to be hurt, and what is heard then
	int mDamageType = ePlayerDamageType_BloodSplash;
	tString msDamageSound;
		
	iGameEntity* CreateEntity() override;
};

class cGameDamageArea : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameDamageArea;
public:
	cGameDamageArea(cInit *apInit,const tString& asName);
	~cGameDamageArea(void) override;

	void OnPlayerPick() override;

	void Update(float afTimeStep) override;

	void SetDamage(float afDamage){ mfDamage = afDamage;}
	void SetUpdatesPerSec(float afUpdatesPerSec){ mfUpdatesPerSec = afUpdatesPerSec;}
	void SetStrength(int alStrength){ mlStrength = alStrength;}
	void SetDisableObjects(bool abDisableObjects){ mbDisableObjects = abDisableObjects;}
	void SetDisableEnemies(bool abDisableEnemies){ mbDisableEnemies = abDisableEnemies;}
	void SetDamageType(ePlayerDamageType aType){ mDamageType = aType;}
	void SetDamageSound(const tString& asSound){ msDamageSound = asSound;}
	
	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
	void SetupSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	float mfDamage;
	float mfUpdatesPerSec;
	int mlStrength;
	bool mbDisableObjects;
	bool mbDisableEnemies;
	// Black Plague: how the player is shown to be hurt, and what is heard then
	int mDamageType = ePlayerDamageType_BloodSplash;
	tString msDamageSound;

	float mfDamageDelaySeconds;
};

class cAreaLoader_GameDamageArea : public iArea3DLoader
{

public:
	cAreaLoader_GameDamageArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameDamageArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_DAMAGE_AREA_H
