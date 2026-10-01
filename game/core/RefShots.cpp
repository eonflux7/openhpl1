/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/RefShots.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdarg>

// Game time a map may take to load before the run gives up on it
static const float kfMapTimeout = 60.0f;

// Logic steps to wait: after a map has loaded (fades, start scripts), after
// the player has been moved to a start position, and after each turn (the
// hands and the flashlight follow the camera with a delay).
static const int klMapSettleFrames = 90;
static const int klStartSettleFrames = 30;
static const int klTurnSettleFrames = 10;

static const int klAngles = 4;

// Lights per map to look around from
static const size_t klMaxLightViews = 6;

static const int klRandomSeed = 1;

//-----------------------------------------------------------------------

cRefShots::cRefShots(cInit *apInit, const tString &asSpec) : iUpdateable("RefShots")
{
	mpInit = apInit;
	mlMap = 0;
	mlView = 0;
	mlAngle = 0;
	mfStartYaw = 0;
	mStep = eStep_WaitForMap; // cInit loads the first map itself
	mlFrame = 0;
	mlShots = 0;
	mlFailures = 0;

	const char *pDir = SDL_getenv("OVERTURE_REFSHOTS_DIR");
	msDir = pDir && pDir[0] ? pDir : ".";

	// The same frames every run: one logic step per drawn frame, and the
	// same random numbers. Radio subtitles are off, they move on when a
	// voice clip ends, which is in real time. The settings are put back
	// before the game saves them.
	mbPrevLimitFPS = mpInit->mpGame->GetLimitFPS();
	mbPrevSubtitles = mpInit->mbSubtitles;
	mpInit->mpGame->GetLogicTimer()->SetFixedStep(true);
	mpInit->mpGame->SetLimitFPS(true);
	mpInit->mbSubtitles = false;
	cMath::Randomize(klRandomSeed);

	// Only the 3D view: no fades, flashes, damage or hiding effects, HUD or
	// menus. They show the game's state more than the renderer's.
	mpInit->mpGame->GetGraphics()->GetDrawer()->SetSkipDrawing(true);

	// Enemies, falls and water must not end the run.
	mpInit->mpPlayer->SetGodMode(true);

	tWStringList lstFiles;
	mpInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstFiles, _W("maps"), _W("*.dae"));
	tString sTestMapDir = tString(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "testmap";
	mpInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstFiles,
		cString::To16Char(sTestMapDir), _W("*.dae"));

	// Case does not matter: the files are level01_07_lead_mine_shafts_A.dae
	// and the like.
	tStringVec vFilters;
	if(asSpec != "1" && asSpec != "all")
		cString::GetStringVec(cString::ToLowerCase(asSpec), vFilters, nullptr);

	for(tWStringListIt it = lstFiles.begin(); it != lstFiles.end(); ++it)
	{
		tString sFile = cString::To8Char(*it);
		tString sLower = cString::ToLowerCase(sFile);
		bool bMatch = vFilters.empty();
		for(size_t i = 0; i < vFilters.size(); ++i)
			if(sLower.find(vFilters[i]) != tString::npos) bMatch = true;

		if(bMatch) mvMaps.push_back(sFile);
	}
	std::sort(mvMaps.begin(), mvMaps.end());

	Log("REFSHOTS: %d maps, frames to '%s'\n", (int)mvMaps.size(), msDir.c_str());
	if(mvMaps.empty()) Fail("no maps matched '%s'", asSpec.c_str());
}

//-----------------------------------------------------------------------

bool cRefShots::IsRequested()
{
	return GetSpec() != "";
}

tString cRefShots::GetSpec()
{
	const char *pSpec = SDL_getenv("OVERTURE_REFSHOTS");
	return pSpec ? tString(pSpec) : tString("");
}

//-----------------------------------------------------------------------

const tString& cRefShots::GetFirstMap()
{
	static const tString sNone = "";
	return mvMaps.empty() ? sNone : mvMaps[0];
}

//-----------------------------------------------------------------------

