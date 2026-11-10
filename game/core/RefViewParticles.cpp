/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
// The F2 panel's Particles window: a dot and a name on the screen at each
// particle system of the map, a list of them, and switching the systems an
// addon replaces (flipbook materials, say) between the addon's
// look ("new") and the game's own files ("old"), one picked system or all.
#include "core/StdAfx.h"
#include "core/RefView.h"
#include "core/Init.h"
#include "player/Player.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace
{
	struct cParticleMarker
	{
		cParticleSystem3D *mpPS;
		float mfDist;
		ImVec2 mvScreen;
		bool mbOnScreen;
	};

	// The screen position of a point in the world, in ImGui's coordinates;
	// false behind the camera
	bool ToScreen(cCamera3D *apCamera, const cVector3f &avWorld, ImVec2 &avScreen)
	{
		cVector3f vView = cMath::MatrixMul(apCamera->GetViewMatrix(), avWorld);
		if(vView.z > -apCamera->GetNearClipPlane()) return false;
		cVector3f vClip = cMath::MatrixMulDivideW(apCamera->GetProjectionMatrix(), vView);
		if(std::fabs(vClip.x) > 1.2f || std::fabs(vClip.y) > 1.2f) return false;
		const ImVec2 &vSize = ImGui::GetIO().DisplaySize;
		avScreen = ImVec2((vClip.x * 0.5f + 0.5f) * vSize.x, (0.5f - vClip.y * 0.5f) * vSize.y);
		return true;
	}

	// Green: the addon's look. Orange: the game's. Grey: no addon replaces it.
	ImU32 MarkerColor(cParticleSystem3D *apPS, bool abPicked, bool abDimmed)
	{
		int r = 170, g = 170, b = 170;
		if(apPS->HasOriginal())
		{
			if(apPS->IsOriginal()) { r = 255; g = 160; b = 40; }
			else { r = 80; g = 225; b = 110; }
		}
		if(abPicked)
		{
			// Lighter
			r += (255 - r) / 2;
			g += (255 - g) / 2;
			b += (255 - b) / 2;
		}
		return IM_COL32(r, g, b, abDimmed ? 110 : 255);
	}

	void Tip(const char *apText)
	{
		if(ImGui::BeginTooltip()==false) return;
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28);
		ImGui::TextUnformatted(apText);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}

	const char *VariantName(cParticleSystem3D *apPS)
	{
		if(apPS->HasOriginal() == false) return "-";
		return apPS->IsOriginal() ? "old" : "new";
	}

	cWorld3D *MapWorld(cInit *apInit)
	{
		if(apInit->mpGame->GetUpdater()->GetCurrentContainerName() != "Default") return nullptr;
		return apInit->mpGame->GetScene()->GetWorld3D();
	}

	// abScreen: where they are on the screen (inside an ImGui frame only)
	void Gather(cInit *apInit, std::vector<cParticleMarker> &avMarkers, bool abScreen)
	{
		avMarkers.clear();
		cWorld3D *pWorld = MapWorld(apInit);
		if(pWorld == nullptr) return;
		cCamera3D *pCamera = apInit->mpPlayer->GetCamera();
		cVector3f vCamPos = pCamera->GetPosition();
		cParticleSystem3DIterator it = pWorld->GetParticleSystemIterator();
		while(it.HasNext())
		{
			cParticleMarker marker;
			marker.mpPS = it.Next();
			cVector3f vPos = marker.mpPS->GetWorldPosition();
			marker.mfDist = cMath::Vector3Dist(vPos, vCamPos);
			marker.mbOnScreen = abScreen && ToScreen(pCamera, vPos, marker.mvScreen);
			avMarkers.push_back(marker);
		}
		std::sort(avMarkers.begin(), avMarkers.end(), [](const cParticleMarker &a, const cParticleMarker &b)
			{ return a.mfDist < b.mfDist; });
	}
}

//-----------------------------------------------------------------------

