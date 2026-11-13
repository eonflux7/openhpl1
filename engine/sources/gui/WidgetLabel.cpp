// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetLabel.h"

#include "system/LowLevelSystem.h"
#include "system/String.h"

#include "graphics/FontData.h"

#include "math/Math.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

namespace hpl {

	cWidgetLabel::cWidgetLabel(cGuiSet *apSet, cGuiSkin *apSkin) : iWidget(eWidgetType_Label,apSet, apSkin)
	{
		mbWordWrap = false;
		mTextAlign = eFontAlign_Left;

		mlMaxCharacters = -1;
	}

	cWidgetLabel::~cWidgetLabel()
	{
	}

	void cWidgetLabel::SetMaxTextLength(int alLength)
	{
		if(mlMaxCharacters == alLength) return;

		mlMaxCharacters = alLength;
	}

	void cWidgetLabel::OnLoadGraphics()
	{
	}

	void cWidgetLabel::OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Texts
		cVector3f vOffset = 0;
		if(mTextAlign == eFontAlign_Right)	vOffset.x += mvSize.x;

		if(mbWordWrap)
		{
			int lChars =0;
			bool bEnabled = IsEnabled();
			float fHeight = mvDefaultFontSize.y+2;
			tWStringVec vRows;
			mpDefaultFontType->GetWordWrapRows(mvSize.x,fHeight,
												mvDefaultFontSize, msText,
												&vRows);

			for(size_t i=0; i< vRows.size(); ++i)
			{
				bool bBreak = false;
				if(mlMaxCharacters>=0)
				{
					if(lChars + (int)vRows[i].length() > mlMaxCharacters)
					{
						vRows[i] = cString::SubW(vRows[i],0, mlMaxCharacters - lChars);
						bBreak = true;
					}
					lChars += (int)vRows[i].length();
				}

				if(bEnabled)
					DrawDefaultText(vRows[i], GetGlobalPosition()+vOffset,mTextAlign);
				else
					DrawSkinText(vRows[i],eGuiSkinFont_Disabled,GetGlobalPosition()+vOffset,mTextAlign);
				vOffset.y += fHeight;

				if(bBreak) break;
			}
		}
		else
		{
			if(mlMaxCharacters>=0 && (int)msText.length() > mlMaxCharacters)
			{
				if(IsEnabled())
					DrawDefaultText(cString::SubW(msText,0,mlMaxCharacters), GetGlobalPosition()+vOffset,mTextAlign);
				else
					DrawSkinText(cString::SubW(msText,0,mlMaxCharacters),eGuiSkinFont_Disabled,GetGlobalPosition()+vOffset,mTextAlign);
			}
			else
			{
				if(IsEnabled())
					DrawDefaultText(msText, GetGlobalPosition()+vOffset,mTextAlign);
				else
					DrawSkinText(msText,eGuiSkinFont_Disabled,GetGlobalPosition()+vOffset,mTextAlign);
			}
		}
	}

	bool cWidgetLabel::OnMouseMove(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetLabel::OnMouseDown(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetLabel::OnMouseUp(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetLabel::OnMouseEnter(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetLabel::OnMouseLeave(cGuiMessageData &aData)
	{
		return false;
	}
}
