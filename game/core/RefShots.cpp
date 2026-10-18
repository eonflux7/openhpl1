/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/RefShots.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/LampAppearanceController.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/GameMessageHandler.h"
#include "ui/Inventory.h"
#include "ui/Credits.h"
#include "ui/DeathMenu.h"
#include "ui/IntroStory.h"
#include "ui/MainMenu.h"
#include "ui/Notebook.h"
#include "ui/NumericalPanel.h"

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

// Lights per map to look around from
static const size_t klMaxLightViews = 6;

static const int klRandomSeed = 1;

// The 2D screens, in order. Setup is run once, then the frame is saved
// after the given number of logic steps (for the fades to end).
struct c2DScreen
{
	const char *msName;
	int mlFrames;
	void (*mpSetup)(cInit *apInit);
};

static const c2DScreen kv2DScreens[] = {
	{"hud", 30, [](cInit *apInit){}},
	{"inventory", 90, [](cInit *apInit){
		apInit->mpInventory->AddItemFromFile("battery", "item_battery.ent", -1);
		apInit->mpInventory->AddItemFromFile("boltcutter", "item_boltcutter.ent", -1);
		apInit->mpInventory->AddItemFromFile("blank_note", "item_blank_note.ent", -1);
		apInit->mpPlayer->StartInventory();
	}},
	{"notebook", 90, [](cInit *apInit){
		apInit->mpNotebook->AddTask("refshots_task", _W("A task for the reference images"));
		apInit->mpNotebook->SetActive(true);
	}},
	{"notebook_tasks", 90, [](cInit *apInit){
		apInit->mpNotebook->GetStateMachine()->ChangeState(eNotebookState_TaskList);
	}},
	{"notebook_note", 90, [](cInit *apInit){
		cNotebook_Note *pNote = apInit->mpNotebook->AddNote(_W("A note for the reference images"),
															"Notes", "spiderhater");
		apInit->mpNotebook->SetSelectedNote(pNote);
		apInit->mpNotebook->GetStateMachine()->ChangeState(eNotebookState_Note);
	}},
	{"keypad", 90, [](cInit *apInit){
		apInit->mpNotebook->SetActive(false);
		apInit->mpNumericalPanel->SetActive(true);
	}},
	{"message", 90, [](cInit *apInit){
		apInit->mpNumericalPanel->SetActive(false);
		apInit->mpGameMessageHandler->Add(_W("A message for the reference images, long enough "
											"to take more than one line of text."));
	}},
	{"menu", 90, [](cInit *apInit){ apInit->mpMainMenu->SetActive(true); }},
	{"menu_options", 30, [](cInit *apInit){ apInit->mpMainMenu->SetState(eMainMenuState_Options); }},
	{"menu_graphics", 30, [](cInit *apInit){ apInit->mpMainMenu->SetState(eMainMenuState_OptionsGraphics); }},
	{"menu_advanced", 30, [](cInit *apInit){
		apInit->mpMainMenu->SetState(eMainMenuState_OptionsGraphicsAdvanced);
	}},
	{"menu_lighting", 30, [](cInit *apInit){
		apInit->mpMainMenu->SetState(eMainMenuState_OptionsGraphicsLighting);
	}},
	{"death", 240, [](cInit *apInit){
		apInit->mpMainMenu->SetActive(false);
		apInit->mpPlayer->SetGodMode(false);
		apInit->mpPlayer->SetHealth(0);
	}},
	{"intro", 180, [](cInit *apInit){
		apInit->mpDeathMenu->SetActive(false);
		apInit->mpIntroStory->SetActive(true);
	}},
	{"credits", 180, [](cInit *apInit){
		apInit->mpIntroStory->SetActive(false);
		apInit->mpCredits->SetActive(true);
	}},
};

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
	mb2D = false;
	ml2DScreen = 0;

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

	// Case does not matter: the files are level01_07_lead_mine_shafts_A.dae
	// and the like.
	tStringVec vFilters;
	if(asSpec != "1" && asSpec != "all")
		cString::GetStringVec(cString::ToLowerCase(asSpec), vFilters, nullptr);

	tStringVec vAllMaps;
	FindMaps(mpInit, vAllMaps);
	for(const tString &sFile : vAllMaps)
	{
		tString sLower = cString::ToLowerCase(sFile);
		bool bMatch = vFilters.empty();
		for(size_t i = 0; i < vFilters.size(); ++i)
			if(sLower.find(vFilters[i]) != tString::npos) bMatch = true;

		if(bMatch) mvMaps.push_back(sFile);
	}

	Log("REFSHOTS: %d maps, frames to '%s'\n", (int)mvMaps.size(), msDir.c_str());
	if(mvMaps.empty()) Fail("no maps matched '%s'", asSpec.c_str());
}

