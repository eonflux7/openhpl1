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
#ifndef GAME_GAME_LADDER_H
#define GAME_GAME_LADDER_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

//-----------------------------------------

class cGameLadder_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameLadder_SaveData);
public:
	cVector3f mvSize;

	tString msAttachSound;
	tString msClimbUpSound;
	tString msClimbDownSound;

	iGameEntity* CreateEntity() override;
};

//------------------------------------------

class cGameLadder :	public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameLadder;
public:
	cGameLadder(cInit *apInit,const tString& asName);
	~cGameLadder(void) override;

	void OnPlayerPick() override;
	void OnPlayerInteract() override;

	void OnPostSceneDraw() override;
	
	void Setup();

	float GetPlayerDistanceXZ();
	// Signed vertical offset from the player centre to the picked point.
	float GetPickedHeightOffset();

	const cVector3f& GetForward(){ return mvForward;}
	float GetMaxY(){ return mfMaxY;}
	float GetMinY(){ return mfMinY;}

	const tString& GetAttachSound(){ return msAttachSound;}
	const tString& GetClimbUpSound(){ return msClimbUpSound;}
	const tString& GetClimbDownSound(){ return msClimbDownSound;}

	void SetAttachSound(const tString& asSound){ msAttachSound = asSound;}
	void SetClimbUpSound(const tString& asSound){ msClimbUpSound = asSound;}
	void SetClimbDownSound(const tString& asSound){ msClimbDownSound = asSound;}

	cVector3f GetStartRotation();
	// Initial climb position, before the entry collision-clearance search.
	cVector3f GetStartPosition();

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
private:
	cVector3f mvForward;
	float mfMaxY;
	float mfMinY;

	tString msAttachSound;
	tString msClimbUpSound;
	tString msClimbDownSound;
};

//--------------------------------------

class cAreaLoader_GameLadder : public iArea3DLoader
{

public:
	cAreaLoader_GameLadder(const tString &asName, cInit *apInit);
	~cAreaLoader_GameLadder() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_LADDER_H
