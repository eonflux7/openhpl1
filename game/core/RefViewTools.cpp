/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
// The panel's Move and God windows: getting around the maps quickly, and
// what makes testing easier (cannot be hurt, any item in the inventory).
#include "core/StdAfx.h"
#include "core/RefView.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "entities/GameItem.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "ui/Inventory.h"

#include <imgui.h>

#include <algorithm>

//-----------------------------------------------------------------------

static const char *ItemTypeName(eGameItemType aType)
{
	static const char *vNames[] = {"Quest", "Notebook", "Note", "Battery", "Flashlight", "Food", "Map",
								   "Glow stick", "Flare", "Painkillers", "Weapon", "Throw", "Gas mask",
								   "Collectable"};
	static_assert(sizeof(vNames) / sizeof(vNames[0]) == eGameItemType_LastEnum, "a name for every item type");
	return aType >= 0 && aType < eGameItemType_LastEnum ? vNames[aType] : "?";
}

// The name an item file is given when it is put in the inventory:
// item_crowbar.ent is "crowbar". The scripts may know it by another.
static tString ItemNameFromFile(const tString &asFile)
{
	tString sName = cString::SetFileExt(asFile, "");
	for(const char *sPrefix : {"items_", "item_"})
	{
		size_t lLen = strlen(sPrefix);
		if(sName.size() > lLen && sName.compare(0, lLen, sPrefix) == 0) return sName.substr(lLen);
	}
	return sName;
}

//-----------------------------------------------------------------------

void cRefView::DrawMoveWindow()
{
	if(BeginWindow("Move", &mbShowMove, 1, 0, 22, 0)==false) return;

	const float fEm = ImGui::GetFontSize();
	bool bInGame = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	ImGui::BeginDisabled(bInGame==false);

	ImGui::Checkbox("Fly", &mbFly);
	if(ImGui::IsItemHovered())
		Tip("With the panel closed (F2): the movement keys move the camera where it looks, "
			"Run is four times faster, Jump and Crouch go up and down. The player's body is switched "
			"off meanwhile. Unticking puts the player where the camera is.");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SliderFloat("##speed", &mfFlySpeed, 0.5f, 20.0f, "%.1f m/s", ImGuiSliderFlags_Logarithmic);

	if(bInGame)
	{
		cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
		const cVector3f &vPos = pCamera->GetPosition();
		ImGui::TextDisabled("%.2f %.2f %.2f", vPos.x, vPos.y, vPos.z);
		ImGui::SameLine();
		if(ImGui::SmallButton("Copy"))
		{
			char sText[96];
			snprintf(sText, sizeof(sText), "%.3f %.3f %.3f", vPos.x, vPos.y, vPos.z);
			ImGui::SetClipboardText(sText);
		}
		ImGui::SameLine();
		if(ImGui::SmallButton("To fields"))
		{
			mvCoords[0] = vPos.x;
			mvCoords[1] = vPos.y;
			mvCoords[2] = vPos.z;
		}
	}
	ImGui::SetNextItemWidth(fEm * 15);
	ImGui::InputFloat3("##coords", mvCoords, "%.2f");
	ImGui::SameLine();
	if(ImGui::Button("Go##coords")) mbGoToCoords = true;
	if(ImGui::IsItemHovered()) Tip("Puts the camera (the player's eyes) there, looking the same way.");

	ImGui::EndDisabled();

	if(ImGui::CollapsingHeader("Maps", ImGuiTreeNodeFlags_DefaultOpen)) DrawMaps();
	if(ImGui::CollapsingHeader("Start positions", ImGuiTreeNodeFlags_DefaultOpen)) DrawStartPositions();
	if(ImGui::CollapsingHeader("Bookmarks", ImGuiTreeNodeFlags_DefaultOpen)) DrawBookmarks();

	ImGui::End();
}

//-----------------------------------------------------------------------

void cRefView::DrawStartPositions()
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	bool bInGame = pWorld && msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	if(bInGame==false)
	{
		ImGui::TextDisabled("No map loaded.");
		return;
	}

	tStartPosEntityList *pStarts = pWorld->GetStartPosEntityList();
	int lRows = cMath::Min((int)pStarts->size(), 6);
	float fHeight = ImGui::GetTextLineHeightWithSpacing() * cMath::Max(lRows, 1) + ImGui::GetStyle().FramePadding.y * 2;
	if(ImGui::BeginListBox("##starts", ImVec2(-FLT_MIN, fHeight)))
	{
		for(cStartPosEntity *pStart : *pStarts)
		{
			if(ImGui::Selectable(pStart->GetName().c_str())) msGoToStart = pStart->GetName();
			if(ImGui::IsItemHovered())
			{
				cVector3f vPos = pStart->GetWorldMatrix().GetTranslation();
				ImGui::SetTooltip("%.2f %.2f %.2f", vPos.x, vPos.y, vPos.z);
			}
		}
		ImGui::EndListBox();
	}
}