//-----------------------------------------------------------------------

void cRefShots::FindMaps(cInit *apInit, tStringVec &avMaps)
{
	tWStringList lstFiles;
	apInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstFiles, _W("maps"), _W("*.dae"));

	for(tWStringListIt it = lstFiles.begin(); it != lstFiles.end(); ++it)
		avMaps.push_back(cString::To8Char(*it));
	std::sort(avMaps.begin(), avMaps.end());
}

//-----------------------------------------------------------------------

tString cRefShots::GetViewsFile()
{
	const char *pFile = SDL_getenv("OVERTURE_REFVIEWS_FILE");
	if(pFile && pFile[0]) return pFile;
	return OVERTURE_REFVIEWS_FILE;
}

//-----------------------------------------------------------------------

void cRefShots::LoadViewsFile(tViewMap &aViews)
{
	aViews.clear();

	FILE *pFile = fopen(GetViewsFile().c_str(), "r");
	if(pFile==nullptr) return;

	char sLine[512];
	while(fgets(sLine, sizeof(sLine), pFile))
	{
		char sMap[128], sName[128];
		cView view;
		view.mbStartPos = false;
		view.mbManual = true;
		// Comments start with #
		if(sLine[0] == '#') continue;
		int lFlashLight = 1;
		if(sscanf(sLine, "%127s %127s %f %f %f %f %f %d", sMap, sName, &view.mvPos.x, &view.mvPos.y,
					&view.mvPos.z, &view.mfYaw, &view.mfPitch, &lFlashLight) < 7) continue;
		view.mbFlashLight = lFlashLight != 0;
		view.msName = sName;
		aViews[sMap].push_back(view);
	}
	fclose(pFile);
}

//-----------------------------------------------------------------------

bool cRefShots::SaveViewsFile(const tViewMap &aViews)
{
	FILE *pFile = fopen(GetViewsFile().c_str(), "w");
	if(pFile==nullptr)
	{
		Error("Couldn't write '%s'\n", GetViewsFile().c_str());
		return false;
	}

	fprintf(pFile, "# Viewpoints for the reference images (tools/refshots.py), set in the game's\n"
					"# F2 panel. map name x y z yaw pitch flashlight\n");
	for(const auto &mapViews : aViews)
		for(const cView &view : mapViews.second)
			fprintf(pFile, "%s %s %.3f %.3f %.3f %.4f %.4f %d\n", mapViews.first.c_str(), view.msName.c_str(),
					view.mvPos.x, view.mvPos.y, view.mvPos.z, view.mfYaw, view.mfPitch,
					view.mbFlashLight ? 1 : 0);
	fclose(pFile);
	return true;
}

//-----------------------------------------------------------------------

size_t cRefShots::FindViews(cWorld3D *apWorld, const tString &asMap, std::vector<cView> &avViews,
							size_t &alStarts)
{
	auto ByName = [](const cView &a, const cView &b){ return a.msName < b.msName; };
	auto SameName = [](const cView &a, const cView &b){ return a.msName == b.msName; };

	// Viewpoints chosen by hand replace the automatic places
	tViewMap mapViews;
	LoadViewsFile(mapViews);
	tViewMap::iterator viewsIt = mapViews.find(cString::ToLowerCase(cString::SetFileExt(asMap, "")));
	if(viewsIt != mapViews.end() && viewsIt->second.empty()==false)
	{
		avViews = viewsIt->second;
		alStarts = 0;
		return 0;
	}

	avViews.clear();
	tStartPosEntityList *pStarts = apWorld->GetStartPosEntityList();
	for(tStartPosEntityListIt it = pStarts->begin(); it != pStarts->end(); ++it)
		avViews.push_back({(*it)->GetName(), true, cVector3f(0,0,0)});
	std::sort(avViews.begin(), avViews.end(), ByName);
	avViews.erase(std::unique(avViews.begin(), avViews.end(), SameName), avViews.end());
	alStarts = avViews.size();

	// The lights the map file has, spread evenly over their names.
	// Scripts and entities add lights too, those are left out.
	std::vector<cView> vLights;
	cLight3DListIterator lightIt = apWorld->GetLightIterator();
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
		avViews.push_back(vLights[i * vLights.size() / lLightViews]);

	return vLights.size();
}

//-----------------------------------------------------------------------

