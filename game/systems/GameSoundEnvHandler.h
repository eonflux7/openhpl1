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
#ifndef GAME_GAME_SOUND_ENV_HANDLER_H
#define GAME_GAME_SOUND_ENV_HANDLER_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//////////////////////////////////////////////////////////////////////////
// Black Plague: the reverb (EFX sound environment, soundenvs/*.snv) of the
// sector the player is in. Where sectors overlap the highest priority wins;
// outside all of them the map's default is heard. Written after the binary.
//////////////////////////////////////////////////////////////////////////

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

	float mfFadeCount;
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