//-----------------------------------------------------------------------

void cRefView::DrawBookmarks()
{
	const float fEm = ImGui::GetFontSize();
	bool bInGame = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";

	ImGui::BeginDisabled(bInGame==false);
	ImGui::SetNextItemWidth(fEm * 12);
	ImGui::InputTextWithHint("##bookmark", "name (optional)", msBookmarkName, sizeof(msBookmarkName));
	ImGui::SameLine();
	if(ImGui::Button("Save here")) mbAddBookmark = true;
	if(ImGui::IsItemHovered())
		Tip("Keeps where the camera is and where it looks, until the game is quit. "
			"A bookmark in another map loads that map first.");
	ImGui::EndDisabled();

	if(mvBookmarks.empty())
	{
		ImGui::TextDisabled("No bookmarks.");
		return;
	}

	int lDelete = -1;
	ImGui::BeginDisabled(CanLoadMap()==false);
	for(size_t i = 0; i < mvBookmarks.size(); ++i)
	{
		const cBookmark &bookmark = mvBookmarks[i];
		ImGui::PushID((int)i);
		if(ImGui::SmallButton("x")) lDelete = (int)i;
		ImGui::SameLine();
		tString sLabel = bookmark.msName;
		if(bookmark.msMap != msViewsMap) sLabel += "  (" + bookmark.msMap + ")";
		if(ImGui::Selectable(sLabel.c_str(), mlGoToBookmark == (int)i)) mlGoToBookmark = (int)i;
		if(ImGui::IsItemHovered())
			ImGui::SetTooltip("%s: %.2f %.2f %.2f", bookmark.msMap.c_str(), bookmark.mvPos.x, bookmark.mvPos.y,
							  bookmark.mvPos.z);
		ImGui::PopID();
	}
	ImGui::EndDisabled();

	if(lDelete >= 0)
	{
		mvBookmarks.erase(mvBookmarks.begin() + lDelete);
		if(mlGoToBookmark == lDelete) mlGoToBookmark = -1;
		else if(mlGoToBookmark > lDelete) --mlGoToBookmark;
	}
}

//-----------------------------------------------------------------------

