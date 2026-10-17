/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/RefView.h"
#include "core/DogTestMap.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/LampAppearanceController.h"
#include "core/GraphicsHelper.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/EffectHandler.h"
#include "systems/FadeHandler.h"
#include "ui/MainMenu.h"
#include "ui/Inventory.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <ctime>
#include <set>

// A tooltip that wraps instead of running off the screen
static void Tip(const char *apText)
{
	if(ImGui::BeginTooltip()==false) return;
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28);
	ImGui::TextUnformatted(apText);
	ImGui::PopTextWrapPos();
	ImGui::EndTooltip();
}

// Real time a map may take to load before the panel stops waiting for it
static const float kfMapTimeout = 30.0f;

//-----------------------------------------------------------------------

cRefView::cRefView(cInit *apInit) : iUpdateable("RefView")
{
	mpInit = apInit;
	mbOpen = false;
	msMapFilter[0] = 0;

	mlStarts = 0;
	mlLights = 0;

	mbAtView = false;
	mlView = 0;
	mlAngle = 0;
	mfStartYaw = 0;
	mbHoldDirection = false;

	mlGoToView = -1;
	mlGoToAngle = 0;
	mfWaitTime = 0;

	mbUsed = false;
	mbFlashLight = false;
	mbNoFlicker = true;
	mbHide2D = false;
	mbGodMode = true;
	mbSkippingDrawing = false;
	mbSetFlashLight = false;
	mlGiveItems = 0;

	mbFly = false;
	mbFlying = false;
	mvFlyPos = cVector3f(0,0,0);
	mfFlySpeed = 3.0f;
	mvWalkPitchLimits = cVector2f(0,0);
	mlFlySeed = 0;
	msViewpointName[0] = 0;
	mbAddViewpoint = false;
	mbDeleteViewpoint = false;

	mbShadowAllLights = false;
	mbPlaceTestLight = false;
	mbRemoveTestLight = false;
	mbCarryTestLight = false;
	mfTestLightRadius = 6.0f;
	mfTestLightBrightness = 0.8f;

	const char *pOpen = SDL_getenv("OVERTURE_REFVIEW");
	if(pOpen && pOpen[0] && pOpen[0] != '0') SetOpen(true);
	const char *pMap = SDL_getenv("OVERTURE_MAP");
	if(pMap && tString(pMap).find("effectslab") == 0) SetOpen(true);
	const char *pCapture = SDL_getenv("OVERTURE_EFFECTS_CAPTURE");
	if(pCapture && pCapture[0])
	{
		msEffectsCapture = pCapture;
		mbEffectPreviewAnimate = false;
		mpInit->mpGame->GetLogicTimer()->SetFixedStep(true);
		mpInit->mpGame->SetLimitFPS(true);
		SetOpen(false);
	}
}

cRefView::~cRefView()
{
	SetOpen(false);
}

//-----------------------------------------------------------------------

void cRefView::SetOpen(bool abX)
{
	if(mbOpen == abX) return;
	mbOpen = abX;

	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	if(mbOpen)
	{
		if(mvMaps.empty())
		{
			cRefShots::FindMaps(mpInit, mvMaps);
			if(mpInit->mpGame->GetResources()->GetFileSearcher()->GetFilePath("dogtest.dae") != "")
				mvMaps.push_back("dogtest.dae");
		}
		pLowLevel->SetDebugPanel([this](){ Draw(); });
	}
	else
	{
		pLowLevel->SetDebugPanel({});
		// The player looks around from here
		mbHoldDirection = false;
	}
	pLowLevel->SetDebugPanelInput(mbOpen);
}

//-----------------------------------------------------------------------

void cRefView::Update(float afTimeStep)
{
	UpdatePlayerEffects(afTimeStep);

	bool bDefault = mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";

	if(msLoadMap != "")
	{
		tString sMap = msLoadMap;
		msLoadMap = "";
		if(CanLoadMap()) LoadMap(sMap);
		return; // the container may have changed
	}

	// The places of the map that is loaded, however it got loaded
	const tString &sCurrentMap = mpInit->mpMapHandler->GetCurrentMapName();
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(sCurrentMap != msViewsMap && mpInit->mpMapHandler->IsChangingMap()==false)
	{
		msViewsMap = sCurrentMap;
		mvViews.clear();
		mlStarts = 0;
		mlLights = 0;
		if(pWorld && sCurrentMap != "") mlLights = cRefShots::FindViews(pWorld, sCurrentMap, mvViews, mlStarts);
		mbAtView = false;
		mlFlySeed = 2;
	}

	if(msWaitMap != "")
	{
		mfWaitTime += afTimeStep;
		if(msViewsMap == msWaitMap)
		{
			msWaitMap = "";
			// Start at the map's first frame
			if(mlGoToView < 0)
			{
				mlGoToView = 0;
				mlGoToAngle = 0;
			}
		}
		else if(mfWaitTime > kfMapTimeout)
		{
			Warning("RefView: %s did not load\n", msWaitMap.c_str());
			msWaitMap = "";
		}
	}

	if(mlGoToView >= 0 && msWaitMap == "" && bDefault)
	{
		if((size_t)mlGoToView < mvViews.size()) GoTo((size_t)mlGoToView, mlGoToAngle);
		mlGoToView = -1;
	}

	if(bDefault && mbAtView && pWorld) SetView();

	// A place is held only while the panel is open. With it closed the
	// player stands there and walks and looks on as usual: a camera kept
	// at the place while the body walks off reads as a stuck view.
	if(bDefault && mbAtView && pWorld && mbOpen==false) Release();

	if(bDefault && pWorld) UpdateFly(afTimeStep);

	if(bDefault && pWorld) UpdateTestLight();
	mbPlaceTestLight = mbRemoveTestLight = false;

	if(mbAddViewpoint || mbDeleteViewpoint)
	{
		if(bDefault && pWorld && msViewsMap != "")
		{
			if(mbAddViewpoint) AddViewpoint();
			else DeleteViewpoint();
		}
		mbAddViewpoint = mbDeleteViewpoint = false;
	}

	ApplyOptions();
}

//-----------------------------------------------------------------------

void cRefView::UpdateFly(float afTimeStep)
{
	iCharacterBody *pBody = mpInit->mpPlayer->GetCharacterBody();
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();

	if(mbFly != mbFlying)
	{
		// What is seen while choosing viewpoints is what the frames show
		mbUsed = true;
		mbFlying = mbFly;
		if(mbFlying)
		{
			// The body would walk off with the same keys
			pBody->SetActive(false);
			mvWalkPitchLimits = pCamera->GetPitchLimits();
			mlFlySeed = 1;
		}
		else
		{
			pCamera->SetPitchLimits(mvWalkPitchLimits);
			pCamera->SetPitch(pCamera->GetPitch());
			pBody->SetActive(true);
			// The player goes on from where the camera got to
			PutPlayerAtCamera();
		}
	}
	if(mbFlying==false) return;
	// A map change brings the body and the player's limits back
	if(pBody->IsActive()) pBody->SetActive(false);
	pCamera->SetPitchLimits(cVector2f(cMath::ToRad(89), cMath::ToRad(-89)));

	// The mouse turns the camera only in the states where the player looks
	// around freely: in the others it moves a cursor, or is held by what
	// the player has hold of, which was left behind with the body.
	switch(mpInit->mpPlayer->GetState())
	{
	case ePlayerState_Normal:
	case ePlayerState_Message:
	case ePlayerState_WeaponMelee:
		break;
	default:
		mpInit->mpPlayer->ChangeState(ePlayerState_Normal);
		break;
	}

	// From where the camera is, once the player has put it on its head
	// after a move
	if(mlFlySeed > 0)
	{
		mlFlySeed--;
		mvFlyPos = pCamera->GetPosition();
		return;
	}

	if(mbOpen==false)
	{
		cInput *pInput = mpInit->mpGame->GetInput();
		float fSpeed = mfFlySpeed * (pInput->IsTriggerd("Run") ? 4.0f : 1.0f) * afTimeStep;
		cVector3f vMove(0,0,0);
		if(pInput->IsTriggerd("Forward"))	vMove += pCamera->GetForward();
		if(pInput->IsTriggerd("Backward"))	vMove -= pCamera->GetForward();
		if(pInput->IsTriggerd("Right"))		vMove += pCamera->GetRight();
		if(pInput->IsTriggerd("Left"))		vMove -= pCamera->GetRight();
		if(pInput->IsTriggerd("Jump"))		vMove.y += 1;
		if(pInput->IsTriggerd("Crouch"))	vMove.y -= 1;
		mvFlyPos += vMove * fSpeed;
	}

	pCamera->SetPosition(mvFlyPos);
}

//-----------------------------------------------------------------------

// The player's body under the camera, so that the eyes are where the camera
// is, facing the same way. From there the view is the player's again.
void cRefView::PutPlayerAtCamera()
{
	iCharacterBody *pBody = mpInit->mpPlayer->GetCharacterBody();
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();

	// The body puts the camera at its top, plus what the player adds
	cVector3f vFeet = pCamera->GetPosition() - pBody->GetCameraPosAdd();
	vFeet.y -= pBody->GetSize().y;
	pBody->SetFeetPosition(vFeet);
	pBody->SetYaw(pCamera->GetYaw());

	mbAtView = false;
	mbHoldDirection = false;
}

//-----------------------------------------------------------------------

static const char *gsTestLightName = "RefViewTestLight";

// The test light is the world's: it is found by name, and gone with the map.
void cRefView::UpdateTestLight()
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	iLight3D *pLight = pWorld->GetLight(gsTestLightName);

	if(mbRemoveTestLight && pLight)
	{
		pWorld->DestroyLight(pLight);
		return;
	}
	if(mbPlaceTestLight && pLight==nullptr)
	{
		pLight = pWorld->CreateLightPoint(gsTestLightName);
		pLight->SetCastShadows(true);
	}
	if(pLight==nullptr) return;

	if(mbPlaceTestLight || mbCarryTestLight)
	{
		// A little ahead of the eye, so that what is held or stood on
		// does not swallow it
		cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
		pLight->SetPosition(pCamera->GetPosition() + pCamera->GetForward() * 0.3f);
	}
	// A warm white; alpha is how strong the highlights are
	cColor col(mfTestLightBrightness, mfTestLightBrightness * 0.92f, mfTestLightBrightness * 0.8f, 1);
	if(!(pLight->GetDiffuseColor() == col)) pLight->SetDiffuseColor(col);
	if(pLight->GetFarAttenuation() != mfTestLightRadius) pLight->SetFarAttenuation(mfTestLightRadius);
}

