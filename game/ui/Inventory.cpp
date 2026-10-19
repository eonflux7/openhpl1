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
#include "ui/Inventory.h"

#include "core/Init.h"
#include "player/Player.h"
#include "entities/GameItem.h"
#include "entities/GameItemType.h"
#include "systems/GameMessageHandler.h"
#include "core/SaveHandler.h"
#include "systems/EffectHandler.h"
#include "systems/RadioHandler.h"
#include "ui/Notebook.h"
#include "player/HapticGameCamera.h"

#include "core/GlobalInit.h"
#include "core/GraphicsHelper.h"
#include "core/ButtonHandler.h"

#include <SDL3/SDL.h>

// The remastered drawer, in virtual screen units: a grid of 4x6 cells at the
// right edge of the screen, the hovered item's name and description under
// it, health and battery at the bottom, and the nine hotkey slots in a row
// at the bottom left of it.
static const float kfDrawerWidth = 300;
static const float kfDrawerCell = 62;
static const float kfDrawerGap = 4;
static const int klDrawerColumns = 4;
static const float kfDrawerTop = 26;
static const float kfDrawerTextTop = kfDrawerTop + 6*kfDrawerCell + 5*kfDrawerGap + 16;
static const float kfDrawerStatusTop = 546;
static const float kfDrawerStatusHeight = 30;
static const float kfHotkeyCell = 48;
static const float kfHotkeyGap = 4;
static const float kfHotkeyBottom = 20;
// An icon is drawn this much inside its cell
static const float kfIconInset = 5;
// The cell's frame takes this much of its edge; text and marks stay inside
static const float kfCellFrame = 6;
// The health and battery bars: trough, and the fill inside it
static const cVector2f kvMeterSize(124, 9);
static const cVector2f kvMeterFillPos(2, 2);
static const cVector2f kvMeterFillSize(120, 5);
static const float kfMeterIcon = 14;

static cColor MixColor(const cColor &aA, const cColor &aB, float afT)
{
	afT = cMath::Clamp(afT, 0.0f, 1.0f);
	return cColor(aA.r + (aB.r-aA.r)*afT, aA.g + (aB.g-aA.g)*afT, aA.b + (aB.b-aA.b)*afT, 1);
}

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cInventory::cInventory(cInit *apInit)  : iUpdateable("Inventory")
{
	mpInit = apInit;
	mpDrawer = apInit->mpGame->GetGraphics()->GetDrawer();

	mpGfxBackground = mpDrawer->CreateGfxObject("inventory_background.bmp","diffalpha2d");

	//mpBatteryMeter = mpDrawer->CreateGfxObject("inventory_battery_meter.bmp","diffalpha2d");
	//mpBatteryMeterBar = mpDrawer->CreateGfxObject("inventory_battery_meter_bar.bmp","diffalpha2d");

	//mpBagpack = mpDrawer->CreateGfxObject("inventory_backpack.bmp","diffalpha2d");

	//mpHealthFrame = mpDrawer->CreateGfxObject("inventory_health_frame.bmp","diffalpha2d");
	//mpHealthBack = mpDrawer->CreateGfxObject("inventory_health_background.bmp","diffalpha2d");
	//mpHealthMan_Fine = mpDrawer->CreateGfxObject("inventory_health_fine.bmp","diffalpha2d");
	//mpHealthMan_Caution = mpDrawer->CreateGfxObject("inventory_health_caution.bmp","diffalpha2d"); 
	//mpHealthMan_Danger = mpDrawer->CreateGfxObject("inventory_health_danger.bmp","diffalpha2d");
	//mpHealthTextFrame = mpDrawer->CreateGfxObject("inventory_health_text_slot.bmp","diffalpha2d");

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");

	mpContext = hplNew( cInventoryContext, (mpInit) );

	mbMessageActive = false;
	msMessage = _W("");
	mfMessageAlpha =0;

	mpMessageBackground = mpDrawer->CreateGfxObject("effect_black.bmp","diffalpha2d");

	///////////////////////////////////
	//Init normal slots
	cInventorySlot *pSlot = nullptr;
	int lCount=0;
    
	cVector2f vSlotBegin(400 - 77*2.5f, 15 + 69 + 5);
	for(float y=0; y<4; ++y)
	for(float x=0; x<6; ++x)
	{
		pSlot = hplNew( cInventorySlot, (mpInit,vSlotBegin + cVector2f(x * 77,y*69),false,lCount++) );
		AddWidget(pSlot);
		mlstSlots.push_back(pSlot);
	}

	///////////////////////////////////
	//Init equip slots
	int lEquipSlot = 0;
	//cVector2f vEquipSlotBegin = cVector2f(400 - 77*5,10);
	cVector2f vEquipSlotBegin = cVector2f(400 - 77*4.5f,15);
	for(float x=0; x<9; ++x)
	{
		pSlot = hplNew( cInventorySlot, (mpInit,vEquipSlotBegin + cVector2f(x*77,0),true,lCount++) );
		AddWidget(pSlot);
		mlstSlots.push_back(pSlot);
		pSlot->SetEquipIndex(lEquipSlot++);
		mvEquipSlots.push_back(pSlot);
	}

	///////////////////////////////////
	//Init other widgests
	mpBattery = hplNew( cInventoryBattery,(mpInit,
														cRect2f(400 - 77*3.5f, 15 + 69 + 5,77,135),
														nullptr,30.0f) );
	AddWidget(mpBattery);

	mpHealth = hplNew( cInventoryHealth, (mpInit,
														cRect2f(400 - 77*3.5f, 15 + 69*3 + 5,
																77,135),
														nullptr,30.0f) );
	AddWidget(mpHealth);

	///////////////////////////////////
	//Remastered drawer
	mbDrawer = mpInit->mbHighResHud;
	const char *pLook = SDL_getenv("OPENHPL_INVENTORY_LOOK");
	mbPaperDrawer = pLook && tString(pLook) == "paper";
	mpGfxDrawer = nullptr;
	mpGfxCell = nullptr;
	mpGfxCellHover = nullptr;
	mpGfxHotkeyCell = nullptr;
	mpGfxHotkeyCellHover = nullptr;
	mpGfxQuest = nullptr;
	mpGfxMeter = nullptr;
	mpGfxMeterFill = nullptr;
	mpGfxMeterTicks = nullptr;
	mpGfxIconHealth = nullptr;
	mpGfxIconBattery = nullptr;
	mfMeterTime = 0;
	mpDrawerFont = mpFont;
	mpDrawerNameFont = mpFont;
	if(mbDrawer)
	{
		tString sLook = mbPaperDrawer ? "paper" : "metal";
		mpGfxDrawer = mpDrawer->CreateGfxObject("inventory_drawer_"+sLook,"diffalpha2d");
		mpGfxCell = mpDrawer->CreateGfxObject("inventory_cell_"+sLook,"diffalpha2d");
		mpGfxCellHover = mpDrawer->CreateGfxObject("inventory_cell_"+sLook+"_hover","diffalpha2d");
		mpGfxHotkeyCell = mpDrawer->CreateGfxObject("inventory_hotkey_"+sLook,"diffalpha2d");
		mpGfxHotkeyCellHover = mpDrawer->CreateGfxObject("inventory_hotkey_"+sLook+"_hover","diffalpha2d");
		mpGfxQuest = mpDrawer->CreateGfxObject("inventory_quest","diffalpha2d");
		mpGfxMeter = mpDrawer->CreateGfxObject("inventory_meter","diffalpha2d");
		mpGfxMeterFill = mpDrawer->CreateGfxObject("inventory_meter_fill","diffalpha2d");
		mpGfxMeterTicks = mpDrawer->CreateGfxObject("inventory_meter_ticks","diffalpha2d");
		mpGfxIconHealth = mpDrawer->CreateGfxObject("inventory_icon_health","diffalpha2d");
		mpGfxIconBattery = mpDrawer->CreateGfxObject("inventory_icon_battery","diffalpha2d");

		//Without its images the original layout is used
		if(	mpGfxDrawer==nullptr || mpGfxCell==nullptr || mpGfxCellHover==nullptr ||
			mpGfxHotkeyCell==nullptr || mpGfxHotkeyCellHover==nullptr || mpGfxQuest==nullptr ||
			mpGfxMeter==nullptr || mpGfxMeterFill==nullptr || mpGfxMeterTicks==nullptr ||
			mpGfxIconHealth==nullptr || mpGfxIconBattery==nullptr)
		{
			Warning("Inventory drawer images are missing; using the original inventory\n");
			mbDrawer = false;
		}

		//Paper is written on in the notebook's fonts
		cFontManager *pFonts = mpInit->mpGame->GetResources()->GetFontManager();
		if(mbPaperDrawer && pFonts->HasVectorFont("cour.fnt"))
		{
			mpDrawerFont = pFonts->CreateFontData("cour.fnt");
			if(pFonts->HasVectorFont("notebook_bold.fnt"))
				mpDrawerNameFont = pFonts->CreateFontData("notebook_bold.fnt");
			else
				mpDrawerNameFont = mpDrawerFont;
		}
	}

	///////////////////////////////////
	//Init items types
	mvItemTypes.resize(eGameItemType_LastEnum, nullptr);

	mvItemTypes[eGameItemType_Normal] = hplNew( cGameItemType_Normal,(mpInit) );
	mvItemTypes[eGameItemType_Notebook] = hplNew( cGameItemType_Notebook,(mpInit) );
	mvItemTypes[eGameItemType_Note] = hplNew( cGameItemType_Note,(mpInit));
	mvItemTypes[eGameItemType_Battery] = hplNew( cGameItemType_Battery,(mpInit) );
	mvItemTypes[eGameItemType_Flashlight] = hplNew( cGameItemType_Flashlight,(mpInit) );
	mvItemTypes[eGameItemType_GlowStick] = hplNew( cGameItemType_GlowStick,(mpInit) );
	mvItemTypes[eGameItemType_Flare] = hplNew( cGameItemType_Flare,(mpInit) );
	mvItemTypes[eGameItemType_Painkillers] = hplNew( cGameItemType_Painkillers,(mpInit) );
	if(mpInit->mbHasHaptics)
		mvItemTypes[eGameItemType_WeaponMelee] = hplNew( cGameItemType_WeaponMeleeHaptX,(mpInit) );
	else
		mvItemTypes[eGameItemType_WeaponMelee] = hplNew( cGameItemType_WeaponMelee,(mpInit) );
	mvItemTypes[eGameItemType_Throw] = hplNew( cGameItemType_Throw,(mpInit) );
	mvItemTypes[eGameItemType_GasMask] = hplNew( cGameItemType_GasMask,(mpInit) );
	mvItemTypes[eGameItemType_Collectable] = hplNew( cGameItemType_Collectable,(mpInit) );

	Reset();
}

