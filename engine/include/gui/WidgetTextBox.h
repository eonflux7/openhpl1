// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_TEXT_BOX_H
#define HPL_WIDGET_TEXT_BOX_H

#include "gui/Widget.h"

namespace hpl {

	class cGuiSkinFont;

	class cWidgetTextBox : public iWidget
	{
	public:
		cWidgetTextBox(cGuiSet *apSet, cGuiSkin *apSkin);
		~cWidgetTextBox() override;

		void SetMaxTextLength(int alLength);
		int GetMaxTextLength(){return mlMaxCharacters;}

		cVector2f GetBackgroundSize();

		void SetMaxTextSizeNeg(float afX);

		void SetCanEdit(bool abX);
		bool GetCanEdit(){return mbCanEdit;}

	protected:
		// Own functions
		int WorldToCharPos(const cVector2f &avWorldPos);
		float CharToLocalPos(int alChar);
		void SetMarkerPos(int alPos);
		int GetLastCharInSize(int alStartPos, float afMaxSize, float afLengthAdd);
		int GetFirstCharInSize(int alStartPos, float afMaxSize, float afLengthAdd);

		// Implemented functions
		void OnLoadGraphics() override;
		void OnChangeSize() override;
		void OnChangeText() override;

		void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion) override;

		bool OnMouseMove(cGuiMessageData &aData) override;
		bool OnMouseDown(cGuiMessageData &aData) override;
		bool OnMouseDoubleClick(cGuiMessageData &aData) override;
		bool OnMouseUp(cGuiMessageData &aData) override;
		bool OnMouseEnter(cGuiMessageData &aData) override;
		bool OnMouseLeave(cGuiMessageData &aData) override;

		bool OnLostFocus(cGuiMessageData &aData) override;

		bool OnKeyPress(cGuiMessageData &aData) override;

		// Data
		bool mbPressed;
		int mlMarkerCharPos;
		int mlSelectedTextEnd;
		int mlFirstVisibleChar;
		int mlVisibleCharSize;
		float mfTextMaxSize;

		int mlMaxCharacters;

		float mfMaxTextSizeNeg;

		bool mbCanEdit;

		cGuiGfxElement *mpGfxMarker;
		cGuiGfxElement *mpGfxSelectedTextBack;

		cGuiGfxElement *mpGfxBackground;

		cGuiGfxElement *mvGfxBorders[4];
		cGuiGfxElement *mvGfxCorners[4];
	};

};
#endif // HPL_WIDGET_TEXT_BOX_H
