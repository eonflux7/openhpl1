// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_LINK_H
#define GAME_GAME_LINK_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameLink_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameLink_SaveData);
public:
	cVector3f mvSize;

	tString msMapFile;
	tString msMapPos;
	tString msStartSound;
	tString msStopSound;
	float mfFadeOutTime;
	float mfFadeInTime;
	tString msLoadTextCat;
	tString msLoadTextEntry;

	tString msLoadImage;
	bool mbLocked = false;
	tString msLockedSound;
	tWString msLockedDesc;
	
	iGameEntity* CreateEntity() override;
};

class cGameLink :	public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameLink;
public:
	cGameLink(cInit *apInit,const tString& asName);
	~cGameLink(void) override;

	void OnPlayerPick() override;
	void OnPlayerInteract() override;
	
	tString msMapFile;
	tString msMapPos;
	tString msStartSound;
	tString msStopSound;
	float mfFadeOutTime;
	float mfFadeInTime;
	tString msLoadTextCat;
	tString msLoadTextEntry;

	// Black Plague/Requiem loading-screen image.
	tString msLoadImage;
	// Locked interactions play feedback instead of requesting a map change.
	bool mbLocked;
	tString msLockedSound;
	tWString msLockedDesc;

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
};

class cAreaLoader_GameLink : public iArea3DLoader
{

public:
	cAreaLoader_GameLink(const tString &asName, cInit *apInit);
	~cAreaLoader_GameLink() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_LINK_H