void cRefView::DrawGodWindow()
{
	if(BeginWindow("God", &mbShowGod, 1, 1, 26, 34)==false) return;

	bool bInGame = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	ImGui::BeginDisabled(bInGame==false);

	if(ImGui::Checkbox("Cannot be hurt", &mbGodMode)) mbUsed = true;
	ImGui::SameLine();
	if(ImGui::SmallButton("Heal")) mbHeal = true;
	ImGui::SameLine();
	if(mbSetFlashLight==false && bInGame) mbFlashLight = mpInit->mpPlayer->GetFlashLight()->IsActive();
	if(ImGui::Checkbox("Flashlight", &mbFlashLight))
	{
		mbSetFlashLight = true;
		mbUsed = true;
	}
	if(ImGui::IsItemHovered())
		Tip("Switches it whether it is in the inventory or not; it does not run down. Its key works with "
			"the flashlight in the inventory. A viewpoint is saved with the flashlight as it is.");
	if(ImGui::Checkbox("No flicker", &mbNoFlicker)) mbUsed = true;
	if(ImGui::IsItemHovered())
		Tip("Flickering lights stay on. Lights already stopped stay so until the map is loaded again.");
	ImGui::SameLine();
	if(ImGui::Checkbox("3D view only", &mbHide2D)) mbUsed = true;
	if(ImGui::IsItemHovered()) Tip("No HUD, fades, damage or hiding effects.");
	if(mbUsed==false)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if(ImGui::IsItemHovered())
			Tip("These are applied once something in the panel is used: until then the game is as it was.");
	}

	ImGui::TextUnformatted("Give");
	ImGui::SameLine();
	if(ImGui::SmallButton("Flashlight##give")) mlGiveItems |= eGiveItem_Flashlight;
	if(ImGui::IsItemHovered()) Tip("Puts the flashlight in the inventory, if it is not there: its key works then.");
	ImGui::SameLine();
	if(ImGui::SmallButton("Glow stick")) mlGiveItems |= eGiveItem_GlowStick;
	if(ImGui::IsItemHovered()) Tip("Puts the glow stick in the inventory, if it is not there: its key works then.");
	ImGui::SameLine();
	if(ImGui::SmallButton("Flare")) mlGiveItems |= eGiveItem_Flare;
	if(ImGui::IsItemHovered()) Tip("One more flare in the inventory. It is lit from there, and thrown when put away.");
	ImGui::EndDisabled();

	if(ImGui::BeginTabBar("##god"))
	{
		if(ImGui::BeginTabItem("Quest items"))
		{
			DrawItems(true);
			ImGui::EndTabItem();
		}
		if(ImGui::IsItemHovered())
			Tip("Items of type Normal: keys, tools and the like, used on things in the maps.");
		if(ImGui::BeginTabItem("Other items"))
		{
			DrawItems(false);
			ImGui::EndTabItem();
		}
		if(ImGui::IsItemHovered()) Tip("Supplies, weapons, notes and the rest.");
		if(ImGui::BeginTabItem("In this map"))
		{
			DrawMapItems();
			ImGui::EndTabItem();
		}
		if(ImGui::IsItemHovered())
			Tip("The items lying in the map that is loaded, by the names the scripts know them by.");
		if(msViewsMap != "effectslab" && ImGui::BeginTabItem("Effects"))
		{
			DrawPlayerEffects();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	if(msItemMessage != "") ImGui::TextDisabled("%s", msItemMessage.c_str());

	ImGui::End();
}

//-----------------------------------------------------------------------

// The line under the God window's tabs that says what was last given
float cRefView::ItemMessageHeight()
{
	return msItemMessage != "" ? ImGui::GetTextLineHeightWithSpacing() : 0.0f;
}

//-----------------------------------------------------------------------

void cRefView::FindItemFiles()
{
	mbItemFilesFound = true;
	mvItemFiles.clear();

	const tFilePathMap &files = mpInit->mpGame->GetResources()->GetFileSearcher()->GetFiles();
	for(const auto &file : files)
	{
		if(cString::ToLowerCase(cString::GetFileExt(file.first)) != "ent") continue;

		TiXmlDocument doc;
		if(doc.LoadFile(file.second.msPath.c_str())==false) continue;
		TiXmlElement *pRootElem = doc.FirstChildElement();
		if(pRootElem==nullptr) continue;
		TiXmlElement *pMainElem = pRootElem->FirstChildElement("MAIN");
		TiXmlElement *pGameElem = pRootElem->FirstChildElement("GAME");
		if(pMainElem==nullptr || pGameElem==nullptr) continue;
		if(cString::ToLowerCase(cString::ToString(pMainElem->Attribute("Type"), "")) != "item") continue;

		cItemFile item;
		item.msFile = file.first;
		item.mlType = cEntityLoader_GameItem::ToItemType(pGameElem->Attribute("ItemType"));
		item.mbQuest = item.mlType == eGameItemType_Normal;
		item.msName = ItemNameFromFile(file.first);

		tString sNameCat = cString::ToString(pGameElem->Attribute("NameCat"), "");
		tString sNameEntry = cString::ToString(pGameElem->Attribute("NameEntry"), "");
		if(sNameCat != "" && sNameEntry != "")
			item.msLabel = cString::To8Char(mpInit->mpGame->GetResources()->Translate(sNameCat, sNameEntry));
		if(item.msLabel == "") item.msLabel = item.msName;

		mvItemFiles.push_back(item);
	}

	std::sort(mvItemFiles.begin(), mvItemFiles.end(), [](const cItemFile &a, const cItemFile &b)
	{
		if(a.mlType != b.mlType) return a.mlType < b.mlType;
		return cString::ToLowerCase(a.msLabel) < cString::ToLowerCase(b.msLabel);
	});
	mlSpawnItem = -1;
}

//-----------------------------------------------------------------------

void cRefView::DrawItems(bool abQuest)
{
	if(mbItemFilesFound==false) FindItemFiles();

	const float fEm = ImGui::GetFontSize();
	bool bInGame = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";

	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##filter", "filter", msItemFilter, sizeof(msItemFilter));
	tString sFilter = cString::ToLowerCase(msItemFilter);

	const ImGuiTableFlags lFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV |
									ImGuiTableFlags_SizingStretchProp;
	// To the bottom of the window, above the name and Give
	float fFooter = ImGui::GetFrameHeightWithSpacing() + ItemMessageHeight();
	float fHeight = cMath::Max(ImGui::GetContentRegionAvail().y - fFooter, fEm * 6);
	if(ImGui::BeginTable("##items", 2, lFlags, ImVec2(0, fHeight)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(abQuest ? "Held" : "Type", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();

		cInventory *pInventory = mpInit->mpInventory;
		for(size_t i = 0; i < mvItemFiles.size(); ++i)
		{
			const cItemFile &item = mvItemFiles[i];
			if(item.mbQuest != abQuest) continue;
			if(sFilter != "" && cString::ToLowerCase(item.msLabel).find(sFilter) == tString::npos &&
				item.msFile.find(sFilter) == tString::npos)
				continue;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::PushID((int)i);
			if(ImGui::Selectable(item.msLabel.c_str(), mlSpawnItem == (int)i,
								 ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
			{
				mlSpawnItem = (int)i;
				snprintf(msSpawnName, sizeof(msSpawnName), "%s", item.msName.c_str());
				if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && bInGame) mbSpawnItem = true;
			}
			if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s\nDouble-click to give it", item.msFile.c_str());
			// Several items share a name in the game's language (the chemicals)
			if(item.msLabel != item.msName)
			{
				ImGui::SameLine();
				ImGui::TextDisabled("%s", item.msName.c_str());
			}
			ImGui::PopID();
			ImGui::TableNextColumn();
			if(abQuest)
				ImGui::TextDisabled("%s", bInGame && pInventory->GetItem(item.msName) ? "yes" : "");
			else
				ImGui::TextDisabled("%s", ItemTypeName((eGameItemType)item.mlType));
		}
		ImGui::EndTable();
	}

	// The selected one, under the name given to it
	bool bSelected = mlSpawnItem >= 0 && mlSpawnItem < (int)mvItemFiles.size() &&
					 mvItemFiles[mlSpawnItem].mbQuest == abQuest;
	ImGui::BeginDisabled(bSelected==false || bInGame==false);
	ImGui::SetNextItemWidth(fEm * 13);
	ImGui::InputTextWithHint("##name", "name in the inventory", msSpawnName, sizeof(msSpawnName));
	if(ImGui::IsItemHovered())
		Tip("The scripts know a quest item by the name it has in its map (coffinkey, chemicalA...), "
			"not by its file: give it that name for it to work on things. The In this map tab "
			"has the loaded map's items under their own names.");
	ImGui::SameLine();
	if(ImGui::Button("Give")) mbSpawnItem = true;
	ImGui::EndDisabled();
}

//-----------------------------------------------------------------------

void cRefView::DrawMapItems()
{
	bool bInGame = msViewsMap != "" && msWaitMap == "" &&
					mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";
	if(bInGame==false)
	{
		ImGui::TextDisabled("No map loaded.");
		return;
	}

	const float fEm = ImGui::GetFontSize();
	const ImGuiTableFlags lFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV |
									ImGuiTableFlags_SizingStretchProp;
	int lCount = 0;
	float fHeight = cMath::Max(ImGui::GetContentRegionAvail().y - ItemMessageHeight(), fEm * 6);
	if(ImGui::BeginTable("##mapitems", 3, lFlags, ImVec2(0, fHeight)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();

		tGameEntityIterator it = mpInit->mpMapHandler->GetGameEntityIterator();
		while(it.HasNext())
		{
			iGameEntity *pEntity = it.Next();
			if(pEntity->GetType() != eGameEntityType_Item || pEntity->GetDestroyMe()) continue;
			cGameItem *pItem = static_cast<cGameItem*>(pEntity);
			++lCount;

			ImGui::TableNextRow();
			ImGui::PushID(pItem->GetName().c_str());
			ImGui::TableNextColumn();
			// Items a script has yet to show
			if(pItem->IsActive()) ImGui::TextUnformatted(pItem->GetName().c_str());
			else ImGui::TextDisabled("%s (hidden)", pItem->GetName().c_str());
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("%s\n%s", cString::To8Char(pItem->GetGameName()).c_str(),
								  pItem->GetFileName().c_str());
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", ItemTypeName(pItem->GetItemType()));
			ImGui::TableNextColumn();
			if(ImGui::SmallButton("Take")) msTakeItem = pItem->GetName();
			if(ImGui::IsItemHovered())
				ImGui::SetTooltip("Picks it up as if the player had: its pickup callbacks run");
			ImGui::PopID();
		}
		if(lCount == 0)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("No items left in this map.");
		}
		ImGui::EndTable();
	}
}

//-----------------------------------------------------------------------

// The camera (the player's eyes) at a place, looking that way; the player
// goes on from there, or flies on.
void cRefView::MoveCameraTo(const cVector3f &avPos, float afYaw, float afPitch)
{
	mbUsed = true;
	Release();
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	pCamera->SetPosition(avPos);
	pCamera->SetYaw(afYaw);
	pCamera->SetPitch(afPitch);
	if(mbFlying)
	{
		mvFlyPos = avPos;
		mlFlySeed = 0;
	}
	else
	{
		PutPlayerAtCamera();
	}
}

//-----------------------------------------------------------------------

// What the Move and God windows asked for, on a step of the game with a
// map loaded
void cRefView::UpdateMoveAndGod()
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();

	if(msGoToStart != "")
	{
		mbUsed = true;
		Release();
		mpInit->mpPlayer->SetStartPos(msGoToStart);
		// Flying goes on from the player's head, once it is there
		mlFlySeed = 2;
		msGoToStart = "";
	}

	if(mbGoToCoords)
	{
		MoveCameraTo(cVector3f(mvCoords[0], mvCoords[1], mvCoords[2]), pCamera->GetYaw(), pCamera->GetPitch());
		mbGoToCoords = false;
	}

	if(mbAddBookmark)
	{
		cBookmark bookmark;
		bookmark.msName = msBookmarkName[0] ? tString(msBookmarkName)
											 : "Bookmark " + cString::ToString((int)mvBookmarks.size() + 1);
		bookmark.msMap = msViewsMap;
		bookmark.mvPos = pCamera->GetPosition();
		bookmark.mfYaw = pCamera->GetYaw();
		bookmark.mfPitch = pCamera->GetPitch();
		mvBookmarks.push_back(bookmark);
		msBookmarkName[0] = 0;
		mbAddBookmark = false;
	}

	if(mlGoToBookmark >= 0)
	{
		if(mlGoToBookmark >= (int)mvBookmarks.size())
		{
			mlGoToBookmark = -1;
		}
		else if(mvBookmarks[mlGoToBookmark].msMap == msViewsMap)
		{
			const cBookmark &bookmark = mvBookmarks[mlGoToBookmark];
			MoveCameraTo(bookmark.mvPos, bookmark.mfYaw, bookmark.mfPitch);
			mlGoToBookmark = -1;
		}
		else if(msWaitMap == "" && msLoadMap == "")
		{
			// Its map first: this is done again once that is loaded. Once
			// only, in case the map does not load.
			if(mbBookmarkMapAsked==false && CanLoadMap())
			{
				msLoadMap = cString::SetFileExt(mvBookmarks[mlGoToBookmark].msMap, "dae");
				mbBookmarkMapAsked = true;
			}
			else
			{
				mlGoToBookmark = -1;
			}
		}
		if(mlGoToBookmark < 0) mbBookmarkMapAsked = false;
	}

	if(mbSpawnItem)
	{
		mbSpawnItem = false;
		if(mlSpawnItem >= 0 && mlSpawnItem < (int)mvItemFiles.size())
		{
			const cItemFile &item = mvItemFiles[mlSpawnItem];
			tString sName = msSpawnName[0] ? tString(msSpawnName) : item.msName;
			mpInit->mpInventory->AddItemFromFile(sName, item.msFile, -1);
			msItemMessage = "Gave " + sName + " (" + item.msFile + ")";
		}
	}

	if(msTakeItem != "")
	{
		iGameEntity *pEntity = mpInit->mpMapHandler->GetGameEntity(msTakeItem, false);
		if(pEntity && pEntity->GetType() == eGameEntityType_Item && pEntity->GetDestroyMe()==false)
		{
			static_cast<cGameItem*>(pEntity)->TakeIntoInventory();
			msItemMessage = "Took " + msTakeItem;
		}
		msTakeItem = "";
	}

	if(mbHeal)
	{
		mpInit->mpPlayer->SetHealth(100);
		mbHeal = false;
	}
}
