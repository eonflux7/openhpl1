// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_LIST_BOX_H
#define HPL_WIDGET_LIST_BOX_H

#include "gui/Widget.h"
#include "gui/WidgetBaseClasses.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetSlider;

	class cWidgetListBox : public iWidget, public iWidgetItemContainer
	{
	public:
		cWidgetListBox(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetListBox() override;

		void SetBackgroundZ(float afZ){mfBackgroundZ = afZ;}
		float GetBackgroundZ(){ return mfBackgroundZ;}

		void SetSelectedItem(int alX,bool abMoveList=false);
		int GetSelectedItem(){ return mlSelectedItem;}

	protected:
		// Own functions
		void UpdateProperties() override;

		bool DrawText(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(DrawText);

		bool MoveSlider(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(MoveSlider);

		// Implemented functions
		void OnLoadGraphics() override;
		void OnInit() override;
		void OnChangeSize() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		bool OnKeyPress(cGuiMessageData &aData) override;

		// Data
		float mfBackgroundZ;
		float mfSliderWidth;

		int mlFirstItem;
		int mlMaxItems;

		int mlSelectedItem;

		cGuiGfxElement *mpGfxBackground;

		cGuiGfxElement *mpGfxSelection;

		cGuiGfxElement *mvGfxBorders[4];
		cGuiGfxElement *mvGfxCorners[4];

		cWidgetSlider *mpSlider;
	};

};
#endif // HPL_WIDGET_LIST_BOX_H
