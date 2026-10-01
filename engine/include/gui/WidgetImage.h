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
		/////////////////////////
		// Implemented functions
		void OnLoadGraphics() override;
		void OnChangeSize() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		/////////////////////////
		// Data
		cGuiGfxElement *mpGfxImage;
		cColor mColor;
	};

};
#endif // HPL_WIDGET_IMAGE_H
