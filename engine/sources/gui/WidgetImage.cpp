// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetImage.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

namespace hpl {

	cWidgetImage::cWidgetImage(cGuiSet *apSet, cGuiSkin *apSkin) : iWidget(eWidgetType_Image,apSet, apSkin)
	{
		mpGfxImage = nullptr;
		mColor = cColor(1,1);
	}

	cWidgetImage::~cWidgetImage()
	{
	}

	void cWidgetImage::SetImage(cGuiGfxElement *apGfx)
	{
		if(mpGfxImage == apGfx) return;

		mpGfxImage = apGfx;
	}

	void cWidgetImage::OnLoadGraphics()
	{
	}

	void cWidgetImage::OnChangeSize()
	{
	}

	void cWidgetImage::OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Background
		if(mpGfxImage)
		{
			mpSet->DrawGfx(	mpGfxImage,	GetGlobalPosition(),mvSize,mColor);
		}
	}

	bool cWidgetImage::OnMouseMove(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetImage::OnMouseDown(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetImage::OnMouseUp(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetImage::OnMouseEnter(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetImage::OnMouseLeave(cGuiMessageData &aData)
	{
		return false;
	}
}
