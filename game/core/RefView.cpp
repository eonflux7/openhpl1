/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/RefView.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "ui/MainMenu.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
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
	mbFlashLight = true;
	mbNoFlicker = true;
	mbHide2D = true;
	mbGodMode = true;
	mbSkippingDrawing = false;
	mbLinearLighting = false;
	mbSetLinearLighting = false;
	mbSetFlashLight = false;

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
		if(mvMaps.empty()) cRefShots::FindMaps(mpInit, mvMaps);
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
	bool bDefault = mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";

	if(mbSetLinearLighting)
	{
		mbSetLinearLighting = false;
		mpInit->mpGame->GetGraphics()->SetLinearLighting(mbLinearLighting);
	}

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
		mpInit->mpMapHandler->Load(asMap, "");
	}
	else
	{
		// Loading a map again gives its places anew
		msViewsMap = "";
		mpInit->mpMapHandler->ChangeMap(asMap, "", "", "", 0, 0, "", "");
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
	// The flashlight as the frame has it
	mbFlashLight = mvViews[mlView].mbFlashLight;
	mbSetFlashLight = true;

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
		ImGui::TextWrapped("F2 closes the panel and gives the mouse back: look around from the "
							"place. The camera stays there until Release, which puts the player "
							"there, or Fly (Player window).");
	}
	ImGui::End();

	DrawShadows();
	DrawPlayer();

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
		Tip("Its key works too, and it does not run down. A viewpoint is saved with the flashlight as it is.");
	ImGui::SameLine();
	if(ImGui::Checkbox("Cannot be hurt", &mbGodMode)) mbUsed = true;
	if(ImGui::Checkbox("No flicker", &mbNoFlicker)) mbUsed = true;
	if(ImGui::IsItemHovered())
		Tip("Flickering lights stay on. Lights already stopped stay so until the map is loaded again.");
	ImGui::SameLine();
	if(ImGui::Checkbox("3D view only", &mbHide2D)) mbUsed = true;
	if(ImGui::IsItemHovered()) Tip("No HUD, fades, damage or hiding effects.");

	ImGui::EndDisabled();
	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawRenderer()
{
	// Not while a click waits for the next step: the panel can be drawn
	// several times before it
	if(mbSetLinearLighting==false) mbLinearLighting = mpInit->mpGame->GetGraphics()->GetLinearLighting();
	if(ImGui::Checkbox("Linear lighting", &mbLinearLighting)) mbSetLinearLighting = true;

	// The values it is tuned with: kept in settings.cfg, like the option
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cLinearLightingTuning tuning = pLowLevel->GetLinearLightingTuning();
	bool bChanged = false;
	ImGui::BeginDisabled(mbLinearLighting==false);
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
		Tip("Particles, see-through materials and fog: 1 covers as much as without "
							"linear lighting, 0 blends them as light, which makes smoke, snow and fog heavier.");
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
	ImGui::EndDisabled();
	if(bChanged) pLowLevel->SetLinearLightingTuning(tuning);

	// Bloom: these too are kept in settings.cfg
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	int lBloom = pPostEffects->GetBloomActive() ? 1 + (int)pPostEffects->GetBloomType() : 0;
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
	if(ImGui::Combo("Bloom", &lBloom, "Off\0Original\0Soft\0"))
	{
		pPostEffects->SetBloomActive(lBloom > 0);
		if(lBloom > 0) pPostEffects->SetBloomType((eBloomType)(lBloom - 1));
	}
	cBloomTuning bloom = pPostEffects->GetBloomTuning();
	bChanged = false;
	ImGui::BeginDisabled(lBloom != 1 + eBloomType_Soft);
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
							"Above 1 only what is brighter than white glows (needs linear lighting).");
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

	int lType = pRenderer->GetShadowType();
	if(ImGui::Combo("Kind", &lType, "Original (volumes)\0Shadow maps\0"))
		pRenderer->SetShadowType((eShadowType)lType);
	if(ImGui::IsItemHovered())
		Tip("Original: the game's hard-edged shadows.\n"
							"Shadow maps: what each light sees is drawn into a depth picture, "
							"which the lit surfaces look themselves up in.");

	ImGui::BeginDisabled(lType != eShadowType_Maps || lShow == eRendererShowShadows_None);

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

	ImGui::PopItemWidth();
	if(bChanged) pRenderer->SetShadowMapSettings(settings);

	if(ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) DrawShadowLights();

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
