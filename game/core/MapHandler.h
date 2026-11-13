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
#ifndef GAME_MAP_HANDLER_H
#define GAME_MAP_HANDLER_H

#include "core/StdAfx.h"

// Linux X11 Defines This
#undef DestroyAll

using namespace hpl;

class cInit;
class iGameEntity;
class iGameEnemy;
class cMapHandler_GlobalSave;
class cGameStickArea;
class cSavedWorld;
class cGameTimer;
class cGameItem;

typedef std::multimap<tString,iGameEntity*> tGameEntityMap;
typedef tGameEntityMap::iterator tGameEntityMapIt;

typedef std::vector<iGameEntity*> tGameEntityVec;
typedef tGameEntityVec::iterator tGameEntityVecIt;

typedef std::list<iGameEnemy*> tGameEnemyList;
typedef tGameEnemyList::iterator tGameEnemyListIt;

typedef std::list<cGameItem*> tGameItemList;
typedef tGameItemList::iterator tGameItemListIt;

typedef std::list<cGameStickArea*> tGameStickAreaList;
typedef tGameStickAreaList::iterator tGameStickAreaListIt;

typedef cSTLIterator<iGameEnemy*,tGameEnemyList,tGameEnemyListIt> tGameEnemyIterator;
typedef cSTLMapIterator<iGameEntity*,tGameEntityMap,tGameEntityMapIt> tGameEntityIterator;

class cMapChanger
{
public:
	bool mbActive;
	tString msNewMap;
	tString msPosName;
	tString msDoneSound;
	float mfFadeInTime;
	tString msLoadTextCat;
	tString msLoadTextEntry;
	// Black Plague: the picture shown while loading
	tString msLoadImage;
};

class cWorldCache
{
public:
	cWorldCache(cInit *apInit);
	~cWorldCache();

	void AddResources();
	void DecResources();

	int GetCount(){ return mlCount;}

private:
	cInit *mpInit;
	cResources *mpResources;

	int mlCount;

	tResourceBaseList mlstMaterials;
	tResourceBaseList mlstMeshes;
	tResourceBaseList mlstAnimations;
};

class cLoadedMap
{
public:
	tString msName;
	double mfTime;
};

typedef std::list<cGameTimer*> tGameTimerList;
typedef tGameTimerList::iterator tGameTimerListIt;

class cEffectLightFlash
{
public:
	cEffectLightFlash(cInit *apInit,const cVector3f& avPos,float afRadius, const cColor &aColor, float afFadeInSeconds, float afFadeOutSeconds);
	~cEffectLightFlash();

	void Update(float afTimeStep);

	bool IsDead(){return mbDead;}

private:
	cInit *mpInit;

	iLight3D *mpLight;

	float mfRadius;
	float mfFadeOutSeconds;

	bool mbIsDying;

	bool mbDead;
};

typedef std::list<cEffectLightFlash*> tEffectLightFlashList;
typedef tEffectLightFlashList::iterator tEffectLightFlashListIt;

class cMapHandlerSoundCallback : public iSoundEntityGlobalCallback
{
public:
	cMapHandlerSoundCallback(cInit *apInit);

	void OnStart(cSoundEntity *apSoundEntity) override;

private:
	cInit *mpInit;
	tStringVec mvEnemyHearableSounds;
};

class cMapHandler : public iUpdateable
{
friend class cMapHandlerSoundCallback;
public:
	cMapHandler(cInit *apInit);
	~cMapHandler() override;

	bool Load(const tString &asFile,const tString& asStartPos);
	bool LoadSimple(const tString &asFile, bool abLoadEntities);

	void ChangeMap(const tString &asMap, const tString &asPos, const tString &asStartSound,
					const tString &asStopSound, float afFadeOutTime, float afFadeInTime,
					tString asLoadTextCat, tString asLoadTextEntry,
					tString asLoadImage = "");

	void AddSaveData(cSavedWorld* apSavedWorld);
	void LoadSaveData(cSavedWorld* apSavedWorld);

	void DestroyAll();
	void LiftBodiesOutOfFloors(cWorld3D *apWorld);
	void FreeSlidersFromStaticBodies(cWorld3D *apWorld);

	void OnWorldLoad();
	void OnStart() override;
	void OnPostSceneDraw() override;
	void Update(float afTimeStep) override;
	void Reset() override;
	void OnDraw() override;

	cWorldCache* GetWorldCache(){ return mpWorldCache;}

