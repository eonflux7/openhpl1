// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_WINDOW_H
#define HPL_WIDGET_WINDOW_H

#include <list>
#include <vector>

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetWindow : public iWidget
	{
	public:
		cWidgetWindow(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetWindow() override;

		void SetStatic(bool abX);
		bool GetStatic(){return mbStatic;}


	protected:
		// Implemented functions
		void OnLoadGraphics() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		// Data
		cGuiSkinFont *mpLabelFont;

		cGuiGfxElement *mpGfxBackground;
		cGuiGfxElement *mpGfxLabel;

		cGuiGfxElement *mvGfxBorders[4];
		cGuiGfxElement *mvGfxCorners[4];

		cVector3f mvLabelTextOffset;

		bool mbStatic;

		cVector3f mvRelMousePos;
		bool mbMoving;
	};

};
#endif // HPL_WIDGET_H
