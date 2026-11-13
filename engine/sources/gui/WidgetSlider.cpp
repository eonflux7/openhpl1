// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetSlider.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "system/String.h"

#include "gui/Gui.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

#include "gui/WidgetButton.h"

#include "graphics/FontData.h"

namespace hpl {

	cWidgetSlider::cWidgetSlider(cGuiSet *apSet, cGuiSkin *apSkin,
								eWidgetSliderOrientation aOrientation)
					: iWidget(eWidgetType_Slider,apSet, apSkin)
	{
		mOrientation = aOrientation;

		mbPressed = false;

		mlValue =0;
		mlMaxValue =10;
		mlButtonValueAdd =1;
		mlBarValueSize = 1;

		for(int i=0; i<2; ++i) mvButtons[i] = nullptr;

		mfButtonSize = mpSkin->GetAttribute(eGuiSkinAttribute_SliderButtonSize).x;

		LoadGraphics();
	}

	cWidgetSlider::~cWidgetSlider()
	{
		if(mpSet->IsDestroyingSet()==false)
		{
			for(int i=0; i<2; ++i) mpSet->DestroyWidget(mvButtons[i]);
		}
	}

	void cWidgetSlider::SetValue(int alValue)
	{
		if(mlValue == alValue) return;

		mlValue = alValue;
		if(mlValue <0)mlValue =0;
		if(mlValue >mlMaxValue) mlValue = mlMaxValue;

		cGuiMessageData data = cGuiMessageData(mlValue);
		ProcessMessage(eGuiMessage_SliderMove, data);
		UpdateBarProperties();
	}

	void cWidgetSlider::SetMaxValue(int alMax)
	{
		if(mlMaxValue == alMax) return;

		mlMaxValue = alMax;
		if(mlMaxValue <0)mlMaxValue=0;

		if(mlBarValueSize > mlMaxValue) mlBarValueSize = mlMaxValue;

		UpdateBarProperties();
	}

	void cWidgetSlider::SetButtonValueAdd(int alAdd)
	{
		mlButtonValueAdd = alAdd;
	}

	void cWidgetSlider::SetBarValueSize(int alSize)
	{
		if(mlBarValueSize == alSize) return;

		mlBarValueSize  = alSize;
		if(mlBarValueSize > mlMaxValue+1) mlBarValueSize = mlMaxValue+1;

		UpdateBarProperties();
	}

	void cWidgetSlider::UpdateBarProperties()
	{
		// Vertical
		if(mOrientation == eWidgetSliderOrientation_Vertical)
		{
			mfSliderSize = mvSize.y-mfButtonSize*2;

			mvBarSize = cVector2f(mvSize.x,
				((float)mlBarValueSize / (float)(mlMaxValue+1))*(mfSliderSize));

			float fMinSize = mvGfxCorners[0]->GetActiveSize().y + mvGfxCorners[3]->GetActiveSize().y + 2;
			if(mvBarSize.y < fMinSize) mvBarSize.y = fMinSize;

			//The set taken for each value
			if(mlMaxValue > 0)
				mfValueStep =	(1 / (float)(mlMaxValue)) * (mfSliderSize - mvBarSize.y);
			else
				mfValueStep = 0;

			mvBarPos = cVector3f(0, mfButtonSize + mfValueStep*(float)mlValue,0.2f);
		}
		// Horizontal
		else
		{
			mfSliderSize = mvSize.x-mfButtonSize*2;

			mvBarSize = cVector2f(((float)mlBarValueSize / (float)(mlMaxValue+1))*(mfSliderSize),
									mvSize.y);

			float fMinSize = mvGfxCorners[0]->GetActiveSize().x + mvGfxCorners[3]->GetActiveSize().x + 2;
			if(mvBarSize.x < fMinSize) mvBarSize.x = fMinSize;

			//The set taken for each value
			if(mlMaxValue > 0)
				mfValueStep =	(1 / (float)(mlMaxValue)) * (mfSliderSize - mvBarSize.x);
			else
				mfValueStep =0;

			mvBarPos = cVector3f(mfButtonSize + mfValueStep*(float)mlValue,0,0.2f);
		}

		mBarRect.x = GetGlobalPosition().x + mvBarPos.x;
		mBarRect.y = GetGlobalPosition().y + mvBarPos.y;
		mBarRect.w = mvBarSize.x;
		mBarRect.h = mvBarSize.y;


		mfMaxPos = mfButtonSize + mfValueStep * (float)(mlMaxValue);
		mfMinPos = mfButtonSize;
	}