cParticleSystem3D* cRefView::PickedParticleSystem()
{
	cWorld3D *pWorld = MapWorld(mpInit);
	if(mpParticlePicked && (pWorld == nullptr || pWorld->ParticleSystemExists(mpParticlePicked) == false))
		mpParticlePicked = nullptr;
	return mpParticlePicked;
}

//-----------------------------------------------------------------------

int cRefView::SetParticlesOriginal(const tString &asName, bool abOriginal)
{
	cWorld3D *pWorld = MapWorld(mpInit);
	if(asName == "") mpInit->mpGame->GetResources()->GetParticleManager()->SetUseOriginal(abOriginal);
	if(pWorld == nullptr) return 0;

	int lCount = 0;
	tString sLower = cString::ToLowerCase(asName);
	// Switching makes no systems and deletes none: the list stays as it is
	cParticleSystem3DIterator it = pWorld->GetParticleSystemIterator();
	while(it.HasNext())
	{
		cParticleSystem3D *pPS = it.Next();
		if(pPS->HasOriginal() == false) continue;
		if(sLower != "" && cString::ToLowerCase(pPS->GetName()) != sLower &&
			cString::ToLowerCase(cString::SetFileExt(pPS->GetDataName(), "")) != sLower)
			continue;
		if(pPS->IsOriginal() != abOriginal) ++lCount;
		pWorld->SetParticleSystemOriginal(pPS, abOriginal);
	}
	return lCount;
}

//-----------------------------------------------------------------------

tString cRefView::ListParticles()
{
	std::vector<cParticleMarker> vMarkers;
	Gather(mpInit, vMarkers, false);
	tString sList;
	char sLine[512];
	for(const cParticleMarker &marker : vMarkers)
	{
		cVector3f vPos = marker.mpPS->GetWorldPosition();
		snprintf(sLine, sizeof(sLine), "%s %s %.1f %s %.2f %.2f %.2f\n", marker.mpPS->GetName().c_str(),
				cString::SetFileExt(marker.mpPS->GetDataName(), "").c_str(), marker.mfDist,
				VariantName(marker.mpPS), vPos.x, vPos.y, vPos.z);
		sList += sLine;
	}
	return sList;
}

//-----------------------------------------------------------------------

bool cRefView::PickParticles(const tString &asName)
{
	mpParticlePicked = nullptr;
	if(asName == "" || asName == "none") return true;
	std::vector<cParticleMarker> vMarkers;
	Gather(mpInit, vMarkers, false);
	tString sLower = cString::ToLowerCase(asName);
	for(const cParticleMarker &marker : vMarkers)
	{
		if(cString::ToLowerCase(marker.mpPS->GetName()) == sLower ||
			cString::ToLowerCase(cString::SetFileExt(marker.mpPS->GetDataName(), "")) == sLower)
		{
			mpParticlePicked = marker.mpPS;
			return true;
		}
	}
	return false;
}

void cRefView::CycleParticles()
{
	mlParticleSwitch = PickedParticleSystem() ? eParticleSwitch_Picked : eParticleSwitch_All;
	UpdateParticles();
}

//-----------------------------------------------------------------------

void cRefView::UpdateParticles()
{
	int lSwitch = mlParticleSwitch;
	mlParticleSwitch = eParticleSwitch_None;
	if(lSwitch == eParticleSwitch_None) return;

	cWorld3D *pWorld = MapWorld(mpInit);
	cParticleManager *pManager = mpInit->mpGame->GetResources()->GetParticleManager();
	cParticleSystem3D *pPicked = PickedParticleSystem();
	if(lSwitch == eParticleSwitch_Picked)
	{
		if(pWorld && pPicked) pWorld->SetParticleSystemOriginal(pPicked, !pPicked->IsOriginal());
		return;
	}
	bool bOriginal = lSwitch == eParticleSwitch_AllOld ||
					(lSwitch == eParticleSwitch_All && pManager->GetUseOriginal() == false);
	SetParticlesOriginal("", bOriginal);
}

//-----------------------------------------------------------------------

