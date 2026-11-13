// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_IMAGE_H
#define HPL_WIDGET_IMAGE_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetImage : public iWidget
	{
	public:
		cWidgetImage(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetImage() override;

		void SetImage(cGuiGfxElement *apGfx);
		cGuiGfxElement* GetImage(){ return mpGfxImage;}

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
		cGuiGfxElement *mpGfxImage;
		cColor mColor;
	};

};
#endif // HPL_WIDGET_IMAGE_H