	bool cWidgetSlider::ArrowButtonDown(iWidget* apWidget,cGuiMessageData& aData)
	{
		int i;
		for(i=0; i<2; ++i) if(mvButtons[i] == apWidget) break;

		if(i==0) SetValue(mlValue-1);
		if(i==1) SetValue(mlValue+1);

		//Call callbacks.
		cGuiMessageData data = cGuiMessageData(mlValue);
		ProcessMessage(eGuiMessage_SliderMove, data);

		return true;
	}
	kGuiCalllbackDeclaredFuncEnd(cWidgetSlider,ArrowButtonDown)

	void cWidgetSlider::OnInit()
	{
		mvButtons[0] = mpSet->CreateWidgetButton(0,0,_W(""), this);
		mvButtons[1] = mpSet->CreateWidgetButton(0,0,_W(""), this);

		for(int i=0; i<2;i++)
		{
			mvButtons[i]->SetImage(mvGfxArrow[i],false);

			mvButtons[i]->AddCallback(eGuiMessage_MouseDown,this,kGuiCallback(ArrowButtonDown));
		}

		OnChangeSize();
	}

	void cWidgetSlider::OnLoadGraphics()
	{
		//Background, corners and borders
		mpGfxButtonBackground = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpBackground);

		mvGfxBorders[0] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpBorderRight);
		mvGfxBorders[1] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpBorderLeft);
		mvGfxBorders[2] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpBorderUp);
		mvGfxBorders[3] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpBorderDown);

		mvGfxCorners[0] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpCornerLU);
		mvGfxCorners[1] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpCornerRU);
		mvGfxCorners[2] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpCornerRD);
		mvGfxCorners[3] = mpSkin->GetGfx(eGuiSkinGfx_ButtonUpCornerLD);

		// Vertical
		if(mOrientation == eWidgetSliderOrientation_Vertical)
		{
			mvGfxArrow[0] = mpSkin->GetGfx(eGuiSkinGfx_SliderVertArrowUp);
			mvGfxArrow[1] = mpSkin->GetGfx(eGuiSkinGfx_SliderVertArrowDown);
			mpGfxBackground = mpSkin->GetGfx(eGuiSkinGfx_SliderVertBackground);
		}
		// Horizontal
		else
		{
			mvGfxArrow[0] = mpSkin->GetGfx(eGuiSkinGfx_SliderHoriArrowUp);
			mvGfxArrow[1] = mpSkin->GetGfx(eGuiSkinGfx_SliderHoriArrowDown);
			mpGfxBackground = mpSkin->GetGfx(eGuiSkinGfx_SliderHoriBackground);
		}

		mfButtonSize = mpSkin->GetAttribute(eGuiSkinAttribute_SliderButtonSize).x;
	}

	void cWidgetSlider::OnChangeSize()
	{
		if(mvButtons[0] == nullptr || mvButtons[1] == nullptr) return;

		// Vertical
		if(mOrientation == eWidgetSliderOrientation_Vertical)
		{
			mvButtons[0]->SetPosition(cVector3f(0,0,0.2f));
			mvButtons[0]->SetSize(cVector2f(mvSize.x, mfButtonSize));

			mvButtons[1]->SetPosition(cVector3f(0,mvSize.y-mfButtonSize,0.2f));
			mvButtons[1]->SetSize(cVector2f(mvSize.x, mfButtonSize));
		}
		// Horizontal
		else
		{
			mvButtons[0]->SetPosition(cVector3f(0,0,0.2f));
			mvButtons[0]->SetSize(cVector2f(mfButtonSize,mvSize.y));

			mvButtons[1]->SetPosition(cVector3f(mvSize.x-mfButtonSize,0,0.2f));
			mvButtons[1]->SetSize(cVector2f(mfButtonSize, mvSize.y));
		}

		UpdateBarProperties();
	}

	void cWidgetSlider::OnChangePosition()
	{
		UpdateBarProperties();
	}

	void cWidgetSlider::OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion)
	{
		// Background

		// Vertical
		if(mOrientation == eWidgetSliderOrientation_Vertical)
		{
			mpSet->DrawGfx(mpGfxBackground, GetGlobalPosition() +
							cVector3f(0,mfButtonSize,0.1f),
							cVector2f(mvSize.x,mvSize.y - mfButtonSize*2));
		}
		// Horizontal
		else
		{
			mpSet->DrawGfx(mpGfxBackground, GetGlobalPosition() +
				cVector3f(mfButtonSize,0,0.1f),
				cVector2f(mvSize.x - mfButtonSize*2,mvSize.y));
		}

		// Borders and button background
		DrawBordersAndCorners(	mpGfxButtonBackground, mvGfxBorders, mvGfxCorners,
								GetGlobalPosition() + mvBarPos, mvBarSize);
	}

	bool cWidgetSlider::OnMouseMove(cGuiMessageData &aData)
	{
		if(mbPressed)
		{


			int lVal;
			// Vertical
			if(mOrientation == eWidgetSliderOrientation_Vertical)
			{
				mvBarPos.y = WorldToLocalPosition(aData.mvPos).y + mvRelMousePos.y;

				if(mvBarPos.y > mfMaxPos) mvBarPos.y = mfMaxPos;
				if(mvBarPos.y < mfMinPos) mvBarPos.y = mfMinPos;

				mBarRect.y = GetGlobalPosition().y + mvBarPos.y;
				lVal = (int)((mvBarPos.y-mfButtonSize) / mfValueStep + 0.5f);
			}
			// Horizontal
			else
			{
				mvBarPos.x = WorldToLocalPosition(aData.mvPos).x + mvRelMousePos.x;

				if(mvBarPos.x > mfMaxPos) mvBarPos.x = mfMaxPos;
				if(mvBarPos.x < mfMinPos) mvBarPos.x = mfMinPos;

				mBarRect.x = GetGlobalPosition().x + mvBarPos.x;
				lVal = (int)((mvBarPos.x-mfButtonSize) / mfValueStep + 0.5f);
			}


			if(lVal>mlMaxValue)lVal = mlMaxValue;
			if(lVal<0)lVal = 0;
			if(lVal != mlValue)
			{
				mlValue = lVal;
				cGuiMessageData data = cGuiMessageData(mlValue);
				ProcessMessage(eGuiMessage_SliderMove, data);
			}
		}

		return true;
	}

	bool cWidgetSlider::OnMouseDown(cGuiMessageData &aData)
	{
		if(cMath::PointBoxCollision(aData.mvPos,mBarRect))
		{
			mbPressed = true;
			cVector3f vRel = mvBarPos - WorldToLocalPosition(aData.mvPos);
			mvRelMousePos.x = vRel.x;
			mvRelMousePos.y = vRel.y;
		}

		return true;
	}

	bool cWidgetSlider::OnMouseUp(cGuiMessageData &aData)
	{
		if(mbPressed)
		{
			mbPressed = false;
			UpdateBarProperties();
		}

		return true;
	}

	bool cWidgetSlider::OnMouseEnter(cGuiMessageData &aData)
	{
		return false;
	}

	bool cWidgetSlider::OnMouseLeave(cGuiMessageData &aData)
	{


		return false;
	}

	bool cWidgetSlider::OnLostFocus(cGuiMessageData &aData)
	{
		if(mbPressed)
		{
			mbPressed = false;
			UpdateBarProperties();
		}

		return false;
	}
}
