// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "systems/GameSoundEnvHandler.h"

#include "core/Init.h"
#include "core/SaveHandler.h"
#include "player/Player.h"

cGameSoundEnvHandler::cGameSoundEnvHandler(cInit *apInit) : iUpdateable("GameSoundEnvHandler")
{
	mpInit = apInit;
	mpLowLevelSound = mpInit->mpGame->GetSound()->GetLowLevel();

	mfFadeElapsedSeconds = 0;
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

void cGameSoundEnvHandler::OnStart()
{
}

void cGameSoundEnvHandler::Update(float afTimeStep)
{
	if(mpLowLevelSound->IsEnvAudioAvailable()==false) return;

	// Fading to the new one
	if(mbFading)
	{
		if(mpCurrentEnv == mpNewEnv)
		{
			mbFading = false;
			return;
		}

		if(mfFadeElapsedSeconds < mfFadeTime)
		{
			mfFadeElapsedSeconds += afTimeStep;
			mpLowLevelSound->FadeSoundEnvironment(mpCurrentEnv, mpNewEnv, mfFadeElapsedSeconds / mfFadeTime);
		}
		else
		{
			mfFadeElapsedSeconds = mfFadeTime;
			mpCurrentEnv = mpNewEnv;
			mpLowLevelSound->SetSoundEnvironment(mpNewEnv);
			mbFading = false;

			Log("Sound environment: '%s'\n", mpNewEnv ? mpNewEnv->GetFileName().c_str() : "none");
		}
		return;
	}

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
		mfFadeElapsedSeconds = 0;
		mpNewEnv = pBest->mpSoundEnv;
		mfFadeTime = pBest->mfFadeTime;
		mlCurrentPrio = pBest->mlPrio;
	}
	// Outside all: the default
	else if(mpCurrentEnv != mpDefaultEnv)
	{
		mbFading = true;
		mpCurrentSector = nullptr;
		mfFadeElapsedSeconds = 0;
		mpNewEnv = mpDefaultEnv;
		mfFadeTime = mfDefaultFadeTime;
		mlCurrentPrio = -1;
	}
}

void cGameSoundEnvHandler::Reset()
{
	mpLowLevelSound->SetSoundEnvironment(nullptr);

	mfFadeElapsedSeconds = 0;
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

void cGameSoundEnvHandler::OnWorldLoad()
{
	mpLowLevelSound->SetSoundEnvironment(nullptr);

	// The fade under way when the last map was left is not lost
	if(mbFading) mpLowLevelSound->SetSoundEnvironment(mpNewEnv);
}

void cGameSoundEnvHandler::OnWorldExit()
{
	mpDefaultEnv = nullptr;
	mfDefaultFadeTime = 1;

	if(mbFading)
	{
		mfFadeElapsedSeconds = mfFadeTime;
		mpCurrentEnv = mpNewEnv;
		mpLowLevelSound->SetSoundEnvironment(mpNewEnv);
		mbFading = false;
	}

	// The sectors are those of the map left
	mpCurrentSector = nullptr;
	STLDeleteAll(mlstSectorEnvs);
}

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

	cSectorSoundEnv *pEnv = new cSectorSoundEnv();
	pEnv->mpSector = apSector;
	pEnv->mpSoundEnv = apSoundEnv;
	pEnv->mfFadeTime = afFadeTime;
	pEnv->mlPrio = alPrio;
	mlstSectorEnvs.push_back(pEnv);
}

void cGameSoundEnvHandler::SetDefaultSoundEnv(iSoundEnvironment *apSoundEnv, float afFadeTime)
{
	mpDefaultEnv = apSoundEnv;
	mfDefaultFadeTime = afFadeTime;
}

void cGameSoundEnvHandler::AddSaveData(cSavedWorld *apSavedWorld)
{
	apSavedWorld->msDefaultSoundEnv = mpDefaultEnv ? mpDefaultEnv->GetFileName() : "";
	apSavedWorld->mfDefaultSoundEnvFadeTime = mfDefaultFadeTime;

	apSavedWorld->mlstSoundEnvs.Clear();
	for(cSectorSoundEnv *pSectorEnv : mlstSectorEnvs)
	{
		cSaveWorldSoundEnv savedEnv;
		savedEnv.msSector = pSectorEnv->mpSector ? pSectorEnv->mpSector->GetId() : "";
		savedEnv.msSoundEnv = pSectorEnv->mpSoundEnv ? pSectorEnv->mpSoundEnv->GetFileName() : "";
		savedEnv.mfFadeTime = pSectorEnv->mfFadeTime;
		savedEnv.mlPrio = pSectorEnv->mlPrio;
		apSavedWorld->mlstSoundEnvs.Add(savedEnv);
	}
}

void cGameSoundEnvHandler::LoadSaveData(cSavedWorld *apSavedWorld)
{
	cPortalContainer *pContainer = mpInit->mpGame->GetScene()->GetWorld3D()->GetPortalContainer();

	mpDefaultEnv = apSavedWorld->msDefaultSoundEnv=="" ? nullptr : GetSoundEnv(apSavedWorld->msDefaultSoundEnv);
	mfDefaultFadeTime = apSavedWorld->mfDefaultSoundEnvFadeTime;

	cContainerListIterator<cSaveWorldSoundEnv> it = apSavedWorld->mlstSoundEnvs.GetIterator();
	while(it.HasNext())
	{
		cSaveWorldSoundEnv &savedEnv = it.Next();

		cSectorSoundEnv *pSectorEnv = new cSectorSoundEnv();
		pSectorEnv->mpSoundEnv = savedEnv.msSoundEnv=="" ? nullptr : GetSoundEnv(savedEnv.msSoundEnv);
		pSectorEnv->mpSector = savedEnv.msSector=="" ? nullptr : pContainer->GetSector(savedEnv.msSector);
		pSectorEnv->mfFadeTime = savedEnv.mfFadeTime;
		pSectorEnv->mlPrio = savedEnv.mlPrio;
		mlstSectorEnvs.push_back(pSectorEnv);
	}
}

iSoundEnvironment* cGameSoundEnvHandler::GetSoundEnv(const tString &asFile)
{
	if(mpLowLevelSound->IsEnvAudioAvailable()==false) return nullptr;

	// The scripts give the path from the game's folder: "soundenvs/x.snv"
	return mpLowLevelSound->LoadSoundEnvironment(asFile);
}