void cRefView::DrawParticleMarkers()
{
	if(mbParticleMarkers == false) return;
	std::vector<cParticleMarker> vMarkers;
	Gather(mpInit, vMarkers, true);
	cParticleSystem3D *pPicked = PickedParticleSystem();

	ImDrawList *pList = ImGui::GetBackgroundDrawList();
	const float fEm = ImGui::GetFontSize();
	const float fRadius = fEm * 0.3f;

	// A click on the screen, not on a window, picks the nearest dot, or
	// nothing. The level editor picks its own nodes with clicks.
	bool bClick = mpInit->mpEditor == nullptr && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
				ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) == false && ImGui::IsAnyItemHovered() == false;
	ImVec2 vMouse = ImGui::GetIO().MousePos;
	cParticleSystem3D *pClicked = nullptr;
	float fClickDist = fEm * 1.2f;

	// Far ones first, so that near ones are drawn over them
	for(auto it = vMarkers.rbegin(); it != vMarkers.rend(); ++it)
	{
		const cParticleMarker &marker = *it;
		cParticleSystem3D *pPS = marker.mpPS;
		bool bIsPicked = pPS == pPicked;
		if(marker.mbOnScreen == false) continue;
		if(bIsPicked == false && marker.mfDist > mfParticleRange) continue;
		if(bIsPicked == false && mbParticleOnlyReplaced && pPS->HasOriginal() == false) continue;

		ImU32 lColor = MarkerColor(pPS, bIsPicked, pPicked != nullptr && bIsPicked == false);
		const ImVec2 &vPos = marker.mvScreen;
		pList->AddCircleFilled(vPos, bIsPicked ? fRadius * 1.5f : fRadius, lColor);
		pList->AddCircle(vPos, (bIsPicked ? fRadius * 1.5f : fRadius) + 1.0f, IM_COL32(0, 0, 0, 160), 0, 1.5f);
		if(bIsPicked) pList->AddCircle(vPos, fRadius * 2.6f, IM_COL32(255, 255, 255, 230), 0, 2.0f);

		if(mbParticleNames || bIsPicked)
		{
			tString sLabel = cString::SetFileExt(pPS->GetDataName(), "");
			if(pPS->HasOriginal()) sLabel += pPS->IsOriginal() ? "  old" : "  new";
			if(bIsPicked && pPS->GetName() != "" && pPS->GetName() != sLabel)
				sLabel += "  (" + pPS->GetName() + ")";
			ImVec2 vText(vPos.x + fRadius * 2.2f, vPos.y - fEm * 0.5f);
			ImVec2 vSize = ImGui::CalcTextSize(sLabel.c_str());
			pList->AddRectFilled(ImVec2(vText.x - 2, vText.y - 1), ImVec2(vText.x + vSize.x + 2, vText.y + vSize.y + 1),
								IM_COL32(0, 0, 0, bIsPicked ? 190 : 110), 3.0f);
			pList->AddText(vText, lColor, sLabel.c_str());
		}

		if(bClick)
		{
			float fDist = std::sqrt((vMouse.x - vPos.x) * (vMouse.x - vPos.x) + (vMouse.y - vPos.y) * (vMouse.y - vPos.y));
			if(fDist < fClickDist)
			{
				fClickDist = fDist;
				pClicked = pPS;
			}
		}
	}
	if(bClick) mpParticlePicked = pClicked;
}

//-----------------------------------------------------------------------

