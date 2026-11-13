// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_VENT_AREA_H
#define GAME_GAME_VENT_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

// Black Plague/Requiem vent entry: crouch a jumping player, then carry them
// toward the floor centre if clearance permits. Written after the game's binary.

class cGameVentArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameVentArea_SaveData);
public:
	cVector3f mvSize;

	bool mbPlayerInside;

	iGameEntity* CreateEntity() override;
};

class cGameVentArea : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameVentArea;
public:
	cGameVentArea(cInit *apInit,const tString& asName);
	~cGameVentArea(void) override;

	void OnPlayerPick() override;

	void Update(float afTimeStep) override;

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	bool mbMovingPlayer;
	cVector3f mvEntryVelocity;
	float mfEntryRemainingSeconds;

	// Entry-attempt latch; cleared outside the bounds or below the floor.
	bool mbPlayerInside;
};

class cAreaLoader_GameVentArea : public iArea3DLoader
{

public:
	cAreaLoader_GameVentArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameVentArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:
	cInit *mpInit;
};

#endif // GAME_GAME_VENT_AREA_H
