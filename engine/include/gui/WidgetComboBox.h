// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_COMBO_BOX_H
#define HPL_WIDGET_COMBO_BOX_H

#include "gui/Widget.h"
#include "gui/WidgetBaseClasses.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetTextBox;
	class cWidgetButton;
	class cWidgetSlider;

	class cWidgetComboBox : public iWidget, public iWidgetItemContainer
	{
	public:
		cWidgetComboBox(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetComboBox() override;

		void SetSelectedItem(int alX,bool abMoveList=false);
		int GetSelectedItem(){ return mlSelectedItem;}

		void SetCanEdit(bool abX);
		bool GetCanEdit();

		void SetMaxShownItems(int alX);
		int GetMaxShownItems(){ return mlMaxItems;}

	protected:
		// Own functions
		void UpdateProperties() override;

		void OpenMenu();
		void CloseMenu();

		bool ButtonPress(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(ButtonPress);

		bool DrawText(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(DrawText);

		bool SliderMove(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(SliderMove);

		bool SliderLostFocus(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(SliderLostFocus);

		// Implemented functions
		void OnLoadGraphics() override;
		void OnChangeSize() override;
		void OnChangeText() override;
		void OnInit() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		bool OnLostFocus(cGuiMessageData &aData) override;

		// Data
		cWidgetTextBox *mpText;
		cWidgetButton *mpButton;
		cWidgetSlider *mpSlider;

		bool mbMenuOpen;
		float mfMenuHeight;

		float mfButtonWidth;
		float mfSliderWidth;

		int mlMouseOverSelection;
		int mlSelectedItem;
		int mlFirstItem;
		int mlMaxItems;
		int mlItemsShown;

		cGuiGfxElement *mpGfxBackground;

		cGuiGfxElement *mpGfxSelection;

		cGuiGfxElement *mvGfxBorders[4];
		cGuiGfxElement *mvGfxCorners[4];
	};

};
#endif // HPL_WIDGET_COMBO_BOX_H
