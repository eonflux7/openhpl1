// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_SLIDER_H
#define HPL_WIDGET_SLIDER_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetButton;

	class cWidgetSlider : public iWidget
	{
	public:
		cWidgetSlider(cGuiSet *apSet, cGuiSkin *apSkin, eWidgetSliderOrientation aOrientation);
		~cWidgetSlider() override;

		int GetValue(){ return mlValue;}
		void SetValue(int alValue);

		int GetMaxValue(){ return mlMaxValue;}
		void SetMaxValue(int alMax);

		int GetButtonValueAdd(){ return mlButtonValueAdd;}
		void SetButtonValueAdd(int alAdd);

		int GetBarValueSize(){ return mlBarValueSize;}
		void SetBarValueSize(int alSize);

		float GetButtonSize() const{return mfButtonSize;}

	protected:
		// Own functions
		void UpdateBarProperties();

		bool ArrowButtonDown(iWidget* apWidget,cGuiMessageData& aData);
		kGuiCalllbackDeclarationEnd(ArrowButtonDown);

		// Implemented functions
		void OnInit() override;
		void OnLoadGraphics() override;
		void OnChangeSize() override;
		void OnChangePosition() override;


		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		bool OnLostFocus(cGuiMessageData &aData) override;

		// Data
		eWidgetSliderOrientation mOrientation;

		bool mbPressed;

		float mfButtonSize;

		int mlValue;
		int mlMaxValue;
		int mlButtonValueAdd;
		int mlBarValueSize; //This is how big the bar is compared to the max value of the slider
		float mfValueStep;

		cVector3f mvBarPos;
		cVector2f mvBarSize;
		float mfSliderSize;
		cRect2f mBarRect;
		float mfMaxPos;
		float mfMinPos;

		cVector2f mvRelMousePos;

		cWidgetButton* mvButtons[2];

		cGuiGfxElement *mpGfxButtonBackground;
		cGuiGfxElement *mvGfxBorders[4];
		cGuiGfxElement *mvGfxCorners[4];

		cGuiGfxElement *mvGfxArrow[2];
		cGuiGfxElement *mpGfxBackground;
	};

};
#endif // HPL_WIDGET_SLIDER_H