void cRefShots::Update(float afTimeStep)
{
	if(mvMaps.empty())
	{
		Finish();
		return;
	}

	// Cutscene and text screens run in their own updater containers; the
	// run is in all of them but only drives the game from Default.
	if(mpInit->mpGame->GetUpdater()->GetCurrentContainerName() != "Default")
	{
		if(++mlFrame * afTimeStep > kfMapTimeout)
		{
			Fail("stuck in container '%s'",
				mpInit->mpGame->GetUpdater()->GetCurrentContainerName().c_str());
			mpInit->mpGame->GetUpdater()->SetContainer("Default");
			NextMap();
		}
		return;
	}

	// The maps are dark without it. Scripts disable it for a while (the
	// start of level01_01_outside); jumping between maps can skip the part
	// that enables it again.
	cPlayerFlashLight *pFlashLight = mpInit->mpPlayer->GetFlashLight();
	if(mpInit->mpPlayer->GetPower() < 100) mpInit->mpPlayer->SetPower(100);
	if(pFlashLight->IsActive()==false && mStep != eStep_Done)
	{
		pFlashLight->SetDisabled(false);
		pFlashLight->SetActive(true);
	}

	// Flickering lights draw random numbers, and so do sounds when their
	// playback ends, which is in real time: the flicker would differ from run
	// to run. Keep them on. Scripts can start flickering at any time.
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld && mStep != eStep_Done)
	{
		cLight3DListIterator lightIt = pWorld->GetLightIterator();
		while(lightIt.HasNext())
		{
			iLight3D *pLight = lightIt.Next();
			if(pLight->GetFlickerActive()==false) continue;
			pLight->SetFlickerActive(false);
			pLight->FadeTo(pLight->GetFlickerOnColor(), pLight->GetFlickerOnRadius(), 0);
		}
	}

	// Scripts can still kill the player
	if(mpInit->mpPlayer->IsDead() && mStep != eStep_Done && msDeadOn != mvMaps[mlMap])
	{
		msDeadOn = mvMaps[mlMap];
		Fail("the player died on %s", mvMaps[mlMap].c_str());
	}

	tString sMap = mStep == eStep_Done ? "" :
		cString::ToLowerCase(cString::SetFileExt(mvMaps[mlMap], ""));

	switch(mStep)
	{
	case eStep_ChangeMap:
		mpInit->mpMapHandler->ChangeMap(mvMaps[mlMap], "", "", "", 0, 0, "", "");
		mStep = eStep_WaitForMap;
		mlFrame = 0;
		break;

	case eStep_WaitForMap:
		if(mpInit->mpMapHandler->GetCurrentMapName() == sMap)
		{
			// What a map shows must not depend on the maps before it.
			cMath::Randomize(klRandomSeed);

			auto ByName = [](const cView &a, const cView &b){ return a.msName < b.msName; };
			auto SameName = [](const cView &a, const cView &b){ return a.msName == b.msName; };

			mvViews.clear();
			tStartPosEntityList *pStarts = pWorld->GetStartPosEntityList();
			for(tStartPosEntityListIt it = pStarts->begin(); it != pStarts->end(); ++it)
				mvViews.push_back({(*it)->GetName(), true, cVector3f(0,0,0)});
			std::sort(mvViews.begin(), mvViews.end(), ByName);
			mvViews.erase(std::unique(mvViews.begin(), mvViews.end(), SameName), mvViews.end());
			size_t lStarts = mvViews.size();

			// The lights the map file has, spread evenly over their names.
			// Scripts and entities add lights too, those are left out.
			std::vector<cView> vLights;
			cLight3DListIterator lightIt = pWorld->GetLightIterator();
			while(lightIt.HasNext())
			{
				iLight3D *pLight = lightIt.Next();
				if(pLight->GetName() == "" || pLight->GetEntityParent() != nullptr) continue;
				// Made by the game for the hiding effect, not part of the map
				if(pLight->GetName() == "HiddenLight") continue;
				vLights.push_back({"light_" + pLight->GetName(), false, pLight->GetWorldPosition()});
			}
			std::sort(vLights.begin(), vLights.end(), ByName);
			vLights.erase(std::unique(vLights.begin(), vLights.end(), SameName), vLights.end());
			size_t lLightViews = std::min(vLights.size(), klMaxLightViews);
			for(size_t i = 0; i < lLightViews; ++i)
				mvViews.push_back(vLights[i * vLights.size() / lLightViews]);

			Log("REFSHOTS: [%d/%d] %s, %d start positions, %d of %d lights\n", (int)mlMap + 1,
				(int)mvMaps.size(), mvMaps[mlMap].c_str(), (int)lStarts, (int)lLightViews,
				(int)vLights.size());
			if(mvViews.empty())
			{
				Fail("%s has no start positions or lights", mvMaps[mlMap].c_str());
				NextMap();
				break;
			}

			mlView = 0;
			mStep = eStep_SettleMap;
			mlFrame = 0;
		}
		else if(++mlFrame * afTimeStep > kfMapTimeout)
		{
			Fail("%s did not load", mvMaps[mlMap].c_str());
			NextMap();
		}
		break;

	case eStep_SettleMap:
		if(++mlFrame < klMapSettleFrames) break;

		mfStartYaw = 0;
		if(mvViews[mlView].mbStartPos)
		{
			mpInit->mpPlayer->SetStartPos(mvViews[mlView].msName);
			mfStartYaw = mpInit->mpPlayer->GetCamera()->GetYaw();
		}
		mlAngle = 0;
		SetView();
		mStep = eStep_SettleStart;
		mlFrame = 0;
		break;

	case eStep_SettleStart:
		SetView();
		if(++mlFrame < klStartSettleFrames) break;

		mStep = eStep_Shoot;
		mlFrame = klTurnSettleFrames; // already facing the first direction
		[[fallthrough]];

	case eStep_Shoot:
	{
		// Scripts and physics may turn the camera; keep it where it should be.
		SetView();
		if(++mlFrame < klTurnSettleFrames) break;

		char sAngle[16];
		snprintf(sAngle, sizeof(sAngle), "%03d", mlAngle * (360 / klAngles));
		tString sFile = msDir + "/" + sMap + "-" + mvViews[mlView].msName + "-" + sAngle + ".png";

		// This step's frame is drawn after the update, then saved.
		mpInit->mpGame->GetGraphics()->GetLowLevel()->SaveNextFrameToPNG(sFile);
		mlShots++;

		mlFrame = 0;
		mlAngle++;
		if(mlAngle >= klAngles) NextView();
		break;
	}

	case eStep_Done:
		break;
	}
}

