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
	mlFlySeed = 0;
	msViewpointName[0] = 0;
	mbAddViewpoint = false;
	mbDeleteViewpoint = false;

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
		// The body would walk off with the same keys
		pBody->SetActive(!mbFlying);
		mlFlySeed = 1;
	}
	if(mbFlying==false) return;
	// A map change brings the body back
	if(pBody->IsActive()) pBody->SetActive(false);

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
		float fSpeed = (pInput->IsTriggerd("Run") ? 12.0f : 3.0f) * afTimeStep;
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

void cRefView::Release()
{
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
		if(ImGui::CollapsingHeader("As in the reference images")) DrawOptions();
		if(ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) DrawRenderer();

		ImGui::Separator();
		ImGui::TextWrapped("F2 closes the panel and gives the mouse back: look and walk, or fly, "
							"from the place. Without Fly the camera stays at a viewpoint or light "
							"until released.");
	}
	ImGui::End();

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

	ImGui::Checkbox("Fly", &mbFly);
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("With the panel closed: the movement keys move the camera, Run is faster, "
							"Jump and Crouch go up and down. The player's body stays where it is.");

	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
	ImGui::InputTextWithHint("##viewpoint", "name (optional)", msViewpointName, sizeof(msViewpointName));
	ImGui::SameLine();
	if(ImGui::Button("Add viewpoint here")) mbAddViewpoint = true;
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Saves where the camera is, where it looks and whether the flashlight is on. "
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

void cRefView::DrawOptions()
{
	if(mbUsed==false) ImGui::TextDisabled("Applied once a map or place is picked here.");
	if(mbSetFlashLight==false) mbFlashLight = mpInit->mpPlayer->GetFlashLight()->IsActive();
	if(ImGui::Checkbox("Flashlight on", &mbFlashLight))
	{
		mbSetFlashLight = true;
		mbUsed = true;
	}
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Its key works too. A viewpoint is saved with the flashlight as it is.");
	ImGui::SameLine();
	ImGui::Checkbox("No flicker", &mbNoFlicker);
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Flickering lights stay on. Lights already stopped stay so until the map is loaded again.");
	ImGui::Checkbox("3D view only", &mbHide2D);
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("No HUD, fades, damage or hiding effects.");
	ImGui::SameLine();
	ImGui::Checkbox("Cannot be hurt", &mbGodMode);
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
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("The sliders back to their defaults. Ctrl+click a slider to type a value.");
	ImGui::PushItemWidth(ImGui::GetFontSize() * 14);
	bChanged |= ImGui::SliderFloat("Light power", &tuning.mfLightExp, 1.0f, 2.2f, "%.2f");
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("How a light spreads. 2.2: each light as in the original. "
							"Lower: wider and flatter, brighter towards its edge.");
	bChanged |= ImGui::SliderFloat("Exposure", &tuning.mfExposure, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("Brightness of the whole 3D view, in light: 2 is one stop up.");
	bChanged |= ImGui::SliderFloat("Glow", &tuning.mfGlow, 0.25f, 8.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Light of halos, lamps' lit parts, additive materials and particles: "
							"how far their brightest parts go past white. Faint parts are left alone.");
	bChanged |= ImGui::SliderFloat("Blend match", &tuning.mfBlendMatch, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Particles, see-through materials and fog: 1 covers as much as without "
							"linear lighting, 0 blends them as light, which makes smoke, snow and fog heavier.");
	bChanged |= ImGui::SliderFloat("Shoulder", &tuning.mfShoulder, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Tonemapping: the screen brightness above which bright parts roll off "
							"towards white instead of clipping. Darker parts are untouched. 1 is off.");
	ImGui::BeginDisabled(tuning.mfShoulder > 0.999f);
	bChanged |= ImGui::SliderFloat("White", &tuning.mfWhite, 1.0f, 16.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Tonemapping: how many times brighter than white something has to be "
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
		ImGui::SetTooltip("The screen brightness at which things begin to glow; 1 is white. "
							"Above 1 only what is brighter than white glows (needs linear lighting).");
	bChanged |= ImGui::SliderFloat("Intensity", &bloom.mfIntensity, 0.0f, 2.0f, "%.2f");
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("How strong the glow is.");
	bChanged |= ImGui::SliderFloat("Spread", &bloom.mfScatter, 0.0f, 1.0f, "%.2f");
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("How far the glow reaches: tight around the source, or a wide haze.");
	ImGui::PopItemWidth();
	ImGui::EndDisabled();
	if(bChanged) pPostEffects->SetBloomTuning(bloom);
}

//-----------------------------------------------------------------------