//-----------------------------------------------------------------------

// The camera as it is becomes a viewpoint of the map, or replaces the one
// of the same name.
void cRefView::AddViewpoint()
{
	mbUsed = true;
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();

	cRefShots::tViewMap mapViews;
	cRefShots::LoadViewsFile(mapViews);
	std::vector<cRefShots::cView> &vViews = mapViews[msViewsMap];

	// Names end up in file names
	tString sName;
	for(const char *p = msViewpointName; *p; ++p)
		sName += (isalnum((unsigned char)*p) || *p == '_') ? *p : '_';
	for(int i = 1; sName == ""; ++i)
	{
		char sNum[16];
		snprintf(sNum, sizeof(sNum), "view%02d", i);
		bool bUsed = false;
		for(const cRefShots::cView &view : vViews) bUsed = bUsed || view.msName == sNum;
		if(bUsed==false) sName = sNum;
	}

	cRefShots::cView view;
	view.msName = sName;
	view.mbStartPos = false;
	view.mvPos = pCamera->GetPosition();
	view.mbManual = true;
	view.mfYaw = pCamera->GetYaw();
	view.mfPitch = pCamera->GetPitch();
	view.mbFlashLight = mpInit->mpPlayer->GetFlashLight()->IsActive();

	size_t lIdx = vViews.size();
	for(size_t i = 0; i < vViews.size(); ++i)
		if(vViews[i].msName == sName) lIdx = i;
	if(lIdx == vViews.size()) vViews.push_back(view);
	else vViews[lIdx] = view;

	if(cRefShots::SaveViewsFile(mapViews))
	{
		msViewpointMessage = "Saved " + sName;
		msViewpointName[0] = 0;
		// The map's places are its viewpoints from now on
		mvViews = vViews;
		mlStarts = 0;
		mbAtView = true;
		mbHoldDirection = mbOpen;
		mlView = lIdx;
		mlAngle = 0;
	}
	else
	{
		msViewpointMessage = "Couldn't write " + cRefShots::GetViewsFile();
	}
}

//-----------------------------------------------------------------------

void cRefView::DeleteViewpoint()
{
	if(mbAtView==false || mlView >= mvViews.size() || mvViews[mlView].mbManual==false) return;

	cRefShots::tViewMap mapViews;
	cRefShots::LoadViewsFile(mapViews);
	std::vector<cRefShots::cView> &vViews = mapViews[msViewsMap];
	tString sName = mvViews[mlView].msName;
	for(size_t i = 0; i < vViews.size(); ++i)
	{
		if(vViews[i].msName != sName) continue;
		vViews.erase(vViews.begin() + i);
		break;
	}
	if(vViews.empty()) mapViews.erase(msViewsMap);

	if(cRefShots::SaveViewsFile(mapViews)==false) return;
	msViewpointMessage = "Deleted " + sName;

	// With none left the automatic places are back
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	mlLights = cRefShots::FindViews(pWorld, msViewsMap, mvViews, mlStarts);
	mbAtView = false;
}

//-----------------------------------------------------------------------

void cRefView::Reset()
{
	// Changing map resets every updateable; the panel keeps its state.
	mlEffectPreview = ePreview_None;
	mbEffectTour = false;
	mfEffectPreviewClock = 0;
	mlEffectAction = 0;
	mfEffectBlackFade = 0;
}

//-----------------------------------------------------------------------

bool cRefView::CanLoadMap()
{
	if(msWaitMap != "" || mpInit->mpMapHandler->IsChangingMap()) return false;

	// Not from the intro, a text screen or the credits
	const tString &sContainer = mpInit->mpGame->GetUpdater()->GetCurrentContainerName();
	return sContainer == "Default" || sContainer == "MainMenu";
}

//-----------------------------------------------------------------------

void cRefView::LoadMap(const tString &asMap)
{
	const tString sStart = cString::SetFileExt(asMap, "") == "effectslab" ? "link01" : "";
	mbUsed = true;
	mbAtView = false;
	mlGoToView = -1;
	msWaitMap = cString::ToLowerCase(cString::SetFileExt(asMap, ""));
	mfWaitTime = 0;

	if(mpInit->mpMainMenu->IsActive()) mpInit->mpMainMenu->SetActive(false);

	if(mpInit->mpMapHandler->GetCurrentMapName() == "")
	{
		// No game yet: start one, as the menu does
		mpInit->ResetGame(true);
		mpInit->mpGame->GetUpdater()->SetContainer("Default");
		mpInit->mpGame->GetScene()->SetDrawScene(true);
		mpInit->mpMapHandler->Load(asMap, sStart);
	}
	else
	{
		// Loading a map again gives its places anew
		msViewsMap = "";
		mpInit->mpMapHandler->ChangeMap(asMap, sStart, "", "", 0, 0, "", "");
	}
}

//-----------------------------------------------------------------------

void cRefView::GoTo(size_t alView, int alAngle)
{
	mbUsed = true;
	mbAtView = true;
	mbHoldDirection = mbOpen;
	mlView = alView;
	mlAngle = alAngle;

	// Start positions have a direction of their own, the frames turn from it
	mfStartYaw = 0;
	if(mvViews[mlView].mbStartPos)
	{
		mpInit->mpPlayer->SetStartPos(mvViews[mlView].msName);
		mfStartYaw = mpInit->mpPlayer->GetCamera()->GetYaw();
		// Flying goes on from the player's head, once it is there
		mlFlySeed = 2;
	}
	else
	{
		mvFlyPos = mvViews[mlView].mvPos;
		mlFlySeed = 0;
	}
	// The direction is set even if it is not held after this
	bool bHold = mbHoldDirection;
	mbHoldDirection = true;
	SetView();
	mbHoldDirection = bHold;
}

//-----------------------------------------------------------------------

// The player goes on from the place: the camera does not jump back to
// wherever the body was left.
void cRefView::Release()
{
	if(mbAtView && mbFlying==false) PutPlayerAtCamera();
	mbAtView = false;
	mbHoldDirection = false;
}

void cRefView::ReleaseForDogTest()
{
	mbFly = false;
	if(mbFlying)
		mpInit->mpPlayer->GetCamera()->SetPitchLimits(mvWalkPitchLimits);
	mbFlying = false;
	mpInit->mpPlayer->GetCharacterBody()->SetActive(true);
	mbAtView = mbHoldDirection = false;
	mlGoToView = -1;
	SetOpen(false);
}

//-----------------------------------------------------------------------

// As cRefShots::SetView(), but the direction is the player's once the panel
// has been closed.
void cRefView::SetView()
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	const cRefShots::cView &view = mvViews[mlView];

	if(mbHoldDirection && view.mbManual)
	{
		pCamera->SetYaw(view.mfYaw);
		pCamera->SetPitch(view.mfPitch);
	}
	else if(mbHoldDirection)
	{
		pCamera->SetYaw(mfStartYaw + mlAngle * (k2Pif / cRefShots::klAngles));
		pCamera->SetPitch(0);
		if(view.mbStartPos) mpInit->mpPlayer->GetCharacterBody()->SetYaw(pCamera->GetYaw());
	}

	// The player has put the camera on its head earlier in this step. When
	// flying the camera is moved from the place, not kept at it.
	if(view.mbStartPos==false && mbFlying==false) pCamera->SetPosition(view.mvPos);
}

//-----------------------------------------------------------------------

// What cRefShots does on every step of its run
void cRefView::ApplyOptions()
{
	bool bDefault = mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";

	// The menus and text screens are drawn in 2D
	bool bSkip = mbUsed && mbHide2D && bDefault;
	if(bSkip != mbSkippingDrawing)
	{
		mpInit->mpGame->GetGraphics()->GetDrawer()->SetSkipDrawing(bSkip);
		mbSkippingDrawing = bSkip;
	}

	// Items asked for in the Player window. The flashlight and the glow
	// stick are had once; flares add up.
	if(mlGiveItems != 0 && bDefault)
	{
		cInventory *pInventory = mpInit->mpInventory;
		if((mlGiveItems & eGiveItem_Flashlight) && pInventory->GetItem("flashlight")==nullptr)
			pInventory->AddItemFromFile("flashlight","item_flashlight.ent",-1);
		if((mlGiveItems & eGiveItem_GlowStick) && pInventory->GetItem("glowstick")==nullptr &&
			pInventory->GetItem("glowst1")==nullptr)
			pInventory->AddItemFromFile("glowstick","item_glowstick.ent",-1);
		if(mlGiveItems & eGiveItem_Flare)
			pInventory->AddItemFromFile("flare_panel","items_flare.ent",-1);
		mlGiveItems = 0;
	}

	if(mbUsed==false || bDefault==false) return;

	mpInit->mpPlayer->SetGodMode(mbGodMode);

	// The flashlight is the player's to switch (its key works while
	// flying too); it is only set when asked for here, and kept charged.
	cPlayerFlashLight *pFlashLight = mpInit->mpPlayer->GetFlashLight();
	if(mpInit->mpPlayer->GetPower() < 100) mpInit->mpPlayer->SetPower(100);
	if(mbSetFlashLight)
	{
		mbSetFlashLight = false;
		if(pFlashLight->IsActive() != mbFlashLight)
		{
			pFlashLight->SetDisabled(false);
			pFlashLight->SetActive(mbFlashLight);
		}
	}

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	mpInit->mpLampAppearanceController->SetStill(mbNoFlicker);
	if(mbNoFlicker && pWorld)
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
}

//-----------------------------------------------------------------------

