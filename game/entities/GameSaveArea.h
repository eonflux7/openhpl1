// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_SAVE_AREA_H
#define GAME_GAME_SAVE_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameSaveArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameSaveArea_SaveData);
public:
	cVector3f mvSize;

	tString msMessageCat;
	tString msMessageEntry;

	tString msSound;

	bool mbHasBeenUsed;
	
	iGameEntity* CreateEntity() override;
};

class cGameSaveArea : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameSaveArea;
public:
	cGameSaveArea(cInit *apInit,const tString& asName);
	~cGameSaveArea(void) override;

	void OnPlayerPick() override;
	void OnPlayerInteract() override;

	void SetMessageCat(const tString &asCat){ msMessageCat = asCat; }
	void SetMessageEntry(const tString &asEntry){ msMessageEntry = asEntry;}

	void SetSound(const tString &asSound){ msSound = asSound;}

	void SetHasBeenUsed(bool abUsed){ mbHasBeenUsed = abUsed;}

	tString& GetMessageCat(){ return msMessageCat; }
	tString& GetMessageEntry(){ return msMessageEntry;}

	tString& GetSound(){ return msSound;}

	bool GetHasBeenUsed(){ return mbHasBeenUsed;}
		
	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
private:
	tString msMessageCat;
	tString msMessageEntry;

	tString msSound;

	bool mbHasBeenUsed;
};

class cAreaLoader_GameSaveArea : public iArea3DLoader
{

public:
	cAreaLoader_GameSaveArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameSaveArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_SAVE_AREA_H
