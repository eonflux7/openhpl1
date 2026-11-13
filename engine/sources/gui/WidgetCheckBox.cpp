// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetCheckBox.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

namespace hpl {

	cWidgetCheckBox::cWidgetCheckBox(cGuiSet *apSet, cGuiSkin *apSkin) : iWidget(eWidgetType_CheckBox,apSet, apSkin)
	{
		mbChecked = false;
		mbPressed = false;

		LoadGraphics();
	}

	cWidgetCheckBox::~cWidgetCheckBox()
	{
	}

	void cWidgetCheckBox::SetChecked(bool abX)
	{
		if(mbChecked == abX) return;

		mbChecked = abX;

		cGuiMessageData data = cGuiMessageData(mbChecked);
		ProcessMessage(eGuiMessage_CheckChange,data);
	}

	void cWidgetCheckBox::OnLoadGraphics()
	{
		mvGfxBox[1][0] = mpSkin->GetGfx(eGuiSkinGfx_CheckBoxEnabledUnchecked);
		mvGfxBox[1][1] = mpSkin->GetGfx(eGuiSkinGfx_CheckBoxEnabledChecked);
		mvGfxBox[0][0] = mpSkin->GetGfx(eGuiSkinGfx_CheckBoxDisabledUnchecked);
		mvGfxBox[0][1] = mpSkin->GetGfx(eGuiSkinGfx_CheckBoxDisabledChecked);
	}

	void cWidgetCheckBox::OnChangeSize()
	{
		if(mvSize.y < mvDefaultFontSize.y) mvSize.y = mvDefaultFontSize.y;
		if(mvSize.x < mvGfxBox[0][0]->GetActiveSize().x) mvSize.x = mvGfxBox[0][0]->GetActiveSize().x;
	}

	void cWidgetCheckBox::OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Box
		mpSet->DrawGfx( mvGfxBox[IsEnabled()][mbChecked], GetGlobalPosition() +
						cVector3f(0,mvSize.y/2 - mvGfxBox[0][0]->GetActiveSize().y/2,0));

		// Text
		eGuiSkinFont font = IsEnabled() ? eGuiSkinFont_Default : eGuiSkinFont_Disabled;
		DrawSkinText(			msText,font, GetGlobalPosition() +
							cVector3f(	mvGfxBox[0][0]->GetActiveSize().x +3.0f,
										mvSize.y/2 - mvDefaultFontSize.y/2,0),
							eFontAlign_Left);
	}

	bool cWidgetCheckBox::OnMouseMove(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetCheckBox::OnMouseDown(cGuiMessageData &aData)
	{
		mbPressed = true;
		return true;
	}

	bool cWidgetCheckBox::OnMouseUp(cGuiMessageData &aData)
	{
		if(mbPressed) SetChecked(!mbChecked);

		mbPressed = false;
		return true;
	}

	bool cWidgetCheckBox::OnMouseEnter(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetCheckBox::OnMouseLeave(cGuiMessageData &aData)
	{
		mbPressed = false;
		return false;
	}
}