void cRefView::Draw()
{
	if(mbOpen==false) return;
	if(mpInit->mpDogTestMap->IsActive() && !mpInit->mpDogTestMap->DrawControls()) return;

	if(msViewsMap == "effectslab")
	{
		DrawEffectsLab();
		if(!mbEffectsLabFullPanel) return;
	}
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + pViewport->WorkSize.x - fEm, pViewport->WorkPos.y + fEm),
							ImGuiCond_FirstUseEver, ImVec2(1, 0));
	// A fixed width, as high as what is in it
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 30, 0), ImVec2(fEm * 30, pViewport->WorkSize.y));
	ImGui::SetNextWindowBgAlpha(0.85f);

	bool bOpen = true;
	if(ImGui::Begin("Reference views (F2)", &bOpen, ImGuiWindowFlags_NoSavedSettings |
					ImGuiWindowFlags_AlwaysAutoResize))
	{
		// The frame the view is at, named as tools/refshots.py saves it
		if(mbAtView && mbHoldDirection)
			ImGui::Text("%s.png", cRefShots::GetFrameName(msViewsMap, mvViews[mlView], mlAngle).c_str());
		else if(mbAtView)
			ImGui::Text("%s, %s: looking freely", msViewsMap.c_str(), mvViews[mlView].msName.c_str());
		else if(msViewsMap != "")
			ImGui::Text("%s: not at a place", msViewsMap.c_str());
		else
			ImGui::TextUnformatted("No map loaded");
		ImGui::Separator();

		// The sections fold, to leave room for the one in use
		if(ImGui::CollapsingHeader("Map", ImGuiTreeNodeFlags_DefaultOpen)) DrawMaps();
		if(ImGui::CollapsingHeader("Place", ImGuiTreeNodeFlags_DefaultOpen)) DrawViews();
		if(ImGui::CollapsingHeader("Viewpoints")) DrawViewpoints();
		if(ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) DrawRenderer();

		ImGui::Separator();
		ImGui::TextWrapped("F2 closes the panel and gives the mouse back: the player stands at "
							"the place and walks on from there, or flies with Fly (Player window). "
							"While the panel is open the camera is kept at the place.");
	}
	ImGui::End();

	DrawShadows();
	DrawPlayer();
	DrawAmbientOcclusion();
	DrawLensAndFilm();
	DrawAntiAliasing();
	DrawShading();

	// Closed with the window's button
	if(bOpen==false) SetOpen(false);
}

//-----------------------------------------------------------------------

void cRefView::DrawMaps()
{
	const float fEm = ImGui::GetFontSize();

	if(msWaitMap != "") ImGui::Text("Loading %s...", msWaitMap.c_str());

	ImGui::SetNextItemWidth(-1);
	ImGui::InputTextWithHint("##filter", "filter", msMapFilter, sizeof(msMapFilter));
	tString sFilter = cString::ToLowerCase(msMapFilter);

	ImGui::BeginDisabled(CanLoadMap()==false);
	if(ImGui::BeginListBox("##maps", ImVec2(-1, fEm * 5)))
	{
		for(const tString &sFile : mvMaps)
		{
			tString sName = cString::ToLowerCase(cString::SetFileExt(sFile, ""));
			if(sFilter != "" && sName.find(sFilter) == tString::npos) continue;

			if(ImGui::Selectable(sName.c_str(), sName == msViewsMap)) msLoadMap = sFile;
		}
		ImGui::EndListBox();
	}
	ImGui::EndDisabled();
}

//-----------------------------------------------------------------------

void cRefView::DrawViews()
{
	const float fEm = ImGui::GetFontSize();
	const int lAngles = cRefShots::klAngles;

	if(mvViews.empty())
	{
		ImGui::TextDisabled("Pick a map.");
		return;
	}
	bool bManual = mvViews[0].mbManual;
	if(bManual)
		ImGui::Text("%d viewpoints", (int)mvViews.size());
	else
		ImGui::Text("No viewpoints yet: %d start positions, %d of %d lights", (int)mlStarts,
					(int)(mvViews.size() - mlStarts), (int)mlLights);

	bool bEnabled = msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	ImGui::BeginDisabled(bEnabled==false);

	if(ImGui::BeginListBox("##views", ImVec2(-1, fEm * 4)))
	{
		for(size_t i = 0; i < mvViews.size(); ++i)
		{
			if(ImGui::Selectable(mvViews[i].msName.c_str(), mbAtView && mlView == i))
			{
				mlGoToView = (int)i;
				mlGoToAngle = mbAtView ? mlAngle : 0;
			}
		}
		ImGui::EndListBox();
	}

	size_t lView = mbAtView ? mlView : 0;
	for(int i = 0; i < lAngles && bManual==false; ++i)
	{
		char sLabel[16];
		snprintf(sLabel, sizeof(sLabel), "%03d", i * (360 / lAngles));
		if(i > 0) ImGui::SameLine();
		if(ImGui::RadioButton(sLabel, mbAtView && mbHoldDirection && mlAngle == i))
		{
			mlGoToView = (int)lView;
			mlGoToAngle = i;
		}
	}

	// Through the map's frames in the order of their names
	int lViewAngles = bManual ? 1 : lAngles;
	int lFrames = (int)mvViews.size() * lViewAngles;
	int lFrame = (int)mlView * lViewAngles + mlAngle;
	bool bTyping = ImGui::GetIO().WantTextInput;
	int lStep = 0;
	if(ImGui::Button("< Previous") || (bTyping==false && ImGui::IsKeyPressed(ImGuiKey_PageUp))) lStep = -1;
	ImGui::SameLine();
	if(ImGui::Button("Next >") || (bTyping==false && ImGui::IsKeyPressed(ImGuiKey_PageDown))) lStep = 1;
	ImGui::SameLine();
	ImGui::TextDisabled("PgUp / PgDn");
	if(lStep != 0 && bEnabled)
	{
		// From nowhere, or from looking freely, to the frame itself first
		if(mbAtView==false) lFrame = 0;
		else if(mbHoldDirection) lFrame = (lFrame + lStep + lFrames) % lFrames;
		mlGoToView = lFrame / lViewAngles;
		mlGoToAngle = lFrame % lViewAngles;
	}

	ImGui::SameLine();
	ImGui::BeginDisabled(mbAtView==false);
	if(ImGui::Button("Release")) Release();
	ImGui::EndDisabled();

	ImGui::EndDisabled();
}

//-----------------------------------------------------------------------

void cRefView::DrawViewpoints()
{
	bool bEnabled = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	ImGui::BeginDisabled(bEnabled==false);

	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
	ImGui::InputTextWithHint("##viewpoint", "name (optional)", msViewpointName, sizeof(msViewpointName));
	ImGui::SameLine();
	if(ImGui::Button("Add viewpoint here")) mbAddViewpoint = true;
	if(ImGui::IsItemHovered())
		Tip("Saves where the camera is, where it looks and whether the flashlight is on. "
							"An existing name is replaced.");

	bool bAtManual = mbAtView && mlView < mvViews.size() && mvViews[mlView].mbManual;
	ImGui::BeginDisabled(bAtManual==false);
	if(ImGui::Button("Delete this viewpoint")) mbDeleteViewpoint = true;
	ImGui::EndDisabled();
	if(msViewpointMessage != "")
	{
		ImGui::SameLine();
		ImGui::TextDisabled("%s", msViewpointMessage.c_str());
	}

	ImGui::EndDisabled();
}

//-----------------------------------------------------------------------