	cGameTimer * CreateTimer(const tString& asName,float afTime, const tString& asCallback, bool abGlobal);
	cGameTimer * GetTimer(const tString& asName);
	// Stops (or restarts) the current map's timers, e.g. for a tool that
	// wants no scripted events
	void SetLocalTimersPaused(bool abPaused);

	void AddLightFlash(const cVector3f& avPos,float afRadius, const cColor &aColor, float afFadeInSeconds, float afFadeOutSeconds);

	void AddGameEnemy(iGameEnemy *apEnemy);
	tGameEnemyIterator GetGameEnemyIterator();

	void AddGameItem(cGameItem *apItem);
	void RemoveGameItem(cGameItem *apItem);

	void AddStickArea(cGameStickArea *apArea);
	void RemoveStickArea(cGameStickArea *apArea);
	cGameStickArea* GetBodyStickArea(iPhysicsBody *apBody);

	void AddGameEntity(iGameEntity *apEntity);
	void RemoveGameEntity(iGameEntity *apEntity);
	void DestroyGameEntityNow(iGameEntity *apEntity);
	iGameEntity* GetGameEntity(const tString &asName, bool abErrorMessage=true);
	tGameEntityIterator GetGameEntityIterator();

	iGameEntity* GetLatestEntity();
	const tString& GetCurrentMapName(){ return msCurrentMap;}
	void SetCurrentMapName(const tString& asName){ msCurrentMap= asName;}
	double GetGameTime(){ return mfGameTime;}
	bool IsPreUpdating(){ return mbPreUpdating;}

	void SetMapGameName(const tWString &asName){ msMapGameName = asName;}
	tWString GetMapGameName(){ return msMapGameName;}

	bool IsDestroyingAll(){ return mbDestroyingAll;}

	bool IsChangingMap(){ return mMapChanger.mbActive;}

	/**
	 * Loads the current map again from its files at the next update, as on a
	 * first visit (its saved state is not used), and puts the player back
	 * where they are. Meshes, materials and textures only the map used are
	 * read again, and files added to the resource folders since are found.
	 * For editing (docs/modding.md).
	 */
	void RequestReload() { mbReloadRequested = true; }
	bool IsReloadRequested() { return mbReloadRequested; }
	// A random seed set before each reload (>= 0), so that a reloaded map
	// starts the same each time (cLiveControl's fixed steps); -1 for none
	void SetReloadSeed(int alSeed) { mlReloadSeed = alSeed; }
	// Counts the maps loaded, reloads too: what was read from a map's file
	// is out of date when this changes
	int GetLoadCount() { return mlLoadCount; }
	// Counts the reloads that kept the loaded map (its file can't be read)
	int GetReloadFailCount() { return mlReloadFailCount; }
	// The level editor holds the map still: no timers, no game entity
	// updates (the world's update is cScene::SetUpdateMap)
	void SetPaused(bool abX) { mbPaused = abX; }
	bool IsPaused() { return mbPaused; }

	//Saving
	void SaveToGlobal(cMapHandler_GlobalSave *apSave);
	void LoadFromGlobal(cMapHandler_GlobalSave *apSave);

private:
	void RenderItemEffect();

	void PrintSoundsPlaying();

	void PreUpdate(double afSecondsSinceVisit);

	double AddLoadedMap(cWorld3D *apWorld);

	void Reload();

	void UpdateTimers(float afTimeStep);
	void RemoveLocalTimers();

	double mfGameTime;

	cInit *mpInit;

    cScene *mpScene;
	cResources *mpResources;

	iTexture *mpMapChangeTexture;

	//The flash on items of materials without programs of their own
	iGpuProgram *mpFlashProgram;

	cMapChanger mMapChanger;
	cWorldCache *mpWorldCache;
	bool mbReloadRequested;
	bool mbPaused;
	bool mbReloading;
	int mlLoadCount;
	int mlReloadFailCount;
	int mlReloadSeed;

	iGameEntity* mpLatestEntity;
	tString msCurrentMap;
	tWString msMapGameName;

	std::vector<cLoadedMap> mvLoadedMaps;

	tGameTimerList mlstTimers;

	cMapHandlerSoundCallback* mpSoundCallback;

	tGameEntityMap m_mapGameEntities;
	tGameEnemyList mlstGameEnemies;
	tGameItemList mlstGameItems;
	tEffectLightFlashList mlstLightFlashes;
	tGameStickAreaList mlstGameStickAreas;

	bool mbPreUpdating;

	bool mbDestroyingAll;
};


#endif // GAME_MAP_HANDLER_H
