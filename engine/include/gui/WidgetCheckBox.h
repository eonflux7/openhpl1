// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_CHECK_BOX_H
#define HPL_WIDGET_CHECK_BOX_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetCheckBox : public iWidget
	{
	public:
		cWidgetCheckBox(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetCheckBox() override;

		void SetChecked(bool abX);
		bool IsChecked(){return mbChecked;}

	protected:
		// Implemented functions
		void OnLoadGraphics() override;
		void OnChangeSize() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		// Data
		bool mbChecked;
		bool mbPressed;

		cGuiGfxElement *mvGfxBox[2][2];

	};

};
#endif // HPL_WIDGET_CHECK_BOX_H