// What changes how the game plays while testing.
void cRefView::DrawPlayer()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	// Between the other two windows
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + pViewport->WorkSize.x * 0.5f, pViewport->WorkPos.y + fEm),
							ImGuiCond_FirstUseEver, ImVec2(0.5f, 0));
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 24, 0), ImVec2(fEm * 24, pViewport->WorkSize.y));
	ImGui::SetNextWindowBgAlpha(0.85f);

	if(ImGui::Begin("Player", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)==false)
	{
		ImGui::End();
		return;
	}

	bool bInGame = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	ImGui::BeginDisabled(bInGame==false);

	ImGui::Checkbox("Fly", &mbFly);
	if(ImGui::IsItemHovered())
		Tip("With the panel closed (F2): the movement keys move the camera where it looks, "
							"Run is faster, Jump and Crouch go up and down. The player's body is switched "
							"off meanwhile. Unticking puts the player where the camera is.");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(fEm * 10);
	ImGui::SliderFloat("Speed", &mfFlySpeed, 0.5f, 20.0f, "%.1f m/s", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered()) Tip("Four times this with Run held.");

	if(bInGame)
	{
		cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
		const cVector3f &vPos = pCamera->GetPosition();
		ImGui::TextDisabled("Camera %.2f %.2f %.2f", vPos.x, vPos.y, vPos.z);
	}

	ImGui::Separator();
	if(mbUsed==false) ImGui::TextDisabled("Applied once something is picked in the panel.");
	if(mbSetFlashLight==false && bInGame) mbFlashLight = mpInit->mpPlayer->GetFlashLight()->IsActive();
	if(ImGui::Checkbox("Flashlight on", &mbFlashLight))
	{
		mbSetFlashLight = true;
		mbUsed = true;
	}
	if(ImGui::IsItemHovered())
		Tip("Switches it whether it is in the inventory or not; it does not run down. Its key works with "
			"the flashlight in the inventory. A viewpoint is saved with the flashlight as it is.");
	ImGui::SameLine();
	if(ImGui::Checkbox("Cannot be hurt", &mbGodMode)) mbUsed = true;
	if(ImGui::Checkbox("No flicker", &mbNoFlicker)) mbUsed = true;
	if(ImGui::IsItemHovered())
		Tip("Flickering lights stay on. Lights already stopped stay so until the map is loaded again.");
	ImGui::SameLine();
	if(ImGui::Checkbox("3D view only", &mbHide2D)) mbUsed = true;
	if(ImGui::IsItemHovered()) Tip("No HUD, fades, damage or hiding effects.");

	ImGui::Separator();
	ImGui::TextUnformatted("Give");
	ImGui::SameLine();
	if(ImGui::Button("Flashlight")) mlGiveItems |= eGiveItem_Flashlight;
	if(ImGui::IsItemHovered()) Tip("Puts the flashlight in the inventory, if it is not there: its key works then.");
	ImGui::SameLine();
	if(ImGui::Button("Glow stick")) mlGiveItems |= eGiveItem_GlowStick;
	if(ImGui::IsItemHovered()) Tip("Puts the glow stick in the inventory, if it is not there: its key works then.");
	ImGui::SameLine();
	if(ImGui::Button("Flare")) mlGiveItems |= eGiveItem_Flare;
	if(ImGui::IsItemHovered()) Tip("One more flare in the inventory. It is lit from there, and thrown when put away.");

	ImGui::EndDisabled();
	if(msViewsMap != "effectslab" && ImGui::CollapsingHeader("Player effects")) DrawPlayerEffects();
	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawRenderer()
{
	// The values the lighting is tuned with: kept in settings.cfg
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cLinearLightingTuning tuning = pLowLevel->GetLinearLightingTuning();
	bool bChanged = false;
	ImGui::TextUnformatted("Lighting");
	ImGui::SameLine();
	if(ImGui::Button("Reset"))
	{
		tuning = cLinearLightingTuning();
		bChanged = true;
	}
	if(ImGui::IsItemHovered()) Tip("The sliders back to their defaults. Ctrl+click a slider to type a value.");
	ImGui::PushItemWidth(ImGui::GetFontSize() * 14);
	bChanged |= ImGui::SliderFloat("Light power", &tuning.mfLightExp, 1.0f, 2.2f, "%.2f");
	if(ImGui::IsItemHovered())
		Tip("How a light spreads. 2.2: each light as in the original. "
							"Lower: wider and flatter, brighter towards its edge.");
	bChanged |= ImGui::SliderFloat("Exposure", &tuning.mfExposure, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered()) Tip("Brightness of the whole 3D view, in light: 2 is one stop up.");
	bChanged |= ImGui::SliderFloat("Glow", &tuning.mfGlow, 0.25f, 8.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered())
		Tip("Light of halos, lamps' lit parts, additive materials and particles: "
							"how far their brightest parts go past white. Faint parts are left alone.");
	bChanged |= ImGui::SliderFloat("Blend match", &tuning.mfBlendMatch, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered())
		Tip("Particles, see-through materials and fog: 1 covers as much as in the "
							"original, 0 blends them as light, which makes smoke, snow and fog heavier.");
	bChanged |= ImGui::SliderFloat("Shoulder", &tuning.mfShoulder, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered())
		Tip("Tonemapping: the screen brightness above which bright parts roll off "
							"towards white instead of clipping. Darker parts are untouched. 1 is off.");
	ImGui::BeginDisabled(tuning.mfShoulder > 0.999f);
	bChanged |= ImGui::SliderFloat("White", &tuning.mfWhite, 1.0f, 16.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered())
		Tip("Tonemapping: how many times brighter than white something has to be "
							"to come out white. Higher keeps more detail in bright parts and makes them greyer.");
	ImGui::EndDisabled();
	ImGui::PopItemWidth();
	if(bChanged) pLowLevel->SetLinearLightingTuning(tuning);

	// Bloom: these too are kept in settings.cfg
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	bool bBloom = pPostEffects->GetBloomActive();
	if(ImGui::Checkbox("Bloom", &bBloom)) pPostEffects->SetBloomActive(bBloom);
	cBloomTuning bloom = pPostEffects->GetBloomTuning();
	bChanged = false;
	ImGui::BeginDisabled(bBloom==false);
	ImGui::SameLine();
	if(ImGui::Button("Reset##bloom"))
	{
		bloom = cBloomTuning();
		bChanged = true;
	}
	ImGui::PushItemWidth(ImGui::GetFontSize() * 14);
	bChanged |= ImGui::SliderFloat("Threshold", &bloom.mfThreshold, 0.0f, 1.5f, "%.2f");
	if(ImGui::IsItemHovered())
		Tip("The screen brightness at which things begin to glow; 1 is white. "
							"Above 1 only what is brighter than white glows.");
	bChanged |= ImGui::SliderFloat("Intensity", &bloom.mfIntensity, 0.0f, 2.0f, "%.2f");
	if(ImGui::IsItemHovered()) Tip("How strong the glow is.");
	bChanged |= ImGui::SliderFloat("Spread", &bloom.mfScatter, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered()) Tip("How far the glow reaches: tight around the source, or a wide haze.");
	ImGui::PopItemWidth();
	ImGui::EndDisabled();
	if(bChanged) pPostEffects->SetBloomTuning(bloom);
}

//-----------------------------------------------------------------------