cInventory::~cInventory(void)
{
	hplDelete( mpContext );

	ClearCallbacks();
	
	for(size_t i=0; i<mvItemTypes.size(); ++i)
	{
		if(mvItemTypes[i])  hplDelete(mvItemTypes[i]);
	}

	STLDeleteAll(mlstWidgets);
	STLMapDeleteAll(m_mapItems);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC WIDGET METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

iInventoryWidget::iInventoryWidget(cInit *apInit, const cRect2f &aRect, cGfxObject* apGfxObject, float afZ)
{
	mpInit = apInit;
	mRect = aRect;
	mpGfxObject = apGfxObject;
	mfZ = afZ;

	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
}

void iInventoryWidget::Draw()
{
	if(mpGfxObject==nullptr) return;
	
	cVector2l vSize = mpGfxObject->GetMaterial()->GetImage(eMaterialTexture_Diffuse)->GetSize();
	mpDrawer->DrawGfxObject(mpGfxObject, cVector3f(mRect.x,mRect.y,mfZ),
							cVector2f((float)vSize.x, (float)vSize.y),
							cColor(1, mpInit->mpInventory->GetAlpha()));
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// INVENTORY SLOT METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cInventorySlot::cInventorySlot(cInit *apInit, const cVector2f &avPos, bool abEquip, int alIndex)
: iInventoryWidget(apInit, cRect2f(avPos.x, avPos.y,77, 66),nullptr, 10.0f)
{
	mpGfxObject = mpDrawer->CreateGfxObject("inventory_slot.bmp","diffalpha2d");
	mpItem = nullptr;
	mbEquip = abEquip;
	mlIndex = alIndex;

	mpGfxBack = nullptr;

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");
}

//-----------------------------------------------------------------------

void cInventorySlot::Draw()
{
	if(mpInit->mpInventory->IsDrawer()==false)
	{
		iInventoryWidget::Draw();
		return;
	}

	float fAlpha = mpInit->mpInventory->GetAlpha();
	bool bHover = cMath::PointBoxCollision(mpInit->mpInventory->GetMousePos(), mRect) &&
					mpInit->mpInventory->GetContext()->IsActive()==false;
	mpDrawer->DrawGfxObject(mpInit->mpInventory->GetCellGfx(mbEquip, bHover), cVector3f(mRect.x,mRect.y,3),
							cVector2f(mRect.w,mRect.h), cColor(1,fAlpha));
}

void cInventorySlot::OnDraw()
{
	if(mpInit->mpInventory->IsDrawer())
	{
		cInventory *pInventory = mpInit->mpInventory;
		float fAlpha = pInventory->GetAlpha();
		iFontData *pFont = pInventory->GetDrawerFont();
		bool bPaper = pInventory->IsPaperDrawer();

		if(mbEquip)
		{
			pFont->Draw(cVector3f(mRect.x + kfCellFrame + 2, mRect.y + kfCellFrame, 7), 10, pInventory->GetDrawerTextColor(fAlpha*0.75f),
						eFontAlign_Left, _W("%d"), mlEquipIndex+1);
		}
		if(mpItem==nullptr) return;

		cRect2f rectIcon = cInventory::GetIconRect(mRect);
		pInventory->DrawItemIcon(mpItem, cVector3f(rectIcon.x, rectIcon.y, 5), rectIcon.w, cColor(1,fAlpha));

		if(cInventory::IsQuestItem(mpItem))
		{
			float fMark = mbEquip ? 10 : 12;
			mpDrawer->DrawGfxObject(pInventory->GetQuestGfx(),
									cVector3f(mRect.x + mRect.w - fMark - kfCellFrame, mRect.y + kfCellFrame, 7),
									cVector2f(fMark,fMark), cColor(1,fAlpha));
		}

		cGameItemType *pType = pInventory->GetItemType(mpItem->GetItemType());
		tWString sString;
		if(pType && pType->GetString(mpItem)!=_W(""))	sString = pType->GetString(mpItem);
		else if(mpItem->HasCount())						sString = cString::To16Char(cString::ToString(mpItem->GetCount()));
		if(sString != _W(""))
		{
			cVector3f vPos(mRect.x + mRect.w - kfCellFrame - 2, mRect.y + mRect.h - kfCellFrame - 13, 7);
			cColor colText = bPaper ? pInventory->GetDrawerTextColor(fAlpha) : cColor(1,fAlpha);
			if(bPaper==false)
				pFont->Draw(vPos + cVector3f(1,1,-1), 12, cColor(0,fAlpha), eFontAlign_Right, _W("%ls"), sString.c_str());
			pFont->Draw(vPos, 12, colText, eFontAlign_Right, _W("%ls"), sString.c_str());
		}
		return;
	}

	if(mpItem)
	{
		cVector3f vPos(mRect.x + 4,	mRect.y + 4 ,5);
		cVector2l vSize = mpItem->GetGfxObject()->GetMaterial()->GetImage(eMaterialTexture_Diffuse)->GetSize();
		
		mpDrawer->DrawGfxObject(mpItem->GetGfxObject(), vPos,
								cVector2f((float)vSize.x, (float)vSize.y),
								cColor(1, mpInit->mpInventory->GetAlpha()));
		
		cGameItemType *pType = mpInit->mpInventory->GetItemType(mpItem->GetItemType());

		if(pType && pType->GetString(mpItem)!=_W(""))
		{
			tWString sString = pType->GetString(mpItem);
			mpFont->Draw(vPos + cVector3f(0,0,1),12,cColor(1,mpInit->mpInventory->GetAlpha()),eFontAlign_Left,
						sString.c_str());
		}
		else if(mpItem->HasCount())
		{
			mpFont->Draw(vPos + cVector3f(0,0,1),12,cColor(1,mpInit->mpInventory->GetAlpha()),eFontAlign_Left,
						_W("%d"),mpItem->GetCount());
		}
	}
	
	// Equip slot specifics
	if(mbEquip)
	{
		if(mpGfxBack == nullptr)
		{
			mpGfxBack = mpDrawer->CreateGfxObject("inventory_slot_equip"+cString::ToString(mlEquipIndex+1)+".bmp","diffalpha2d");
		}

		mpDrawer->DrawGfxObject(mpGfxBack,cVector3f(mRect.x,mRect.y,1),cVector2f(77,66),
								cColor(1,mpInit->mpInventory->GetAlpha()*0.23f));
		/*cVector3f vPos(mRect.x + 2,mRect.y + 3 ,11);
		tString sString = cString::ToString(mlEquipIndex+1);
		
		cColor Col(1,1,1,mpInit->mpInventory->GetAlpha());
		
		//mpFont->Draw(vPos,13,Col,eFontAlign_Center,sString.c_str());
		
		//Col = cColor(0,0,0,mpInit->mpInventory->GetAlpha());
		mpFont->Draw(vPos+cVector3f(1,1,-1),13,Col,eFontAlign_Center,sString.c_str());
		mpFont->Draw(vPos+cVector3f(-1,-1,-1),13,Col,eFontAlign_Center,sString.c_str());*/
	}
}

//-----------------------------------------------------------------------

void cInventorySlot::OnMouseOver()
{	
	/////////////////////////////
	//Moving item
	if(mpInit->mpInventory->GetCurrentItem())
	{
		cInventoryItem *pCurrentItem = mpInit->mpInventory->GetCurrentItem();
		if(mpInit->mpInventory->GetCurrentSlot() == this)
		{
			cGameItemType *pType = mpInit->mpInventory->GetItemType(pCurrentItem->GetItemType());
			tString sShortcutAction = pType->GetShortCutAction(pCurrentItem);
			if(sShortcutAction != "")
			{
				tWString wsName = pCurrentItem->GetGameName();

				iAction *pAction = mpInit->mpGame->GetInput()->GetAction(sShortcutAction);
				if(pAction) wsName = wsName + _W(" (")+ cString::To16Char(pAction->GetInputName()) +_W(")");

				mpInit->mpInventory->SetItemName(wsName);
			}
			else
			{
				mpInit->mpInventory->SetItemName(pCurrentItem->GetGameName());
			}
			mpInit->mpInventory->SetItemDesc(pCurrentItem->GetDescription());
		}
		else if(mpItem)
		{
			mpInit->mpInventory->SetItemDesc(_W(""));
			
            tWString sDesc = kTranslate("Inventory", "Combine") +_W(" ")+ pCurrentItem->GetGameName() +_W(" ") +
							kTranslate("Inventory", "with") +_W(" ")+mpItem->GetGameName();
			mpInit->mpInventory->SetItemName(sDesc);
		}
	}
	/////////////////////////////
	//Not moving item
	else
	{
		if(mpItem)
		{
			cGameItemType *pType = mpInit->mpInventory->GetItemType(mpItem->GetItemType());
			tString sShortcutAction = pType->GetShortCutAction(mpItem);
			if(sShortcutAction != "")
			{
				tWString wsName = mpItem->GetGameName();

				iAction *pAction = mpInit->mpGame->GetInput()->GetAction(sShortcutAction);
				if(pAction) wsName = wsName + _W(" (")+ cString::To16Char(pAction->GetInputName()) +_W(")");

				mpInit->mpInventory->SetItemName(wsName);
			}
			else
			{
				mpInit->mpInventory->SetItemName(mpItem->GetGameName());
			}
			mpInit->mpInventory->SetItemDesc(mpItem->GetDescription());
		}
	}
}

//-----------------------------------------------------------------------

void cInventorySlot::OnMouseDown(eMButton aButton)
{
	/////////////////////////////////
	//Left button
	if(aButton == eMButton_Left)
	{
		if(mpItem!=nullptr)
		{
			mpInit->mpInventory->SetCurrentItem(mpItem);
			mpInit->mpInventory->SetCurrentSlot(this);
			mpItem = nullptr;

			cVector2f vOffset = cVector2f(mRect.x + 4,	mRect.y + 4) - 
								mpInit->mpInventory->GetMousePos();
			if(mpInit->mpInventory->IsDrawer())
			{
				cRect2f rectIcon = cInventory::GetIconRect(mRect);
				vOffset = cVector2f(rectIcon.x, rectIcon.y) - mpInit->mpInventory->GetMousePos();
			}
            mpInit->mpInventory->SetCurrentItemOffset(vOffset);			
		}
	}
	/////////////////////////////////
	//Right button
	else if(aButton == eMButton_Right)
	{
		if(mpItem)
		{
			mpInit->mpInventory->GetContext()->SetActive(true);
			mpInit->mpInventory->GetContext()->Setup(mpItem, mpInit->mpInventory->GetMousePos());
		}
	}
}

//-----------------------------------------------------------------------

void cInventorySlot::OnMouseUp(eMButton aButton)
{
	/////////////////////////////////
	//Left mouse
	if(aButton == eMButton_Left)
	{
		if(mpInit->mpInventory->GetCurrentItem())
		{
			if(mpItem == nullptr)
			{
				mpItem = mpInit->mpInventory->GetCurrentItem();
			}
			else
			{
				cInventoryItem *pCurrentItem = mpInit->mpInventory->GetCurrentItem();
				tString sCurrentItemName = pCurrentItem->GetName();
				tString sItemName = mpItem->GetName();

				//Black Plague: the items' types may combine them themselves
				//(a battery put in the flashlight), the script is asked too.
				bool bShowMessage = true;
				if(cInit::IsOverture()==false)
				{
					cGameItemType *pSlotType = mpInit->mpInventory->GetItemType(mpItem->GetItemType());
					cGameItemType *pCurrentType = mpInit->mpInventory->GetItemType(pCurrentItem->GetItemType());
					bool bCombined = pSlotType && pSlotType->OnCombine(mpItem, pCurrentItem, mlIndex);
					if(	mpItem && mpInit->mpInventory->GetCurrentItem() && pCurrentType &&
						pCurrentType->OnCombine(pCurrentItem, mpItem, mlIndex))
					{
						bCombined = true;
					}
					bShowMessage = bCombined==false;
				}

				if(mpItem==nullptr || mpInit->mpInventory->GetCurrentItem()==nullptr)
				{
					//One was used up by the combining; the other goes back
					if(mpInit->mpInventory->GetCurrentItem())
						mpInit->mpInventory->GetCurrentSlot()->SetItem(mpInit->mpInventory->GetCurrentItem());
				}
				else if(mpInit->mpInventory->CheckCombineCallback(sItemName,sCurrentItemName, mlIndex, bShowMessage))
				{
					//mpInit->mpInventory->RemoveItem(mpInit->mpInventory->GetCurrentItem());
					if(mpInit->mpInventory->GetItem(sCurrentItemName))
						mpInit->mpInventory->GetCurrentSlot()->SetItem(mpInit->mpInventory->GetCurrentItem());
				}
				else
				{
					mpInit->mpInventory->GetCurrentSlot()->SetItem(mpInit->mpInventory->GetCurrentItem());
				}
			}
			mpInit->mpInventory->SetCurrentItem(nullptr);
			mpInit->mpInventory->SetCurrentSlot(nullptr);

			mpInit->mpInventory->SetDroppedInSlot(true);
		}
	}
}

//-----------------------------------------------------------------------

void cInventorySlot::OnDoubleClick(eMButton aButton)
{
	/////////////////////////////////
	//Left mouse
	if(aButton == eMButton_Left)
	{
		//The player might be holding the item when double clicking.
		cInventoryItem *pItem = mpItem;
		if(pItem == nullptr && mpInit->mpInventory->GetCurrentSlot() == this)
		{
			pItem = mpInit->mpInventory->GetCurrentItem();
			if(pItem){
				mpInit->mpInventory->GetCurrentSlot()->SetItem(mpInit->mpInventory->GetCurrentItem());
				mpInit->mpInventory->SetCurrentItem(nullptr);
				mpInit->mpInventory->SetCurrentSlot(nullptr);
			}
		}

		if(pItem)
		{
			cGameItemType *pItemType = mpInit->mpInventory->GetItemType(pItem->GetItemType());
			if(pItemType==nullptr){
				Warning("Itemtype %d does not exist\n",pItem->GetItemType());
				return;
			}

			//Check there is a callback for the item with object = ""
			if(mpInit->mpInventory->CheckUseCallback(pItem->GetName(), ""))
			{
				mpInit->mpInventory->SetActive(false);				
			}

            //Use the action of the item
			else if(pItemType->OnAction(pItem, 0)==false)
			{
				mpInit->mpInventory->SetActive(false);
			}
		}
	}
}

//-----------------------------------------------------------------------

void cInventorySlot::OnShortcutDown(int alNum)
{
	if(mpItem)
	{
		cInventorySlot *pSlot = mpInit->mpInventory->GetEquipSlot(alNum);

		if(pSlot == this) return;

		cInventoryItem *pThisItem = mpItem;

		if(pSlot->mpItem)	SetItem(pSlot->mpItem);
		else				SetItem(nullptr);
		
		pSlot->SetItem(pThisItem);
	}
}

//-----------------------------------------------------------------------

void cInventorySlot::OnUpdate(float afTimeStep)
{

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC ITEM METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cInventoryItem::cInventoryItem(cInit *apInit)
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
	mpGfxObject = nullptr;
	mpGfxObjectAdditive = nullptr;
}

cInventoryItem::~cInventoryItem()
{
	if(mpGfxObject) mpDrawer->DestroyGfxObject(mpGfxObject);
	if(mpGfxObjectAdditive) mpDrawer->DestroyGfxObject(mpGfxObjectAdditive);
}

//-----------------------------------------------------------------------

bool cInventoryItem::Init(cGameItem *apGameItem)
{
	//Init all stuff
	msName = apGameItem->GetName();

	if(apGameItem->GetImageFile()!="")
	{
		mpGfxObject = mpDrawer->CreateGfxObject(apGameItem->GetImageFile(),"diffalpha2d");
		mpGfxObjectAdditive = mpDrawer->CreateGfxObject(apGameItem->GetImageFile(),"diffadditive2d");
	}
	else
		mpGfxObject = nullptr;

	msDescription = apGameItem->GetDescription();
	msGameName = apGameItem->GetGameName();

	mItemType = apGameItem->GetItemType();

	msSubType = apGameItem->GetSubType();

	msHudModelFile = apGameItem->GetHudModelFile(); 
	msHudModelName = apGameItem->GetHudModelName();

	mbCanBeDropped = apGameItem->CanBeDropped();
	mbHasCount = apGameItem->HasCount();
	mlCount = apGameItem->GetCount();

	msEntityFile = apGameItem->GetMeshEntity()->GetSourceFile();

	return true;
}

//-----------------------------------------------------------------------

bool cInventoryItem::InitFromFile(const tString &asFile)
{
	tString sEntityFile = cString::SetFileExt(asFile,"ent");
	tString sPath = mpInit->mpGame->GetResources()->GetFileSearcher()->GetFilePath(sEntityFile);

	if(sPath!="")
	{
		TiXmlDocument *pEntityDoc = hplNew( TiXmlDocument, () );
		if(pEntityDoc->LoadFile(sPath.c_str())==false)
		{
			Error("Couldn't load '%s'!\n",sPath.c_str());
			return false;
		}
		else
		{
			TiXmlElement *pRootElem = pEntityDoc->FirstChildElement();
			TiXmlElement *pMainElem = pRootElem->FirstChildElement("MAIN");
			TiXmlElement *pGameElem = pRootElem->FirstChildElement("GAME");

			msSubType = cString::ToString(pMainElem->Attribute("Subtype"),"");

			tString sImageFile = cString::ToString(pGameElem->Attribute("ImageFile"),"");
			mbCanBeDropped = cString::ToBool(pGameElem->Attribute("CanBeDropped"),true);
			mbHasCount = cString::ToBool(pGameElem->Attribute("HasCount"),false);
			mlCount = cString::ToInt(pGameElem->Attribute("Count"),1);

			msHudModelFile =  cString::ToString(pGameElem->Attribute("HudModelFile"),"");
			msHudModelName =  cString::ToString(pGameElem->Attribute("HudModelName"),"");
						
			tString sNameCat = cString::ToString(pGameElem->Attribute("NameCat"),"");
			tString sNameEntry = cString::ToString(pGameElem->Attribute("NameEntry"),"");
			tString sDescCat = cString::ToString(pGameElem->Attribute("DescCat"),"");
			tString sDescEntry = cString::ToString(pGameElem->Attribute("DescEntry"),"");

			msGameName = kTranslate(sNameCat, sNameEntry);
			msDescription = kTranslate(sDescCat, sDescEntry);

			mItemType = cEntityLoader_GameItem::ToItemType(pGameElem->Attribute("ItemType"));

			if(sImageFile != "")
			{
				mpGfxObject = mpDrawer->CreateGfxObject(sImageFile,"diffalpha2d");
				mpGfxObjectAdditive = mpDrawer->CreateGfxObject(sImageFile,"diffadditive2d");
			}
			else 
				mpGfxObject = nullptr;
			
			msEntityFile = sEntityFile;
		}
		hplDelete( pEntityDoc );
	}
	else
	{
		Error("Entity file '%s' was not found!\n",asFile.c_str());
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------

void cInventoryItem::Drop()
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();

	cMatrixf mtxItem = cMatrixf::Identity;
	mtxItem.SetTranslation(pCamera->GetPosition());

	iEntity3D* pEntity = pWorld->CreateEntity(msName, mtxItem,msEntityFile, true);
	if(pEntity)
	{
		cMeshEntity *pMesh = static_cast<cMeshEntity*>(pEntity);

		if(pMesh->GetBody())
		{
			pMesh->GetBody()->AddImpulse(pCamera->GetForward() * 3.2f);
		}
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// INVENTORY BATTERY
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cInventoryBattery::cInventoryBattery(cInit *apInit, const cRect2f &aRect, cGfxObject* apGfxObject, 
									   float afZ)
		: iInventoryWidget(apInit,aRect,apGfxObject,afZ)
{
	mpGfxObject = mpDrawer->CreateGfxObject("inventory_slot_double.bmp","diffalpha2d");
	mpGfxBatteryMeter = mpDrawer->CreateGfxObject("inventory_battery_meter.bmp","diffalpha2d");
	mpGfxBatteryMeterBar = mpDrawer->CreateGfxObject("inventory_battery_meter_bar.bmp","diffalpha2d");
}

cInventoryBattery::~cInventoryBattery()
{
	mpDrawer->DestroyGfxObject(mpGfxBatteryMeter);
	mpDrawer->DestroyGfxObject(mpGfxBatteryMeterBar);
}

// Black Plague shows the meter only once the flashlight has been found
bool cInventoryBattery::IsShown()
{
	return cInit::IsOverture() || mpInit->mpPlayer->mbHasFlashlight;
}

void cInventoryBattery::Draw()
{
	if(mpInit->mpInventory->IsDrawer()==false) iInventoryWidget::Draw();
}

void cInventoryBattery::OnDraw()
{
	if(IsShown()==false) return;

	//Drawer: a bar, pale when full, amber then red as it runs down
	if(mpInit->mpInventory->IsDrawer())
	{
		float fPercent = cMath::Clamp(mpInit->mpPlayer->GetPower()/100.0f, 0.0f, 1.0f);
		cColor colBar;
		if(fPercent > 0.2f)	colBar = MixColor(cColor(0.86f,0.6f,0.2f), cColor(0.88f,0.84f,0.62f), (fPercent-0.2f)/0.2f);
		else				colBar = MixColor(cColor(0.78f,0.2f,0.1f), cColor(0.86f,0.6f,0.2f), fPercent/0.2f);
		tWString sLabel = cInit::IsOverture() ? kTranslate("Inventory", "BatteryLevel") : kTranslate("Inventory", "Flashlight");
		mpInit->mpInventory->DrawMeter(mRect, mfZ, false, sLabel, fPercent, colBar, fPercent <= 0.15f ? 1.0f : 0.0f);
		return;
	}

	cVector3f vPos = cVector3f(mRect.x, mRect.y, mfZ+1);
	cVector2f vSize(mRect.w, mRect.h);
	float fAlpha = mpInit->mpInventory->GetAlpha();
	float fPercent = mpInit->mpPlayer->GetPower()/100.0f;

	mpDrawer->DrawGfxObject(mpGfxBatteryMeter,vPos,vSize,cColor(1,fAlpha));
	
	mpDrawer->DrawGfxObject(mpGfxBatteryMeterBar,cVector3f(vPos.x+26,vPos.y+16,4),
							cVector2f(21,102),
							cColor(0,0,0,fAlpha));
	mpDrawer->DrawGfxObject(mpGfxBatteryMeterBar,
							cVector3f(vPos.x+26,vPos.y+16 + (102*(1-fPercent)),5),
							cVector2f(21, 102*fPercent),
							cColor(	1.0f-fPercent,fPercent,	0,fAlpha));
	
	//mpFont->Draw(cVector3f(vBatteryPos.x+50, vBatteryPos.y+6,10),20,cColor(0.91f,1,0.91f,mfAlpha),eFontAlign_Left,
	//_W("%.0f%%"),mpInit->mpPlayer->GetPower());
}

void cInventoryBattery::OnMouseOver()
{
	char sPercent[256];
	sprintf(sPercent," %.0f%% ",mpInit->mpPlayer->GetPower());

	if(cInit::IsOverture())
	{
		mpInit->mpInventory->SetItemName(kTranslate("Inventory", "BatteryLevel"));
		mpInit->mpInventory->SetItemDesc(	kTranslate("Inventory", "BatteryLevelDesc1") +
											cString::To16Char(sPercent) +
											kTranslate("Inventory", "BatteryLevelDesc2"));
		return;
	}

	//Black Plague: the meter is the flashlight, and a battery can be put in it
	if(IsShown()==false) return;

	cInventoryItem *pCurrentItem = mpInit->mpInventory->GetCurrentItem();
	if(pCurrentItem && pCurrentItem->GetItemType() == eGameItemType_Battery)
	{
		mpInit->mpInventory->SetItemName(kTranslate("Inventory", "BatteryOnFlashlight"));
		mpInit->mpInventory->SetItemDesc(_W(""));
		return;
	}

	mpInit->mpInventory->SetItemName(kTranslate("Inventory", "Flashlight"));
	mpInit->mpInventory->SetItemDesc(	kTranslate("Inventory", "FlashlightDesc1") +
										cString::To16Char(sPercent) +
										kTranslate("Inventory", "FlashlightDesc2"));
}

void cInventoryBattery::OnMouseUp(eMButton aButton)
{
	//Black Plague: a battery dropped on the meter goes in the flashlight
	if(cInit::IsOverture() || IsShown()==false || aButton != eMButton_Left) return;

	cInventoryItem *pCurrentItem = mpInit->mpInventory->GetCurrentItem();
	if(pCurrentItem==nullptr || pCurrentItem->GetItemType() != eGameItemType_Battery) return;

	mpInit->mpInventory->GetItemType(eGameItemType_Battery)->OnAction(pCurrentItem, 0);
}

void cInventoryBattery::OnUpdate(float afTimeStep)
{

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// INVENTORY HEALTH
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cInventoryHealth::cInventoryHealth(cInit *apInit, const cRect2f &aRect, cGfxObject* apGfxObject, 
									 float afZ)
									 : iInventoryWidget(apInit,aRect,apGfxObject,afZ)
{
	mpGfxObject = mpDrawer->CreateGfxObject("inventory_slot_double.bmp","diffalpha2d");
	mpGfxFine = mpDrawer->CreateGfxObject("inventory_health_fine.bmp","diffalpha2d");
	mpGfxCaution = mpDrawer->CreateGfxObject("inventory_health_caution.bmp","diffalpha2d");
	mpGfxDanger = mpDrawer->CreateGfxObject("inventory_health_danger.bmp","diffalpha2d");
}

cInventoryHealth::~cInventoryHealth()
{
	mpDrawer->DestroyGfxObject(mpGfxFine);
	mpDrawer->DestroyGfxObject(mpGfxCaution);
	mpDrawer->DestroyGfxObject(mpGfxDanger);
}

void cInventoryHealth::Draw()
{
	if(mpInit->mpInventory->IsDrawer()==false) iInventoryWidget::Draw();
}

void cInventoryHealth::OnDraw()
{
	cVector3f vPos = cVector3f(mRect.x, mRect.y, mfZ+1);
	cVector2f vSize(mRect.w, mRect.h);
	float fAlpha = mpInit->mpInventory->GetAlpha();
	float fPercent = mpInit->mpPlayer->GetHealth()/100.0f;
	
	cGfxObject *pGfxMan;
	if(fPercent > 0.75f){		pGfxMan = mpGfxFine;
	}
	else if(fPercent > 0.3f){	pGfxMan = mpGfxCaution;
	}
	else{						pGfxMan = mpGfxDanger;
	}

	//Drawer: a bar, green to amber to red, pulsing when it is low
	if(mpInit->mpInventory->IsDrawer())
	{
		float fHealth = cMath::Clamp(fPercent, 0.0f, 1.0f);
		cColor colBar;
		if(fHealth > 0.35f)	colBar = MixColor(cColor(0.86f,0.58f,0.18f), cColor(0.42f,0.64f,0.3f), (fHealth-0.35f)/0.25f);
		else				colBar = MixColor(cColor(0.78f,0.16f,0.1f), cColor(0.86f,0.58f,0.18f), (fHealth-0.15f)/0.2f);
		mpInit->mpInventory->DrawMeter(mRect, mfZ, true, kTranslate("Inventory", "Health"), fHealth, colBar,
										fHealth <= 0.3f ? 1.0f : 0.0f);
		return;
	}

	mpDrawer->DrawGfxObject(pGfxMan,cVector3f(vPos.x+6,vPos.y+6,2),cVector2f(mRect.w-12,mRect.h-12),
							cColor(	1.0f-fPercent,fPercent,	0,fAlpha));
}

void cInventoryHealth::OnMouseOver()
{
	//Black Plague: with an item in hand it says what using it here does
	cInventoryItem *pCurrentItem = mpInit->mpInventory->GetCurrentItem();
	if(pCurrentItem && cInit::IsOverture()==false)
	{
		mpInit->mpInventory->SetItemName(_W(""));
		mpInit->mpInventory->SetItemDesc(kTranslate("Inventory","Use") + _W(" ") +
										pCurrentItem->GetGameName() + _W(" ") +
										kTranslate("Inventory","on myself"));
		return;
	}

	float fPercent = mpInit->mpPlayer->GetHealth()/100.0f;
	tWString wsText;

	if(fPercent > 0.75f){
		wsText = kTranslate("Inventory","HealthFine");
	}
	else if(fPercent > 0.3f){
		wsText = kTranslate("Inventory","HealthCaution");
	}
	else{
		wsText = kTranslate("Inventory","HealthDanger");
	}

	mpInit->mpInventory->SetItemName(kTranslate("Inventory", "Health"));
	mpInit->mpInventory->SetItemDesc(wsText);
		
}

void cInventoryHealth::OnUpdate(float afTimeStep)
{

}

//-----------------------------------------------------------------------



//////////////////////////////////////////////////////////////////////////
// INVENTORY CONTEXT METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cInventoryContext::cInventoryContext(cInit *apInit)
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	mbActive = false;

	mpGfxBack = mpDrawer->CreateGfxObject("inventory_context_back.bmp","diffalpha2d");
	mpGfxCorner11 = mpDrawer->CreateGfxObject("inventory_context_11.bmp","diffalpha2d");
	mpGfxCorner12 = mpDrawer->CreateGfxObject("inventory_context_12.bmp","diffalpha2d");
	mpGfxCorner21 = mpDrawer->CreateGfxObject("inventory_context_21.bmp","diffalpha2d");
	mpGfxCorner22 = mpDrawer->CreateGfxObject("inventory_context_22.bmp","diffalpha2d");
	mpGfxRight = mpDrawer->CreateGfxObject("inventory_context_right.bmp","diffalpha2d");
	mpGfxLeft = mpDrawer->CreateGfxObject("inventory_context_left.bmp","diffalpha2d");
	mpGfxTop = mpDrawer->CreateGfxObject("inventory_context_top.bmp","diffalpha2d");
	mpGfxBottom = mpDrawer->CreateGfxObject("inventory_context_bottom.bmp","diffalpha2d");

	
	mvPos.z = 40.0f;

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");

	mfRowStart =0;
	mfRowSize = 15;
	mfColLength = 100;

	mlSelectedRow = -1;

	mfAlpha = 0.0f;

	mpActionVec = nullptr;
}

//-----------------------------------------------------------------------

cInventoryContext::~cInventoryContext()
{

}

//-----------------------------------------------------------------------

void cInventoryContext::SetActive(bool abActive)
{
	mbActive = abActive;

	if(mbActive)
	{

	}
	else
	{

	}
}

//-----------------------------------------------------------------------

void cInventoryContext::Draw()
{
	if(mfAlpha <= 0.0f || mpActionVec==nullptr) return;

	float fTotalAlpha = mfAlpha * mpInit->mpInventory->GetAlpha();

	mpDrawer->DrawGfxObject(mpGfxBack,mvPos,mvSize,cColor(1,fTotalAlpha));
	
	//Corners
	cVector2f vCornerSize(3,3);
	mpDrawer->DrawGfxObject(mpGfxCorner11,mvPos - vCornerSize,vCornerSize,cColor(1,fTotalAlpha));
	mpDrawer->DrawGfxObject(mpGfxCorner21,mvPos + cVector3f(mfColLength, -vCornerSize.y,0),
							vCornerSize,cColor(1,fTotalAlpha));
	mpDrawer->DrawGfxObject(mpGfxCorner12,mvPos + cVector3f(-vCornerSize.x,mvSize.y,0),
							vCornerSize,cColor(1,fTotalAlpha));
	mpDrawer->DrawGfxObject(mpGfxCorner22,mvPos + cVector3f(mvSize.x,mvSize.y,0),
							vCornerSize,cColor(1,fTotalAlpha));
	
	//Sides
	mpDrawer->DrawGfxObject(mpGfxLeft,mvPos + cVector3f(-vCornerSize.x,0,0), 
							cVector2f(vCornerSize.x, mvSize.y), cColor(1,fTotalAlpha));
	mpDrawer->DrawGfxObject(mpGfxRight,mvPos + cVector3f(mvSize.x,0,0), 
							cVector2f(vCornerSize.x, mvSize.y), cColor(1,fTotalAlpha));
	mpDrawer->DrawGfxObject(mpGfxTop,mvPos + cVector3f(0,-vCornerSize.y,0), 
							cVector2f(mvSize.x,vCornerSize.y), cColor(1,fTotalAlpha));
	mpDrawer->DrawGfxObject(mpGfxTop,mvPos + cVector3f(0,mvSize.y,0), 
							cVector2f(mvSize.x,vCornerSize.y), cColor(1,fTotalAlpha));


	for(int i=0; i< (int)mpActionVec->size(); i++)
	{
		cColor Col = mlSelectedRow == i? cColor(0.2f,1.0f,0.2f,mfAlpha) : cColor(0.65f,0.65f,0.65f,fTotalAlpha);
		
		//mpFont->Draw(mvPos + cVector3f(2.0f,(float)i*mfRowSize,1),14,Col,
		//			eFontAlign_Left,(*mpActionVec)[i].c_str());
	}
}

//-----------------------------------------------------------------------

void cInventoryContext::Update(float afTimeStep)
{
	////////////////////////////////
	// Change alpha
	if(mbActive==false)
	{
		mfAlpha -= 2.5f * afTimeStep;
		if(mfAlpha<0) mfAlpha = 0;

		return;
	}
	else
	{
		mfAlpha += 2.3f * afTimeStep;
		if(mfAlpha>1) mfAlpha = 1;
	}
	
    mpInit->mpInventory->SetItemDesc(mpItem->GetDescription());
	mpInit->mpInventory->SetItemName(mpItem->GetGameName());
	
	///////////////////////////////////////
	//Check if the mouse is over any row
	cVector2f vMousePos = mpInit->mpInventory->GetMousePos();
	mlSelectedRow = -1;
	
	for(int i=0; i< (int)mpActionVec->size(); i++)
	{
		cRect2f Rect(mvPos.x,mvPos.y + mfRowStart + (float)i * mfRowSize,
						mfColLength, mfRowSize);
		if(cMath::PointBoxCollision(vMousePos,Rect))
		{
			mlSelectedRow = i;
			break;
		}
	}
}

//-----------------------------------------------------------------------

void cInventoryContext::OnMouseDown(eMButton aButton)
{
	/////////////////////////////////
	//Left button
	if(aButton == eMButton_Left)
	{
		if(mlSelectedRow != -1)
		{
			cGameItemType *pItemType = mpInit->mpInventory->GetItemType(mpItem->GetItemType());
			
			if(pItemType->OnAction(mpItem, mlSelectedRow)==false)
			{
				mpInit->mpInventory->SetActive(false);
			}
		}
		mbActive = false;
	}
	/////////////////////////////////
	//Right button
	else if(aButton == eMButton_Right)
	{
		mbActive = false;
	}
}

//-----------------------------------------------------------------------

void cInventoryContext::OnMouseUp(eMButton aButton)
{

}


//-----------------------------------------------------------------------

void cInventoryContext::Setup(cInventoryItem *apItem, const cVector2f& avPos)
{
	mpItem = apItem;
	mvPos = avPos;

	cGameItemType *pItemType = mpInit->mpInventory->GetItemType(apItem->GetItemType());
	mpActionVec = pItemType->GetActions(apItem->CanBeDropped());

	mvSize = cVector2f(mfColLength, (float)mpActionVec->size()* mfRowSize + 8);

	mRect = cRect2f(mvPos.x, mvPos.y, mvSize.x, mvSize.y);
}


//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// REMASTERED DRAWER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

// Items whose type does nothing of its own: keys, parts and the like, which
// scripts use on the world or combine. They are the ones the story needs.
bool cInventory::IsQuestItem(cInventoryItem *apItem)
{
	return apItem->GetItemType() == eGameItemType_Normal;
}

cColor cInventory::GetDrawerTextColor(float afAlpha)
{
	if(mbPaperDrawer) return cColor(0.12f,0.1f,0.08f,afAlpha);
	return cColor(0.86f,0.84f,0.8f,afAlpha);
}

cRect2f cInventory::GetIconRect(const cRect2f &aCell)
{
	return cRect2f(aCell.x + kfIconInset, aCell.y + kfIconInset,
					aCell.w - kfIconInset*2, aCell.h - kfIconInset*2);
}

void cInventory::DrawItemIcon(cInventoryItem *apItem, const cVector3f &avPos, float afBox, const cColor &aColor)
{
	cGfxObject *pGfx = apItem->GetGfxObject();
	if(pGfx==nullptr) return;

	cVector2l vSize = pGfx->GetMaterial()->GetImage(eMaterialTexture_Diffuse)->GetSize();
	if(vSize.x <= 0 || vSize.y <= 0) return;
	float fScale = afBox / (float)std::max(vSize.x, vSize.y);
	cVector2f vDrawSize((float)vSize.x * fScale, (float)vSize.y * fScale);
	cVector3f vPos(avPos.x + (afBox - vDrawSize.x)*0.5f, avPos.y + (afBox - vDrawSize.y)*0.5f, avPos.z);

	mpDrawer->DrawGfxObject(pGfx, vPos, vDrawSize, aColor);
}

void cInventory::DrawMeter(const cRect2f &aRect, float afZ, bool abHealth, const tWString &asLabel,
							float afPercent, const cColor &aColor, float afPulse)
{
	float fAlpha = mfAlpha;
	float fGlow = 1.0f - afPulse*0.45f*(0.5f + 0.5f*cos(mfMeterTime*5.0f));

	//Icon, label and percentage
	mpDrawer->DrawGfxObject(abHealth ? mpGfxIconHealth : mpGfxIconBattery, cVector3f(aRect.x, aRect.y, afZ+1),
							cVector2f(kfMeterIcon,kfMeterIcon), cColor(aColor.r, aColor.g, aColor.b, fAlpha*fGlow));
	mpDrawerFont->Draw(cVector3f(aRect.x + kfMeterIcon + 5, aRect.y, afZ+1), 13, GetDrawerTextColor(fAlpha),
						eFontAlign_Left, _W("%ls"), asLabel.c_str());
	mpDrawerFont->Draw(cVector3f(aRect.x + kvMeterSize.x, aRect.y, afZ+1), 13, GetDrawerTextColor(fAlpha*0.75f),
						eFontAlign_Right, _W("%.0f%%"), afPercent*100.0f);

	//The bar: trough, fill, segment ticks over it
	cVector3f vBar(aRect.x, aRect.y + 18, afZ);
	mpDrawer->DrawGfxObject(mpGfxMeter, vBar, kvMeterSize, cColor(1,fAlpha));
	if(afPercent > 0)
	{
		mpDrawer->DrawGfxObject(mpGfxMeterFill,
								cVector3f(vBar.x + kvMeterFillPos.x, vBar.y + kvMeterFillPos.y, afZ+0.5f),
								cVector2f(kvMeterFillSize.x*afPercent, kvMeterFillSize.y),
								cColor(aColor.r, aColor.g, aColor.b, fAlpha*fGlow));
	}
	mpDrawer->DrawGfxObject(mpGfxMeterTicks, cVector3f(vBar.x, vBar.y, afZ+0.7f), kvMeterSize, cColor(1,fAlpha));
}

//-----------------------------------------------------------------------

// The drawer slides in from the right edge of the screen (also past the
// 4:3 area on a wide screen), the hotkey row up from the bottom.
void cInventory::UpdateDrawerLayout()
{
	cVector3f vScreenPos = mpInit->mpGraphicsHelper->GetScreenPos();
	cVector2f vScreenSize = mpInit->mpGraphicsHelper->GetScreenSize();

	float fT = 1.0f - mfAlpha;
	float fSlide = 1.0f - fT*fT*fT;

	float fRight = vScreenPos.x + vScreenSize.x;
	float fLeft = fRight - kfDrawerWidth*fSlide;
	mDrawerRect = cRect2f(fLeft, vScreenPos.y, kfDrawerWidth, vScreenSize.y);

	float fGridWidth = klDrawerColumns*kfDrawerCell + (klDrawerColumns-1)*kfDrawerGap;
	float fGridLeft = fLeft + (kfDrawerWidth - fGridWidth)*0.5f;

	int lNormal = 0;
	for(cInventorySlot *pSlot : mlstSlots)
	{
		if(pSlot->mbEquip) continue;
		int lX = lNormal % klDrawerColumns;
		int lY = lNormal / klDrawerColumns;
		pSlot->SetRect(cRect2f(fGridLeft + lX*(kfDrawerCell+kfDrawerGap),
								kfDrawerTop + lY*(kfDrawerCell+kfDrawerGap),
								kfDrawerCell, kfDrawerCell));
		++lNormal;
	}

	float fHotkeyWidth = 9*kfHotkeyCell + 8*kfHotkeyGap;
	float fHotkeyLeft = fRight - kfDrawerWidth - 18 - fHotkeyWidth;
	float fHotkeyTop = 600 - kfHotkeyBottom - kfHotkeyCell + (kfHotkeyBottom + kfHotkeyCell + 8)*(1-fSlide);
	mHotkeyRect = cRect2f(fHotkeyLeft, fHotkeyTop, fHotkeyWidth, kfHotkeyCell);
	for(size_t i=0; i<mvEquipSlots.size(); ++i)
	{
		mvEquipSlots[i]->SetRect(cRect2f(fHotkeyLeft + (float)i*(kfHotkeyCell+kfHotkeyGap), fHotkeyTop,
										kfHotkeyCell, kfHotkeyCell));
	}

	float fStatusLeft = fGridLeft;
	mpHealth->SetRect(cRect2f(fStatusLeft, kfDrawerStatusTop, kvMeterSize.x, kfDrawerStatusHeight));
	mpBattery->SetRect(cRect2f(fStatusLeft + fGridWidth - kvMeterSize.x, kfDrawerStatusTop,
								kvMeterSize.x, kfDrawerStatusHeight));
}

bool cInventory::IsOverDrawer(const cVector2f &avPos)
{
	return cMath::PointBoxCollision(avPos, mDrawerRect) ||
			cMath::PointBoxCollision(avPos, cRect2f(mHotkeyRect.x - kfHotkeyGap, mHotkeyRect.y - kfHotkeyGap,
													mHotkeyRect.w + kfHotkeyGap*2, mHotkeyRect.h + kfHotkeyGap*2));
}

//-----------------------------------------------------------------------

void cInventory::DrawDrawer()
{
	mpDrawer->DrawGfxObject(mpGfxDrawer, cVector3f(mDrawerRect.x, mDrawerRect.y, 2),
							cVector2f(mDrawerRect.w, mDrawerRect.h), cColor(1,mfAlpha));

	///////////////////////////////
	//Name and description of what the mouse is over
	bool bRadioText = cInit::IsOverture() ?	mpInit->mpRadioHandler->IsActive() && mpInit->mbSubtitles :
											mpInit->mpRadioHandler->IsShowingText();
	if(bRadioText) return;

	float fTextAlpha = mfAlpha*mfTextAlpha * (1- mfMessageAlpha);
	if(fTextAlpha <= 0) return;

	float fGridWidth = klDrawerColumns*kfDrawerCell + (klDrawerColumns-1)*kfDrawerGap;
	float fTextLeft = mDrawerRect.x + (kfDrawerWidth - fGridWidth)*0.5f;
	cColor colText = GetDrawerTextColor(fTextAlpha);

	mpDrawerNameFont->Draw(cVector3f(fTextLeft, kfDrawerTextTop, 10), 17, colText, eFontAlign_Left,
							_W("%ls"), msItemName.c_str());
	if(mbPaperDrawer==false)
		mpDrawerNameFont->Draw(cVector3f(fTextLeft+1, kfDrawerTextTop+1, 9), 17, cColor(0,fTextAlpha),
								eFontAlign_Left, _W("%ls"), msItemName.c_str());

	cColor colDesc = mbPaperDrawer ? cColor(0.17f,0.14f,0.11f,fTextAlpha*0.92f) : cColor(0.72f,0.7f,0.66f,fTextAlpha);
	mpDrawerFont->DrawWordWrap(cVector3f(fTextLeft, kfDrawerTextTop + 22, 10), fGridWidth, 16, 14, colDesc,
								eFontAlign_Left, msItemDesc);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cInventory::OnStart()
{
}

//-----------------------------------------------------------------------

void cInventory::Update(float afTimeStep)
{
	////////////////////////////////
	// Change alpha
	if(mbActive==false)
	{
		mfAlpha -= 2.5f * afTimeStep;
		if(mfAlpha<0) mfAlpha = 0;

		mfMessageAlpha -= 3.1f *afTimeStep;
		if(mfMessageAlpha<0) mfMessageAlpha =0;

		return;
	}
	else
	{
		mfAlpha += 2.3f * afTimeStep;
		if(mfAlpha>1) mfAlpha = 1;
	}

	if(mbDrawer) UpdateDrawerLayout();
	mfMeterTime += afTimeStep;
	
	///////////////////////////////
	//Iterate widgets
	if(mpContext->IsActive()==false)
	{
		mbDrawText = false;

		tInventoryWidgetListIt it = mlstWidgets.begin();
		for(; it != mlstWidgets.end(); ++it)
		{
			iInventoryWidget *pWidget = *it;

			pWidget->OnUpdate(afTimeStep);

			if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
			{
				pWidget->OnMouseOver();
			}
		}
	}
	/////////////////////
	// Context Menu
	mpContext->Update(afTimeStep);

	/////////////////////
	// Inventory message
	if(mbMessageActive)
	{
		mfMessageAlpha += 2.7f *afTimeStep;
		if(mfMessageAlpha>1) mfMessageAlpha =1;
	}
	else
	{
		mfMessageAlpha -= 3.1f *afTimeStep;
		if(mfMessageAlpha<0) mfMessageAlpha =0;
	}

	/////////////////////
	// Text alpha
	if(mbDrawText)
	{
		mfTextAlpha += 2.8f *afTimeStep;
		if(mfTextAlpha>1.0f) mfTextAlpha = 1.0f;
	}
	else
	{
		mfTextAlpha -= 3.1f *afTimeStep;
		if(mfTextAlpha<0.0f) mfTextAlpha = 0.0f;
	}
}

//-----------------------------------------------------------------------

void cInventory::Reset()
{
	//reset variables
	mLastCrossHairState = eCrossHairState_Active;

	mbActive = false;

	mfAlpha = 0.0f;
	mfTextAlpha = 0.0f;

	msItemName = _W("");
	msItemDesc = _W("");

	mpCurrentItem = nullptr;
	mpCurrentSlot = nullptr;

	mbCheckingCombineItems = false;

	mbMessageActive = false;

	mbNoteBookIsActive = false;
	
	//Remove all callbacks
	ClearCallbacks();

	//Clear all the slots
	//Normal
	tInventorySlotListIt it = mlstSlots.begin();
	for(; it != mlstSlots.end(); ++it)
	{
		cInventorySlot *pSlot = *it;
		pSlot->SetItem(nullptr);
	}
	//Equip
	for(size_t i=0; i < mvEquipSlots.size(); ++i)
	{
		mvEquipSlots[i]->SetItem(nullptr);
	}

	STLMapDeleteAll(m_mapItems);
}

//-----------------------------------------------------------------------

void cInventory::OnDraw()
{
	if(mfAlpha<=0.0f) return;

	///////////////////////////////
	//Draw background
	mpDrawer->DrawGfxObject(mpGfxBackground,mpInit->mpGraphicsHelper->GetScreenPos(),mpInit->mpGraphicsHelper->GetScreenSize(),cColor(1,mfAlpha));

	if(mbDrawer)
	{
		UpdateDrawerLayout();
		DrawDrawer();
	}
	
	//Draw backpack and hands
	//mpDrawer->DrawGfxObject(mpBagpack,cVector3f(0,600 - (160*sqrt(mfAlpha)),-1),cVector2f(800,160),cColor(1,mfAlpha));

	///////////////////////////////
	//Draw power remaining:
	
	///////////////////////////////
	//Draw health
	/*fPercent = mpInit->mpPlayer->GetHealth()/100.0f;
	cVector2f vManPos(640,120);
	mpDrawer->DrawGfxObject(mpHealthFrame,cVector3f(vManPos.x,vManPos.y,2),cVector2f(150,266),cColor(1,mfAlpha));
	mpDrawer->DrawGfxObject(mpHealthBack,cVector3f(vManPos.x,vManPos.y,2),cVector2f(150,266),
							cColor((1-fPercent)*0.7f + 0.3f,fPercent*0.7f + 0.3f, 0.3f, mfAlpha));
	
	cGfxObject *pGfxMan;
	tWString wsText;
	if(fPercent > 0.75f){		pGfxMan = mpHealthMan_Fine;
								wsText = kTranslate("Inventory","HealthFine");
	}
	else if(fPercent > 0.3f){	pGfxMan = mpHealthMan_Caution;
								wsText = kTranslate("Inventory","HealthCaution");
	}
	else{						pGfxMan = mpHealthMan_Danger;
								wsText = kTranslate("Inventory","HealthDanger");
	}

	mpDrawer->DrawGfxObject(pGfxMan,cVector3f(vManPos.x,vManPos.y,2),cVector2f(150,266),cColor(1,mfAlpha));

	mpDrawer->DrawGfxObject(mpHealthTextFrame,cVector3f(vManPos.x,vManPos.y+266+10,2),cVector2f(150,66),cColor(1,mfAlpha));
	mpFont->Draw(cVector3f(vManPos.x+75,vManPos.y+266+10+4,10),20,cColor(0.91f,1,0.91f,mfAlpha),eFontAlign_Center,
					wsText.c_str());*/

	
	
	///////////////////////////////
	//Draw Widgets
	tInventoryWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		iInventoryWidget *pWidget = *it;

		pWidget->Draw();
		pWidget->OnDraw();
	}

	//////////////////////////////
	//Draw Selected item
	if(mpCurrentItem)
	{
		// Stay with the cursor, which is drawn ahead of its position
		// between two logic steps.
		cVector2f vMousePos = mvMousePos + mpInit->mpButtonHandler->GetCursorDrawOffset(false);
		mpInit->mpGraphicsHelper->ClampToScreen(vMousePos);
		cVector3f vPos(vMousePos.x + mvCurrentItemOffset.x, 
						vMousePos.y + mvCurrentItemOffset.y,15);
		if(mbDrawer)
		{
			DrawItemIcon(mpCurrentItem, vPos, kfDrawerCell - kfIconInset*2, cColor(1,mfAlpha));
		}
		else
		{
		cVector2l vSize = mpCurrentItem->GetGfxObject()->GetMaterial()->GetImage(eMaterialTexture_Diffuse)->GetSize();

		mpDrawer->DrawGfxObject(mpCurrentItem->GetGfxObject(), vPos,
								cVector2f((float)vSize.x, (float)vSize.y),
								cColor(1, mpInit->mpInventory->GetAlpha()));
		}
	}

	//////////////////////////
	//Draw context
    mpContext->Draw();

	//////////////////////////
	//Draw message
	if(mfMessageAlpha>0)
	{
		float fMessAlpha = mfAlpha*mfMessageAlpha;

		cVector3f vMessPos = cVector3f(40,275,110);

		mpDrawer->DrawGfxObject(mpMessageBackground,
								cVector3f(mpInit->mpGraphicsHelper->GetScreenPos().x,vMessPos.y - 8,vMessPos.z-2),
								cVector2f(mpInit->mpGraphicsHelper->GetScreenSize().x,17*4+8*2),cColor(1,1,1,fMessAlpha *0.92f));
		
		mpFont->DrawWordWrap(vMessPos + cVector3f(0,0,0),720,16,17,cColor(1,1,1,fMessAlpha),eFontAlign_Left,
								msMessage.c_str());
		mpFont->DrawWordWrap(vMessPos + cVector3f(0,1,-1),720,16,17,cColor(0,0,0,fMessAlpha),eFontAlign_Left,
								msMessage.c_str());
	}
	
	//////////////////////////
	//Draw text
	//Black Plague: hidden only while radio text is shown (a far message has none)
	bool bRadioText = cInit::IsOverture() ?	mpInit->mpRadioHandler->IsActive() && mpInit->mbSubtitles :
											mpInit->mpRadioHandler->IsShowingText();
	if(bRadioText==false && mbDrawer==false)
	{
		float fTextAlpha = mfAlpha*mfTextAlpha * (1- mfMessageAlpha);
		mpFont->Draw(cVector3f(400, 460,10),19,cColor(1,1,1,fTextAlpha),eFontAlign_Center,
							_W("%ls"),msItemName.c_str());
		mpFont->Draw(cVector3f(400+1, 460+1,9),19,cColor(0,0,0,fTextAlpha),eFontAlign_Center,
							_W("%ls"),msItemName.c_str());

		mpFont->DrawWordWrap(cVector3f(80,480,10),640,16,17,cColor(1,1,1,fTextAlpha),eFontAlign_Left,
							msItemDesc.c_str());
		mpFont->DrawWordWrap(cVector3f(80+1,480+1,9),640,16,17,cColor(0,0,0,fTextAlpha),eFontAlign_Left,
							msItemDesc.c_str());
	}
}

//-----------------------------------------------------------------------

void cInventory::SetActive(bool abActive)
{
	if(mbActive == abActive) return;
	mbActive = abActive;
    
	if(mbActive)
	{
		if(mpInit->mbHasHaptics)
			mpInit->mpPlayer->GetHapticCamera()->SetActive(false);

		mLastCrossHairState = mpInit->mpPlayer->GetCrossHairState();
		
		mvMousePos = cVector2f(400,300);
		//Drawer: over the middle of its grid
		if(mbDrawer)
		{
			mvMousePos = cVector2f(mpInit->mpGraphicsHelper->GetScreenPos().x + mpInit->mpGraphicsHelper->GetScreenSize().x -
									kfDrawerWidth*0.5f, kfDrawerTop + 3*kfDrawerCell);
		}
		mpInit->mpPlayer->SetCrossHairPos(mvMousePos);
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Pointer);
	}
	else
	{
		if(mpInit->mbHasHaptics)
			mpInit->mpPlayer->GetHapticCamera()->SetActive(true);

		mpInit->mpPlayer->SetCrossHairState(mLastCrossHairState);
		mbMessageActive = false;
	}
}

//-----------------------------------------------------------------------

bool cInventory::IsActive()
{
	return mbActive;
}

//-----------------------------------------------------------------------

void cInventory::OnInventoryDown()
{
	if(mbMessageActive)
	{
		mbMessageActive = false;
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Pointer);
		return;		
	}

	SetActive(false);
	mpInit->mpPlayer->SetCrossHairPos(cVector2f(400,300));
	mvMousePos = cVector2f(400,300);
}

//-----------------------------------------------------------------------

void cInventory::AddWidget(iInventoryWidget* apWidget)
{
	mlstWidgets.push_back(apWidget);
}

//-----------------------------------------------------------------------

void cInventory::AddItem(cGameItem *apGameItem)
{
	tWString sMessage = apGameItem->GetGameName();//kTranslate("Inventory","YouPickedUp") + _W(" ")+apGameItem->GetGameName();
	mpInit->mpEffectHandler->GetSubTitle()->Add(sMessage,2.0f,true);

	//////////////////////////
	//If the item has a count, check if it exists
	if(apGameItem->HasCount())
	{
		cInventoryItem *pFoundItem = nullptr;
		
		tInventoryItemMapIt it = m_mapItems.begin();
		for(; it != m_mapItems.end(); ++it)
		{
			cInventoryItem *pItem = it->second;
			if(	pItem->GetItemType() == apGameItem->GetItemType() &&
				pItem->GetSubType() == apGameItem->GetSubType())
			{
				pFoundItem = pItem;
				break;
			}
		}

		//Black Plague: the item's type has a say first, else it is added as a new item
		if(pFoundItem && (cInit::IsOverture() ||
			GetItemType(pFoundItem->GetItemType())->OnPickUp(pFoundItem,true)))
		{
			pFoundItem->AddCount(apGameItem->GetCount());
			CheckPickupCallback(apGameItem->GetName());
			return;
		}
	}

	//////////////////////////
	//Create item
	cInventoryItem *pItem = hplNew( cInventoryItem, (mpInit) );
	pItem->Init(apGameItem);
	
	cGameItemType *pType = GetItemType(pItem->GetItemType());
    if(pType->OnPickUp(pItem,true)==false)
	{
		CheckPickupCallback(pItem->GetName());
		hplDelete( pItem );
		return;
	}
	
	m_mapItems.insert(tInventoryItemMap::value_type(pItem->GetName(),pItem));

	tInventorySlotListIt it = mlstSlots.begin();
	for(; it != mlstSlots.end(); ++it)
	{
		cInventorySlot *pSlot = *it;
		
		if(pSlot->GetItem()==nullptr)
		{
			pSlot->SetItem(pItem);
			CheckPickupCallback(pItem->GetName());
			return;
		}
	}

	//TODO: Show message that there is no room for the item.
}

//-----------------------------------------------------------------------

void cInventory::AddItemFromFile(const tString &asName,const tString &asFile, int alSlotIndex)
{
	/////////////////////////////////
	//Create Item	
	cInventoryItem *pItem = hplNew( cInventoryItem, (mpInit) );
	pItem->SetName(asName);
	pItem->InitFromFile(asFile);

	//As Black Plague: an unknown type is an error, not a crash
	int lType = pItem->GetItemType();
	cGameItemType *pType = lType >= 0 && lType < (int)mvItemTypes.size() ? mvItemTypes[lType] : nullptr;
	if(pType == nullptr)
	{
		Error("Item type %d does not exist!\n", lType);
		hplDelete( pItem );
		return;
	}
	if(pType->OnPickUp(pItem,false)==false)
	{
		CheckPickupCallback(pItem->GetName());
		hplDelete( pItem );
		return;
	}

	//////////////////////////
	//If the item has a count, check if it exists
	if(pItem->HasCount())
	{
		cInventoryItem *pFoundItem = nullptr;

		tInventoryItemMapIt it = m_mapItems.begin();
		for(; it != m_mapItems.end(); ++it)
		{
			cInventoryItem *pTempItem = it->second;
			if(	pTempItem->GetItemType() == pItem->GetItemType() &&
				pItem->GetSubType() == pTempItem->GetSubType())
			{
				pFoundItem = pTempItem;
				break;
			}
		}

		if(pFoundItem)
		{
			pFoundItem->AddCount(pItem->GetCount());
			CheckPickupCallback(pItem->GetName());
			hplDelete( pItem ); 
			return;
		}
	}
	
	//////////////////////////
	//Add to inventory
	m_mapItems.insert(tInventoryItemMap::value_type(pItem->GetName(),pItem));

	int lCount=0;
	int lCurrentSlot = -1;
	
	tInventorySlotListIt it = mlstSlots.begin();
	for(; it != mlstSlots.end(); ++it)
	{
		cInventorySlot *pSlot = *it;

		if( (alSlotIndex <0 && pSlot->GetItem()==nullptr &&  (mpCurrentItem==nullptr || mpCurrentSlot != pSlot))
			||
			(alSlotIndex == lCount))
		{
			pSlot->SetItem(pItem);
			CheckPickupCallback(pItem->GetName());
			break;
		}
		lCount++;
	}
}

//-----------------------------------------------------------------------

void cInventory::RemoveItem(cInventoryItem *apItem)
{
	tInventorySlotListIt SlotIt = mlstSlots.begin();
	for(; SlotIt != mlstSlots.end();SlotIt++)
	{
		cInventorySlot *pSlot = *SlotIt;

		if(pSlot->GetItem() == apItem)
		{
			pSlot->SetItem(nullptr);
		}
	}

	tInventoryItemMapIt it = m_mapItems.begin();
	for(; it != m_mapItems.end(); ++it)
	{
		if(it->second == apItem){
			m_mapItems.erase(it);
			break;
		}
	}

	if(mpCurrentItem == apItem)
	{
		mpCurrentItem = nullptr;
	}

	hplDelete( apItem );
}

//-----------------------------------------------------------------------

cInventoryItem *cInventory::GetItem(const tString &asName)
{
	tInventoryItemMapIt it = m_mapItems.find(asName);
	if(it != m_mapItems.end())
	{
		return it->second;
	}

	return nullptr;
}

//-----------------------------------------------------------------------

void cInventory::OnMouseDown(eMButton aButton)
{
	if(mbMessageActive)
	{
		mbMessageActive = false;
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Pointer);
		return;
	}

	// Right-click context menus are disabled here.
	if(aButton == eMButton_Right) return;
	
	/////////////////////////////////
	// Context is active
	if(mpContext->IsActive())
	{
		mpContext->OnMouseDown(aButton);
	}
	/////////////////////////////////
	// Normal
	else
	{
		tInventoryWidgetListIt it = mlstWidgets.begin();
		for(; it != mlstWidgets.end(); ++it)
		{
			iInventoryWidget *pWidget = *it;

			if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
			{
				pWidget->OnMouseDown(aButton);
			}
		}
	}
}

void cInventory::OnMouseUp(eMButton aButton)
{
	/////////////////////////////////
	// Context is active
	if(mpContext->IsActive())
	{
		mpContext->OnMouseUp(aButton);
	}
	/////////////////////////////////
	// Normal
	else
	{
		mbDroppedInSlot = false;
		
		tInventoryWidgetListIt it = mlstWidgets.begin();
		for(; it != mlstWidgets.end(); ++it)
		{
			iInventoryWidget *pWidget = *it;

			if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
			{
				pWidget->OnMouseUp(aButton);
			}
		}
		
		/////////////////////////////////////////
		// Item is dropped outside of slots or returned to its previous slot.
		if(mpCurrentItem && mbDroppedInSlot == false && aButton == eMButton_Left)
		{
			//Drawer: only let go of over the scene is an item dropped
			if(mpCurrentItem->CanBeDropped() && mpCurrentItem->HasCount()==false &&
				(mbDrawer==false || IsOverDrawer(mvMousePos)==false))
			{
				mpCurrentItem->Drop();
				RemoveItem(mpCurrentItem);
			}
			else
			{
				mpInit->mpInventory->GetCurrentSlot()->SetItem(mpInit->mpInventory->GetCurrentItem());
			}

			mpCurrentItem = nullptr;
			mpCurrentSlot = nullptr;
		}
	}
}

//-----------------------------------------------------------------------

void cInventory::OnDoubleClick(eMButton aButton)
{
	if(mbMessageActive) return;

	if(mpContext->IsActive()==false)
	{
		tInventoryWidgetListIt it = mlstWidgets.begin();
		for(; it != mlstWidgets.end(); ++it)
		{
			iInventoryWidget *pWidget = *it;

			if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
			{
				pWidget->OnDoubleClick(aButton);
			}
		}
	}
}

//-----------------------------------------------------------------------

void cInventory::AddMousePos(const cVector2f &avRel)
{
	if(mbMessageActive) return;

	mvMousePos += avRel;
	
	mpInit->mpGraphicsHelper->ClampToScreen(mvMousePos);

	mpInit->mpPlayer->SetCrossHairPos(mvMousePos);
}

void cInventory::SetMousePos(const cVector2f &avPos)
{
	if(mbMessageActive) return;
	
	mvMousePos = avPos;

	mpInit->mpPlayer->SetCrossHairPos(mvMousePos);
}


//-----------------------------------------------------------------------

void cInventory::OnShortcutDown(int alNum)
{	
	if(mbMessageActive) return;

	if(mbActive)
	{
		tInventoryWidgetListIt it = mlstWidgets.begin();
		for(; it != mlstWidgets.end(); ++it)
		{
			iInventoryWidget *pWidget = *it;

			if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
			{
				pWidget->OnShortcutDown(alNum);
			}
		}
	}
	else
	{
		cInventoryItem *pItem = mvEquipSlots[alNum]->GetItem();
		if(pItem != nullptr)
		{
			//Check there is a callback for the item with object = ""
			if(mpInit->mpInventory->CheckUseCallback(pItem->GetName(), ""))
			{
				return;
			}

			cGameItemType *pType = mpInit->mpInventory->GetItemType(mvEquipSlots[alNum]->GetItem()->GetItemType());
			
			pType->OnAction(mvEquipSlots[alNum]->GetItem(),0);
		}
	}
}

//-----------------------------------------------------------------------

void cInventory::SetMessage(const tWString &asMessage)
{
	mbMessageActive = true;
	msMessage = asMessage;

	mpInit->mpPlayer->SetCrossHairState(eCrossHairState_None);
}

//-----------------------------------------------------------------------

void cInventory::AddPickupCallback(const tString &asItem, const tString &asFunction)
{
	//Log("Adding callback %s %s\n",asItem.c_str(),asFunction.c_str());

	cInventoryPickupCallback *pCallback = hplNew( cInventoryPickupCallback, () );
	pCallback->msFunction = asFunction;
	pCallback->msItem = asItem;

	m_mapPickupCallbacks.insert(tInventoryPickupCallbackMap::value_type(asItem,pCallback));
}

//-----------------------------------------------------------------------

void cInventory::AddUseCallback(const tString &asItem, const tString &asObject, const tString &asFunction)
{
	//Check if the item - object combo already exist.
	tInventoryUseCallbackMapIt it = m_mapUseCallbacks.find(asItem);
	if(it != m_mapUseCallbacks.end())
	{
		int lCount = (int)m_mapUseCallbacks.count(asItem);
        for(int i=0; i<lCount; ++i)
		{
			cInventoryUseCallback *pCallback = it->second;
			if(pCallback->msObject == asObject)
			{
				pCallback->msFunction = asFunction;
				return;
			}
		}
	}

	//Add new.
	cInventoryUseCallback *pCallback = hplNew( cInventoryUseCallback,());
	pCallback->msFunction = asFunction;
	pCallback->msItem = asItem;
	pCallback->msObject = asObject;

	m_mapUseCallbacks.insert(tInventoryUseCallbackMap::value_type(asItem,pCallback));
}

//-----------------------------------------------------------------------

void cInventory::AddCombineCallback(const tString &asItem1,const tString &asItem2, const tString &asFunction)
{
	cInventoryCombineCallback *pCallback = hplNew( cInventoryCombineCallback, () );
	pCallback->msFunction = asFunction;
	pCallback->msItem1 = asItem1;
	pCallback->msItem2 = asItem2;
	pCallback->bKillMe = false;

	mlstCombineCallbacks.push_back(pCallback);
}

//-----------------------------------------------------------------------

void cInventory::RemovePickupCallback(const tString &asFunction)
{
	tInventoryPickupCallbackMapIt it = m_mapPickupCallbacks.begin();
	for(; it != m_mapPickupCallbacks.end(); )
	{
		cInventoryPickupCallback *pCallback = it->second;
		if(pCallback->msFunction == asFunction)
		{
			m_mapPickupCallbacks.erase(it++);
			hplDelete( pCallback );
		}
		else
		{
			it++;
		}
	}
}

void cInventory::RemoveUseCallback(const tString &asFunction)
{
	tInventoryUseCallbackMapIt it = m_mapUseCallbacks.begin();
	for(; it != m_mapUseCallbacks.end(); )
	{
		cInventoryUseCallback *pCallback = it->second;
		if(pCallback->msFunction == asFunction)
		{
			m_mapUseCallbacks.erase(it++);
			hplDelete( pCallback );
		}
		else
		{
			it++;
		}
	}
}
void cInventory::RemoveCombineCallback(const tString &asFunction)
{
	tInventoryCombineCallbackListIt it = mlstCombineCallbacks.begin();
	for(; it != mlstCombineCallbacks.end(); ++it)
	{
		cInventoryCombineCallback *pCallback = *it;
		
        if(pCallback->msFunction == asFunction)
		{
			if(mbCheckingCombineItems)
			{
				pCallback->bKillMe = true;
			}
			else
			{
				mlstCombineCallbacks.erase(it);
				hplDelete( pCallback );
			}
			return;
		}
	}
}

//-----------------------------------------------------------------------

void cInventory::CheckPickupCallback(const tString &asItem)
{
	tInventoryPickupCallbackMapIt it = m_mapPickupCallbacks.find(asItem);
	if(it== m_mapPickupCallbacks.end()) return;
	cInventoryPickupCallback *pCallback = it->second;

	tString sCommand = pCallback->msFunction + "(\""+asItem+"\")";
	mpInit->RunScriptCommand(sCommand);
}

//-----------------------------------------------------------------------

bool cInventory::CheckUseCallback(const tString &asItem, const tString &asObject)
{
	tInventoryUseCallbackMapIt it = m_mapUseCallbacks.find(asItem);
	if(it== m_mapUseCallbacks.end()) return false;
	
	int lItemCount = (int)m_mapUseCallbacks.count(asItem);
	tString sCommand="";
	
	for(int i=0; i<lItemCount; ++i)
	{
		cInventoryUseCallback *pCallback = it->second;
		if(pCallback->msObject == asObject)
		{
			sCommand = pCallback->msFunction + "(\""+asItem+"\", \""+asObject+"\")";
			break;
		}
		++it;
	}

	if(sCommand!="")
	{
		mpInit->RunScriptCommand(sCommand);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------

bool cInventory::CheckCombineCallback(const tString &asItem1,const tString &asItem2, int alSlotIndex,
										bool abShowMessage)
{
	cInventoryCombineCallback *pFinalCallback=nullptr;

	mbCheckingCombineItems = true;

	tInventoryCombineCallbackListIt it = mlstCombineCallbacks.begin();
	for(; it != mlstCombineCallbacks.end(); ++it)
	{
		cInventoryCombineCallback *pCallback = *it;

		if( (pCallback->msItem1 == asItem1 && pCallback->msItem2 == asItem2) ||
			(pCallback->msItem1 == asItem2 && pCallback->msItem2 == asItem1) )
		{
			pFinalCallback = pCallback;
			break;	
		}
	}

	mbCheckingCombineItems = false;

	if(pFinalCallback)
	{
		char sString[30];sprintf(sString,"%d",alSlotIndex);

		tString sCommand = pFinalCallback->msFunction + "(\""+asItem1+"\", \""+asItem2+"\", "+
														cString::ToString(sString,"")+")";
		mpInit->RunScriptCommand(sCommand);
		return true;
	}
	else if(abShowMessage)
	{
		//SetActive(false);
		tString sEntry = "CannotCombineItems" + cString::ToString(cMath::RandRectl(1,5));
		//mpInit->mpGameMessageHandler->Add(kTranslate("Inventory", sEntry));
		SetMessage(kTranslate("Inventory", sEntry));
	}

	//Check if any callbacks should be killed
	for(; it != mlstCombineCallbacks.end(); )
	{
		cInventoryCombineCallback *pCallback = *it;
		if(pCallback->bKillMe)
		{
			it = mlstCombineCallbacks.erase(it);
			hplDelete( pCallback );
		}
		else
		{
			++it;
		}
	}

	return false;
}

//-----------------------------------------------------------------------


void cInventory::ClearCallbacks()
{
	STLMapDeleteAll(m_mapPickupCallbacks);
	STLMapDeleteAll(m_mapUseCallbacks);
	STLDeleteAll(mlstCombineCallbacks);
}

//-----------------------------------------------------------------------

void cInventory::SaveToGlobal(cInventory_GlobalSave *apSave)
{
	apSave->mbNoteBookActive = mbNoteBookIsActive;

	////////////////////////////
	//Items
    tInventoryItemMapIt ItemIt = m_mapItems.begin();
	for(; ItemIt != m_mapItems.end(); ++ItemIt)
	{
		cInventoryItem *pItem = ItemIt->second;
		cInventoryItem_GlobalSave saveItem;

		saveItem.msName = pItem->msName;
		saveItem.msGameName = pItem->msGameName;
		saveItem.msDescription = pItem->msDescription;

		saveItem.mItemType = pItem->mItemType;

		saveItem.msSubType = pItem->msSubType;

		saveItem.msEntityFile = pItem->msEntityFile;
		saveItem.msGfxObjectFile = pItem->mpGfxObject->GetSourceFile();

		saveItem.msHudModelFile = pItem->msHudModelFile;
		saveItem.msHudModelName = pItem->msHudModelName;

		saveItem.mbCanBeDropped = pItem->mbCanBeDropped;
		saveItem.mbHasCount = pItem->mbHasCount;
		saveItem.mlCount = pItem->mlCount;

		apSave->mlstItems.Add(saveItem);
	}
	
	////////////////////////////
	//Equip slots
	apSave->mvEquipSlots.Resize(mvEquipSlots.size());
	for(size_t i=0; i<mvEquipSlots.size(); i++)
	{
		if(mvEquipSlots[i]->mpItem)
			apSave->mvEquipSlots[i].msItemName = mvEquipSlots[i]->mpItem->GetName();
		else
			apSave->mvEquipSlots[i].msItemName = "";
	}

	////////////////////////////
	//Normal slots
	tInventorySlotListIt SlotIt = mlstSlots.begin();
	for(; SlotIt != mlstSlots.end(); ++SlotIt)
	{
		cInventorySlot *pSlot = *SlotIt;
		cInventorySlot_GlobalSave saveSlot;

		if(pSlot->mpItem)
			saveSlot.msItemName = pSlot->mpItem->GetName();
		else
			saveSlot.msItemName = "";

		apSave->mlstSlots.Add(saveSlot);
	}
}

//-----------------------------------------------------------------------


void cInventory::LoadFromGlobal(cInventory_GlobalSave *apSave)
{
	mbNoteBookIsActive = apSave->mbNoteBookActive;

	////////////////////////////
	//Items
	cContainerListIterator<cInventoryItem_GlobalSave> ItemIt = apSave->mlstItems.GetIterator();
	while(ItemIt.HasNext())
	{
		cInventoryItem *pItem = hplNew( cInventoryItem, (mpInit) );
		cInventoryItem_GlobalSave saveItem = ItemIt.Next();

		pItem->msName = saveItem.msName;
		pItem->msGameName = saveItem.msGameName;
		pItem->msDescription = saveItem.msDescription;

		pItem->mItemType = saveItem.mItemType;

		pItem->msSubType = saveItem.msSubType;

		pItem->msEntityFile = saveItem.msEntityFile;
		pItem->mpGfxObject = mpDrawer->CreateGfxObject(saveItem.msGfxObjectFile, "diffalpha2d");
		pItem->mpGfxObjectAdditive = mpDrawer->CreateGfxObject(saveItem.msGfxObjectFile,"diffadditive2d");

		pItem->msHudModelFile = saveItem.msHudModelFile;
		pItem->msHudModelName = saveItem.msHudModelName;

		pItem->mbCanBeDropped = saveItem.mbCanBeDropped;
		pItem->mbHasCount = saveItem.mbHasCount;
		pItem->mlCount = saveItem.mlCount;

		m_mapItems.insert(tInventoryItemMap::value_type(pItem->msName,pItem));
	}

	////////////////////////////
	//Equip slots
	for(size_t i=0; i<mvEquipSlots.size(); i++)
	{	
		if(apSave->mvEquipSlots[i].msItemName!="")
			mvEquipSlots[i]->mpItem = GetItem(apSave->mvEquipSlots[i].msItemName);
		else
			mvEquipSlots[i]->mpItem = nullptr;
	}

	////////////////////////////
	//Normal slots
	cContainerListIterator<cInventorySlot_GlobalSave> SlotIt = apSave->mlstSlots.GetIterator();
	tInventorySlotListIt SlotListIt = mlstSlots.begin();
	while(SlotIt.HasNext())
	{
		cInventorySlot *pSlot = *SlotListIt;
		cInventorySlot_GlobalSave saveSlot = SlotIt.Next();

		if(saveSlot.msItemName!="")
			pSlot->mpItem = GetItem(saveSlot.msItemName);
		else
			pSlot->mpItem = nullptr;

		++SlotListIt;
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SAVE OBJECT STUFF
//////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------

kBeginSerializeBase(cInventoryUseCallback)
kSerializeVar(msItem, eSerializeType_String)
kSerializeVar(msObject, eSerializeType_String)
kSerializeVar(msFunction, eSerializeType_String)
kEndSerialize()

//-------------------------------------------------------------------

kBeginSerializeBase(cInventoryPickupCallback)
kSerializeVar(msItem, eSerializeType_String)
kSerializeVar(msFunction, eSerializeType_String)
kEndSerialize()

//-----------------------------------------------------------------------

kBeginSerializeBase(cInventoryCombineCallback)
kSerializeVar(msItem1, eSerializeType_String)
kSerializeVar(msItem2, eSerializeType_String)
kSerializeVar(msFunction, eSerializeType_String)
kSerializeVar(bKillMe, eSerializeType_Bool)
kEndSerialize()

//-----------------------------------------------------------------------

kBeginSerializeBase(cSaveData_cInventory)
kSerializeClassContainer(mlstUseCallbacks,cInventoryUseCallback, eSerializeType_Class)
kSerializeClassContainer(mlstPickupCallbacks,cInventoryPickupCallback, eSerializeType_Class)
kSerializeClassContainer(mlstCombineCallbacks, cInventoryCombineCallback, eSerializeType_Class)
kEndSerialize()

//-----------------------------------------------------------------------

iSaveObject* cSaveData_cInventory::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
{
	///////////////////////
	//Use callbacks
	{
		cContainerListIterator<cInventoryUseCallback> it = mlstUseCallbacks.GetIterator();
		while(it.HasNext())
		{
			cInventoryUseCallback &temp = it.Next();
			gpInit->mpInventory->AddUseCallback(temp.msItem,temp.msObject,temp.msFunction);
		}
	}

	///////////////////////
	//Pickup callbacks
	{
		cContainerListIterator<cInventoryPickupCallback> it = mlstPickupCallbacks.GetIterator();
		while(it.HasNext())
		{
			cInventoryPickupCallback &temp = it.Next();
			gpInit->mpInventory->AddPickupCallback(temp.msItem,temp.msFunction);
		}
	}
	///////////////////////
	//Combine callbacks
	{
		cContainerListIterator<cInventoryCombineCallback> it = mlstCombineCallbacks.GetIterator();
		while(it.HasNext())
		{
			cInventoryCombineCallback &temp = it.Next();
			gpInit->mpInventory->AddCombineCallback(temp.msItem1, temp.msItem2, temp.msFunction);
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------

int cSaveData_cInventory::GetSaveCreatePrio()
{
	return 4;
}


//-----------------------------------------------------------------------

iSaveData* cInventory::CreateSaveData()
{
	cSaveData_cInventory *pData = hplNew( cSaveData_cInventory, () );
	
	//////////////////////
	//Use callbacks
	{
		tInventoryUseCallbackMapIt it = m_mapUseCallbacks.begin();
		for(; it != m_mapUseCallbacks.end(); ++it)
		{
			pData->mlstUseCallbacks.Add(*(it->second));
		}
	}
	
	//////////////////////
	//Pickup callbacks
	{
		tInventoryPickupCallbackMapIt it = m_mapPickupCallbacks.begin();
		for(; it != m_mapPickupCallbacks.end(); ++it)
		{
			pData->mlstPickupCallbacks.Add(*(it->second));
		}
	}
	
	//////////////////////
	//Combine callbacks
	{
		tInventoryCombineCallbackListIt it = mlstCombineCallbacks.begin();
		for(; it != mlstCombineCallbacks.end(); ++it)
		{
			pData->mlstCombineCallbacks.Add(*(*it));
		}
	}


	return pData;
}

//-----------------------------------------------------------------------
