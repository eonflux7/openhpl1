// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetWindow.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

#include "resources/Resources.h"
#include "resources/FontManager.h"

namespace hpl {

	cWidgetWindow::cWidgetWindow(cGuiSet *apSet, cGuiSkin *apSkin) : iWidget(eWidgetType_Window,apSet, apSkin)
	{
		mvRelMousePos =0;
		mbMoving = false;
		mbStatic = false;
	}

	cWidgetWindow::~cWidgetWindow()
	{
	}

	void cWidgetWindow::SetStatic(bool abX)
	{
		mbStatic = abX;

		if(mbStatic==false)
		{
			mbMoving = false;
		}
	}

	void cWidgetWindow::OnLoadGraphics()
	{
		mpGfxBackground = mpSkin->GetGfx(eGuiSkinGfx_WindowBackground);

		mpGfxLabel = mpSkin->GetGfx(eGuiSkinGfx_WindowLabel);

		mvGfxBorders[0] = mpSkin->GetGfx(eGuiSkinGfx_WindowBorderRight);
		mvGfxBorders[1] = mpSkin->GetGfx(eGuiSkinGfx_WindowBorderLeft);
		mvGfxBorders[2] = mpSkin->GetGfx(eGuiSkinGfx_WindowBorderUp);
		mvGfxBorders[3] = mpSkin->GetGfx(eGuiSkinGfx_WindowBorderDown);

		mvGfxCorners[0] = mpSkin->GetGfx(eGuiSkinGfx_WindowCornerLU);
		mvGfxCorners[1] = mpSkin->GetGfx(eGuiSkinGfx_WindowCornerRU);
		mvGfxCorners[2] = mpSkin->GetGfx(eGuiSkinGfx_WindowCornerRD);
		mvGfxCorners[3] = mpSkin->GetGfx(eGuiSkinGfx_WindowCornerLD);

		mpLabelFont = mpSkin->GetFont(eGuiSkinFont_WindowLabel);

		mvLabelTextOffset = mpSkin->GetAttribute(eGuiSkinAttribute_WindowLabelTextOffset);
	}

	void cWidgetWindow::OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Calc label size
		cVector2f vLabelSize;
		vLabelSize.x = mvSize.x - (mvGfxCorners[0]->GetActiveSize().x + mvGfxCorners[1]->GetActiveSize().x);
		vLabelSize.y = mpLabelFont->mvSize.y + mvLabelTextOffset.y*2;

		// Label
		mpSet->DrawGfx(	mpGfxLabel,GetGlobalPosition() +
						cVector3f(mvGfxCorners[0]->GetActiveSize().x,mvGfxCorners[0]->GetActiveSize().y,0.2f),
						vLabelSize);

		// Label text
		DrawSkinText(	msText,eGuiSkinFont_WindowLabel,GetGlobalPosition()+
						cVector3f(	mvGfxCorners[0]->GetActiveSize().x+mvLabelTextOffset.x,
									mvGfxCorners[0]->GetActiveSize().y+mvLabelTextOffset.y,0.4f));

		// Borders and background
		DrawBordersAndCorners(	mpGfxBackground, mvGfxBorders, mvGfxCorners, GetGlobalPosition(), mvSize);
	}

	bool cWidgetWindow::OnMouseMove(cGuiMessageData &aData)
	{
		if(mbMoving) SetGlobalPosition(mvRelMousePos + cVector3f(aData.mvPos.x, aData.mvPos.y,0));
		return true;
	}

	bool cWidgetWindow::OnMouseDown(cGuiMessageData &aData)
	{
		if(mbStatic) return false;

		// Calculate label rectangle
		cRect2f labelRect;
		labelRect.w = mvSize.x - (mvGfxCorners[0]->GetActiveSize().x + mvGfxCorners[1]->GetActiveSize().x);
		labelRect.h = mpLabelFont->mvSize.y + mvLabelTextOffset.y*2;
		labelRect.x = GetGlobalPosition().x + mvGfxCorners[0]->GetActiveSize().x;
		labelRect.y = GetGlobalPosition().y + mvGfxCorners[0]->GetActiveSize().y;

		// Check for collision
		if(cMath::PointBoxCollision(aData.mvPos,labelRect) && aData.mlVal & eGuiMouseButton_Left)
		{
			mbMoving = true;
			mvRelMousePos = GetPosRelativeToMouse(aData);
			mvRelMousePos.z = GetGlobalPosition().z;
		}

		return true;
	}

	bool cWidgetWindow::OnMouseUp(cGuiMessageData &aData)
	{
		if(aData.mlVal & eGuiMouseButton_Left) mbMoving = false;

		return true;
	}

	bool cWidgetWindow::OnMouseEnter(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetWindow::OnMouseLeave(cGuiMessageData &aData)
	{
		cVector3f vLastGlobal = GetGlobalPosition();

		if(mbMoving) SetGlobalPosition(mvRelMousePos + cVector3f(aData.mvPos.x, aData.mvPos.y,0));

		//Check so that mouse is not outside of clip area.
		if(PointIsInside(aData.mvPos, false)==false)
		{
			SetGlobalPosition(vLastGlobal);
			mbMoving = false;
		}

		return false;
	}
}