void cRefView::DrawShadows()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + fEm, pViewport->WorkPos.y + fEm), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 30, 0), ImVec2(fEm * 30, pViewport->WorkSize.y));
	ImGui::SetNextWindowBgAlpha(0.85f);

	if(ImGui::Begin("Shadows", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)==false)
	{
		ImGui::End();
		return;
	}

	// Kept in settings.cfg, like the renderer's other options
	cRenderer3D *pRenderer = mpInit->mpGame->GetGraphics()->GetRenderer3D();
	cShadowMapSettings settings = pRenderer->GetShadowMapSettings();
	bool bChanged = false;
	ImGui::PushItemWidth(fEm * 14);

	// In the order of eRendererShowShadows
	int lShow = pRenderer->GetShowShadows();
	if(ImGui::Combo("Shadows", &lShow, "On\0Only of what never moves\0Off\0"))
		pRenderer->SetShowShadows((eRendererShowShadows)lShow);

	ImGui::BeginDisabled(lShow == eRendererShowShadows_None);

	if(ImGui::CollapsingHeader("Edges", ImGuiTreeNodeFlags_DefaultOpen))
	{
		int lFilter = settings.mFilter;
		if(ImGui::Combo("Edge", &lFilter, "Hard\0Soft\0Soft by distance\0"))
		{
			settings.mFilter = (eShadowMapFilter)lFilter;
			bChanged = true;
		}
		ImGui::SameLine();
		if(ImGui::Button("Reset"))
		{
			// The debug choices are not settings
			cShadowMapSettings defaults;
			defaults.mbShowOnly = settings.mbShowOnly;
			defaults.msOnlyLight = settings.msOnlyLight;
			settings = defaults;
			bChanged = true;
		}
		if(ImGui::IsItemHovered()) Tip("Everything in this window back to its default. Ctrl+click a slider to type a value.");

		// Shown in centimetres
		if(settings.mFilter == eShadowMapFilter_Soft)
		{
			float fSoftness = settings.mfSoftness * 100.0f;
			if(ImGui::SliderFloat("Softness", &fSoftness, 0.0f, 20.0f, "%.1f cm"))
			{
				settings.mfSoftness = fSoftness / 100.0f;
				bChanged = true;
			}
			if(ImGui::IsItemHovered())
				Tip("How far the edge is blurred to each side, at 1 m from the light. "
									"Twice that at 2 m, and so on.");
		}
		if(settings.mFilter == eShadowMapFilter_SoftByDistance)
		{
			float fLightSize = settings.mfLightSize * 100.0f;
			if(ImGui::SliderFloat("Light size", &fLightSize, 0.0f, 50.0f, "%.1f cm"))
			{
				settings.mfLightSize = fLightSize / 100.0f;
				bChanged = true;
			}
			if(ImGui::IsItemHovered())
				Tip("The radius the lights are taken to have. A bigger light gives wider "
									"edges; where a shadow starts, at what casts it, it stays sharp.");
		}
		if(settings.mFilter != eShadowMapFilter_Hard)
		{
			float fMaxBlur = settings.mfMaxBlur * 100.0f;
			if(ImGui::SliderFloat("Widest blur", &fMaxBlur, 1.0f, 50.0f, "%.1f cm"))
			{
				settings.mfMaxBlur = fMaxBlur / 100.0f;
				bChanged = true;
			}
			if(ImGui::IsItemHovered())
				Tip("No edge is blurred further than this (at 1 m from the light). "
									"Wide blurs need more samples to stay smooth.");

			bChanged |= ImGui::SliderInt("Samples", &settings.mlSamples, 4, 64);
			if(ImGui::IsItemHovered())
				Tip("Looks into the shadow map for every lit pixel, for every light. "
									"More: smoother edges, slower.");
			if(settings.mFilter == eShadowMapFilter_SoftByDistance)
			{
				bChanged |= ImGui::SliderInt("Search samples", &settings.mlBlockerSamples, 4, 64);
				if(ImGui::IsItemHovered())
					Tip("Looks for what casts the shadow, to know how wide the edge is. "
										"Too few: thin things (bars, wires) give speckled shadows.");
			}
			bChanged |= ImGui::Checkbox("Noise", &settings.mbNoise);
			if(ImGui::IsItemHovered())
				Tip("Turns the samples differently in every pixel: grain instead of bands.");
		}
	}

	if(ImGui::CollapsingHeader("Maps", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bChanged |= ImGui::SliderInt("Shadow cache (MiB)", &settings.mlCacheSizeMB, 0, 512);
		if(ImGui::IsItemHovered()) Tip("Keeps unchanged shadow maps between frames. Zero redraws every map.");
		static const int vSizes[] = {256, 512, 1024, 2048, 4096};
		const char *sSizes = "256\0" "512\0" "1024\0" "2048\0" "4096\0";
		int lSpot = 0, lPoint = 0;
		for(int i=0; i<5; ++i)
		{
			if(vSizes[i] <= settings.mlSpotSize) lSpot = i;
			if(vSizes[i] <= settings.mlPointSize) lPoint = i;
		}
		if(ImGui::Combo("Spotlight map", &lSpot, sSizes))
		{
			settings.mlSpotSize = vSizes[lSpot];
			bChanged = true;
		}
		if(ImGui::IsItemHovered()) Tip("Pixels a side. Smaller: blockier and blurrier edges, faster.");
		if(ImGui::Combo("Point light map", &lPoint, sSizes))
		{
			settings.mlPointSize = vSizes[lPoint];
			bChanged = true;
		}
		if(ImGui::IsItemHovered()) Tip("Pixels a side of each of the six directions a point light is drawn in.");

		float fBias = settings.mfBias * 1000.0f;
		if(ImGui::SliderFloat("Bias", &fBias, 0.0f, 50.0f, "%.1f mm"))
		{
			settings.mfBias = fBias / 1000.0f;
			bChanged = true;
		}
		if(ImGui::IsItemHovered())
			Tip("How far a surface is taken towards the light before it is looked up. "
								"Too little: lit surfaces get dark stripes or speckles. "
								"Too much: shadows come loose from what casts them.");
		bChanged |= ImGui::SliderFloat("Slope bias", &settings.mfSlopeBias, 0.0f, 8.0f, "%.1f texels");
		if(ImGui::IsItemHovered())
			Tip("More bias for surfaces the light only grazes, in pixels of the map.");

		int lFaces = settings.mFaces;
		if(ImGui::Combo("Sides drawn", &lFaces, "Both\0Towards the light\0Away from the light\0"))
		{
			settings.mFaces = (eShadowMapFaces)lFaces;
			bChanged = true;
		}
		if(ImGui::IsItemHovered())
			Tip("Which sides of things cast the shadows.\n"
								"Towards the light: as the original shadows; a wall seen from behind casts none.\n"
								"Away from the light: no stripes on lit sides, but thin, one-sided things cast none.");
		bChanged |= ImGui::Checkbox("Holes in fences and gratings", &settings.mbAlphaCasters);
		if(ImGui::IsItemHovered())
			Tip("Materials with holes cut into them cast shadows with the holes. "
								"The original shadows leave these materials out.");
	}

	ImGui::EndDisabled();

	bChanged |= ImGui::Checkbox("Show the shadows alone", &settings.mbShowOnly);
	if(ImGui::IsItemHovered())
		Tip("Grey where a light reaches, dark where its shadow is; lights add up. Works for both kinds.");

	const cShadowMapStats &stats = pRenderer->GetShadowMapStats();
	ImGui::TextDisabled("Last frame: %d lights, %d maps, %d casters", stats.mlLights, stats.mlMaps, stats.mlCasters);
	if(ImGui::IsItemHovered())
		Tip("Lights that cast shadows, the maps drawn for them (one for a spotlight, up to six for "
							"a point light) and the things drawn into those. F3 shows the time they take ('Shadow maps').");
	ImGui::TextDisabled("Cached maps reused: %d", stats.mlCachedMaps);

	ImGui::PopItemWidth();
	if(bChanged) pRenderer->SetShadowMapSettings(settings);

	if(ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) DrawShadowLights();

	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawAmbientOcclusion()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	// Under the Player window
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + pViewport->WorkSize.x * 0.5f, pViewport->WorkPos.y + fEm * 11),
							ImGuiCond_FirstUseEver, ImVec2(0.5f, 0));
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 24, 0), ImVec2(fEm * 24, pViewport->WorkSize.y));
	ImGui::SetNextWindowBgAlpha(0.85f);

	if(ImGui::Begin("Ambient occlusion", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)==false)
	{
		ImGui::End();
		return;
	}

	// Kept in settings.cfg, like the renderer's other options
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	cAmbientOcclusionSettings settings = pPostEffects->GetAmbientOcclusionSettings();
	bool bChanged = false;

	bool bActive = pPostEffects->GetAmbientOcclusionActive();
	if(ImGui::Checkbox("Ambient occlusion", &bActive)) pPostEffects->SetAmbientOcclusionActive(bActive);
	if(ImGui::IsItemHovered())
		Tip("Darkens corners, creases and where things touch: the places light from all around "
			"reaches least. Worked out from the depth of what is on screen, for what is lit; "
			"lamps' lit parts, fog, particles and see-through things are left alone.");

	ImGui::BeginDisabled(bActive==false);
	ImGui::SameLine();
	if(ImGui::Button("Reset"))
	{
		// The debug choice is not a setting
		cAmbientOcclusionSettings defaults;
		defaults.mbShowOnly = settings.mbShowOnly;
		settings = defaults;
		bChanged = true;
	}
	if(ImGui::IsItemHovered()) Tip("Everything in this window back to its default. Ctrl+click a slider to type a value.");

	ImGui::PushItemWidth(fEm * 12);

	int lQuality = settings.GetQuality();
	if(ImGui::Combo("Quality", &lQuality, "Low\0Medium\0High\0Very high\0Ultra\0Custom\0") &&
		lQuality < cAmbientOcclusionSettings::klQualityNum)
	{
		settings.SetQuality(lQuality);
		bChanged = true;
	}
	if(ImGui::IsItemHovered())
		Tip("How many looks into the depth every pixel takes. A larger radius needs more of them. "
			"F3 shows the time it takes ('Ambient occlusion').");
	bChanged |= ImGui::SliderInt("Directions", &settings.mlSlices, 1, 8);
	if(ImGui::IsItemHovered()) Tip("Directions on screen each pixel is searched in. More: less grain.");
	bChanged |= ImGui::SliderInt("Steps", &settings.mlSteps, 1, 24);
	if(ImGui::IsItemHovered())
		Tip("Steps to each side of a direction. Too few: thin things are stepped over, and the shading comes in bands.");

	bChanged |= ImGui::Checkbox("Every other pixel in a frame", &settings.mbHalfRate);
	if(ImGui::IsItemHovered())
		Tip("Half of the pixels are searched in a frame and the other half in the next. "
			"Half the cost; when the view turns fast, the shading trails by a frame in half of its pixels.");

	bool bScreenRadius = settings.mfScreenRadius > 0;
	if(bScreenRadius)
	{
		float fPercent = settings.mfScreenRadius * 100.0f;
		if(ImGui::SliderFloat("Radius", &fPercent, 1.0f, 30.0f, "%.1f %% of height"))
		{
			settings.mfScreenRadius = fPercent / 100.0f;
			bChanged = true;
		}
	}
	else
	{
		bChanged |= ImGui::SliderFloat("Radius", &settings.mfRadius, 0.05f, 3.0f, "%.2f m", ImGuiSliderFlags_Logarithmic);
	}
	if(ImGui::IsItemHovered())
		Tip("How far from a point things keep light from it. Small: thin lines in creases. "
			"Large: soft shading over whole corners, with less detail.");
	if(ImGui::Checkbox("Radius grows with the distance", &bScreenRadius))
	{
		settings.mfScreenRadius = bScreenRadius ? 0.08f : 0.0f;
		bChanged = true;
	}
	if(ImGui::IsItemHovered())
		Tip("The same size on screen at every distance, instead of a length in the world: "
			"what is far away gets as much shading as what is near.");

	bChanged |= ImGui::SliderFloat("Strength", &settings.mfStrength, 0.0f, 3.0f, "%.2f");
	if(ImGui::IsItemHovered()) Tip("1 darkens by the share of the surroundings that is covered. More is darker.");
	bChanged |= ImGui::SliderFloat("Less in light", &settings.mfLightFade, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered())
		Tip("How much less what is brightly lit is darkened. 0: everything alike. "
			"1: only what is dim, which is where light from all around matters.");
	bChanged |= ImGui::SliderFloat("Fades by", &settings.mfFadeDistance, 2.0f, 100.0f, "%.0f m", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered()) Tip("The distance at which it has faded away.");
	bChanged |= ImGui::Combo("Filter", &settings.mlFilter, "Off\0Once\0Twice\0");
	if(ImGui::IsItemHovered())
		Tip("Every pixel of a 4x4 block searches differently; the filter averages the blocks, "
			"keeping edges. Off shows the pattern. Twice is smoother.");

	bChanged |= ImGui::Checkbox("Show the occlusion alone", &settings.mbShowOnly);
	if(ImGui::IsItemHovered()) Tip("White where nothing is darkened.");

	ImGui::PopItemWidth();
	ImGui::EndDisabled();
	if(bChanged) pPostEffects->SetAmbientOcclusionSettings(settings);

	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawLensAndFilm()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	// At the bottom, in the middle
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + pViewport->WorkSize.x * 0.5f,
									pViewport->WorkPos.y + pViewport->WorkSize.y - fEm),
							ImGuiCond_FirstUseEver, ImVec2(0.5f, 1));
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 26, 0), ImVec2(fEm * 26, pViewport->WorkSize.y));
	ImGui::SetNextWindowBgAlpha(0.85f);

	if(ImGui::Begin("Lens and film", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)==false)
	{
		ImGui::End();
		return;
	}

	// Kept in settings.cfg, like the renderer's other options
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	ImGui::PushItemWidth(fEm * 12);

	///////////////////////////////////////////
	// Depth of field
	if(ImGui::CollapsingHeader("Depth of field##section", ImGuiTreeNodeFlags_DefaultOpen))
	{
		cEffect_DepthOfField *pDoF = mpInit->mpEffectHandler->GetDepthOfField();
		cDepthOfFieldSettings settings = pPostEffects->GetDepthOfFieldSettings();
		bool bChanged = false;

		bool bDof = !pDoF->IsDisabled();
		if(ImGui::Checkbox("Depth of field", &bDof)) pDoF->SetDisabled(!bDof);
		if(ImGui::IsItemHovered())
			Tip("The game blurs what is out of focus when you look closely at something you picked up, "
				"and where a map's script asks for it. Every pixel is spread over a disc, from the depth of "
				"what is on screen; bright points become discs and what is in focus keeps its edges.");

		ImGui::BeginDisabled(bDof==false);
		ImGui::SameLine();
		if(ImGui::Button("Reset##dof"))
		{
			// The test is not a setting
			cDepthOfFieldSettings defaults;
			settings.mfRadius = defaults.mfRadius;
			settings.mlRings = defaults.mlRings;
			bChanged = true;
		}
		float fPercent = settings.mfRadius * 100.0f;
		if(ImGui::SliderFloat("Blur radius", &fPercent, 0.2f, 4.0f, "%.2f %% of height"))
		{
			settings.mfRadius = fPercent / 100.0f;
			bChanged = true;
		}
		if(ImGui::IsItemHovered()) Tip("How wide the blur of what is fully out of focus is.");
		bChanged |= ImGui::SliderInt("Rings", &settings.mlRings, 1, 5);
		if(ImGui::IsItemHovered())
			Tip("Rings of samples around every pixel: 8, 22, 43, 71 or 106 samples. Too few for the radius "
				"and a bright point shows as dots instead of a disc. F3 shows the time ('Depth of field').");
		ImGui::EndDisabled();

		ImGui::BeginDisabled(bDof==false);
		bChanged |= ImGui::Checkbox("Try it here", &settings.mbTest);
		if(ImGui::IsItemHovered())
			Tip("Switches the depth of field on now, focused as set below, whatever the game asks for. "
				"Not kept in the settings.");
		ImGui::BeginDisabled(settings.mbTest==false);
		bChanged |= ImGui::SliderFloat("In focus at", &settings.mfTestFocus, 0.2f, 30.0f, "%.2f m", ImGuiSliderFlags_Logarithmic);
		bChanged |= ImGui::SliderFloat("Fully blurred", &settings.mfTestRange, 0.05f, 30.0f, "%.2f m away", ImGuiSliderFlags_Logarithmic);
		if(ImGui::IsItemHovered()) Tip("How far in front of and behind the distance in focus the blur is full.");
		ImGui::EndDisabled();
		ImGui::EndDisabled();

		if(bChanged) pPostEffects->SetDepthOfFieldSettings(settings);
	}

	///////////////////////////////////////////
	// Motion blur
	if(ImGui::CollapsingHeader("Motion blur##section", ImGuiTreeNodeFlags_DefaultOpen))
	{
		cMotionBlurSettings settings = pPostEffects->GetMotionBlurSettings();
		bool bChanged = false;

		bool bMotion = pPostEffects->GetMotionBlurActive();
		if(ImGui::Checkbox("Motion blur", &bMotion)) pPostEffects->SetMotionBlurActive(bMotion);
		if(ImGui::IsItemHovered())
			Tip("One pass that blurs the picture along the way the camera's own turning and walking "
				"moves it, from the depth of what is on screen. Things that move by themselves stay sharp.");

		ImGui::BeginDisabled(bMotion==false);
		ImGui::SameLine();
		if(ImGui::Button("Reset##motion"))
		{
			settings = cMotionBlurSettings();
			bChanged = true;
		}
		float fMs = settings.mfShutterTime * 1000.0f;
		if(ImGui::SliderFloat("Shutter", &fMs, 0.5f, 40.0f, "%.1f ms", ImGuiSliderFlags_Logarithmic))
		{
			settings.mfShutterTime = fMs / 1000.0f;
			bChanged = true;
		}
		if(ImGui::IsItemHovered())
			Tip("How long the shutter is open: the picture is blurred over the way it moves in that time, "
				"the same at any frame rate. 8.3 ms is half of a frame at 60 a second, as a film camera has it; "
				"16.7 ms is all of it.");
		float fPercent = settings.mfMaxLength * 100.0f;
		if(ImGui::SliderFloat("Longest blur", &fPercent, 0.5f, 15.0f, "%.1f %% of height"))
		{
			settings.mfMaxLength = fPercent / 100.0f;
			bChanged = true;
		}
		if(ImGui::IsItemHovered()) Tip("However fast the view turns, the blur is no longer than this.");
		bChanged |= ImGui::SliderInt("Samples", &settings.mlSamples, 2, 32);
		if(ImGui::IsItemHovered()) Tip("Samples along the blur. Fewer: more grain in it.");
		bChanged |= ImGui::SliderFloat("Sharp nearer than", &settings.mfNearLimit, 0.0f, 2.0f, "%.2f m");
		if(ImGui::IsItemHovered())
			Tip("What is nearer than this is not blurred: what you hold moves with the camera, "
				"not across the picture.");
		ImGui::EndDisabled();

		if(bChanged) pPostEffects->SetMotionBlurSettings(settings);
	}

	///////////////////////////////////////////
	// Grain
	if(ImGui::CollapsingHeader("Noise filter##section", ImGuiTreeNodeFlags_DefaultOpen))
	{
		iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
		cPlayerNoiseFilter *pFilter = mpInit->mpPlayer->GetNoiseFilter();
		bool bChanged = false;

		int lType = pFilter->IsActive() ? 1 + (int)pFilter->GetType() : 0;
		if(ImGui::Combo("Noise filter", &lType, "Off\0Original\0Grain\0Film grain\0"))
		{
			pFilter->SetActive(lType > 0);
			if(lType > 0) pFilter->SetType((eNoiseFilterType)(lType - 1));
		}
		if(ImGui::IsItemHovered())
			Tip("Original: tiles of noise images over the whole screen, HUD included. "
				"Grain: noise added to the 3D picture, under the HUD. "
				"Film grain: the picture as made of grains that are developed or not, as on film: "
				"none in black or white, most in between, specks in the dark.");

		// After the filter has set the kind
		cFilmGrainSettings grain = pLowLevel->GetFilmGrain();

		ImGui::BeginDisabled(grain.mType == eFilmGrain_Off);
		ImGui::SameLine();
		if(ImGui::Button("Reset##grain"))
		{
			cFilmGrainSettings defaults;
			defaults.mType = grain.mType;
			grain = defaults;
			bChanged = true;
		}
		bChanged |= ImGui::SliderFloat("Strength", &grain.mfIntensity, 0.0f, 1.0f, "%.2f");
		bChanged |= ImGui::SliderFloat("Grain size", &grain.mfSize, 0.5f, 6.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("In pixels of a picture 1080 high. It grows with the picture, so the grain looks "
				"the same at any resolution, and is never smaller than a pixel.");
		bChanged |= ImGui::SliderFloat("Colour", &grain.mfColor, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("0: grey grain. 1: red, green and blue each have their own.");
		bChanged |= ImGui::SliderFloat("In the dark", &grain.mfShadows, 0.0f, 1.0f, "%.2f");
		bChanged |= ImGui::SliderFloat("In the light", &grain.mfHighlights, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("How much of the grain the dark and the bright parts of the picture get.");
		bChanged |= ImGui::SliderFloat("Changes", &grain.mfRate, 0.0f, 60.0f, "%.0f times a second");
		if(ImGui::IsItemHovered())
			Tip("How often the grain is new. Film runs at 24. 0: every frame, which at a high frame rate "
				"blends into a haze.");
		ImGui::BeginDisabled(grain.mType != eFilmGrain_Film);
		bChanged |= ImGui::SliderFloat("Size differs", &grain.mfDispersion, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("Film grain: how much the grains differ in size.");
		ImGui::EndDisabled();
		ImGui::EndDisabled();

		if(bChanged) pLowLevel->SetFilmGrain(grain);
	}

	ImGui::PopItemWidth();
	ImGui::End();
}

//-----------------------------------------------------------------------

// Multisampling, the texture filter and SMAA. Kept in settings.cfg, like
// the renderer's other options.
void cRefView::DrawShading()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	// At the bottom left
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + fEm, pViewport->WorkPos.y + pViewport->WorkSize.y - fEm),
							ImGuiCond_FirstUseEver, ImVec2(0, 1));
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 26, 0), ImVec2(fEm * 26, pViewport->WorkSize.y));
	ImGui::SetNextWindowBgAlpha(0.85f);

	if(ImGui::Begin("Shading", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)==false)
	{
		ImGui::End();
		return;
	}

	// Kept in settings.cfg, like the renderer's other options
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cShadingSettings shading = pLowLevel->GetShading();
	bool bChanged = false;

	ImGui::TextUnformatted("Shading");
	if(ImGui::IsItemHovered())
		Tip("Highlights with a width and strength of their own for each surface, coloured by "
			"the light, and ambient light that shows the normal maps.");
	ImGui::SameLine();
	if(ImGui::Button("Reset##shading"))
	{
		shading = cShadingSettings();
		bChanged = true;
	}

	ImGui::PushItemWidth(fEm * 12);

	if(ImGui::CollapsingHeader("Highlights", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bChanged |= ImGui::SliderFloat("Strength", &shading.mfSpecular, 0.0f, 4.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("Scales the highlights of all lights. 0: none.");
		bChanged |= ImGui::SliderFloat("Reflectance", &shading.mfReflectance, 0.02f, 1.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(ImGui::IsItemHovered())
			Tip("The share of light mirrored, faced straight on, where a material's specular map is white. "
				"A mid-grey map gets about a fifth of it. 0.04 is plastic or wet stone, above 0.5 bare metal. "
				"Materials whose normal map has no specular amount count as white.");
		bChanged |= ImGui::SliderFloat("Roughness, dull", &shading.mfRoughDull, 0.05f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("Where the specular map is dark: higher is a wider, fainter highlight.");
		bChanged |= ImGui::SliderFloat("Roughness, shiny", &shading.mfRoughShiny, 0.05f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("Where the specular map is white: lower is a smaller, brighter highlight.");
	}

	if(ImGui::CollapsingHeader("Ambient light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bChanged |= ImGui::SliderFloat("From above", &shading.mfAmbientUp, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How much more ambient light what faces up gets, and how much less what faces down. "
				"0: the same from every side, as in the original.");
		bChanged |= ImGui::SliderFloat("Towards the viewer", &shading.mfAmbientFacing, 0.0f, 1.5f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How much more ambient light what faces the viewer gets than what is seen at a slant. "
				"Brings out the normal maps in the dark. 0: the same from every side.");
		bChanged |= ImGui::SliderFloat("Sheen", &shading.mfAmbientSheen, 0.0f, 2.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("Ambient light mirrored at grazing angles by what has specular, more by what is glossy.");
	}

	ImGui::PopItemWidth();

	if(bChanged) pLowLevel->SetShading(shading);

	///////////////////////////////////////////
	// Flashlight
	if(ImGui::CollapsingHeader("Flashlight##section", ImGuiTreeNodeFlags_DefaultOpen))
	{
		cPlayerFlashLight *pFlashLight = mpInit->mpPlayer->GetFlashLight();
		cFlashlightSettings flashlight = pFlashLight->GetSettings();
		bChanged = false;

		if(ImGui::Button("Reset##flashlight"))
		{
			flashlight = cFlashlightSettings();
			bChanged = true;
		}
		if(ImGui::IsItemHovered())
			Tip("Brighter near and fading with the distance, a beam with a hotspot made by the "
				"shader in place of the 256 pixel picture, a warmer colour, and light coming back from "
				"what the beam lands on.");

		ImGui::PushItemWidth(fEm * 12);
		bChanged |= ImGui::SliderFloat("Brightness", &flashlight.mfBrightness, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if(ImGui::IsItemHovered()) Tip("The amount of light: 2 is twice as much.");
		bChanged |= ImGui::SliderFloat("Warmth", &flashlight.mfWarmth, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("0: the white of the original. 1: the yellow of a weak bulb.");
		float fReachMetres = flashlight.mfReach * 12.0f;
		if(ImGui::SliderFloat("Reach", &fReachMetres, 6.0f, 36.0f, "%.0f m"))
		{
			flashlight.mfReach = fReachMetres / 12.0f;
			bChanged = true;
		}
		if(ImGui::IsItemHovered())
			Tip("How far the beam goes before it has faded out. The original's is 12 m. "
				"Close up the beam stays as bright as it is.");
		bChanged |= ImGui::SliderFloat("Even at", &flashlight.mfEvenAt, 0.1f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("The distance, as a share of the original's 12 m, at which the beam is as bright as its "
				"colour says. Nearer it is brighter, farther dimmer. Higher lights more of the distance.");
		bChanged |= ImGui::SliderFloat("Brightest", &flashlight.mfNearBoost, 1.0f, 8.0f, "%.1f times");
		if(ImGui::IsItemHovered()) Tip("How many times brighter than that it gets at most, close up.");
		bChanged |= ImGui::SliderFloat("Hotspot", &flashlight.mfHotspot, 0.05f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("The size of the bright middle, as a share of the cone.");
		bChanged |= ImGui::SliderFloat("Field", &flashlight.mfField, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How bright the beam is around the hotspot: 0 only the hotspot, 1 an even disc.");
		bChanged |= ImGui::SliderFloat("Width", &flashlight.mfWidth, 0.3f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("The size of the beam, as a share of the cone the light has.");
		bChanged |= ImGui::SliderFloat("Bounce", &flashlight.mfBounce, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("Light coming back from what the beam lands on: a weak light there, without shadows. 0: none.");
		ImGui::PopItemWidth();

		if(bChanged) pFlashLight->SetSettings(flashlight);
	}

	///////////////////////////////////////////
	// Lamps
	if(ImGui::CollapsingHeader("Lamps##section", ImGuiTreeNodeFlags_DefaultOpen))
	{
		cLampAppearanceController *pLampAppearanceController = mpInit->mpLampAppearanceController;
		cLampSettings lamps = pLampAppearanceController->GetSettings();
		bChanged = false;

		if(ImGui::Button("Reset##lamps"))
		{
			lamps = cLampSettings();
			bChanged = true;
		}
		if(ImGui::IsItemHovered())
			Tip("The glow stick, the flare and the lamps of the world (paraffin lamps, bulbs, fluorescent "
				"tubes, emergency and tunnel lights): flames waver and sway, and their halos "
				"with them, each kind of lamp has a colour of its own, lamps are brighter close by, and "
				"tubes have long highlights.");
		ImGui::SameLine();
		ImGui::TextDisabled("%d lights with a look in this map", pLampAppearanceController->GetLampCount());

		ImGui::PushItemWidth(fEm * 12);
		bChanged |= ImGui::SliderFloat("Flicker", &lamps.mfFlicker, 0.0f, 3.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How much the brightness of flames wavers (paraffin lamps, the flare). 0: steady. "
				"The \"No flicker\" option of the Player window holds them still too.");
		bChanged |= ImGui::SliderFloat("Sway", &lamps.mfSway, 0.0f, 3.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How far flames wander: 1 is about 3 cm. It is what makes their shadows move.");
		bChanged |= ImGui::SliderFloat("Colour", &lamps.mfColour, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("0: the colours the lamps have in the game. 1: each kind all its own: flames orange, "
				"bulbs warm, tubes and tunnel lights cold, the glow stick green.");
		bChanged |= ImGui::Checkbox("Brighter near the lamp", &lamps.mbFalloff);
		if(ImGui::IsItemHovered())
			Tip("Light that falls off with the distance, as the improved flashlight's does, in place of "
				"the original's ramp, which lights most of a lamp's reach evenly.");
		ImGui::BeginDisabled(lamps.mbFalloff==false);
		bChanged |= ImGui::SliderFloat("Even at##lamps", &lamps.mfEvenAt, 0.1f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("The distance, as a share of the lamp's reach, at which it is as bright as its colour "
				"says. Nearer it is brighter, farther dimmer. Higher lights more of the reach.");
		bChanged |= ImGui::SliderFloat("Brightest##lamps", &lamps.mfNearBoost, 1.0f, 8.0f, "%.1f times");
		if(ImGui::IsItemHovered()) Tip("How many times brighter than that it gets at most, close up.");
		ImGui::EndDisabled();
		bChanged |= ImGui::SliderFloat("Tube length", &lamps.mfTube, 0.0f, 2.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("Fluorescent tubes: how long their highlights are, in times the tube's own length. "
				"0: a point, as in the original.");
		bChanged |= ImGui::Checkbox("Lights of the maps too", &lamps.mbMapLights);
		if(ImGui::IsItemHovered())
			Tip("The lights that are part of a map and not of a lamp (56 in the game): brighter near "
				"them as well, unless they reach further than 20 m or have a falloff of their own, "
				"and the furnace's lights waver. Their colours stay.");
		ImGui::PopItemWidth();

		if(bChanged) pLampAppearanceController->SetSettings(lamps);
	}

	///////////////////////////////////////////
	// Halos, shafts of light
	if(ImGui::CollapsingHeader("Halos, light shafts and particles##section", ImGuiTreeNodeFlags_DefaultOpen))
	{
		cEffectSettings effects = pLowLevel->GetEffects();
		bChanged = false;

		if(ImGui::Button("Reset##effects"))
		{
			effects = cEffectSettings();
			bChanged = true;
		}
		ImGui::PushItemWidth(fEm * 12);
		ImGui::TextUnformatted("Halos");
		if(ImGui::IsItemHovered())
			Tip("The glow round lamps, drawn by a shader from the shape of the halo's picture, "
				"so the glow fades out without steps or blocks, and hidden by what stands in front of "
				"the lamp as the depth buffer has it: it fades as an edge passes, in the same frame.");
		bChanged |= ImGui::SliderFloat("Brightness##halo", &effects.mfHaloBrightness, 0.0f, 3.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("1: as bright as the original's.");
		bChanged |= ImGui::SliderFloat("Rays", &effects.mfHaloStreaks, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How much is kept of what a halo's picture has besides the round glow: the rays of the "
				"star-shaped ones. 0: only the glow.");

		ImGui::TextUnformatted("Light shafts");
		if(ImGui::IsItemHovered())
			Tip("The beams of light under grates and windows: a body of lit air in place of a "
				"flat sheet. It ends softly at walls and the floor, looks right from every side and "
				"can be walked into.");
		bChanged |= ImGui::SliderFloat("Brightness##shaft", &effects.mfShaftBrightness, 0.0f, 3.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("1: as bright as the original's, seen from the side.");
		bChanged |= ImGui::SliderFloat("Dust", &effects.mfShaftDust, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered()) Tip("How uneven the lit air is: streaks that drift along the shaft. 0: even.");

		ImGui::TextUnformatted("Particles");
		if(ImGui::IsItemHovered())
			Tip("Smoke, steam, mist, dust, snow, drops, fire and sparks: they fade out where "
				"they meet the floor and walls and close to the eye, and all but fire and sparks are "
				"lit by the lamps and the flashlight around them: dim in the dark, as bright as the original "
				"draws them in the light. "
				"Light is not stopped by walls on its way to them.");
		bChanged |= ImGui::SliderFloat("Soft edges", &effects.mfParticleSoft, 0.0f, 3.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("The distance over which a particle fades out in front of what is behind it, in times "
				"its own size. 0: a hard line, as in the original.");
		bChanged |= ImGui::SliderFloat("Lighting", &effects.mfParticleLight, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("How much the lights count. 0: as bright everywhere as the original draws them.");
		bChanged |= ImGui::SliderFloat("In the dark", &effects.mfParticleDark, 0.0f, 1.0f, "%.2f");
		if(ImGui::IsItemHovered())
			Tip("Their brightness where no light reaches them, where the original's is 1. The map's "
				"ambient light is added to it.");
		ImGui::PopItemWidth();

		if(bChanged) pLowLevel->SetEffects(effects);
	}

	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawAntiAliasing()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	// At the bottom right, folded until it is wanted: open, it covers the end
	// of the window above it on a small screen. Its corner is put back
	// in the screen's for a few frames when it is folded or unfolded (it
	// takes a frame to know its new size), so that it opens upwards.
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + pViewport->WorkSize.x - fEm,
									pViewport->WorkPos.y + pViewport->WorkSize.y - fEm),
							mlAntiAliasingPlace > 0 ? ImGuiCond_Always : ImGuiCond_FirstUseEver, ImVec2(1, 1));
	if(mlAntiAliasingPlace > 0) --mlAntiAliasingPlace;
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 26, 0), ImVec2(fEm * 26, pViewport->WorkSize.y));
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.85f);

	bool bShown = ImGui::Begin("Anti-aliasing", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
	if(bShown == mbAntiAliasingFolded)
	{
		mbAntiAliasingFolded = !bShown;
		mlAntiAliasingPlace = 3;
	}
	if(bShown==false)
	{
		ImGui::End();
		return;
	}
	ImGui::PushItemWidth(fEm * 12);

	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cMaterialManager *pMaterialManager = mpInit->mpGame->GetResources()->GetMaterialManager();

	// Off, 2x, 4x, 8x: as far as the graphics card goes
	int lMaxSamples = cMath::Min(pLowLevel->GetCaps(eGraphicCaps_MaxMultisampling), 8);
	int lSamples = 0;
	for(int i = pLowLevel->GetMultisampling(); i > 1; i /= 2) ++lSamples;
	if(ImGui::SliderInt("Multisampling", &lSamples, 0, lMaxSamples >= 8 ? 3 : (lMaxSamples >= 4 ? 2 : (lMaxSamples >= 2 ? 1 : 0)),
						lSamples == 0 ? "Off" : (lSamples == 1 ? "2x" : (lSamples == 2 ? "4x" : "8x"))))
	{
		pLowLevel->SetMultisampling(lSamples == 0 ? 0 : 1 << lSamples);
		mpInit->mlFSAA = pLowLevel->GetMultisampling();
	}
	if(ImGui::IsItemHovered())
		Tip("Samples per pixel of the 3D scene, along the edges of surfaces and of cut-out textures. "
			"The effects that are drawn over the finished scene work on one sample a pixel.");

	int lAnisotropy = 0;
	for(int i = (int)pMaterialManager->GetTextureAnisotropy(); i > 1; i /= 2) ++lAnisotropy;
	int lMaxAnisotropy = 0;
	for(int i = pLowLevel->GetCaps(eGraphicCaps_MaxAnisotropicFiltering); i > 1; i /= 2) ++lMaxAnisotropy;
	int lFilter = pMaterialManager->GetTextureFilter() == eTextureFilter_Bilinear ? 0 : 1 + lAnisotropy;
	static const char *vFilters[] = {"Bilinear", "Trilinear", "Anisotropic 2x", "Anisotropic 4x",
									"Anisotropic 8x", "Anisotropic 16x"};
	if(ImGui::SliderInt("Texture filter", &lFilter, 0, std::min(1 + lMaxAnisotropy, 5), vFilters[std::clamp(lFilter, 0, 5)]))
	{
		pMaterialManager->SetTextureFilter(lFilter == 0 ? eTextureFilter_Bilinear : eTextureFilter_Trilinear);
		pMaterialManager->SetTextureAnisotropy(lFilter <= 1 ? 1.0f : (float)(1 << (lFilter - 1)));
	}
	if(ImGui::IsItemHovered())
		Tip("Of every texture with mipmaps. Bilinear shows a line where one mipmap takes over from the "
			"next; anisotropic keeps floors and walls sharp where they are seen at a slant.");

	cSmaaSettings smaa = pLowLevel->GetSmaa();
	bool bChanged = false;

	ImGui::SeparatorText("SMAA");
	bChanged |= ImGui::Checkbox("Edge smoothing (SMAA)", &smaa.mbActive);
	if(ImGui::IsItemHovered())
		Tip("Finds the edges in the finished picture, follows each along to tell its shape, and blends "
			"the pixels on it by how much the true edge would cover. Also smooths what multisampling "
			"does not: shadows, cut-out textures. F3 shows the time ('SMAA').");
	ImGui::BeginDisabled(smaa.mbActive==false);
	ImGui::SameLine();
	if(ImGui::Button("Reset##smaa"))
	{
		cSmaaSettings defaults;
		defaults.mbActive = smaa.mbActive;
		smaa = defaults;
		bChanged = true;
	}
	int lQuality = smaa.GetQuality();
	static const char *vQualities[] = {"Low", "Medium", "High", "Ultra", "Custom"};
	if(ImGui::SliderInt("Quality", &lQuality, 0, cSmaaSettings::klQualityNum - 1, vQualities[lQuality]))
	{
		smaa.SetQuality(lQuality);
		bChanged = true;
	}
	int lEdges = smaa.mEdges;
	if(ImGui::Combo("Edges by", &lEdges, "Brightness\0Colour, largest\0Colour, all\0Depth alone\0"))
	{
		smaa.mEdges = (eSmaaEdges)lEdges;
		bChanged = true;
	}
	if(ImGui::IsItemHovered())
		Tip("What makes an edge between two pixels: a difference in brightness, the largest difference "
			"of red, green and blue, all three together, or a difference in depth alone (outlines only, "
			"nothing on surfaces or in shadows).");
	bChanged |= ImGui::SliderFloat("Threshold", &smaa.mfThreshold, 0.02f, 0.25f, "%.3f");
	if(ImGui::IsItemHovered()) Tip("How much two pixels differ where there is an edge. Lower finds more, and softens more detail.");
	bChanged |= ImGui::SliderInt("Search steps", &smaa.mlSearchSteps, 1, 112);
	if(ImGui::IsItemHovered()) Tip("How far an edge is followed along, in steps of two pixels. Long, nearly level edges need many.");
	bChanged |= ImGui::SliderInt("Diagonal steps", &smaa.mlDiagonalSteps, 0, 20);
	if(ImGui::IsItemHovered()) Tip("The same for diagonal edges, in pixels. 0: diagonals are not looked for.");
	bChanged |= ImGui::SliderInt("Corner rounding", &smaa.mlCornerRounding, 0, 100, "%d %%");
	if(ImGui::IsItemHovered()) Tip("How much corners are smoothed. Low keeps them, and text, sharp.");

	bChanged |= ImGui::Checkbox("Depth helps", &smaa.mbDepthHelp);
	if(ImGui::IsItemHovered())
		Tip("Where the depth has an edge the threshold is lowered: outlines are found though the "
			"colours on both sides are close, as they are in the dark.");
	ImGui::BeginDisabled(smaa.mbDepthHelp==false && smaa.mEdges != eSmaaEdges_Depth);
	float fPercent = smaa.mfDepthThreshold * 100.0f;
	if(ImGui::SliderFloat("Depth edge", &fPercent, 0.5f, 20.0f, "%.1f %% of the distance"))
	{
		smaa.mfDepthThreshold = fPercent / 100.0f;
		bChanged = true;
	}
	ImGui::EndDisabled();
	ImGui::BeginDisabled(smaa.mbDepthHelp==false);
	bChanged |= ImGui::SliderFloat("Threshold there", &smaa.mfDepthStrength, 0.0f, 0.95f, "lower by %.2f");
	bChanged |= ImGui::SliderFloat("Threshold elsewhere", &smaa.mfDepthScale, 1.0f, 4.0f, "x %.2f");
	if(ImGui::IsItemHovered())
		Tip("Above 1 the threshold is raised where the depth has no edge: detail on surfaces is "
			"left alone, and there is less to do.");
	ImGui::EndDisabled();

	bChanged |= ImGui::Combo("Show", &smaa.mlShow, "Picture\0Edges\0Weights\0");
	ImGui::EndDisabled();

	if(bChanged) pLowLevel->SetSmaa(smaa);

	ImGui::PopItemWidth();
	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawShadowLights()
{
	const float fEm = ImGui::GetFontSize();
	cRenderer3D *pRenderer = mpInit->mpGame->GetGraphics()->GetRenderer3D();
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	bool bDefault = mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	if(pWorld==nullptr || bDefault==false || msViewsMap == "")
	{
		ImGui::TextDisabled("No map loaded.");
		return;
	}

	// The lights the last frame drew. Only compared, not used: the list is
	// the world's.
	std::set<iLight3D*> setInView;
	cLight3DIterator viewIt = pRenderer->GetRenderList()->GetLightIt();
	while(viewIt.HasNext()) setInView.insert(viewIt.Next());

	std::vector<iLight3D*> vLights;
	iLight3D *pPicked = nullptr;
	cLight3DListIterator lightIt = pWorld->GetLightIterator();
	while(lightIt.HasNext())
	{
		iLight3D *pLight = lightIt.Next();
		if(pLight->GetName() == msShadowLight) pPicked = pLight;
		if(mbShadowAllLights==false && setInView.count(pLight) == 0) continue;
		vLights.push_back(pLight);
	}
	std::sort(vLights.begin(), vLights.end(), [](iLight3D *apA, iLight3D *apB){ return apA->GetName() < apB->GetName(); });

	ImGui::Text("%d %s", (int)vLights.size(), mbShadowAllLights ? "in the map" : "in view");
	ImGui::SameLine();
	ImGui::Checkbox("All of the map", &mbShadowAllLights);

	if(ImGui::BeginTable("##lights", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit,
							ImVec2(-1, fEm * 8)))
	{
		ImGui::TableSetupColumn("Light", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Kind");
		ImGui::TableSetupColumn("Reach");
		ImGui::TableSetupColumn("Casters");
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();
		for(iLight3D *pLight : vLights)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::PushID(pLight);
			if(ImGui::Selectable(pLight->GetName().c_str(), pLight == pPicked, ImGuiSelectableFlags_SpanAllColumns))
			{
				msShadowLight = pLight == pPicked ? "" : pLight->GetName();
			}
			ImGui::PopID();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(pLight->GetLightType() == eLight3DType_Spot ? "spot" : "point");
			ImGui::TableNextColumn();
			ImGui::Text("%.1f m", pLight->GetFarAttenuation());
			ImGui::TableNextColumn();
			if(pLight->GetCastShadows())
				ImGui::Text("%d + %d", (int)pLight->GetStaticCasters()->size(), (int)pLight->GetDynamicCasters()->size());
			else
				ImGui::TextDisabled("none");
		}
		ImGui::EndTable();
	}
	if(ImGui::IsItemHovered())
		Tip("Casters: things in the light's reach that cast shadows, still + moving. "
							"'none': the light casts no shadows. Click a light to pick it.");

	// The light picked: found by name, it may have gone
	if(msShadowLight != "" && pPicked==nullptr) msShadowLight = "";
	cShadowMapSettings settings = pRenderer->GetShadowMapSettings();
	bool bOnly = pPicked && settings.msOnlyLight == msShadowLight;
	ImGui::BeginDisabled(pPicked==nullptr);
	if(ImGui::Checkbox("Only this light", &bOnly) || (bOnly==false && settings.msOnlyLight != "" && settings.msOnlyLight != msShadowLight))
	{
		settings.msOnlyLight = bOnly ? msShadowLight : "";
		pRenderer->SetShadowMapSettings(settings);
	}
	if(ImGui::IsItemHovered()) Tip("The other lights are not drawn. The ambient light stays.");
	ImGui::SameLine();
	bool bCasts = pPicked && pPicked->GetCastShadows();
	if(ImGui::Checkbox("Casts shadows", &bCasts) && pPicked) pPicked->SetCastShadows(bCasts);
	if(ImGui::IsItemHovered())
		Tip("The maps say which lights cast shadows. This changes it until the map is loaded "
							"again; try it on the flashlight.");
	ImGui::EndDisabled();
	// Picking no light, or losing it, shows all again
	if(pPicked==nullptr && settings.msOnlyLight != "")
	{
		settings.msOnlyLight = "";
		pRenderer->SetShadowMapSettings(settings);
	}

	ImGui::Separator();
	bool bHasTestLight = pWorld->GetLight(gsTestLightName) != nullptr;
	if(ImGui::Button(bHasTestLight ? "Move test light here" : "Put a test light here")) mbPlaceTestLight = true;
	if(ImGui::IsItemHovered())
		Tip("A point light that casts shadows, where the camera is. It is gone when the map changes.");
	ImGui::SameLine();
	ImGui::BeginDisabled(bHasTestLight==false);
	if(ImGui::Button("Remove")) mbRemoveTestLight = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Checkbox("Carry it", &mbCarryTestLight);
	if(ImGui::IsItemHovered()) Tip("The test light stays with the camera: walk or fly, and watch the shadows move.");
	ImGui::PushItemWidth(fEm * 14);
	ImGui::SliderFloat("Reach", &mfTestLightRadius, 1.0f, 20.0f, "%.1f m");
	ImGui::SliderFloat("Brightness", &mfTestLightBrightness, 0.1f, 1.0f, "%.2f");
	ImGui::PopItemWidth();
}

//-----------------------------------------------------------------------
