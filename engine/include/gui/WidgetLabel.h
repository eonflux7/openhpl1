// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_LABEL_H
#define HPL_WIDGET_LABEL_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetLabel : public iWidget
	{
	public:
		cWidgetLabel(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetLabel() override;

		void SetTextAlign(eFontAlign aType){mTextAlign = aType;}
		eFontAlign GetTextAlign(){ return mTextAlign;}

		bool GetWordWrap(){ return mbWordWrap;}
		void SetWordWrap(bool abX){ mbWordWrap = abX;}

		void SetMaxTextLength(int alLength);
		int GetMaxTextLength(){return mlMaxCharacters;}


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
		eFontAlign mTextAlign;
		bool mbWordWrap;

		int mlMaxCharacters;

	};

};
#endif // HPL_WIDGET_LABEL_H
