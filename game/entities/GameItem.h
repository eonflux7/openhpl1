// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_ITEM_H
#define GAME_GAME_ITEM_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameItem_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameItem_SaveData);
public:

	bool mbHasBeenFlashed;
	float mfEnterFlashDist;
	float mfExitFlashDist;
	
	iGameEntity* CreateEntity() override;
};

class cGameItem_InViewRay : public iPhysicsRayCallback
{
public:
	void SetUp(iPhysicsBody *apSkipBody);
	bool OnIntersect(iPhysicsBody *apBody,cPhysicsRayParams *apParams) override;

	bool mbIntersected;
	iPhysicsBody *mpSkipBody;
};

class cGameItem :	public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
friend class cEntityLoader_GameItem;
public:
	cGameItem(cInit *apInit,const tString& asName);
	~cGameItem(void) override;

	void OnWorldLoad() override;
	
	void OnPlayerPick() override;

	void OnPlayerInteract() override;
	// Into the inventory from wherever the player is, as when picked up
	void TakeIntoInventory();
	
	void Update(float afTimeStep) override;

	void OnPostSceneDraw() override;

	float GetFlashAlpha(){ return mfFlashAlpha;}

    const tString& GetImageFile(){ return msImageFile;}
	eGameItemType GetItemType(){ return mItemType;}

	bool CanBeDropped(){ return mbCanBeDropped;}
	
	bool HasCount(){ return mbHasCount;}
	int GetCount(){ return mlCount;}

	tString GetHudModelFile(){ return msHudModelFile;}
	tString GetHudModelName(){ return msHudModelName;}
	
	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	bool IsInView(float afMaxDistance);

	tString msImageFile;
	eGameItemType mItemType;
	bool mbCanBeDropped;
	bool mbHasCount;
	int mlCount;
	
	float mfFlashCheckElapsedSeconds;
	float mfFlashCheckIntervalSeconds;
	float mfFlashEvidenceSeconds;
	float mfFlashAlpha;
	float mfFlashDirection;
	bool mbHasBeenFlashed;
	float mfEnterFlashDist;
	float mfExitFlashDist;

	bool mbSkipRayCheck;

	cGameItem_InViewRay mRayCallback;

	tString msHudModelFile;
	tString msHudModelName;

	tString msPickUpSound;
};

class cEntityLoader_GameItem : public cEntityLoader_Object
{

public:
	cEntityLoader_GameItem(const tString &asName, cInit *apInit);
	~cEntityLoader_GameItem() override;

	static eGameItemType ToItemType(const char* apString);
private:
	void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;
	void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

	cInit *mpInit;
};


#endif // GAME_GAME_ITEM_H
