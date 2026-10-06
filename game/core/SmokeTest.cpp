/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/SmokeTest.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/SaveHandler.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdarg>

// Game time a map may take to load before the test gives up on it
static const float kfMapTimeout = 60.0f;

#ifdef OVERTURE_DOG_REGRESSIONS
int RunDogRegressions(cInit *apInit);
int RunDogArenaRegressions(cInit *apInit);
#endif

//-----------------------------------------------------------------------

cSmokeTest::cSmokeTest(cInit *apInit, const tString &asSpec) : iUpdateable("SmokeTest")
{
	mpInit = apInit;
	mlMap = 0;
	mStep = eStep_WaitForMap; // cInit loads the first map itself
	mlFrame = 0;
	mlFailures = 0;

	const char *pFrames = SDL_getenv("OVERTURE_SMOKE_FRAMES");
	mlFramesPerStep = pFrames ? cString::ToInt(pFrames, 120) : 120;

	tWStringList lstFiles;
	mpInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstFiles, cInit::GetMapsDir(), _W("*.dae"));
	tString sTestMapDir = tString(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "testmap";
	mpInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstFiles,
		cString::To16Char(sTestMapDir), _W("*.dae"));

	tStringVec vFilters;
	if(asSpec != "1" && asSpec != "all")
		cString::GetStringVec(asSpec, vFilters, nullptr);

	for(tWStringListIt it = lstFiles.begin(); it != lstFiles.end(); ++it)
	{
		tString sFile = cString::To8Char(*it);
		bool bMatch = vFilters.empty();
		for(size_t i = 0; i < vFilters.size(); ++i)
			if(sFile.find(vFilters[i]) != tString::npos) bMatch = true;

		if(bMatch) mvMaps.push_back(sFile);
	}
	std::sort(mvMaps.begin(), mvMaps.end());

	Log("SMOKE: %d maps, %d frames per step\n", (int)mvMaps.size(), mlFramesPerStep);
	if(mvMaps.empty()) Fail("no maps matched '%s'", asSpec.c_str());
}

//-----------------------------------------------------------------------

bool cSmokeTest::IsRequested()
{
	return GetSpec() != "";
}

tString cSmokeTest::GetSpec()
{
	const char *pSpec = SDL_getenv("OVERTURE_SMOKE");
	return pSpec ? tString(pSpec) : tString("");
}

//-----------------------------------------------------------------------

const tString& cSmokeTest::GetFirstMap()
{
	static const tString sNone = "";
	return mvMaps.empty() ? sNone : mvMaps[0];
}

//-----------------------------------------------------------------------

void cSmokeTest::Update(float afTimeStep)
{
	if(mvMaps.empty())
	{
		mpInit->mpGame->Exit();
		return;
	}

	// Cutscene and text screens run in their own updater containers; the
	// smoke test is in all of them but only drives the game from Default.
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
			Log("SMOKE: [%d/%d] %s\n", (int)mlMap + 1, (int)mvMaps.size(), mvMaps[mlMap].c_str());
			// Every Overture map has a level script; NULL means it failed to build
			if(mpInit->mpGame->GetScene()->GetWorld3D()->GetScript() == nullptr)
				Fail("%s: level script did not build", mvMaps[mlMap].c_str());
#ifdef OVERTURE_DOG_REGRESSIONS
			if(sMap == "testmap" && SDL_getenv("OVERTURE_DOG_REGRESSIONS"))
				mlFailures += RunDogRegressions(mpInit);
			if(sMap == "dogtest" && SDL_getenv("OVERTURE_DOG_REGRESSIONS"))
				mlFailures += RunDogArenaRegressions(mpInit);
#endif
			mStep = eStep_RunMap;
			mlFrame = 0;
		}
		else if(++mlFrame * afTimeStep > kfMapTimeout)
		{
			Fail("%s did not load", mvMaps[mlMap].c_str());
			NextMap();
		}
		break;

	case eStep_RunMap:
		if(++mlFrame < mlFramesPerStep) break;

		mpInit->mpSaveHandler->SaveGameToFile(SaveFile());
		mpInit->mpSaveHandler->LoadGameFromFile(SaveFile());
		mpInit->mpGame->ResetLogicTimer();

		if(mpInit->mpMapHandler->GetCurrentMapName() != sMap)
		{
			Fail("loading the save of %s gave '%s'", mvMaps[mlMap].c_str(),
				mpInit->mpMapHandler->GetCurrentMapName().c_str());
		}
		mStep = eStep_RunLoaded;
		mlFrame = 0;
		break;

	case eStep_RunLoaded:
		if(++mlFrame >= mlFramesPerStep) NextMap();
		break;

	case eStep_Done:
		break;
	}
}

//-----------------------------------------------------------------------

void cSmokeTest::Reset()
{
	// Loading a save resets every updateable; the test keeps its place.
}

//-----------------------------------------------------------------------

void cSmokeTest::Fail(const char *asFormat, ...)
{
	char sMess[1024];
	va_list ap;
	va_start(ap, asFormat);
	vsnprintf(sMess, sizeof(sMess), asFormat, ap);
	va_end(ap);

	Error("SMOKE: FAIL %s\n", sMess);
	mlFailures++;
}

//-----------------------------------------------------------------------

void cSmokeTest::NextMap()
{
	mlFrame = 0;
	mlMap++;
	if(mlMap < mvMaps.size())
	{
		mStep = eStep_ChangeMap;
		return;
	}

	mStep = eStep_Done;
	RemoveFile(mpInit->mpSaveHandler->GetSaveDir() + SaveFile());
	Log("SMOKE: done, %d failure(s)\n", mlFailures);
	mpInit->mpGame->Exit();
}

//-----------------------------------------------------------------------

tWString cSmokeTest::SaveFile()
{
	return _W("save/smoke_test.sav");
}

//-----------------------------------------------------------------------
