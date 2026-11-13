// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetFrame.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

namespace hpl {

	cWidgetFrame::cWidgetFrame(cGuiSet *apSet, cGuiSkin *apSkin) : iWidget(eWidgetType_Frame,apSet, apSkin)
	{
		mbClipsGraphics = true;

		mbDrawFrame = false;

		mbDrawBackground = false;
		mfBackgroundZ = -0.5;
		mBackGroundColor = cColor(1,1);
	}

	cWidgetFrame::~cWidgetFrame()
	{
	}

	void cWidgetFrame::OnLoadGraphics()
	{
		mpGfxBackground = mpSkin->GetGfx(eGuiSkinGfx_FrameBackground);

		mvGfxBorders[0] = mpSkin->GetGfx(eGuiSkinGfx_FrameBorderRight);
		mvGfxBorders[1] = mpSkin->GetGfx(eGuiSkinGfx_FrameBorderLeft);
		mvGfxBorders[2] = mpSkin->GetGfx(eGuiSkinGfx_FrameBorderUp);
		mvGfxBorders[3] = mpSkin->GetGfx(eGuiSkinGfx_FrameBorderDown);

		mvGfxCorners[0] = mpSkin->GetGfx(eGuiSkinGfx_FrameCornerLU);
		mvGfxCorners[1] = mpSkin->GetGfx(eGuiSkinGfx_FrameCornerRU);
		mvGfxCorners[2] = mpSkin->GetGfx(eGuiSkinGfx_FrameCornerRD);
		mvGfxCorners[3] = mpSkin->GetGfx(eGuiSkinGfx_FrameCornerLD);
	}

	void cWidgetFrame::OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Borders
		if(mbDrawFrame)
		{
			//Background must be drawn first!
			DrawBordersAndCorners(	nullptr, mvGfxBorders, mvGfxCorners,
									GetGlobalPosition() -
										cVector3f(	mvGfxCorners[0]->GetActiveSize().x,
													mvGfxCorners[0]->GetActiveSize().y,0),
									mvSize +	mvGfxCorners[0]->GetActiveSize() +
												mvGfxCorners[2]->GetActiveSize());
		}
	}

	void cWidgetFrame::OnDrawAfterClip(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Background
		if(mbDrawBackground)
		{
			mpSet->DrawGfx(	mpGfxBackground,GetGlobalPosition() +cVector3f(0,0,mfBackgroundZ),
				mvSize,mBackGroundColor);
		}
	}

	bool cWidgetFrame::OnMouseMove(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetFrame::OnMouseDown(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetFrame::OnMouseUp(cGuiMessageData &aData)
	{
		return true;
	}

	bool cWidgetFrame::OnMouseEnter(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetFrame::OnMouseLeave(cGuiMessageData &aData)
	{
		return false;
	}
}
