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
#ifndef GAME_GAME_SWING_DOOR_H
#define GAME_GAME_SWING_DOOR_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

//----------------------------------------------

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

//----------------------------------------------

class cGameSwingJointDefault
{
public:
	float mfMin;
	float mfMax;
};
//----------------------------------------------


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

	void SetLocked(bool abX);
	void SetLockedDesc(const tWString& asX){ msLockedDesc = asX;}
	void SetLockedSound(const tString& asX){ msLockedSound = asX;}

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

//--------------------------------------

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