tString cRefShots::GetFrameName(const tString &asMap, const cView &aView, int alAngle)
{
	tString sName = cString::ToLowerCase(cString::SetFileExt(asMap, "")) + "-" + aView.msName;
	if(aView.mbManual) return sName;

	char sAngle[16];
	snprintf(sAngle, sizeof(sAngle), "%03d", alAngle * (360 / klAngles));
	return sName + "-" + sAngle;
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

	if(mStep == eStep_2D)
	{
		Update2D();
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
	// that enables it again. A viewpoint can ask for it to be off.
	cPlayerFlashLight *pFlashLight = mpInit->mpPlayer->GetFlashLight();
	if(mpInit->mpPlayer->GetPower() < 100) mpInit->mpPlayer->SetPower(100);
	bool bAtView = (mStep == eStep_SettleStart || mStep == eStep_Shoot) && mlView < mvViews.size();
	bool bFlashLight = bAtView==false || mvViews[mlView].mbFlashLight;
	if(pFlashLight->IsActive() != bFlashLight && mStep != eStep_Done)
	{
		pFlashLight->SetDisabled(false);
		pFlashLight->SetActive(bFlashLight);
	}

	// Flickering lights draw random numbers, and so do sounds when their
	// playback ends, which is in real time: the flicker would differ from run
	// to run. Keep them on. Scripts can start flickering at any time.
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	mpInit->mpLampAppearanceController->SetStill(true);
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

			size_t lStarts = 0;
			size_t lLights = FindViews(pWorld, mvMaps[mlMap], mvViews, lStarts);

			if(mvViews.empty()==false && mvViews[0].mbManual)
				Log("REFSHOTS: [%d/%d] %s, %d viewpoints\n", (int)mlMap + 1, (int)mvMaps.size(),
					mvMaps[mlMap].c_str(), (int)mvViews.size());
			else
				Log("REFSHOTS: [%d/%d] %s, %d start positions, %d of %d lights\n", (int)mlMap + 1,
					(int)mvMaps.size(), mvMaps[mlMap].c_str(), (int)lStarts, (int)(mvViews.size() - lStarts),
					(int)lLights);
			if(mvViews.empty())
			{
				Fail("%s has no start positions or lights", mvMaps[mlMap].c_str());
				NextMap();
				break;
			}

			// The 2D screens are shot from the first place only
			if(mb2D && mvViews.size() > 1) mvViews.resize(1);

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

		if(mb2D)
		{
			// What the screens show must not depend on the maps before
			cMath::Randomize(klRandomSeed);
			mpInit->mpGame->GetGraphics()->GetDrawer()->SetSkipDrawing(false);
			mStep = eStep_2D;
			ml2DScreen = 0;
			mlFrame = 0;
			break;
		}

		mStep = eStep_Shoot;
		mlFrame = klTurnSettleFrames; // already facing the first direction
		[[fallthrough]];

	case eStep_Shoot:
	{
		// Scripts and physics may turn the camera; keep it where it should be.
		SetView();
		if(++mlFrame < klTurnSettleFrames) break;

		tString sFile = msDir + "/" + GetFrameName(mvMaps[mlMap], mvViews[mlView], mlAngle) + ".png";

		// This step's frame is drawn after the update, then saved.
		mpInit->mpGame->GetGraphics()->GetLowLevel()->SaveNextFrameToPNG(sFile);
		mlShots++;

		mlFrame = 0;
		mlAngle++;
		if(mlAngle >= GetAngleNum(mvViews[mlView])) NextView();
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
	if(mvViews[mlView].mbManual)
	{
		pCamera->SetYaw(mvViews[mlView].mfYaw);
		pCamera->SetPitch(mvViews[mlView].mfPitch);
	}
	else
	{
		pCamera->SetYaw(mfStartYaw + mlAngle * (k2Pif / klAngles));
		pCamera->SetPitch(0);
	}
	if(mvViews[mlView].mbStartPos)
		mpInit->mpPlayer->GetCharacterBody()->SetYaw(pCamera->GetYaw());
	else
		// The player has put the camera on its head earlier in this step.
		pCamera->SetPosition(mvViews[mlView].mvPos);
}

//-----------------------------------------------------------------------

void cRefShots::Update2D()
{
	// The main menu has its own updater container and stops the scene.
	if(mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default") SetView();

	const c2DScreen &screen = kv2DScreens[ml2DScreen];
	if(mlFrame == 0) screen.mpSetup(mpInit);
	if(++mlFrame < screen.mlFrames) return;

	tString sFile = msDir + "/2d-" + screen.msName + ".png";
	mpInit->mpGame->GetGraphics()->GetLowLevel()->SaveNextFrameToPNG(sFile);
	mlShots++;

	mlFrame = 0;
	ml2DScreen++;
	if(ml2DScreen < sizeof(kv2DScreens) / sizeof(kv2DScreens[0])) return;

	Log("REFSHOTS: done, %d frames, %d failure(s)\n", mlShots, mlFailures);
	Finish();
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
	if(mlMap < mvMaps.size() && mb2D==false)
	{
		mStep = eStep_ChangeMap;
		return;
	}

	if(mb2D==false)
	{
		mb2D = true;
		mlMap = 0;
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