//-----------------------------------------------------------------------

void cRefShots::Reset()
{
	// Changing map resets every updateable; the run keeps its place.
}

//-----------------------------------------------------------------------

void cRefShots::Fail(const char *asFormat, ...)
{
	char sMess[1024];
	va_list ap;
	va_start(ap, asFormat);
	vsnprintf(sMess, sizeof(sMess), asFormat, ap);
	va_end(ap);

	Error("REFSHOTS: FAIL %s\n", sMess);
	mlFailures++;
}

//-----------------------------------------------------------------------

void cRefShots::SetView()
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	pCamera->SetYaw(mfStartYaw + mlAngle * (k2Pif / klAngles));
	pCamera->SetPitch(0);
	if(mvViews[mlView].mbStartPos)
		mpInit->mpPlayer->GetCharacterBody()->SetYaw(pCamera->GetYaw());
	else
		// The player has put the camera on its head earlier in this step.
		pCamera->SetPosition(mvViews[mlView].mvPos);
}

//-----------------------------------------------------------------------

void cRefShots::NextView()
{
	mlFrame = 0;
	mlView++;
	if(mlView < mvViews.size())
	{
		// No map settle time for the later ones
		mStep = eStep_SettleMap;
		mlFrame = klMapSettleFrames;
		return;
	}
	NextMap();
}

//-----------------------------------------------------------------------

void cRefShots::NextMap()
{
	mlFrame = 0;
	mlMap++;
	if(mlMap < mvMaps.size())
	{
		mStep = eStep_ChangeMap;
		return;
	}

	Log("REFSHOTS: done, %d frames, %d failure(s)\n", mlShots, mlFailures);
	Finish();
}

//-----------------------------------------------------------------------

void cRefShots::Finish()
{
	mStep = eStep_Done;
	mpInit->mpGame->GetLogicTimer()->SetFixedStep(false);
	mpInit->mpGame->SetLimitFPS(mbPrevLimitFPS);
	mpInit->mbSubtitles = mbPrevSubtitles;
	mpInit->mpGame->GetGraphics()->GetDrawer()->SetSkipDrawing(false);
	mpInit->mpGame->Exit();
}

//-----------------------------------------------------------------------