void cRefView::DrawParticles()
{
	const float fEm = ImGui::GetFontSize();
	// Off or folded, it shows nothing on the screen either
	if(BeginWindow("Particles", &mbShowParticles, 0, 1, 36, 18) == false) return;

	// C cycles as the button does, unless typing
	bool bKey = ImGui::GetIO().WantTextInput == false && ImGui::IsKeyPressed(ImGuiKey_C, false);

	cWorld3D *pWorld = MapWorld(mpInit);
	cParticleManager *pManager = mpInit->mpGame->GetResources()->GetParticleManager();
	if(pWorld == nullptr)
	{
		ImGui::TextDisabled("No map loaded.");
		ImGui::End();
		return;
	}

	std::vector<cParticleMarker> vMarkers;
	Gather(mpInit, vMarkers, false);
	int lReplaced = 0;
	for(const cParticleMarker &marker : vMarkers)
		if(marker.mpPS->HasOriginal()) ++lReplaced;
	cParticleSystem3D *pPicked = PickedParticleSystem();

	ImGui::Text("%d systems, %d of them from an addon", (int)vMarkers.size(), lReplaced);
	ImGui::TextColored(ImVec4(0.31f, 0.88f, 0.43f, 1), "new");
	ImGui::SameLine(0, 0);
	ImGui::TextUnformatted(": the addon's.");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1.0f, 0.63f, 0.16f, 1), "old");
	ImGui::SameLine(0, 0);
	ImGui::TextUnformatted(": the game's.");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.67f, 0.67f, 0.67f, 1), "grey");
	ImGui::SameLine(0, 0);
	ImGui::TextUnformatted(": no addon.");

	ImGui::Checkbox("Dots", &mbParticleMarkers);
	ImGui::SameLine();
	ImGui::Checkbox("Names", &mbParticleNames);
	ImGui::SameLine();
	ImGui::Checkbox("Only from addons", &mbParticleOnlyReplaced);
	ImGui::SetNextItemWidth(fEm * 12);
	ImGui::SliderFloat("Range", &mfParticleRange, 2.0f, 200.0f, "%.0f m", ImGuiSliderFlags_Logarithmic);

	ImGui::Separator();
	const char *sCycle = pPicked ? "Switch picked (C)" : "Switch all (C)";
	ImGui::BeginDisabled(pPicked != nullptr && pPicked->HasOriginal() == false);
	if(ImGui::Button(sCycle) || bKey)
		mlParticleSwitch = pPicked ? eParticleSwitch_Picked : eParticleSwitch_All;
	ImGui::EndDisabled();
	if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		Tip("With a system picked, switches it between new and old. With none, switches all the "
			"map's and the ones made from now on. Click a dot to pick it, or the screen to pick none.");
	ImGui::SameLine();
	if(ImGui::Button("All new")) mlParticleSwitch = eParticleSwitch_AllNew;
	ImGui::SameLine();
	if(ImGui::Button("All old")) mlParticleSwitch = eParticleSwitch_AllOld;
	ImGui::SameLine();
	ImGui::BeginDisabled(pPicked == nullptr);
	if(ImGui::Button("Unpick")) mpParticlePicked = pPicked = nullptr;
	ImGui::EndDisabled();
	ImGui::TextDisabled("New systems: %s", pManager->GetUseOriginal() ? "old" : "new");

	if(ImGui::BeginTable("##particles", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit,
							ImVec2(0, cMath::Max(ImGui::GetContentRegionAvail().y, fEm * 6))))
	{
		ImGui::TableSetupColumn("System", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("File");
		ImGui::TableSetupColumn("Look");
		ImGui::TableSetupColumn("Away");
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();
		for(const cParticleMarker &marker : vMarkers)
		{
			cParticleSystem3D *pPS = marker.mpPS;
			if(mbParticleOnlyReplaced && pPS->HasOriginal() == false && pPS != pPicked) continue;
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::PushID(pPS);
			tString sName = pPS->GetName() != "" ? pPS->GetName() : "(no name)";
			if(ImGui::Selectable(sName.c_str(), pPS == pPicked, ImGuiSelectableFlags_SpanAllColumns))
				mpParticlePicked = pPS == pPicked ? nullptr : pPS;
			ImGui::PopID();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(cString::SetFileExt(pPS->GetDataName(), "").c_str());
			ImGui::TableNextColumn();
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(MarkerColor(pPS, false, false)), "%s", VariantName(pPS));
			ImGui::TableNextColumn();
			ImGui::Text("%.1f m", marker.mfDist);
		}
		ImGui::EndTable();
	}
	ImGui::End();

	DrawParticleMarkers();
}
