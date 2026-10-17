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
#ifndef GAME_GAME_VENT_AREA_H
#define GAME_GAME_VENT_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

// Black Plague/Requiem vent entry: crouch a jumping player, then carry them
// toward the floor centre if clearance permits. Written after the game's binary.

//------------------------------------------

class cGameVentArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameVentArea_SaveData);
public:
	cVector3f mvSize;

	bool mbPlayerInside;

	iGameEntity* CreateEntity() override;
};

//------------------------------------------

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

//------------------------------------------

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
