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
#ifndef HPL_WIDGET_FRAME_H
#define HPL_WIDGET_FRAME_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetFrame : public iWidget
	{
	public:
		cWidgetFrame(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetFrame() override;

		void SetDrawFrame(bool abX){ mbDrawFrame = abX;}
		bool GetDrawFrame(){ return mbDrawFrame;}

		void SetDrawBackground(bool abX){mbDrawBackground = abX;}
		bool GetDrawBackground(){ return mbDrawBackground;}

		void SetBackgroundZ(float afZ){mfBackgroundZ = afZ;}
		float GetBackgroundZ(){ return mfBackgroundZ;}

		void SetBackGroundColor(const cColor &aColor){ mBackGroundColor = aColor;}
		const cColor& GetBackGroundColor(){ return mBackGroundColor;}

	protected:
		/////////////////////////
		// Implemented functions
		void OnLoadGraphics() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;
		void OnDrawAfterClip(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		/////////////////////////
		// Data
		bool mbDrawFrame;
		bool mbDrawBackground;
		float mfBackgroundZ;
		cColor mBackGroundColor;

		cGuiGfxElement *mpGfxBackground;

		cGuiGfxElement *mvGfxBorders[4];
		cGuiGfxElement *mvGfxCorners[4];
	};

};
#endif // HPL_WIDGET_FRAME_H
