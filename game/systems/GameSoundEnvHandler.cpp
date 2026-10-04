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
#include "core/StdAfx.h"
#include "systems/GameSoundEnvHandler.h"

#include "core/Init.h"
#include "player/Player.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameSoundEnvHandler::cGameSoundEnvHandler(cInit *apInit) : iUpdateable("GameSoundEnvHandler")
{
	mpInit = apInit;
	mpLowLevelSound = mpInit->mpGame->GetSound()->GetLowLevel();

	mfFadeCount = 0;
	mfFadeTime = 0;
	mfDefaultFadeTime = 0;
	mlCurrentPrio = -1;
	mbFading = false;
	mpCurrentSector = nullptr;

	mpCurrentEnv = nullptr;
	mpNewEnv = nullptr;
	mpDefaultEnv = nullptr;
}

cGameSoundEnvHandler::~cGameSoundEnvHandler()
{
	STLDeleteAll(mlstSectorEnvs);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::OnStart()
{
}

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::Update(float afTimeStep)
{
	if(mpLowLevelSound->IsEnvAudioAvailable()==false) return;

	////////////////////////////
	// Fading to the new one
	if(mbFading)
	{
		if(mpCurrentEnv == mpNewEnv)
		{
			mbFading = false;
			return;
		}

		if(mfFadeCount < mfFadeTime)
		{
			mfFadeCount += afTimeStep;
			mpLowLevelSound->FadeSoundEnvironment(mpCurrentEnv, mpNewEnv, mfFadeCount / mfFadeTime);
		}
		else
		{
			mfFadeCount = mfFadeTime;
			mpCurrentEnv = mpNewEnv;
			mpLowLevelSound->SetSoundEnvironment(mpNewEnv);
			mbFading = false;

			Log("Sound environment: '%s'\n", mpNewEnv ? mpNewEnv->GetFileName().c_str() : "none");
		}
		return;
	}

	////////////////////////////
	// The sector of highest priority the player is in
	cVector3f vPos = mpInit->mpPlayer->GetCamera()->GetPosition();

	cSectorSoundEnv *pBest = nullptr;
	int lBestPrio = -1;
	for(tSectorSoundEnvListIt it = mlstSectorEnvs.begin(); it != mlstSectorEnvs.end(); ++it)
	{
		cSectorSoundEnv *pEnv = *it;
		if(pEnv->mlPrio < lBestPrio) continue;

		if(cMath::PointBVCollision(vPos, *pEnv->mpSector->GetBV()))
		{
			pBest = pEnv;
			lBestPrio = pEnv->mlPrio;
		}
	}

	if(pBest)
	{
		if(pBest->mpSector == mpCurrentSector) return;

		mbFading = true;
		mpCurrentSector = pBest->mpSector;
		mfFadeCount = 0;
		mpNewEnv = pBest->mpSoundEnv;
		mfFadeTime = pBest->mfFadeTime;
		mlCurrentPrio = pBest->mlPrio;
	}
	////////////////////////////
	// Outside all: the default
	else if(mpCurrentEnv != mpDefaultEnv)
	{
		mbFading = true;
		mpCurrentSector = nullptr;
		mfFadeCount = 0;
		mpNewEnv = mpDefaultEnv;
		mfFadeTime = mfDefaultFadeTime;
		mlCurrentPrio = -1;
	}
}

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::Reset()
{
	mpLowLevelSound->SetSoundEnvironment(nullptr);

	mfFadeCount = 0;
	mfFadeTime = 0;
	mfDefaultFadeTime = 0;
	mlCurrentPrio = -1;
	mbFading = false;
	mpCurrentSector = nullptr;

	mpCurrentEnv = nullptr;
	mpNewEnv = nullptr;
	mpDefaultEnv = nullptr;

	STLDeleteAll(mlstSectorEnvs);
}

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::OnWorldLoad()
{
	mpLowLevelSound->SetSoundEnvironment(nullptr);

	// The fade under way when the last map was left is not lost
	if(mbFading) mpLowLevelSound->SetSoundEnvironment(mpNewEnv);
}

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::OnWorldExit()
{
	mpDefaultEnv = nullptr;
	mfDefaultFadeTime = 1;

	if(mbFading)
	{
		mfFadeCount = mfFadeTime;
		mpCurrentEnv = mpNewEnv;
		mpLowLevelSound->SetSoundEnvironment(mpNewEnv);
		mbFading = false;
	}

	// The sectors are those of the map left
	mpCurrentSector = nullptr;
	STLDeleteAll(mlstSectorEnvs);
}

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::AddSectorSoundEnv(cSector *apSector, iSoundEnvironment *apSoundEnv, float afFadeTime, int alPrio)
{
	if(afFadeTime < 0 || apSector == nullptr) return;

	for(tSectorSoundEnvListIt it = mlstSectorEnvs.begin(); it != mlstSectorEnvs.end(); ++it)
	{
		cSectorSoundEnv *pEnv = *it;
		if(pEnv->mpSector != apSector) continue;

		pEnv->mpSoundEnv = apSoundEnv;
		pEnv->mfFadeTime = afFadeTime;
		pEnv->mlPrio = alPrio;
		return;
	}

	cSectorSoundEnv *pEnv = hplNew( cSectorSoundEnv, () );
	pEnv->mpSector = apSector;
	pEnv->mpSoundEnv = apSoundEnv;
	pEnv->mfFadeTime = afFadeTime;
	pEnv->mlPrio = alPrio;
	mlstSectorEnvs.push_back(pEnv);
}

//-----------------------------------------------------------------------

void cGameSoundEnvHandler::SetDefaultSoundEnv(iSoundEnvironment *apSoundEnv, float afFadeTime)
{
	mpDefaultEnv = apSoundEnv;
	mfDefaultFadeTime = afFadeTime;
}

//-----------------------------------------------------------------------

iSoundEnvironment* cGameSoundEnvHandler::GetSoundEnv(const tString &asFile)
{
	if(mpLowLevelSound->IsEnvAudioAvailable()==false) return nullptr;

	// The scripts give the path from the game's folder: "soundenvs/x.snv"
	return mpLowLevelSound->LoadSoundEnvironment(asFile);
}

//-----------------------------------------------------------------------
