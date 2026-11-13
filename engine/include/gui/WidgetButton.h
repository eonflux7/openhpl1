// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_BUTTON_H
#define HPL_WIDGET_BUTTON_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetButton : public iWidget
	{
	public:
		cWidgetButton(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetButton() override;

		void SetImage(cGuiGfxElement *apImage, bool abDestroyImage=true);
		cGuiGfxElement *GetImage(){return mpImage;}

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
		bool mbPressed;

		cGuiGfxElement *mpImage;
		bool mbDestroyImage;

		cGuiGfxElement *mpGfxBackgroundUp;
		cGuiGfxElement *mvGfxBordersUp[4];
		cGuiGfxElement *mvGfxCornersUp[4];

		cGuiGfxElement *mpGfxBackgroundDown;
		cGuiGfxElement *mvGfxBordersDown[4];
		cGuiGfxElement *mvGfxCornersDown[4];
	};

};
#endif // HPL_WIDGET_BUTTON_H
