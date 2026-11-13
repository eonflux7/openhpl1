// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_AREA_H
#define GAME_GAME_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameArea_SaveData);
public:
	cVector3f mvSize;

	bool mbUseCustomIcon;
	eCrossHairState mCustomIcon;
	
	iGameEntity* CreateEntity() override;
};

class cGameArea : public iGameEntity
{
	#ifdef __GNUC__
	typedef iGameEntity __super;
	#endif

	friend class cAreaLoader_GameArea;
public:
	cGameArea(cInit *apInit,const tString& asName);
	~cGameArea(void) override;

	void OnPlayerPick() override;

	void SetCustomIcon(eCrossHairState aIcon);
	eCrossHairState GetCustomIcon(){ return mCustomIcon;}
	bool GetUsesCustomIcon(){ return mbUseCustomIcon;}


	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
	
private:
	bool mbUseCustomIcon;
	eCrossHairState mCustomIcon;
};

class cAreaLoader_GameArea : public iArea3DLoader
{

public:
	cAreaLoader_GameArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_AREA_H
