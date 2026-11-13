// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_SWING_DOOR_H
#define GAME_GAME_SWING_DOOR_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameSwingDoor_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameSwingDoor_SaveData);
public:
	bool mbLocked;
	tWString msLockedDesc;
	tString msLockedSound;

	tString msBreakSound;
	tString msBreakEntity;
	tString msBreakPS;

	iGameEntity* CreateEntity() override;
};

class cGameSwingJointDefault
{
public:
	float mfMin;
	float mfMax;
};

class cGameSwingDoor :	public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
friend class cEntityLoader_GameSwingDoor;
public:
	cGameSwingDoor(cInit *apInit,const tString& asName);
	~cGameSwingDoor(void) override;

	void OnPlayerInteract() override;
	void OnPlayerPick() override;

	void Update(float afTimeStep) override;

	void OnDamage(float afDamage) override;
	void OnDeath(float afDamage) override;

	void BreakAction() override;

	void SetupBreakObject();

	void SetupPhysics(cWorld3D *apWorld);

	void SetLocked(bool abLocked);
	void SetLockedDesc(const tWString& asDescription){ msLockedDesc = asDescription;}
	void SetLockedSound(const tString& asSound){ msLockedSound = asSound;}

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	std::vector<iPhysicsController*> mvStopControllers;
	std::vector<cGameSwingJointDefault> mvJointDefaults;

	bool mbLocked;

	// Black Plague: what is said and heard when a locked door is tried
	tWString msLockedDesc;
	tString msLockedSound;

	tString msBreakSound;
	tString msBreakEntity;
	tString msBreakPS;
};

class cEntityLoader_GameSwingDoor : public cEntityLoader_Object
{
public:
	cEntityLoader_GameSwingDoor(const tString &asName, cInit *apInit);
	~cEntityLoader_GameSwingDoor() override;

private:
	void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;
	void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

	cInit *mpInit;
};


#endif // GAME_GAME_SWING_DOOR_H
