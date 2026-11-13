// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_SOUND_ENV_HANDLER_H
#define GAME_GAME_SOUND_ENV_HANDLER_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;
class cSavedWorld;

// Black Plague: the reverb (EFX sound environment, soundenvs/*.snv) of the
// sector the player is in. Where sectors overlap the highest priority wins;
// outside all of them the map's default is heard. Written after the binary.

class cSectorSoundEnv
{
public:
	cSector *mpSector;
	iSoundEnvironment *mpSoundEnv;
	float mfFadeTime;
	int mlPrio;
};

typedef std::list<cSectorSoundEnv*> tSectorSoundEnvList;
typedef tSectorSoundEnvList::iterator tSectorSoundEnvListIt;

class cGameSoundEnvHandler : public iUpdateable
{
public:
	cGameSoundEnvHandler(cInit *apInit);
	~cGameSoundEnvHandler() override;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;

	void OnWorldLoad();
	void OnWorldExit();

	void AddSaveData(cSavedWorld *apSavedWorld);
	void LoadSaveData(cSavedWorld *apSavedWorld);

	/**
	 * afFadeTime: seconds to fade into it from the one before
	 */
	void AddSectorSoundEnv(cSector *apSector, iSoundEnvironment *apSoundEnv, float afFadeTime, int alPrio);
	void SetDefaultSoundEnv(iSoundEnvironment *apSoundEnv, float afFadeTime);

	/**
	 * Loads soundenvs/x.snv, or gives the one loaded before. nullptr without EFX.
	 */
	iSoundEnvironment* GetSoundEnv(const tString &asFile);

	bool IsFading(){ return mbFading;}
	int GetPrio(){ return mlCurrentPrio;}

private:
	cInit *mpInit;
	iLowLevelSound *mpLowLevelSound;

	float mfFadeElapsedSeconds;
	float mfFadeTime;
	float mfDefaultFadeTime;
	int mlCurrentPrio;
	bool mbFading;
	cSector *mpCurrentSector;

	iSoundEnvironment *mpCurrentEnv;
	iSoundEnvironment *mpNewEnv;
	iSoundEnvironment *mpDefaultEnv;

	tSectorSoundEnvList mlstSectorEnvs;
};

#endif // GAME_GAME_SOUND_ENV_HANDLER_H
