/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
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
		/////////////////////////
		// Implemented functions
		void OnLoadGraphics() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		/////////////////////////
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
