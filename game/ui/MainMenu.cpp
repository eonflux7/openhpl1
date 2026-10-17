/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ui/MainMenu.h"

#include "core/Init.h"
#include "core/GraphicsHelper.h"
#include "core/ButtonHandler.h"
#include "core/MapHandler.h"
#include "core/SaveHandler.h"
#include "core/LampAppearanceController.h"
#include "ui/IntroStory.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/EffectHandler.h"
#include "ui/GraphicsPresets.h"
#include "player/HapticGameCamera.h"

#include "OALWrapper/OAL_Init.h"

#include <algorithm>

float gfMenuFadeAmount;
bool gbMustRestart=false;

static eMainMenuState gvMenuBackStates[] = {
		eMainMenuState_Exit,//eMainMenuState_Start,
		eMainMenuState_Start,//eMainMenuState_NewGame,
		eMainMenuState_Start,//eMainMenuState_Exit,
		eMainMenuState_Start,//eMainMenuState_Continue,
		eMainMenuState_Start,//eMainMenuState_Resume,

		eMainMenuState_Start,//eMainMenuState_LoadGameSpot,
		eMainMenuState_Start,//eMainMenuState_LoadGameAuto,
		eMainMenuState_Start,//eMainMenuState_LoadGameFavorite,

		eMainMenuState_Start,//eMainMenuState_Options,
		eMainMenuState_Options,//eMainMenuState_OptionsGraphics,
		eMainMenuState_OptionsGraphics,//eMainMenuState_OptionsGraphicsAdvanced,
		eMainMenuState_OptionsGraphics,//eMainMenuState_OptionsGraphicsLighting,
		eMainMenuState_Options,//eMainMenuState_OptionsControls,
		eMainMenuState_Options,//eMainMenuState_OptionsGame,
		eMainMenuState_Options,//eMainMenuState_OptionsAI,
		eMainMenuState_Options,//eMainMenuState_OptionsChangeGame,
		eMainMenuState_Options,//eMainMenuState_OptionsSound,
		eMainMenuState_OptionsControls,//eMainMenuState_OptionsKeySetupMove,
		eMainMenuState_OptionsControls,//eMainMenuState_OptionsKeySetupAction,
		eMainMenuState_OptionsControls,//eMainMenuState_OptionsKeySetupMisc,

		eMainMenuState_Options,//eMainMenuState_GraphicsRestart,

		eMainMenuState_Start,	//eMainMenuState_FirstStart,

		eMainMenuState_Start,	//eMainMenuState_AskTutorial,
		eMainMenuState_Start,	//eMainMenuState_AskStartTutorial,
};

//////////////////////////////////////////////////////////////////////////
// WIDGET
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget::cMainMenuWidget(cInit *apInit, const cVector3f &avPos, const cVector2f &avSize)
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	mvWidgetPos = cVector3f(avPos.x, avPos.y, 40);
	
	mRect.w = avSize.x;
	mRect.h = avSize.y;
	mRect.x = avPos.x - mRect.w/2;
	mRect.y = avPos.y;

	mbActive = true;
}

cMainMenuWidget::~cMainMenuWidget()
{
}

			  
//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// MAIN BUTTON
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_MainButton::cMainMenuWidget_MainButton(cInit *apInit, const cVector3f &avPos, 
											  const tWString& asText, eMainMenuState aNextState)
											: cMainMenuWidget(apInit,avPos,cVector2f(1,1))
{
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_menu_small.fnt",30);

	msText = asText;
	mvFontSize = 35;
	mbOver = false;

	mfOverTimer = 0;
	mfAlpha =0;

	mRect.w = mpFont->GetLength(mvFontSize,msText.c_str());
	mRect.h = mvFontSize.y +8;
	mRect.x = avPos.x - mRect.w/2;
	mRect.y = avPos.y+3;

	mNextState = aNextState;

	msTip = _W("");
}

cMainMenuWidget_MainButton::~cMainMenuWidget_MainButton()
{
	
}

void cMainMenuWidget_MainButton::OnUpdate(float afTimeStep)
{
	mfOverTimer += afTimeStep*1.3f;

	if(mbOver)
	{
		mfAlpha += 1.8f*afTimeStep;
		if(mfAlpha >1) mfAlpha =1;
	}
	else
	{
		mfAlpha -= 1.3f*afTimeStep;
		if(mfAlpha <0) mfAlpha =0;
	}
}

//-----------------------------------------------------------------------

void cMainMenuWidget_MainButton::OnMouseOver(bool abOver)
{
	mbOver = abOver;

	if(abOver){
		mpInit->mpMainMenu->SetButtonTip(msTip);
	}
}

//-----------------------------------------------------------------------

void cMainMenuWidget_MainButton::OnMouseDown(eMButton aButton)
{
	mpInit->mpMainMenu->SetState(mNextState);
	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);
}

//-----------------------------------------------------------------------

void cMainMenuWidget_MainButton::OnDraw()
{
	mpFont->Draw(mvWidgetPos,mvFontSize,cColor(0.62f + mfAlpha*0.3f,1),eFontAlign_Center,msText.c_str());

	float fAdd = sin(mfOverTimer) * 16.0f;

	if(mfAlpha >0)
	{
		mpFont->Draw(mvWidgetPos + cVector3f(fAdd,0,-1),mvFontSize,cColor(0.56f,0.35f * mfAlpha),eFontAlign_Center,msText.c_str());
		mpFont->Draw(mvWidgetPos + cVector3f(-fAdd,0,-1),mvFontSize,cColor(0.56f,0.35f * mfAlpha),eFontAlign_Center,msText.c_str());
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// BUTTON
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_Button::cMainMenuWidget_Button(cInit *apInit, const cVector3f &avPos, 
													   const tWString& asText, eMainMenuState aNextState,
													   cVector2f avFontSize, eFontAlign aAlignment)
													   : cMainMenuWidget(apInit,avPos,cVector2f(1,1))
{
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_menu_small.fnt",30);

	msText = asText;
	mvFontSize = avFontSize;
	mbOver = false;

	mAlignment = aAlignment;

	mfAlpha =0;
	mfOverTimer =0;

	mRect.w = mpFont->GetLength(mvFontSize,msText.c_str());
	mRect.h = mvFontSize.y +3;
	mRect.y = avPos.y+3;

	if(mAlignment == eFontAlign_Center)
	{
		mRect.x = avPos.x - mRect.w/2;
	}
	else if(mAlignment == eFontAlign_Left)
	{
		mRect.x = avPos.x;
	}
	else if(mAlignment == eFontAlign_Right)
	{
		mRect.x = avPos.x  - mRect.w;
	}

	mNextState = aNextState;
}

cMainMenuWidget_Button::~cMainMenuWidget_Button()
{

}

void cMainMenuWidget_Button::OnUpdate(float afTimeStep)
{
	if(mbOver)
	{
		mfAlpha += 1.8f*afTimeStep;
		if(mfAlpha >1) mfAlpha =1;
	}
	else
	{
		mfAlpha -= 1.3f*afTimeStep;
		if(mfAlpha <0) mfAlpha =0;
	}

	mfOverTimer += afTimeStep*0.4f;
}

//-----------------------------------------------------------------------

void cMainMenuWidget_Button::OnMouseOver(bool abOver)
{
	mbOver = abOver;

	if(mbOver){
		mpInit->mpMainMenu->SetButtonTip(msTip);
	}
}

//-----------------------------------------------------------------------

void cMainMenuWidget_Button::OnMouseDown(eMButton aButton)
{
	mpInit->mpMainMenu->SetState(mNextState);
	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);
}

//-----------------------------------------------------------------------

void cMainMenuWidget_Button::OnDraw()
{
	mpFont->Draw(mvWidgetPos,mvFontSize,cColor(0.62f ,1),mAlignment,msText.c_str());

	if(mfAlpha > 0)
	{
		float fX = 0.8f + sin(mfOverTimer)*0.2f;
		
		mpFont->Draw(mvWidgetPos+cVector3f(0,0,1),mvFontSize,cColor(0.9f,0.95f,1.0f,mfAlpha*fX),mAlignment,msText.c_str());
		mpFont->DrawEffect(eFontEffect_Glow,3,mvWidgetPos+cVector3f(0,0,-1),mvFontSize,cColor(0.1f,0.32f,1.0f,mfAlpha*fX),mAlignment,msText);
	}

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// TEXT
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_Text::cMainMenuWidget_Text(cInit *apInit, const cVector3f &avPos, const tWString& asText,
											cVector2f avFontSize, eFontAlign aAlignment,
											cMainMenuWidget *apExtra, float afMaxWidth)
: cMainMenuWidget(apInit,avPos,cVector2f(1,1))
{
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_menu_small.fnt",30);

	mfMaxWidth = afMaxWidth;

	msText = asText;
	mvFontSize = avFontSize;

	mAlignment = aAlignment;
	
	mRect.w = mpFont->GetLength(mvFontSize,msText.c_str());
	mRect.h = mvFontSize.y +3;
	mRect.y = avPos.y+3;

	if(mAlignment == eFontAlign_Center)
	{
		mRect.x = avPos.x - mRect.w/2;
	}
	else if(mAlignment == eFontAlign_Left)
	{
		mRect.x = avPos.x;
	}
	else if(mAlignment == eFontAlign_Right)
	{
		mRect.x = avPos.x  - mRect.w;
	}

	mpExtra = apExtra;

	mbOver = false;
}

cMainMenuWidget_Text::~cMainMenuWidget_Text()
{
	
}

//-----------------------------------------------------------------------

void cMainMenuWidget_Text::UpdateSize()
{
	mRect.w = mpFont->GetLength(mvFontSize,msText.c_str());
}

void cMainMenuWidget_Text::OnDraw()
{
	if(mfMaxWidth <=0)
		mpFont->Draw(mvWidgetPos,mvFontSize,cColor(0.9f,1),mAlignment,_W("%ls"),msText.c_str());
	else
		mpFont->DrawWordWrap(	mvWidgetPos,mfMaxWidth,mvFontSize.y+1,
								mvFontSize,cColor(0.9f,1),mAlignment,msText.c_str());
}

//-----------------------------------------------------------------------

void cMainMenuWidget_Text::OnMouseDown(eMButton aButton)
{
	if(mpExtra)
	{
		mpExtra->OnMouseDown(aButton);
	}
}

void cMainMenuWidget_Text::OnMouseOver(bool abOver)
{
	//if(abOver == mbOver) return;

	mbOver = abOver;

	if(mpExtra)
	{
		if(mpExtra->mbOver==false)
			mpExtra->OnMouseOver(abOver);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// IMAGE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_Image::cMainMenuWidget_Image(cInit *apInit, const cVector3f &avPos,const cVector2f& avSize,
											 const tString& asImageFile, const tString& asImageMat,
											 const cColor& aColor)
										   : cMainMenuWidget(apInit,avPos,avSize)
{
	mpImage = mpDrawer->CreateGfxObject(asImageFile,asImageMat);

	mColor = aColor;

	mvSize = avSize;
}

cMainMenuWidget_Image::~cMainMenuWidget_Image()
{

}

//-----------------------------------------------------------------------

void cMainMenuWidget_Image::OnDraw()
{
	mpDrawer->DrawGfxObject(mpImage,mvWidgetPos,mvSize,mColor);
}

//////////////////////////////////////////////////////////////////////////
// LIST
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_List::cMainMenuWidget_List(cInit *apInit, const cVector3f &avPos,const cVector2f &avSize,
					 cVector2f avFontSize)
 : cMainMenuWidget(apInit,avPos,avSize)
{
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_menu_small.fnt",30);

	mpBackGfx = mpDrawer->CreateGfxObject("effect_white.jpg","diffalpha2d");

	mpDownGfx = mpDrawer->CreateGfxObject("menu_list_down.bmp","diffalpha2d");
	mpUpGfx = mpDrawer->CreateGfxObject("menu_list_up.bmp","diffalpha2d");
	mpSlideGfx = mpDrawer->CreateGfxObject("menu_list_slide.bmp","diffalpha2d");
	mpBorderLeftGfx = mpDrawer->CreateGfxObject("menu_list_border_left.bmp","diffalpha2d");
	mpBorderTopGfx = mpDrawer->CreateGfxObject("menu_list_border_top.bmp","diffalpha2d");
	mpBorderBottomGfx = mpDrawer->CreateGfxObject("menu_list_border_bottom.bmp","diffalpha2d");
	mpSlideButtonGfx = mpDrawer->CreateGfxObject("menu_list_slider_button.bmp","diffalpha2d");
	
	mvFontSize = avFontSize;

	mRect.x = avPos.x;
	mRect.y = avPos.y;

	mRect.w = avSize.x;
	mRect.h = avSize.y;

	mlMaxRows = (int)floor((avSize.y-6) / (avFontSize.y +2));

	mvPosition = avPos;
	mvSize = avSize;

	mlSelected = -1;

	mlFirstRow =0;

	mbSlideButtonPressed = false;
	mfSlideButtonMove =0;

	mvLastMousePos = mpInit->mpMainMenu->GetMousePos();
}
cMainMenuWidget_List::~cMainMenuWidget_List()
{

}

//-----------------------------------------------------------------------

void cMainMenuWidget_List::OnUpdate(float afTimeStep)
{
	////////////////////////////////
	//Slide button

	//Size
	float fT = (float)mlMaxRows / (float)mvEntries.size();
	if(fT > 1) fT = 1;
	mfSlideButtonSize = (mvSize.y - 28) * fT;

	//Pos
	mfSlideButtonPos =0;
	if((int)mvEntries.size() > mlMaxRows)
	{
		mfSlideButtonPos = ((mvSize.y - 28) - mfSlideButtonSize) *
			(float) mlFirstRow / (float)(mvEntries.size() - mlMaxRows);
	}

	/////////////////////////////////
	//Slide button pressed
	if(mbSlideButtonPressed && (int)mvEntries.size() > mlMaxRows)
	{
		float fMinStep = ((mvSize.y - 28) - mfSlideButtonSize) / (float)mvEntries.size();
		cVector2f vRelMouse = mpInit->mpMainMenu->GetMousePos() - mvLastMousePos;

		mfSlideButtonMove += vRelMouse.y;
		
		
		while(mfSlideButtonMove <= -fMinStep && mlFirstRow >0)
		{
			mlFirstRow--;
			mfSlideButtonMove += fMinStep;
		}
		while(mfSlideButtonMove >= fMinStep && mlFirstRow < (int)mvEntries.size() - mlMaxRows)
		{
			mlFirstRow++;
			mfSlideButtonMove -= fMinStep;
		}
	}

	mvLastMousePos = mpInit->mpMainMenu->GetMousePos();
}

//-----------------------------------------------------------------------

void cMainMenuWidget_List::OnMouseOver(bool abOver)
{
	if(abOver==false){
		mbSlideButtonPressed = false;
		mfSlideButtonMove =0;
	}
}

//-----------------------------------------------------------------------

void cMainMenuWidget_List::OnDraw()
{
	mpDrawer->DrawGfxObject(mpBackGfx,mvWidgetPos-cVector3f(0,0,1),mvSize,cColor(0.05f,0.05f,0.1f,1));
	
	//Up
	mpDrawer->DrawGfxObject(mpUpGfx,
							cVector3f(mvWidgetPos.x + mvSize.x - 14,mvWidgetPos.y,mvWidgetPos.z+1),
							cVector2f(14,14), cColor(1,1));
	//Down
	mpDrawer->DrawGfxObject(mpDownGfx,
							cVector3f(mvWidgetPos.x + mvSize.x - 14,mvWidgetPos.y + mvSize.y - 14,mvWidgetPos.z+1),
							cVector2f(14,14), cColor(1,1));
	//Slide
	mpDrawer->DrawGfxObject(mpSlideGfx,
							cVector3f(mvWidgetPos.x + mvSize.x - 14,mvWidgetPos.y + 14,mvWidgetPos.z+1),
							cVector2f(14,mvSize.y-28), cColor(1,1));

	//Border Top
	mpDrawer->DrawGfxObject(mpBorderTopGfx,
							cVector3f(mvWidgetPos.x + 3,mvWidgetPos.y,mvWidgetPos.z+1),
							cVector2f(mvSize.x-17,3), cColor(1,1));
	//Border Bottom
	mpDrawer->DrawGfxObject(mpBorderBottomGfx,
							cVector3f(mvWidgetPos.x + 3,mvWidgetPos.y+mvSize.y - 3,mvWidgetPos.z+1),
							cVector2f(mvSize.x-17,3), cColor(1,1));

	//Border Left
	mpDrawer->DrawGfxObject(mpBorderLeftGfx,
							cVector3f(mvWidgetPos.x,mvWidgetPos.y,mvWidgetPos.z+1),
							cVector2f(3,mvSize.y), cColor(1,1));

	//Slider Button
	cVector3f vButtonStart = cVector3f(mvWidgetPos.x + mvSize.x - 14,mvWidgetPos.y+14,mvWidgetPos.z+2);
	mpDrawer->DrawGfxObject(mpSlideButtonGfx,vButtonStart + cVector3f(0,mfSlideButtonPos,0),
								cVector2f(14,mfSlideButtonSize), cColor(1,1));
	
	
	cVector3f vPos = mvWidgetPos + cVector3f(5,3,0);

	for(size_t i=mlFirstRow; i< mvEntries.size(); ++i)
	{
		if((int)i-mlFirstRow >= mlMaxRows) break;

		if(mlSelected == i)
		{
			mpFont->Draw(vPos,mvFontSize,cColor(0.95f,1),eFontAlign_Left,mvEntries[i].c_str());
			mpDrawer->DrawGfxObject(mpBackGfx,vPos+cVector3f(0,2,-1),
									cVector2f(mvSize.x-5, mvFontSize.y), 
									cColor(0.0f,0.0f,0.73f,1));
		}
		else
			mpFont->Draw(vPos,mvFontSize,cColor(0.7f,1),eFontAlign_Left,mvEntries[i].c_str());

		vPos.y += mvFontSize.y+2;
	}
}

//-----------------------------------------------------------------------

void cMainMenuWidget_List::OnMouseDown(eMButton aButton)
{
	cVector2f vLocalMouse = mpInit->mpMainMenu->GetMousePos() - 
							cVector2f(mvWidgetPos.x,mvWidgetPos.y);

	//Scrollbar
	if(vLocalMouse.x > mvSize.x - 14)
	{
		//Up Arrow
		if(vLocalMouse.y <= 14)
		{
			if(mlFirstRow>0) mlFirstRow--;
		}
		//Down Arrow
		else if(vLocalMouse.y >= mvSize.y - 14)
		{
			if(mlFirstRow < (int)mvEntries.size() - mlMaxRows) mlFirstRow++;
		}
		//Press slide button
		else if(vLocalMouse.y >= mfSlideButtonPos && 
				vLocalMouse.y <= mfSlideButtonPos + mfSlideButtonSize) 
		{
			mbSlideButtonPressed = true;
		}
	}
	//Entries
	else
	{
		int lSelected = mlFirstRow + (int)floor((vLocalMouse.y-3) / (mvFontSize.y+2));
		if(lSelected < (int)mvEntries.size()) mlSelected = lSelected;
	}
}

void cMainMenuWidget_List::OnMouseUp(eMButton aButton)
{
	mbSlideButtonPressed = false;
	mfSlideButtonMove =0;
}

//-----------------------------------------------------------------------

void cMainMenuWidget_List::AddEntry(const tWString &asText)
{
	mvEntries.push_back(asText);
	if(mlSelected==-1 && mvEntries.size()==1) mlSelected =0;
}

//-----------------------------------------------------------------------

const tWString& cMainMenuWidget_List::GetSelectedEntry()
{
	return mvEntries[0];
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// NEW GAME
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_NewGame::cMainMenuWidget_NewGame(cInit *apInit, const cVector3f &avPos, 
											   const tWString& asText,
											   cVector2f avFontSize, eFontAlign aAlignment,
											   eGameDifficulty aDifficulty)
					: cMainMenuWidget_Button(apInit,avPos,asText,
											eMainMenuState_LastEnum,avFontSize,aAlignment)
{
	mDifficulty = aDifficulty;

	switch(mDifficulty)
	{
	case eGameDifficulty_Easy:	msTip = kTranslate("MainMenu", "TipDifficultyEasy");
								break;
	case eGameDifficulty_Normal:msTip = kTranslate("MainMenu", "TipDifficultyNormal");
								break;
	case eGameDifficulty_Hard:	msTip = kTranslate("MainMenu", "TipDifficultyHard");
								break;
	}
	// Requiem's button is the "Yes" to starting
	if(cInit::GetGameId() == eGameId_Requiem) msTip = _W("");
}
//-----------------------------------------------------------------------

void cMainMenuWidget_NewGame::OnMouseDown(eMButton aButton)
{
	mpInit->mpGraphicsHelper->DrawLoadingScreen("");
	
	mpInit->mpMainMenu->SetActive(false);
	mpInit->ResetGame(true);

	mpInit->mDifficulty = mDifficulty;
	
	// Requiem starts in its first map
	if(mpInit->mbShowIntro && cInit::GetGameId() != eGameId_Requiem)
	{
		mpInit->mpIntroStory->SetActive(true);
	}
	else
	{
		mpInit->mpGame->GetUpdater()->SetContainer("Default");
		mpInit->mpGame->GetScene()->SetDrawScene(true);
		
		mpInit->mpMapHandler->Load(	mpInit->msStartMap,mpInit->msStartLink);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// TUTORIAL (Black Plague)
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

// "Yes" to playing the tutorial: its map, without the game's init script
class cMainMenuWidget_AskTutYes : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_AskTutYes(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment) {}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);

		mpInit->mpMainMenu->SetActive(false);
		mpInit->mpGraphicsHelper->DrawLoadingScreen("");
		mpInit->ResetGame(false);

		mpInit->mpGame->GetUpdater()->SetContainer("Default");
		mpInit->mpGame->GetScene()->SetDrawScene(true);
		mpInit->mpMapHandler->Load("level00_tutorial.dae","link01");

		mpInit->mbAskTutorial = false;
	}
};

// "No": on to choosing the difficulty, and not asked again
class cMainMenuWidget_AskTutNo : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_AskTutNo(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_NewGame,avFontSize,aAlignment) {}

	void OnMouseDown(eMButton aButton) override
	{
		cMainMenuWidget_Button::OnMouseDown(aButton);
		mpInit->mbAskTutorial = false;
	}
};

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONTINUE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_Continue::cMainMenuWidget_Continue(cInit *apInit, const cVector3f &avPos, 
												 const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
												 : cMainMenuWidget_Button(apInit,avPos,asText,
												 eMainMenuState_LastEnum,avFontSize,aAlignment)
{

}
//-----------------------------------------------------------------------

void cMainMenuWidget_Continue::OnMouseDown(eMButton aButton)
{
	//mpInit->mpGraphicsHelper->DrawLoadingScreen("other_loading.jpg");
	
	mpInit->mpMainMenu->SetActive(false);

	mpInit->mpSaveHandler->AutoLoadLatestAutoOrSpot();
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// QUIT
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_Quit::cMainMenuWidget_Quit(cInit *apInit, const cVector3f &avPos, 
												 const tWString& asText,
												 cVector2f avFontSize, eFontAlign aAlignment)
												 : cMainMenuWidget_Button(apInit,avPos,
												 asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
{

}
//-----------------------------------------------------------------------

void cMainMenuWidget_Quit::OnMouseDown(eMButton aButton)
{
	mpInit->mpGame->Exit();
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// RESUME
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenuWidget_Resume::cMainMenuWidget_Resume(cInit *apInit, const cVector3f &avPos, 
										   const tWString& asText)
										   : cMainMenuWidget_MainButton(apInit,avPos,asText,eMainMenuState_LastEnum)
{

}
//-----------------------------------------------------------------------

void cMainMenuWidget_Resume::OnMouseDown(eMButton aButton)
{
	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);

	mpInit->mpMainMenu->SetActive(false);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// LOADGAME
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

tWStringVec gvSaveGameFileVec[3];

class cMainMenuWidget_SaveGameList : public cMainMenuWidget_List
{
public:
	cMainMenuWidget_SaveGameList(cInit *apInit, const cVector3f &avPos,const cVector2f &avSize,
									cVector2f avFontSize,tWString asDir, int alNum)
									: cMainMenuWidget_List(apInit,avPos, avSize, avFontSize)
	{
		msDir = asDir;
		mlNum = alNum;
	}
	
	void OnDoubleClick(eMButton aButton) override
	{
		if(mlSelected <0) return;
		
		tWString sFile = msDir + _W("/") + gvSaveGameFileVec[mlNum][mlSelected];

		mpInit->mpMainMenu->SetActive(false);
		mpInit->ResetGame(true);

		mpInit->mpSaveHandler->LoadGameFromFile(sFile);
	}

private:
	tWString msDir;
	int mlNum;
};

cMainMenuWidget_SaveGameList *gpSaveGameList[3]= {nullptr, nullptr, nullptr};

class cMainMenuWidget_LoadSaveGame : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_LoadSaveGame(cInit *apInit, const cVector3f &avPos, const tWString& asText,
								cVector2f avFontSize, eFontAlign aAlignment, 
								tWString asDir,int alNum)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msDir = asDir;
		mlNum = alNum;
	}

	void OnMouseDown(eMButton aButton) override
	{
		int lSelected = gpSaveGameList[mlNum]->GetSelectedIndex();
		if(lSelected <0) return;

		tWString sFile = msDir + _W("/") + gvSaveGameFileVec[mlNum][lSelected];

		mpInit->mpMainMenu->SetActive(false);
		mpInit->ResetGame(true);

		mpInit->mpSaveHandler->LoadGameFromFile(sFile);
	}

	tWString msDir;
	int mlNum;
};

class cMainMenuWidget_RemoveSaveGame : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_RemoveSaveGame(cInit *apInit, const cVector3f &avPos, const tWString& asText,
		cVector2f avFontSize, eFontAlign aAlignment, 
		tWString asDir,int alNum)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msDir = asDir;
		mlNum = alNum;
	}

	void OnMouseDown(eMButton aButton) override
	{
		int lSelected = gpSaveGameList[mlNum]->GetSelectedIndex();
		if(lSelected <0) return;

		tWString sFile =	mpInit->mpSaveHandler->GetSaveDir() + msDir + 
							_W("/") + gvSaveGameFileVec[mlNum][lSelected];

		RemoveFile(sFile);
		mpInit->mpMainMenu->UpdateWidgets();
	}

	tWString msDir;
	int mlNum;
};

class cMainMenuWidget_FavoriteSaveGame : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_FavoriteSaveGame(cInit *apInit, const cVector3f &avPos, const tWString& asText,
		cVector2f avFontSize, eFontAlign aAlignment, 
		tWString asDir,int alNum)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msDir = asDir;
		mlNum = alNum;
	}

	void OnMouseDown(eMButton aButton) override
	{
		int lSelected = gpSaveGameList[mlNum]->GetSelectedIndex();
		if(lSelected <0) return;

		tWString sFile =	mpInit->mpSaveHandler->GetSaveDir() + msDir + 
							_W("/") + gvSaveGameFileVec[mlNum][lSelected];

		tWString sDest =	mpInit->mpSaveHandler->GetSaveDir() + _W("save/favorite/") + 
							gvSaveGameFileVec[mlNum][lSelected];

		CloneFile(sFile,sDest,true);
		mpInit->mpMainMenu->UpdateWidgets();
	}

	tWString msDir;
	int mlNum;
};

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// OPTIONS CONTROLS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------


cMainMenuWidget_Text *gpInvertMouseYText=nullptr;
cMainMenuWidget_Text *gpMouseSensitivityText=nullptr;
cMainMenuWidget_Text *gpToggleCrouchText=nullptr;
cMainMenuWidget_Text *gpUseHapticsText=nullptr;
cMainMenuWidget_Text *gpWidgetInteractModeCameraSpeedText = nullptr;
cMainMenuWidget_Text *gpWidgetActionModeCameraSpeedText = nullptr;
cMainMenuWidget_Text *gpWidgetWeightForceScaleText = nullptr;


//-----------------------------------------------------------------------

class cMainMenuWidget_UseHaptics : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_UseHaptics(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment){msTip = _W("");}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbHasHapticsOnRestart = !mpInit->mbHasHapticsOnRestart;

		gpUseHapticsText->msText = mpInit->mbHasHapticsOnRestart ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
		gbMustRestart = true;
	}
};

//-----------------------------------------------------------------------

class cMainMenuWidget_WeightForceScale : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_WeightForceScale(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment){msTip = _W("");}

		void OnMouseDown(eMButton aButton) override
		{
			float fForceScale = mpInit->mfHapticForceMul;
			if(aButton == eMButton_Left)
			{
				fForceScale += 0.1f;
				if(fForceScale>3.0f) fForceScale = 3.0f;
			}
			else if(aButton == eMButton_Right)
			{
				fForceScale -= 0.1f;
				if(fForceScale<0.0f) fForceScale = 0.0f;
			}

			char sTempVec[256];
			sprintf(sTempVec,"%.1f",fForceScale);
			gpWidgetWeightForceScaleText->msText = cString::To16Char(sTempVec);

			mpInit->mfHapticForceMul = fForceScale;
		}
};

//-----------------------------------------------------------------------

class cMainMenuWidget_InteractModeCameraSpeed : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_InteractModeCameraSpeed(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment){msTip = _W("");}

		void OnMouseDown(eMButton aButton) override
		{
			float fCameraSpeed = mpInit->mpPlayer->GetHapticCamera()->GetInteractModeCameraSpeed();
			if(aButton == eMButton_Left)
			{
				fCameraSpeed += 0.1f;
				if(fCameraSpeed>3.0f) fCameraSpeed = 3.0f;
			}
			else if(aButton == eMButton_Right)
			{
				fCameraSpeed -= 0.1f;
				if(fCameraSpeed<0.1f) fCameraSpeed = 0.1f;
			}

			char sTempVec[256];
			sprintf(sTempVec,"%.1f",fCameraSpeed);
			gpWidgetInteractModeCameraSpeedText->msText = cString::To16Char(sTempVec);

			mpInit->mpPlayer->GetHapticCamera()->SetInteractModeCameraSpeed(fCameraSpeed);
		}
};

//-----------------------------------------------------------------------

class cMainMenuWidget_ActionModeCameraSpeed : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_ActionModeCameraSpeed(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment){msTip = _W("");}

		void OnMouseDown(eMButton aButton) override
		{
			float fCameraSpeed = mpInit->mpPlayer->GetHapticCamera()->GetActionModeCameraSpeed();
			if(aButton == eMButton_Left)
			{
				fCameraSpeed += 0.1f;
				if(fCameraSpeed>3.0f) fCameraSpeed = 3.0f;
			}
			else if(aButton == eMButton_Right)
			{
				fCameraSpeed -= 0.1f;
				if(fCameraSpeed<0.1f) fCameraSpeed = 0.1f;
			}

			char sTempVec[256];
			sprintf(sTempVec,"%.1f",fCameraSpeed);
			gpWidgetActionModeCameraSpeedText->msText = cString::To16Char(sTempVec);

			mpInit->mpPlayer->GetHapticCamera()->SetActionModeCameraSpeed(fCameraSpeed);
		}
};


//-----------------------------------------------------------------------

class cMainMenuWidget_InvertMouseY : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_InvertMouseY(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
								: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipControlsInvertMouseY");
	}
	
	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mpButtonHandler->mbInvertMouseY = !mpInit->mpButtonHandler->mbInvertMouseY;
		gpInvertMouseYText->msText = mpInit->mpButtonHandler->mbInvertMouseY ? kTranslate("MainMenu","On") : 
																				kTranslate("MainMenu","Off");
	}
};

//------------------------------------------------------------

class cMainMenuWidget_MouseSensitivity : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_MouseSensitivity(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipControlsMouseSensitivity");
	}

	void OnMouseDown(eMButton aButton) override
	{
		if(aButton == eMButton_Left)
		{
			mpInit->mpButtonHandler->mfMouseSensitivity += 0.2f;
			if(mpInit->mpButtonHandler->mfMouseSensitivity>5.0f) 
				mpInit->mpButtonHandler->mfMouseSensitivity= 5.0f;
		}
		else if(aButton == eMButton_Right)
		{
			mpInit->mpButtonHandler->mfMouseSensitivity -= 0.2f;
			if(mpInit->mpButtonHandler->mfMouseSensitivity<0.2f) 
				mpInit->mpButtonHandler->mfMouseSensitivity= 0.2f;
		}

		char sTempVec[256];
		sprintf(sTempVec,"%.1f",mpInit->mpButtonHandler->mfMouseSensitivity);
		gpMouseSensitivityText->msText = cString::To16Char(sTempVec);
	}
};

//------------------------------------------------------------

class cMainMenuWidget_ToggleCrouch : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_ToggleCrouch(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipControlsToggleCrouch");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mpButtonHandler->mbToggleCrouch = !mpInit->mpButtonHandler->mbToggleCrouch;
		gpToggleCrouchText->msText = mpInit->mpButtonHandler->mbToggleCrouch ? kTranslate("MainMenu","On")
																			: kTranslate("MainMenu","Off");
	}
};

//////////////////////////////////////////////////////////////////////////
// OPTIONS SOUND
//////////////////////////////////////////////////////////////////////////

cMainMenuWidget_Text *gpSoundVolumeText=nullptr;
cMainMenuWidget_Text *gpSoundHardwareText=nullptr;
cMainMenuWidget_Text *gpSoundOutputDevice=nullptr;

class cMainMenuWidget_SoundVolume : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_SoundVolume(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipSoundVolume");
	}

	void OnMouseDown(eMButton aButton) override
	{
		float fVolume = mpInit->mpGame->GetSound()->GetLowLevel()->GetVolume();

		if(aButton  == eMButton_Left)
		{
			fVolume += 0.1f;
			if(fVolume > 1.0f) fVolume = 1.0f;
		}
		else if(aButton  == eMButton_Right)
		{
			fVolume -= 0.1f;
			if(fVolume < 0.0f) fVolume = 0.0f;
		}

		mpInit->mpGame->GetSound()->GetLowLevel()->SetVolume(fVolume);

		char sTempVec[256];
		sprintf(sTempVec,"%.0f",mpInit->mpGame->GetSound()->GetLowLevel()->GetVolume()*100);
		gpSoundVolumeText->msText = cString::To16Char(sTempVec);
	}
};

class cMainMenuWidget_SoundHardware : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_SoundHardware(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipSoundHardware");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbUseSoundHardware = !mpInit->mbUseSoundHardware;

		gpSoundHardwareText->msText = mpInit->mbUseSoundHardware ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");

		gbMustRestart = true;
	}
};

// Black Plague: reverb and echo of the rooms (EFX). The label and tip are
// not in the language files; the game has them in English only.
cMainMenuWidget_Text *gpSoundEFXText=nullptr;

class cMainMenuWidget_SoundEFX : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_SoundEFX(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = _W("Enables environmental effects such as reverb and echo. Requires restart.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbUseEnvAudio = !mpInit->mbUseEnvAudio;

		gpSoundEFXText->msText = mpInit->mbUseEnvAudio ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");

		gbMustRestart = true;
	}
};

class cMainMenuWidget_SoundOutputDevice : public cMainMenuWidget_Button
{
public:
	tStringVec mlDevices;

	cMainMenuWidget_SoundOutputDevice(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipSoundOutputDevice");
		mlDevices = OAL_Info_GetOutputDevices();
	}

	void OnMouseDown(eMButton aButton) override
	{
		int lCurrentNum = 0;

		//get current num
        for(int i=0; i<mlDevices.size(); ++i)
		{
			if(mlDevices[i] == mpInit->msDeviceName)
			{
				lCurrentNum = i;
				break;
			}
		}

		if(aButton == eMButton_Left)
		{
			lCurrentNum++;
			if(lCurrentNum >= mlDevices.size()) lCurrentNum =0;
		}
		else if(aButton == eMButton_Right)
		{
			lCurrentNum--;
			if(lCurrentNum < 0) lCurrentNum =mlDevices.size()-1;
		}

		mpInit->msDeviceName = mlDevices[lCurrentNum];

		gpSoundOutputDevice->msText = cString::To16Char(mpInit->msDeviceName);

		gbMustRestart = true;
	}
};

//////////////////////////////////////////////////////////////////////////
// OPTIONS GAME
//////////////////////////////////////////////////////////////////////////

// Mutually exclusive choices. Refresh on draw as F2 can also change the mode.
class cMainMenuWidget_DogAI : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_DogAI(cInit *init, const cVector3f &pos, bool improved)
		: cMainMenuWidget_Button(init,pos,improved ? _W("[ ] Improved") : _W("[ ] Original"),
			eMainMenuState_LastEnum,22,eFontAlign_Center),
		  mbImproved(improved)
	{
		msTip = improved ? _W("Remember sightings and sounds; smoother routes and search.")
			: _W("Original Dog pursuit and search decisions, with shared stability fixes.");
	}
	void OnDraw() override
	{
		msText = mpInit->GetImprovedDogAI() == mbImproved ? _W("[x] ") : _W("[ ] ");
		msText += mbImproved ? _W("Improved") : _W("Original");
		cMainMenuWidget_Button::OnDraw();
	}
	void OnMouseDown(eMButton button) override
	{
		if(button == eMButton_Left) mpInit->SetImprovedDogAI(mbImproved);
	}
private:
	bool mbImproved;
};

// Starts another of the three games: the executable is run again in its folder
class cMainMenuWidget_ChangeGame : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_ChangeGame(cInit *init, const cVector3f &pos, eGameId game)
		: cMainMenuWidget_Button(init,pos,cString::To16Char(cInit::GetGameName(game)),
			eMainMenuState_LastEnum,22,eFontAlign_Center),
		  mGame(game)
	{
	}
	void OnMouseDown(eMButton button) override
	{
		if(button == eMButton_Left) mpInit->RequestGameChange(mGame);
	}
private:
	eGameId mGame;
};

cMainMenuWidget_Text *gpLanguageText=nullptr;
cMainMenuWidget_Text *gpSubtitlesText=nullptr;

cMainMenuWidget_Text *gpSimpleSwingText=nullptr;
cMainMenuWidget_Text *gpAllowQuickSaveText=nullptr;
cMainMenuWidget_Text *gpDisablePersonalText=nullptr;
cMainMenuWidget_Text *gpDifficultyText=nullptr;
cMainMenuWidget_Text *gpFlashItemsText=nullptr;
cMainMenuWidget_Text *gpShowCrossHairText=nullptr;


class cMainMenuWidget_ShowCrossHair : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_ShowCrossHair(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGameShowCrossHair");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbShowCrossHair = !mpInit->mbShowCrossHair;

		gpShowCrossHairText->msText = mpInit->mbShowCrossHair ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

class cMainMenuWidget_FlashItems : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_FlashItems(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGameFlashText");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbFlashItems = !mpInit->mbFlashItems;

		gpFlashItemsText->msText = mpInit->mbFlashItems ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};


tString gvDifficultyLevel[] = {"Easy","Normal","Hard"};
int glDifficultyLevelNum = 3;

class cMainMenuWidget_Difficulty : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Difficulty(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGameDifficulty");
	}

	void OnMouseDown(eMButton aButton) override
	{
		int lCurrent = (int)mpInit->mDifficulty;
		if(aButton == eMButton_Left)
		{
			lCurrent++;
			if(lCurrent >= glDifficultyLevelNum) lCurrent =0;
		}
		else if(aButton == eMButton_Right)
		{
			lCurrent--;
			if(lCurrent < 0) lCurrent =glDifficultyLevelNum-1;
		}

		gpDifficultyText->msText = kTranslate("MainMenu",gvDifficultyLevel[lCurrent]);
		mpInit->mDifficulty = (eGameDifficulty)lCurrent;
	}
};

class cMainMenuWidget_SimpleSwing : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_SimpleSwing(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbSimpleWeaponSwing = !mpInit->mbSimpleWeaponSwing;
		
		gpSimpleSwingText->msText = mpInit->mbSimpleWeaponSwing ?
									kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

class cMainMenuWidget_AllowQuickSave : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_AllowQuickSave(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbAllowQuickSave = !mpInit->mbAllowQuickSave;

		gpAllowQuickSaveText->msText = mpInit->mbAllowQuickSave ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

class cMainMenuWidget_DisablePersonal : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_DisablePersonal(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGameDisablePersonal");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbDisablePersonalNotes = !mpInit->mbDisablePersonalNotes;

		gpDisablePersonalText->msText = mpInit->mbDisablePersonalNotes ?
			kTranslate("MainMenu","Off") : kTranslate("MainMenu","On");
	}
};

class cMainMenuWidget_Subtitles : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Subtitles(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGameSubtitles");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbSubtitles = !mpInit->mbSubtitles;

		gpSubtitlesText->msText = mpInit->mbSubtitles ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

class cMainMenuWidget_Language : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Language(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		tWStringList lstStrings;
		apInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstStrings,_W("config/"),_W("*.lang"));

		mlCurrentFile =0;
		int lIdx=0;
		for(tWStringListIt it = lstStrings.begin(); it != lstStrings.end();++it)
		{
			mvFiles.push_back(*it);
			if(	cString::To16Char(cString::ToLowerCase(apInit->msLanguageFile)) == 
				cString::ToLowerCaseW(*it))
			{
				mlCurrentFile = lIdx;
			}

			++lIdx;
		}

		msTip = kTranslate("MainMenu", "TipGameLanguage");
	}

	void OnMouseDown(eMButton aButton) override
	{
		if(aButton  == eMButton_Left)
		{
			mlCurrentFile++;
			if(mlCurrentFile >= (int)mvFiles.size()) mlCurrentFile=0;
		}
		else if(aButton  == eMButton_Right)
		{
			mlCurrentFile--;
			if(mlCurrentFile < 0) mlCurrentFile= (int)mvFiles.size()-1;
		}

		gpLanguageText->msText = cString::SetFileExtW(mvFiles[mlCurrentFile],_W(""));
		mpInit->msLanguageFile = cString::To8Char(mvFiles[mlCurrentFile]);

		if(mpInit->mpMapHandler->GetCurrentMapName() != "")
		{
			gbMustRestart = true;
		}
		else
		{
			mpInit->mpGame->GetResources()->ClearResourceDirs();
			mpInit->mpGame->GetResources()->AddResourceDir("core/programs");
			mpInit->mpGame->GetResources()->AddResourceDir("core/textures");
			mpInit->mpGame->GetResources()->LoadResourceDirsFile("resources.cfg");
			
			mpInit->mpGame->GetResources()->SetLanguageFile(mpInit->msLanguageFile);

			mpInit->mpMainMenu->UpdateWidgets();
		}
	}

	std::vector<tWString> mvFiles;
	int mlCurrentFile;
};

//------------------------------------------------------------






//------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// OPTIONS GRAPHICS
//////////////////////////////////////////////////////////////////////////

cMainMenuWidget_Text *gpResolutionText=nullptr;
cMainMenuWidget_Text *gpFullscreenText=nullptr;
cMainMenuWidget_Text *gpDisplayText=nullptr;
cMainMenuWidget_Text *gpPostEffectsText=nullptr;
cMainMenuWidget_Text *gpBloomText=nullptr;
cMainMenuWidget_Text *gpMotionBlurText=nullptr;
cMainMenuWidget_Text *gpVSyncText=nullptr;
cMainMenuWidget_Text *gpLimitFPSText=nullptr;
cMainMenuWidget_Text *gpTextureQualityText=nullptr;
cMainMenuWidget_Text *gpShadowsText=nullptr;
cMainMenuWidget_Text *gpTextureFilterText=nullptr;
cMainMenuWidget_Text *gpSmaaText=nullptr;
cMainMenuWidget_Text *gpGammaText=nullptr;
cMainMenuWidget_Text *gpGammaText2=nullptr;
cMainMenuWidget_Text *gpFSAAText=nullptr;
cMainMenuWidget_Text *gpRenderScaleText=nullptr;
cMainMenuWidget_Text *gpAmbientOcclusionText=nullptr;
cMainMenuWidget_Text *gpDoFText=nullptr;

cVector2l gvResolutions[] = {cVector2l(640,480), cVector2l(800,600), cVector2l(1024, 768),
							cVector2l(1152,864),cVector2l(1280,720),cVector2l(1280,768),
							cVector2l(1280,800),cVector2l(1280,960),cVector2l(1280,1024),
							cVector2l(1360,768),cVector2l(1360,1024),cVector2l(1400,1050),
							cVector2l(1440,900),cVector2l(1680,1050),cVector2l(1600,1200), 
							cVector2l(1920,1080),cVector2l(1920,1200)
						};
int glResolutionNum = 17;

tString gvTextureQuality[] = {"High","Medium","Low"};
int glTextureQualityNum = 3;


cMainMenuWidget_Text *gpFOVText=nullptr;

cMainMenuWidget_Text *gpNoiseFilterText=nullptr;

//------------------------------------------------------------

class cMainMenuWidget_Gamma : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Gamma(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment,
							int alGNum)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		mfMax = 3.0f;
		mfMin = 0.1f;
		mfStep = 0.1f;
		
		mlGNum = alGNum;

		msTip = kTranslate("MainMenu", "TipGraphicsGamma");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mfGamma = mpInit->mpGame->GetGraphics()->GetLowLevel()->GetGammaCorrection();

		if(aButton == eMButton_Left)
		{
			mfGamma += mfStep;
			if(mfGamma >mfMax) mfGamma = mfMax;
		}
		else if(aButton == eMButton_Right)
		{
			mfGamma -= mfStep;
			if(mfGamma < mfMin) mfGamma = mfMin;
		}

		mpInit->mpGame->GetGraphics()->GetLowLevel()->SetGammaCorrection(mfGamma);
		
		char sTempVec[256];
		sprintf(sTempVec,"%.1f",mfGamma);
		
		gpGammaText->msText = cString::To16Char(sTempVec);
		if(mlGNum == 1)
			gpGammaText2->msText = cString::To16Char(sTempVec);
	}

	float mfGamma;
	float mfMax;
	float mfMin;
	float mfStep;
	int mlGNum;
};

//------------------------------------------------------------


// Off, the original noise tiles, or the grain made by a shader: plain, or as
// film has it. The last three names are not in the game's language files.
static tWString GetNoiseFilterText(cInit *mpInit)
{
	cPlayerNoiseFilter *pFilter = mpInit->mpPlayer->GetNoiseFilter();
	if(pFilter->IsActive()==false) return kTranslate("MainMenu","Off");
	static const tWString vNames[eNoiseFilterType_LastEnum] = {_W("Original"), _W("Grain"), _W("Film Grain")};
	return vNames[pFilter->GetType()];
}

class cMainMenuWidget_NoiseFilter : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_NoiseFilter(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsNoiseFilter");
	}

	void OnMouseDown(eMButton aButton) override
	{
		cPlayerNoiseFilter *pFilter = mpInit->mpPlayer->GetNoiseFilter();

		// Forwards with the left button, back with the right
		int lIdx = pFilter->IsActive() ? 1 + (int)pFilter->GetType() : 0;
		int lNum = 1 + eNoiseFilterType_LastEnum;
		lIdx = (lIdx + (aButton == eMButton_Right ? lNum - 1 : 1)) % lNum;

		pFilter->SetActive(lIdx > 0);
		if(lIdx > 0) pFilter->SetType((eNoiseFilterType)(lIdx - 1));

		gpNoiseFilterText->msText = GetNoiseFilterText(mpInit);
	}
};

//------------------------------------------------------------

// In the order of eRendererShowShadows
static tWString GetShadowOptionText(cInit *mpInit, int alOption)
{
	switch(alOption)
	{
	case 0: return kTranslate("MainMenu","On");
	case 1: return kTranslate("MainMenu","Only Static");
	}
	return kTranslate("MainMenu","Off");
}

class cMainMenuWidget_Shadows : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Shadows(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsShadows");
	}

	void OnMouseDown(eMButton aButton) override
	{
		// The F2 panel changes this too
		cRenderer3D *pRenderer = mpInit->mpGame->GetGraphics()->GetRenderer3D();
		const int lNum = eRendererShowShadows_LastEnum;
		int lCurrent = (int)pRenderer->GetShowShadows();
		if(aButton == eMButton_Left)		lCurrent = (lCurrent + 1) % lNum;
		else if(aButton == eMButton_Right)	lCurrent = (lCurrent + lNum - 1) % lNum;
		pRenderer->SetShowShadows((eRendererShowShadows)lCurrent);

		gpShadowsText->msText = GetShadowOptionText(mpInit, lCurrent);
	}
};

//------------------------------------------------------------

// One option for how textures are filtered: trilinear, or that with 2 to
// 16 samples along surfaces seen at a slant, as far as the graphics card
// goes. Bilinear is only shown when the settings file asks for it. The
// last names are not in the game's language files.
static tWString GetTextureFilterText(cInit *mpInit)
{
	cMaterialManager *pMaterialManager = mpInit->mpGame->GetResources()->GetMaterialManager();
	int lAnisotropy = (int)pMaterialManager->GetTextureAnisotropy();
	if(lAnisotropy > 1) return cString::To16Char("Anisotropic " + cString::ToString(lAnisotropy) + "x");
	return kTranslate("MainMenu", pMaterialManager->GetTextureFilter() == eTextureFilter_Bilinear ? "Bilinear" : "Trilinear");
}

class cMainMenuWidget_TextureFilter : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_TextureFilter(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		mlMax = cMath::Max(mpInit->mpGame->GetGraphics()->GetLowLevel()->GetCaps(eGraphicCaps_MaxAnisotropicFiltering), 1);
		msTip = kTranslate("MainMenu", "TipGraphicsTextureAnisotropy");
	}

	void OnMouseDown(eMButton aButton) override
	{
		cMaterialManager *pMaterialManager = mpInit->mpGame->GetResources()->GetMaterialManager();
		int lX = cMath::Max((int)pMaterialManager->GetTextureAnisotropy(), 1);

		if(aButton == eMButton_Left)
		{
			lX *= 2;
			if(lX > mlMax) lX = 1;
		}
		else if(aButton == eMButton_Right)
		{
			lX /= 2;
			if(lX < 1) lX = mlMax;
		}

		pMaterialManager->SetTextureFilter(eTextureFilter_Trilinear);
		pMaterialManager->SetTextureAnisotropy((float)lX);
		gpTextureFilterText->msText = GetTextureFilterText(mpInit);
	}

	int mlMax;
};

//------------------------------------------------------------

// Samples per pixel of the 3D scene: off, 2, 4, 8, as far as the graphics
// card goes.
class cMainMenuWidget_FSAA : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_FSAA(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		mlMax = cMath::Min(mpInit->mpGame->GetGraphics()->GetLowLevel()->GetCaps(eGraphicCaps_MaxMultisampling), 8);
		// Not in the game's language files
		msTip = _W("Multisampling: several samples a pixel along the edges of surfaces, and of textures with cut-out parts. Smooths their outlines, at a cost that grows with the samples.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
		int lX = pLowLevel->GetMultisampling();

		if(aButton == eMButton_Left)
		{
			if(lX ==0)	lX = 2;
			else		lX *= 2;

			if(lX > mlMax) lX = 0;
		}
		else if(aButton == eMButton_Right)
		{
			if(lX == 2)			lX = 0;
			else if(lX == 0)	lX = mlMax;
			else				lX /= 2;
		}

		pLowLevel->SetMultisampling(lX);
		lX = pLowLevel->GetMultisampling();

		if(lX!=0)
			gpFSAAText->msText = cString::To16Char(cString::ToString(lX)+"x");
		else
			gpFSAAText->msText = kTranslate("MainMenu","Off");

		mpInit->mlFSAA = lX;
	}

	int mlMax;
};

// Off, or one of its qualities. The names are not in the game's language
// files.
static tWString GetSmaaText(cInit *mpInit)
{
	const cSmaaSettings &settings = mpInit->mpGame->GetGraphics()->GetLowLevel()->GetSmaa();
	if(settings.mbActive==false) return kTranslate("MainMenu","Off");

	static const wchar_t *vNames[] = {L"Low", L"Medium", L"High", L"Ultra", L"Custom"};
	return vNames[settings.GetQuality()];
}

class cMainMenuWidget_Smaa : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Smaa(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = _W("Smooths the stair steps of edges in the finished picture, also those of cut-out textures and shadows. Cheap, and works along with Anti-Aliasing.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
		cSmaaSettings settings = pLowLevel->GetSmaa();

		// Off, then the qualities: forwards with the left button, back with
		// the right. Settings made by hand (F2 panel) count as the first.
		const int lNum = 1 + cSmaaSettings::klQualityNum;
		int lIdx = settings.mbActive ? 1 + settings.GetQuality() % cSmaaSettings::klQualityNum : 0;
		lIdx = (lIdx + (aButton == eMButton_Right ? lNum - 1 : 1)) % lNum;

		settings.mbActive = lIdx > 0;
		if(lIdx > 0) settings.SetQuality(lIdx - 1);
		pLowLevel->SetSmaa(settings);

		gpSmaaText->msText = GetSmaaText(mpInit);
	}
};

static const float gvRenderScales[] = {0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
static const int glRenderScaleNum = sizeof(gvRenderScales) / sizeof(gvRenderScales[0]);

static tWString GetRenderScaleText(float afScale)
{
	return cString::To16Char(cString::ToString((int)(afScale * 100.0f + 0.5f))+"%");
}

class cMainMenuWidget_RenderScale : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_RenderScale(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		// Not in the game's language files
		msTip = _W("Resolution of the 3D view relative to the screen. Lower is faster, higher is sharper. The menus and HUD stay sharp.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		cGraphics *pGraphics = mpInit->mpGame->GetGraphics();

		// The closest step to the current scale, which may come from the
		// settings file
		int lIdx = 0;
		for(int i=1; i<glRenderScaleNum; ++i)
		{
			if(fabs(gvRenderScales[i] - pGraphics->GetRenderScale()) < fabs(gvRenderScales[lIdx] - pGraphics->GetRenderScale()))
				lIdx = i;
		}

		if(aButton == eMButton_Left)		lIdx = (lIdx + 1) % glRenderScaleNum;
		else if(aButton == eMButton_Right)	lIdx = (lIdx + glRenderScaleNum - 1) % glRenderScaleNum;

		pGraphics->SetRenderScale(gvRenderScales[lIdx]);
		gpRenderScaleText->msText = GetRenderScaleText(pGraphics->GetRenderScale());
	}
};

// Off, or one of its qualities. The names are not in the game's language
// files.
static tWString GetAmbientOcclusionText(cInit *mpInit)
{
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	if(pPostEffects->GetAmbientOcclusionActive()==false) return kTranslate("MainMenu","Off");

	static const wchar_t *vNames[] = {L"Low", L"Medium", L"High", L"Very High", L"Ultra", L"Custom"};
	return vNames[pPostEffects->GetAmbientOcclusionSettings().GetQuality()];
}

class cMainMenuWidget_AmbientOcclusion : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_AmbientOcclusion(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = _W("Darkens corners, creases and where things touch, which light from all around reaches least. Off is the original look.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
		cAmbientOcclusionSettings settings = pPostEffects->GetAmbientOcclusionSettings();

		// Off, then the qualities: forwards with the left button, back with
		// the right. Samples set by hand (F2 panel) count as the first.
		const int lNum = 1 + cAmbientOcclusionSettings::klQualityNum;
		int lIdx = pPostEffects->GetAmbientOcclusionActive() ? 1 + settings.GetQuality() % cAmbientOcclusionSettings::klQualityNum : 0;
		lIdx = (lIdx + (aButton == eMButton_Right ? lNum - 1 : 1)) % lNum;

		pPostEffects->SetAmbientOcclusionActive(lIdx > 0);
		if(lIdx > 0)
		{
			settings.SetQuality(lIdx - 1);
			pPostEffects->SetAmbientOcclusionSettings(settings);
		}

		gpAmbientOcclusionText->msText = GetAmbientOcclusionText(mpInit);
	}
};

static tWString GetDoFText(cInit *mpInit)
{
	return kTranslate("MainMenu", mpInit->mpEffectHandler->GetDepthOfField()->IsDisabled() ? "Off" : "On");
}

class cMainMenuWidget_DOF : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_DOF(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsDOF");	
	}

	void OnMouseDown(eMButton aButton) override
	{
		cEffect_DepthOfField *pDoF = mpInit->mpEffectHandler->GetDepthOfField();
		pDoF->SetDisabled(!pDoF->IsDisabled());

		gpDoFText->msText = GetDoFText(mpInit);
	}
};


// Water, some windows and particles drawn through what is behind them.
// Black Plague's option; Overture's language file lacks its texts.
cMainMenuWidget_Text *gpRefractionsText=nullptr;

static tWString TranslateOr(cInit *mpInit, const tString &asEntry, const tWString &asFallback)
{
	tWString sText = kTranslate("MainMenu", asEntry);
	return sText == _W("") ? asFallback : sText;
}

class cMainMenuWidget_Refractions : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Refractions(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = TranslateOr(mpInit,"TipGraphicsRefractions",
			_W("Adds more realism to water, some windows and particle effects. Quite large impact on framerate."));
	}

	void OnMouseDown(eMButton aButton) override
	{
		cRenderer3D *pRenderer = mpInit->mpGame->GetGraphics()->GetRenderer3D();
		pRenderer->SetRefractionUsed(!pRenderer->GetRefractionUsed());

		gpRefractionsText->msText = pRenderer->GetRefractionUsed() ?
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};


//------------------------------------------------------------

class cMainMenuWidget_TextureQuality : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_TextureQuality(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		mlCurrent = apInit->mpGame->GetResources()->GetMaterialManager()->GetTextureSizeLevel();
		msTip = kTranslate("MainMenu", "TipGraphicsTextureQuality");
	}

	void OnMouseDown(eMButton aButton) override
	{
		if(aButton == eMButton_Right)
		{
			mlCurrent++;
			if(mlCurrent >= glTextureQualityNum) mlCurrent =0;
		}
		else if(aButton == eMButton_Left)
		{
			mlCurrent--;
			if(mlCurrent < 0) mlCurrent =glTextureQualityNum-1;
		}

		gpTextureQualityText->msText = kTranslate("MainMenu",gvTextureQuality[mlCurrent]);
		mpInit->mpGame->GetResources()->GetMaterialManager()->SetTextureSizeLevel(mlCurrent);

		gbMustRestart = true;
	}

	int mlCurrent;
};

//------------------------------------------------------------


// The sizes "Resolution" steps through: the desktop size (stored as -1 x -1),
// the fullscreen modes of the display, and the sizes of the original list
// that fit on it, for windows.
static void GetResolutionList(cInit *apInit, std::vector<cVector2l> &avList)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();
	int lDisplay = apInit->mlDisplay;
	if(lDisplay < 0 || lDisplay >= pLowLevel->GetDisplayNum()) lDisplay = pLowLevel->GetDisplay();

	cVector2l vDesktop = pLowLevel->GetDisplaySize(lDisplay);

	pLowLevel->GetDisplayModes(lDisplay, avList);
	for(int i=0; i<glResolutionNum; ++i)
	{
		if(vDesktop.x > 0 && (gvResolutions[i].x > vDesktop.x || gvResolutions[i].y > vDesktop.y)) continue;
		if(std::find(avList.begin(), avList.end(), gvResolutions[i]) == avList.end())
			avList.push_back(gvResolutions[i]);
	}
	std::sort(avList.begin(), avList.end(), [](const cVector2l &a, const cVector2l &b)
				{ return a.x != b.x ? a.x < b.x : a.y < b.y; });

	avList.insert(avList.begin(), cVector2l(-1,-1));
}

static tWString GetResolutionText(cInit *apInit)
{
	// Not in the game's language files
	if(apInit->mvScreenSize.x <= 0 || apInit->mvScreenSize.y <= 0) return _W("Desktop");

	char sTempVec[256];
	sprintf(sTempVec,"%d x %d",apInit->mvScreenSize.x, apInit->mvScreenSize.y);
	return cString::To16Char(sTempVec);
}

class cMainMenuWidget_Resolution : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Resolution(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsResolution");
	}

	void OnMouseDown(eMButton aButton) override
	{
		std::vector<cVector2l> vResolutions;
		GetResolutionList(mpInit, vResolutions);
		int lNum = (int)vResolutions.size();

		// A size that is not in the list (set in the config file) counts as
		// the desktop size.
		int lCurrentNum =0;
		for(int i=1; i<lNum; ++i)
		{
			if(vResolutions[i] == mpInit->mvScreenSize)
			{
				lCurrentNum = i;
				break;
			}
		}

		if(aButton == eMButton_Left)
		{
			lCurrentNum++;
			if(lCurrentNum >= lNum) lCurrentNum =0;
		}
		else if(aButton == eMButton_Right)
		{
			lCurrentNum--;
			if(lCurrentNum < 0) lCurrentNum =lNum-1;
		}

		mpInit->mvScreenSize = vResolutions[lCurrentNum];

		gpResolutionText->msText = GetResolutionText(mpInit);
		
		gbMustRestart = true;
	}
};

//-----------------------------------------------------------

class cMainMenuWidget_Fullscreen : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Fullscreen(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		// Not in the game's language files
		msTip = _W("Fill the screen or run in a window. Requires restart.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbFullScreenSetting = !mpInit->mbFullScreenSetting;

		gpFullscreenText->msText = mpInit->mbFullScreenSetting ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");

		gbMustRestart = true;
	}
};

//-----------------------------------------------------------

static tWString GetDisplayText(cInit *apInit)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();

	// Not in the game's language files
	if(apInit->mlDisplay < 0 || apInit->mlDisplay >= pLowLevel->GetDisplayNum()) return _W("Automatic");

	tString sName = pLowLevel->GetDisplayName(apInit->mlDisplay);
	if(sName.size() > 20) sName = sName.substr(0,20);

	return cString::To16Char(cString::ToString(apInit->mlDisplay + 1) + ": " + sName);
}

class cMainMenuWidget_Display : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Display(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		// Not in the game's language files
		msTip = _W("The monitor to use. Automatic picks the largest. Requires restart.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		int lNum = mpInit->mpGame->GetGraphics()->GetLowLevel()->GetDisplayNum();

		// -1 is automatic
		int lDisplay = mpInit->mlDisplay;
		if(lDisplay >= lNum) lDisplay = -1;

		if(aButton == eMButton_Left)
		{
			lDisplay++;
			if(lDisplay >= lNum) lDisplay = -1;
		}
		else if(aButton == eMButton_Right)
		{
			lDisplay--;
			if(lDisplay < -1) lDisplay = lNum-1;
		}

		mpInit->mlDisplay = lDisplay;

		gpDisplayText->msText = GetDisplayText(mpInit);

		gbMustRestart = true;
	}
};

//-----------------------------------------------------------

class cMainMenuWidget_PostEffects : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_PostEffects(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsPostEffects");
	}

	void OnMouseDown(eMButton aButton) override
	{
		bool bPostEffectsActive = mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetActive();
		mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetActive(!bPostEffectsActive);
		mpInit->mbPostEffects = !bPostEffectsActive;

		gpPostEffectsText->msText = mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetActive() ? 
			kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

//-----------------------------------------------------------

class cMainMenuWidget_FOV : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_FOV(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		mfMax = 100.0f;
		mfMin = 60.0f;
		mfStep = 5.0f;

		// Not in the game's language files
		msTip = _W("Vertical viewing angle in degrees. The original is 70.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		float fFOV = mpInit->mpPlayer->GetViewFOV();

		if(aButton == eMButton_Left)
		{
			fFOV += mfStep;
			if(fFOV > mfMax) fFOV = mfMax;
		}
		else if(aButton == eMButton_Right)
		{
			fFOV -= mfStep;
			if(fFOV < mfMin) fFOV = mfMin;
		}

		mpInit->mpPlayer->SetViewFOV(fFOV);

		char sTempVec[256];
		sprintf(sTempVec,"%.0f",fFOV);
		gpFOVText->msText = cString::To16Char(sTempVec);
	}

	float mfMax;
	float mfMin;
	float mfStep;
};

//-----------------------------------------------------------

static tWString GetBloomText(cInit *mpInit)
{
	return kTranslate("MainMenu", mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetBloomActive() ? "On" : "Off");
}

class cMainMenuWidget_Bloom : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Bloom(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsBloom");
	}

	void OnMouseDown(eMButton aButton) override
	{
		cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
		pPostEffects->SetBloomActive(!pPostEffects->GetBloomActive());

		gpBloomText->msText = GetBloomText(mpInit);
	}
};

//------------------------------------------------------------


static tWString GetMotionBlurText(cInit *mpInit)
{
	return kTranslate("MainMenu", mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetMotionBlurActive() ? "On" : "Off");
}

class cMainMenuWidget_MotionBlur : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_MotionBlur(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsMotionBlur");
	}

	void OnMouseDown(eMButton aButton) override
	{
		cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
		pPostEffects->SetMotionBlurActive(!pPostEffects->GetMotionBlurActive());

		gpMotionBlurText->msText = GetMotionBlurText(mpInit);
	}
};

//------------------------------------------------------------

// Graphics presets. The names are not in the game's language files.

// Shows the preset the options are now, Custom if none: they can be changed
// on the other pages and in the F2 panel.
class cMainMenuWidget_GraphicsPresetText : public cMainMenuWidget_Text
{
public:
	cMainMenuWidget_GraphicsPresetText(cInit *apInit, const cVector3f &avPos, cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Text(apInit,avPos,GetGraphicsPresetName(GetGraphicsPreset(apInit)),avFontSize,aAlignment) {}

	void OnDraw() override
	{
		msText = GetGraphicsPresetName(GetGraphicsPreset(mpInit));
		cMainMenuWidget_Text::OnDraw();
	}
};

// What the pages show after a preset changed their options
static void UpdateGraphicsTexts(cInit *mpInit)
{
	cGraphics *pGraphics = mpInit->mpGame->GetGraphics();
	const tWString sOn = kTranslate("MainMenu","On");
	const tWString sOff = kTranslate("MainMenu","Off");

	gpBloomText->msText = GetBloomText(mpInit);
	gpPostEffectsText->msText = pGraphics->GetRendererPostEffects()->GetActive() ? sOn : sOff;
	gpMotionBlurText->msText = GetMotionBlurText(mpInit);
	gpShadowsText->msText = GetShadowOptionText(mpInit, (int)pGraphics->GetRenderer3D()->GetShowShadows());
	gpTextureFilterText->msText = GetTextureFilterText(mpInit);
	gpSmaaText->msText = GetSmaaText(mpInit);
	gpRenderScaleText->msText = GetRenderScaleText(pGraphics->GetRenderScale());
	gpAmbientOcclusionText->msText = GetAmbientOcclusionText(mpInit);
	gpDoFText->msText = GetDoFText(mpInit);
}

class cMainMenuWidget_GraphicsPreset : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_GraphicsPreset(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = _W("Sets graphics quality to High, Medium or Low. Custom means the options have been changed by hand.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		// The presets in turn, forwards with the left button, back with the
		// right; from Custom the first or last
		int lCurrent = (int)GetGraphicsPreset(mpInit);
		const int lNum = eGraphicsPreset_Custom;
		if(lCurrent == eGraphicsPreset_Custom)	lCurrent = aButton == eMButton_Right ? lNum - 1 : 0;
		else if(aButton == eMButton_Right)		lCurrent = (lCurrent + lNum - 1) % lNum;
		else									lCurrent = (lCurrent + 1) % lNum;

		ApplyGraphicsPreset(mpInit, (eGraphicsPreset)lCurrent);
		UpdateGraphicsTexts(mpInit);
	}
};

//------------------------------------------------------------

class cMainMenuWidget_VSync : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_VSync(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		msTip = kTranslate("MainMenu", "TipGraphicsVSync");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mbVsync = !mpInit->mbVsync;
		mpInit->mpGame->GetGraphics()->GetLowLevel()->SetVsyncActive(mpInit->mbVsync);

		gpVSyncText->msText = mpInit->mbVsync ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

//------------------------------------------------------------

class cMainMenuWidget_LimitFPS : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_LimitFPS(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		// Not in the game's language files
		msTip = _W("Off: frames are drawn as fast as possible, or at the display's rate with VSync on. The game itself still runs at 60 steps per second.");
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mpGame->SetLimitFPS(!mpInit->mpGame->GetLimitFPS());

		gpLimitFPSText->msText = mpInit->mpGame->GetLimitFPS() ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	}
};

//------------------------------------------------------------

class cMainMenuWidget_GfxBack : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_GfxBack(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);
		if(gbMustRestart)
		{
			mpInit->mpMainMenu->SetState(eMainMenuState_GraphicsRestart);
			gbMustRestart = false;
		}
		else
		{
			mpInit->mpMainMenu->SetState(eMainMenuState_Options);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// OPTIONS KEY CONFIG
//////////////////////////////////////////////////////////////////////////

class cMainMenuWidget_KeyButton : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_KeyButton(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, 
								eFontAlign aAlignment, cMainMenuWidget_Text *apKeyWidget,
								const tString &asActionName)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
		mpKeyWidget = apKeyWidget;
		msActionName = asActionName;

		iAction *pAction = mpInit->mpGame->GetInput()->GetAction(asActionName);
		if(pAction)
		{
			tString sKeyName = pAction->GetInputName();
			mpKeyWidget->msText =kTranslate("ButtonNames",sKeyName);
			
			//If translation is missing, set to default
			if(mpKeyWidget->msText == _W(""))
				mpKeyWidget->msText = cString::To16Char(sKeyName);

		}
		else
		{
			mpKeyWidget->msText = kTranslate("MainMenu","Empty");
			//FatalError("Action for %s button does not exist!\n",msText.c_str());
		}

        mpKeyWidget->SetExtraWidget(this);		
		mpKeyWidget->UpdateSize();
	}

	void OnMouseDown(eMButton aButton) override
	{
		mpKeyWidget->msText = _W(".....");
		mpInit->mpMainMenu->SetInputToAction(msActionName,mpKeyWidget);
	}

	void Reset() override
	{
		iAction *pAction = mpInit->mpGame->GetInput()->GetAction(msActionName);
		
		if(pAction)
		{
			tString sKeyName = pAction->GetInputName();
			mpKeyWidget->msText =kTranslate("ButtonNames",sKeyName);

			//If translation is missing, set to default
			if(mpKeyWidget->msText == _W(""))
				mpKeyWidget->msText = cString::To16Char(sKeyName);

            mpKeyWidget->UpdateSize();
		}
		else
		{
			mpKeyWidget->msText = kTranslate("MainMenu","Empty");
		}
	}

private:
	cMainMenuWidget_Text *mpKeyWidget;
	tString msActionName;
};

//------------------------------------------------------------


class cMainMenuWidget_KeyReset : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_KeyReset(cInit *apInit, const cVector3f &avPos, const tWString& asText,cVector2f avFontSize, 
							eFontAlign aAlignment)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
	{
	}

	void OnMouseDown(eMButton aButton) override
	{
		//Log("Setting deafult keys!\n");
		mpInit->mpButtonHandler->SetDefaultKeys();
		mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupMove);
		mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupAction);
		mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupMisc);
	}
private:
	cMainMenuWidget_Text *mpKeyWidget;
	tString msActionName;
};

//------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMainMenu::cMainMenu(cInit *apInit)  : iUpdateable("MainMenu")
{
	mState = eMainMenuState_Start;

	mpLogo = nullptr;
	mpBackground = nullptr;

	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	//Load graphics
	mpGfxBlackQuad = mpDrawer->CreateGfxObject("effect_black.bmp","diffalpha2d");
	mpGfxMouse = mpDrawer->CreateGfxObject("player_crosshair_pointer.bmp","diffalpha2d");

	mpGfxRainDrop = mpDrawer->CreateGfxObject("menu_rain_drop.jpg","diffadditive2d");
	mpGfxRainSplash = mpDrawer->CreateGfxObject("menu_rain_splash.jpg","diffadditive2d");
	mpGfxSnowFlake = mpDrawer->CreateGfxObject("menu_snow_flake.jpg","diffadditive2d");
	
	//Init effects
	mvRainDrops.resize(70);
	mvRainSplashes.resize(180);
	for(size_t i=0; i < mvRainSplashes.size(); ++i){
		mvRainSplashes[i].mCol = cColor(1,0);
		mvRainSplashes[i].mpGfx = mpGfxRainSplash;
	}
	for(size_t i=0; i < mvRainDrops.size(); ++i){
		mvRainDrops[i].mCol = cColor(1,0);
		mvRainDrops[i].mpGfx = mpGfxRainDrop;
	}

	mvSnowFlakes.resize(80);
	for(size_t i=0; i< mvSnowFlakes.size(); ++i){
		mvSnowFlakes[i].mvPos = cVector3f(cMath::RandRectf(350,800),cMath::RandRectf(200,550),20);
		mvSnowFlakes[i].mvVel = cVector3f(0,cMath::RandRectf(15,40),0);
		mvSnowFlakes[i].mvSize = cMath::RandRectf(2,10);
		mvSnowFlakes[i].mpGfx = mpGfxSnowFlake;
	}
	
	//load fonts
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_menu_small.fnt",20,32,255);
	mpTipFont  = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");

	//////////////////////////////////
	//Init widgets
	
	mvStateWidgets.resize(eMainMenuState_LastEnum);
	
	Reset();
}

//-----------------------------------------------------------------------

cMainMenu::~cMainMenu(void)
{
	STLDeleteAll(mlstWidgets);

    mpDrawer->DestroyGfxObject(mpGfxBlackQuad);
	mpDrawer->DestroyGfxObject(mpGfxMouse);
	mpDrawer->DestroyGfxObject(mpGfxRainDrop);
	mpDrawer->DestroyGfxObject(mpGfxRainSplash);
	mpDrawer->DestroyGfxObject(mpGfxSnowFlake);

	if(mpLogo) mpInit->mpGame->GetResources()->GetTextureManager()->Destroy(mpLogo);
	if(mpBackground) mpInit->mpGame->GetResources()->GetTextureManager()->Destroy(mpBackground);
	
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------

void cMainMenu::Reset()
{
	mbActive = false;
	mbFadeIn = false;
	mfAlpha =0;
	mfFadeAmount = 0;

	mbMouseIsDown = false;

	mbUpdateWidgets = false;

	mpCurrentActionText = nullptr;

	mbGameActive = false;

	//Effects:
	mfRainRefreshRemainingSeconds = 0;

}

//-----------------------------------------------------------------------

void cMainMenu::OnPostSceneDraw()
{
	mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));
	
	mpInit->mpGraphicsHelper->DrawTexture(mpLogo,0,cVector3f(800,180,30),cColor(1,1));
	mpInit->mpGraphicsHelper->DrawTexture(mpBackground,cVector3f(0,180,0),cVector3f(800,420,0),cColor(1,1));

	////////////////////////////////
	// Fade in
	if (mbFadeIn)
	{
		mpDrawer->DrawGfxObject(mpGfxBlackQuad,mpInit->mpGraphicsHelper->GetScreenPos(120),mpInit->mpGraphicsHelper->GetScreenSize(), cColor(1,1-mfFadeAmount));
	}
}

//-----------------------------------------------------------------------


void cMainMenu::OnDraw()
{
	////////////////////////////////
	// Draw widgets
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget* pWidget = *it;
		
		if(pWidget->IsActive()) pWidget->OnDraw();
	}

	DrawBackground();

	////////////////////////////
	//Draw tip
	if(msButtonTip != _W(""))
	{
		mpTipFont->DrawWordWrap(cVector3f(10,570,150),780,13,12,cColor(1,1),
								eFontAlign_Left,msButtonTip.c_str());
	}

	////////////////////////////////
	// Draw mouse
	if(mpCurrentActionText) return;
	cResourceImage *pImage = mpGfxMouse->GetMaterial()->GetImage(eMaterialTexture_Diffuse);
	cVector2l vSize = pImage->GetSize();
	cVector2f vPosAdd(((float)vSize.x) / 2.0f, ((float)vSize.y) / 2.0f);
	cVector2f vMousePos = mvMousePos + mpInit->mpButtonHandler->GetCursorDrawOffset(true);
	mpInit->mpGraphicsHelper->ClampToScreen(vMousePos);
	mpDrawer->DrawGfxObject(mpGfxMouse,cVector3f(0,0,100)+(vMousePos - vPosAdd));
}

//-----------------------------------------------------------------------

static void DrawParticle(cGraphicsDrawer *apDrawer, cMainMenuParticle *apParticle)
{
	apDrawer->DrawGfxObject(apParticle->mpGfx,
							apParticle->mvPos- apParticle->mvSize/2,
							apParticle->mvSize,
							apParticle->mCol);
}

void cMainMenu::DrawBackground()
{
	if(mbGameActive)
	{
		for(size_t i=0; i < mvSnowFlakes.size(); ++i) DrawParticle(mpDrawer,&mvSnowFlakes[i]);
	}
	else
	{
		for(size_t i=0; i < mvRainDrops.size(); ++i) DrawParticle(mpDrawer,&mvRainDrops[i]);
		for(size_t i=0; i < mvRainSplashes.size(); ++i) DrawParticle(mpDrawer,&mvRainSplashes[i]);
	}
}

//-----------------------------------------------------------------------

void cMainMenu::Update(float afTimeStep)
{
	if (mbFadeIn)
	{
		if (mfFadeAmount < 1)
			mfFadeAmount += 0.5f * afTimeStep;
		else
		{
			mbFadeIn = false;
			mfFadeAmount = 0;
		}
	}
	
	if(mbUpdateWidgets)
	{
		mbUpdateWidgets = false;
		CreateWidgets();
		SetState(mState);
	}


	if(mpCurrentActionText)
	{
		cInput *pInput = mpInit->mpGame->GetInput();

		if(CheckForInput())
		{
			//Log("Creating action '%s'\n",msCurrentActionName.c_str());
			iAction *pAction = pInput->InputToAction(msCurrentActionName);
			
			mpCurrentActionText->msText = kTranslate("ButtonNames",pAction->GetInputName());
			
			//If translation is missing, set to default
			if(mpCurrentActionText->msText == _W(""))
				mpCurrentActionText->msText = cString::To16Char(pAction->GetInputName());

			mpCurrentActionText->UpdateSize();

			
			tString sAction = mpInit->mpButtonHandler->GetActionName(pAction->GetInputName(),msCurrentActionName);
			if(sAction != "")
			{
				pInput->DestroyAction(sAction);
				
				mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupMove);
				mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupAction);
				mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupMisc);
			}

			mpCurrentActionText = nullptr;
			Log("Reset check for input!\n");
		}
		
	}
	else
	{
	}

	////////////////////////////////
	// Update effect
	if(mbGameActive)
	{
		for(size_t i=0; i< mvSnowFlakes.size(); ++i)
		{
			cMainMenuParticle &flake = mvSnowFlakes[i];
			
			if(flake.mvPos.y >= 600 - (150 - mvSnowFlakes[i].mvSize.x*15))
			{
				mvSnowFlakes[i].mvPos = cVector3f(cMath::RandRectf(350,800),200,20);
				mvSnowFlakes[i].mvVel = cVector3f(0,cMath::RandRectf(15,40),0);
				mvSnowFlakes[i].mvSize = cMath::RandRectf(2,10);
			}

			flake.mvPos += flake.mvVel*afTimeStep;
			flake.mvVel.x += cMath::RandRectf(-2, 2);
			if(flake.mvVel.x <	-25) flake.mvVel.x = -25;
			if(flake.mvVel.x >	25) flake.mvVel.x = 25;

			float fDist = cMath::Vector3Dist(flake.mvPos,cVector3f(550,550,20));
			float fAlpha = 1 - fDist/380;
			if(fAlpha <0)fAlpha =0;
			flake.mCol = cColor(1,fAlpha);
		}
	}
	else
	{
		//Rainsplashes
		for(size_t i=0; i < mvRainSplashes.size(); ++i)
		{
			cMainMenuParticle &splash = mvRainSplashes[i];
			if(splash.mCol.a <= 0)
			{
				splash.mCol.a =1;
				
				float fX,fY;

				if(i<120)
				{
					fX = cMath::RandRectf(400, 700);
					fY = cMath::RandRectf(500, 550);
				}
				else if(i<140)
				{
					fX = cMath::RandRectf(480, 605);
					fY = cMath::RandRectf(278, 320);
				}
				else if(i<160)
				{
					fX = cMath::RandRectf(360, 460);
					fY = cMath::RandRectf(288, 330);
				}
				else
				{
					fX = cMath::RandRectf(615, 760);
					fY = cMath::RandRectf(258, 310);
				}
				
				splash.mvPos = cVector3f(fX,fY,20);
				splash.mvSize = cMath::RandRectf(3,16);
				
				splash.mpGfx = mpGfxRainSplash;

				splash.mvVel.x = cMath::RandRectf(1,5);
			}
			else
			{
				splash.mCol.a -= afTimeStep * splash.mvVel.x;
				if(splash.mCol.a <0)splash.mCol.a =0;
			}
		}

		//Rain drops
		if(mfRainRefreshRemainingSeconds<=0)
		{
			for(size_t i=0; i < mvRainDrops.size(); ++i)
			{
				float fX = cMath::RandRectf(150, 800);
				float fY = cMath::RandRectf(180, 600);
				mvRainDrops[i].mvPos = cVector3f(fX,fY,20);
				mvRainDrops[i].mvSize = cMath::RandRectf(28,34);
				
				float fDist = cMath::Vector3Dist(mvRainDrops[i].mvPos,cVector3f(550,400,20));
				float fAlpha = 1 - fDist/380;
				if(fAlpha <0)fAlpha =0;
				mvRainDrops[i].mCol = cColor(1,fAlpha);

				mvRainDrops[i].mpGfx = mpGfxRainDrop;
			}

			mfRainRefreshRemainingSeconds = 1.0f/37.0f;
		}
		else
		{
			mfRainRefreshRemainingSeconds -= afTimeStep;
		}
	}


	////////////////////////////////
	// Update buttons
	msButtonTip = _W("");
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;

		if(pWidget->IsActive())pWidget->OnUpdate(afTimeStep);

		if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
		{
			if(pWidget->IsActive())pWidget->OnMouseOver(true);
		}
		else
		{
			if(pWidget->IsActive())pWidget->OnMouseOver(false);
		}
	}
}

//-----------------------------------------------------------------------

void cMainMenu::AddMousePos(const cVector2f &avRel)
{
	if(mpCurrentActionText) return;

	mvMousePos += avRel;

	mpInit->mpGraphicsHelper->ClampToScreen(mvMousePos);
}

void cMainMenu::SetMousePos(const cVector2f &avPos)
{
	if(mpCurrentActionText) return;

	mvMousePos = avPos;

}

//-----------------------------------------------------------------------

void cMainMenu::OnMouseDown(eMButton aButton)
{
	if(mpCurrentActionText) return;

	////////////////////////////////
	// Update buttons
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;

		if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
		{
			if(pWidget->IsActive())
			{
				pWidget->OnMouseDown(aButton);
				break;
			}
		}
	}

	mbMouseIsDown = true;
}

void cMainMenu::OnMouseUp(eMButton aButton)
{
	if(mpCurrentActionText) return;

	////////////////////////////////
	// Update buttons
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;

		if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
		{
			if(pWidget->IsActive()) pWidget->OnMouseUp(aButton);
		}
	}

	mbMouseIsDown = false;
}

//-----------------------------------------------------------------------

void cMainMenu::OnMouseDoubleClick(eMButton aButton)
{
	if(mpCurrentActionText) return;

	////////////////////////////////
	// Update buttons
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;

		if(cMath::PointBoxCollision(mvMousePos,pWidget->GetRect()))
		{
			if(pWidget->IsActive()) pWidget->OnDoubleClick(aButton);
		}
	}

	mbMouseIsDown = false;
}

//-----------------------------------------------------------------------

void cMainMenu::SetActive(bool abActive)
{
	if(mbActive == abActive) return;

	
	mbActive = abActive;

	if(mbActive)
	{
		if(mpInit->mbHasHaptics)
			mpInit->mpPlayer->GetHapticCamera()->SetActive(false);
		
		if (!mpInit->mbFullScreen) {
			mpInit->mpGame->GetInput()->GetLowLevel()->LockInput(false);
		}

		mpInit->mpGame->GetUpdater()->SetContainer("MainMenu");
		mpInit->mpGame->GetScene()->SetDrawScene(false);
		mpInit->mpGame->GetScene()->SetUpdateMap(false);
		if(mpInit->mbHasHaptics)
		{
			mpInit->mpGame->GetHaptic()->GetLowLevel()->StopAllForces();
			mpInit->mpGame->GetHaptic()->GetLowLevel()->SetUpdateShapes(false);
		}
		
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_MainMenu);
				
		///////////////////////////////
		//Init menu
		CreateWidgets();
		cSoundHandler *pSoundHandler =mpInit->mpGame->GetSound()->GetSoundHandler();
		
		if(mpInit->mpMapHandler->GetCurrentMapName() != "")
		{
			mpInit->mpGame->GetSound()->GetSoundHandler()->PauseAll(eSoundDest_World | eSoundDest_Gui);
			mpInit->mpGame->GetSound()->GetMusicHandler()->Pause();

			mbGameActive = true;
			pSoundHandler->PlayGui("gui_wind1",true,1);
		}
		else
		{
			mpInit->mpGame->GetSound()->GetMusicHandler()->Play(cInit::GetMenuMusic(),1,5.0f,false);
			
			if(pSoundHandler->IsPlaying("gui_rain1")==false)
				pSoundHandler->PlayGui("gui_rain1",true,1);
			
			mbGameActive = false;
			mbFadeIn = true;
		}
		
		bool bFirstStart = mpInit->mpConfig->GetBool("Game","FirstStart",true);
		if(bFirstStart)
		{
			SetState(eMainMenuState_FirstStart);
			mLastState = eMainMenuState_FirstStart;

			mpInit->mpConfig->SetBool("Game","FirstStart",false);
		}
		else
		{
			SetState(eMainMenuState_Start);
			mLastState = eMainMenuState_Start;
		}

		gbMustRestart=false;

		mpCurrentActionText = nullptr;

		mpLogo = mpInit->mpGame->GetResources()->GetTextureManager()->Create2D("menu_logo.jpg",false);
		
		if(mbGameActive)
			mpBackground = mpInit->mpGame->GetResources()->GetTextureManager()->Create2D("menu_background_ingame.jpg",false);
		else
			mpBackground = mpInit->mpGame->GetResources()->GetTextureManager()->Create2D("menu_background.jpg",false);

	}
	else
	{
		if (!mpInit->mbFullScreen) {
			mpInit->mpGame->GetInput()->GetLowLevel()->LockInput(true);
		}

		if(mpInit->mbHasHaptics)
			mpInit->mpPlayer->GetHapticCamera()->SetActive(true);

		cSoundHandler *pSoundHandler =mpInit->mpGame->GetSound()->GetSoundHandler();
		
		if(mpInit->mpMapHandler->GetCurrentMapName() != "")
		{
			if (pSoundHandler->IsPlaying("gui_wind1"))
				pSoundHandler->Stop("gui_wind1");

			pSoundHandler->ResumeAll(eSoundDest_World | eSoundDest_Gui);
			mpInit->mpGame->GetSound()->GetMusicHandler()->Resume();
		}
		else
		{
			if (pSoundHandler->IsPlaying("gui_rain1"))
				pSoundHandler->Stop("gui_rain1");
			mpInit->mpGame->GetSound()->GetMusicHandler()->Stop(0.3f);
		}

		mpInit->mpGame->GetUpdater()->SetContainer("Default");
		mpInit->mpGame->GetScene()->SetDrawScene(true);
		mpInit->mpGame->GetScene()->SetUpdateMap(true);
		if(mpInit->mbHasHaptics)
			mpInit->mpGame->GetHaptic()->GetLowLevel()->SetUpdateShapes(true);
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_Game);

		if(mpLogo) mpInit->mpGame->GetResources()->GetTextureManager()->Destroy(mpLogo);
		mpLogo = nullptr;
		if(mpBackground) mpInit->mpGame->GetResources()->GetTextureManager()->Destroy(mpBackground);
		mpBackground = nullptr;
	}
}
//-----------------------------------------------------------------------

void cMainMenu::Exit()
{
	if(mpCurrentActionText)
	{
		mpCurrentActionText = nullptr;
	}
	else if(mState==eMainMenuState_Start && mpInit->mpMapHandler->GetCurrentMapName() != "")
	{
		SetActive(false);
	}
	else if((	mState==eMainMenuState_OptionsGraphics || 
				mState==eMainMenuState_OptionsSound ||
				mState==eMainMenuState_OptionsGame)
			&& gbMustRestart)
	{
		SetState(eMainMenuState_GraphicsRestart);

		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);

		gbMustRestart =false;
	}
	else
	{
		SetState(gvMenuBackStates[mState]);
		
		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);
	}
}

//-----------------------------------------------------------------------

void cMainMenu::OnExit()
{
	SetActive(false);
}

//-----------------------------------------------------------------------

void cMainMenu::SetState(eMainMenuState aState)
{
	mLastState = mState;

	mState = aState;
		
	//Set all widgets as not active.
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;
		pWidget->SetActive(false);
	}
		
	//Set all widgets for current state as active.
	it = mvStateWidgets[aState].begin();
	for(; it != mvStateWidgets[aState].end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;
		pWidget->SetActive(true);
	}
}

//-----------------------------------------------------------------------

void cMainMenu::SetInputToAction(const tString &asActionName,cMainMenuWidget_Text *apText)
{
	msCurrentActionName = asActionName;
	mpCurrentActionText = apText;
	InitCheckInput();
}

//-----------------------------------------------------------------------

void cMainMenu::InitCheckInput()
{
	cInput *pInput = mpInit->mpGame->GetInput();

	for(int i=0; i<eKey_LastEnum;++i) 
	{
		mvKeyPressed[i] = pInput->GetKeyboard()->KeyIsDown((eKey)i);
	}

	for(int i=0; i<eMButton_LastEnum; ++i)
	{
		mvMousePressed[i] = pInput->GetMouse()->ButtonIsDown((eMButton)i);
	}
}

//-----------------------------------------------------------------------

bool cMainMenu::CheckForInput()
{
	cInput *pInput = mpInit->mpGame->GetInput();

	////////////////////
	//Keyboard
	for(int i=0; i<eKey_LastEnum;++i) 
	{
		if(pInput->GetKeyboard()->KeyIsDown((eKey)i))
		{
			if(mvKeyPressed[i]==false) return true;
		}
		else
		{
			mvKeyPressed[i] = false;
		}
	}
	
	////////////////////
	//Mouse
	for(int i=0; i<eMButton_LastEnum; ++i)
	{
		if(pInput->GetMouse()->ButtonIsDown((eMButton)i))
		{
			if(mvMousePressed[i]==false) return true;
		}
		else
		{
			mvMousePressed[i] = false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------

void cMainMenu::ResetWidgets(eMainMenuState aState)
{
	tMainMenuWidgetListIt it = mvStateWidgets[aState].begin();
	for(; it != mvStateWidgets[aState].end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;
		pWidget->Reset();
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

class cSaveFileEntry
{
public:
	cSaveFileEntry(const tWString& asFile, const cDate& aDate)
	{
		msFile = asFile;
		mDate = aDate;
	}
	
	bool operator<(const cSaveFileEntry &aA) const
	{
		return mDate < aA.mDate;
	}
	bool operator>(const cSaveFileEntry &aA) const
	{
		return mDate > aA.mDate;
	}
	bool operator==(const cSaveFileEntry &aA) const
	{
		return mDate == aA.mDate;
	}

	tWString msFile;
	cDate mDate;
};

typedef std::multiset<cSaveFileEntry,std::greater<cSaveFileEntry> > tSaveFileEntrySet;
typedef tSaveFileEntrySet::iterator tSaveFileEntrySetIt;


//-----------------------------------------------------------------------

void cMainMenu::CreateWidgets()
{
	cVector3f vPos;
	tWString sText;
	cMainMenuWidget_Text *pTempTextWidget;
	char sTempVec[256];


	cVector3f vTextStart(220, 230, 40);


	///////////////////////////////
	//Erase all previous
	STLDeleteAll(mlstWidgets);
	for(size_t i=0; i< eMainMenuState_LastEnum; ++i) mvStateWidgets[i].clear();

	///////////////////////////////
	// First start
	//////////////////////////////
	bool bFirstStart = mpInit->mpConfig->GetBool("Game","FirstStart",true);
	if(bFirstStart)
	{
		vPos = cVector3f(40, 190, 40);
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu", "Welcome"),15,eFontAlign_Left)) ); 
		vPos.y += 18;
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu", "Too Improve"),15,eFontAlign_Left)) ); 
		vPos.y += 28;
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu", "StartTip1"),15,eFontAlign_Left)) ); 
		vPos.y += 18;
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu", "StartTip2"),15,eFontAlign_Left)) ); 
		vPos.y += 18;
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu", "StartTip3"),15,eFontAlign_Left)) ); 
		vPos.y += 28;
		vPos.x = 395;
		cMainMenuWidget *pGammaFirstButton = hplNew( cMainMenuWidget_Gamma,(mpInit,vPos,kTranslate("MainMenu","Gamma:"),20,eFontAlign_Right,1) );
		AddWidgetToState(eMainMenuState_FirstStart,pGammaFirstButton); 
		vPos.x = 405;
		sprintf(sTempVec,"%.1f",mpInit->mpGame->GetGraphics()->GetLowLevel()->GetGammaCorrection());
		sText = cString::To16Char(sTempVec);
		gpGammaText2 = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_FirstStart,gpGammaText2); 
		gpGammaText2->SetExtraWidget(pGammaFirstButton);

		vPos.y += 31;
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Image,(mpInit,
																			cVector3f(250,vPos.y,30),
																			cVector2f(300,200),
																			"menu_gamma.bmp",
																			"diffalpha2d",
																			cColor(1,1))) );
		vPos.y+=205;
		//AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu", "StartTip4"),15,eFontAlign_Left)); 
		//vPos.y += 28;
		AddWidgetToState(eMainMenuState_FirstStart,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","OK"),eMainMenuState_Start,20,eFontAlign_Center)) );
	}
	


	///////////////////////////////
	//Start menu:
	//////////////////////////////
	vPos = vTextStart;//cVector3f(400, 260, 40);

	if(mpInit->mpMapHandler->GetCurrentMapName() != "")
	{
		AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_Resume,(mpInit,vPos,kTranslate("MainMenu","Resume"))) );
		vPos.y += 60;
	}
	else
	{
		tWString sAuto = mpInit->mpSaveHandler->GetLatest(_W("save/auto/"),_W("*.sav"));
		tWString sSpot = mpInit->mpSaveHandler->GetLatest(_W("save/spot/"),_W("*.sav"));

		if(sAuto != _W("") || sSpot != _W(""))
		{
			AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","Continue"),eMainMenuState_Continue)) ); 
			vPos.y += 51;
		}
	}

	if(cInit::GetGameId() == eGameId_BlackPlague)
	{
		// Asked once; after that the tutorial has its own button
		if(mpInit->mbAskTutorial)
		{
			AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","New Game"),eMainMenuState_AskTutorial)) );
			vPos.y += 51;
		}
		else
		{
			AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","Tutorial"),eMainMenuState_AskStartTutorial)) );
			vPos.y += 51;
			AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","New Game"),eMainMenuState_NewGame)) );
			vPos.y += 51;
		}
	}
	else
	{
		AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","New Game"),eMainMenuState_NewGame)) );
		vPos.y += 51;
	}
	AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","Load Game"),eMainMenuState_LoadGameSpot)) ); 
	vPos.y += 51;
	AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","Options"),eMainMenuState_Options)) ); 
	vPos.y += 51;
	AddWidgetToState(eMainMenuState_Start,hplNew( cMainMenuWidget_MainButton,(mpInit,vPos,kTranslate("MainMenu","Exit"),eMainMenuState_Exit)) );
	
	
	///////////////////////////////////
	// New Game
	///////////////////////////////////

	vPos = vTextStart;//cVector3f(400, 260, 40);
	if(cInit::GetGameId() == eGameId_Requiem)
	{
		// Requiem: no difficulty, only whether to start
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("RequiemMenu","SureNewGame"),20,eFontAlign_Center)) );
		vPos.y += 34;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_NewGame,(mpInit,vPos,kTranslate("MainMenu","Yes"),20,eFontAlign_Center,eGameDifficulty_Normal)) );
		vPos.y += 29;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,20,eFontAlign_Center)) );
	}
	else
	{
		//AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","StartNewGame"),24,eFontAlign_Center));
		//vPos.y += 34;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_NewGame,(mpInit,vPos,kTranslate("MainMenu","Easy"),24,eFontAlign_Center,eGameDifficulty_Easy)) );
		vPos.y += 30;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","EasyDesc"),16,eFontAlign_Center)) );
		vPos.y += 42;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_NewGame,(mpInit,vPos,kTranslate("MainMenu","Normal"),24,eFontAlign_Center,eGameDifficulty_Normal)) );
		vPos.y += 30;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","NormalDesc"),16,eFontAlign_Center)) );
		vPos.y += 42;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_NewGame,(mpInit,vPos,kTranslate("MainMenu","Hard"),24,eFontAlign_Center,eGameDifficulty_Hard)) );
		vPos.y += 30;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","HardDesc"),16,eFontAlign_Center)) );
		vPos.y +=46;
		AddWidgetToState(eMainMenuState_NewGame,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Start,22,eFontAlign_Center)) );
	}

	///////////////////////////////////
	// Continue
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_Continue,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","ContinueLastSave"),20,eFontAlign_Center)) ); 
	vPos.y += 34;
	AddWidgetToState(eMainMenuState_Continue,hplNew( cMainMenuWidget_Continue,(mpInit,vPos,kTranslate("MainMenu","Yes"),20,eFontAlign_Center)) ); 
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_Continue,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,20,eFontAlign_Center)) );

	///////////////////////////////////
	// Load Game
	///////////////////////////////////

	for(size_t i=0; i< 3; ++i)
	{
		vPos = vTextStart;//cVector3f(400, 260, 40);
		
		eMainMenuState state = (eMainMenuState)(eMainMenuState_LoadGameSpot + i);
		
		///////////////////////////
		//Head
		AddWidgetToState(state,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","Load Game"),27,eFontAlign_Center)) ); 
		vPos.y += 42;
		vPos.x -= 110;
		
		///////////////////////////
		//Buttons
		// Requiem's auto saves are its quick saves; its names are in a
		// category of their own
		const bool bRequiem = cInit::GetGameId() == eGameId_Requiem;
		const tString sLoadCategory = bRequiem ? "RequiemMenu" : "MainMenu";
		const tString sAutoName = bRequiem ? "Quick Saves" : "Auto Saves";
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate(sLoadCategory,"Saved Games"),eMainMenuState_LoadGameSpot,25,eFontAlign_Center)) );
		vPos.y += 32;
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate(sLoadCategory,sAutoName),eMainMenuState_LoadGameAuto,25,eFontAlign_Center)) );
		vPos.y += 32;
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate(sLoadCategory,"Favorites"),eMainMenuState_LoadGameFavorite,25,eFontAlign_Center)) );
		
		///////////////////////////
		//Back
		vPos.y += 150;
		vPos.x += 130;
		vPos.y += 32;
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Start,23,eFontAlign_Center)) );
        
		///////////////////////////
		//Load type
		vPos = vTextStart;//cVector3f(400, 260, 40);
		vPos.y += 42;
		vPos.x += 185;
		tString sLoadType = "Saved Games";
		if(i==1)sLoadType = sAutoName;
		if(i==2)sLoadType = "Favorites";
		AddWidgetToState(state,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate(sLoadCategory,sLoadType)+_W(":"),21,eFontAlign_Center)) );
		
		///////////////////////////
		//Saved games list
		
		//Set up
		vPos = vTextStart;//cVector3f(400, 260 , 40);
		vPos.y += 46 + 30;
		vPos.x += 15;
		
		tWString sDir = _W("save/spot");
		if(i == 1)		sDir = _W("save/auto");
		else if(i == 2) sDir = _W("save/favorite");
		
		gpSaveGameList[i] = hplNew( cMainMenuWidget_SaveGameList,(
														mpInit,vPos,cVector2f(355,170),15,sDir,(int)i) );
		AddWidgetToState(state,gpSaveGameList[i]);
			
		iLowLevelResources *pLowLevelResources = mpInit->mpGame->GetResources()->GetLowLevel();
		iLowLevelSystem *pLowLevelSystem = mpInit->mpGame->GetSystem()->GetLowLevel();

		tWStringList lstFiles;
		tSaveFileEntrySet setSaveFiles;
		
		tWString sFullPath = mpInit->mpSaveHandler->GetSaveDir() + sDir;
		pLowLevelResources->FindFilesInDir(lstFiles,sFullPath,_W("*.sav"));

		tWStringListIt fileIt = lstFiles.begin();
		for(; fileIt != lstFiles.end(); ++fileIt)
		{
			tWString sFile = *fileIt;
			cDate date = FileModifiedDate(sFullPath+_W("/")+sFile);
            
			setSaveFiles.insert(cSaveFileEntry(sFile,date));
		}

		//Go through the sorted array and add to File vector and as list entries
		gvSaveGameFileVec[i].clear();
		tSaveFileEntrySetIt dateIt = setSaveFiles.begin();
		for(; dateIt != setSaveFiles.end(); ++dateIt)
		{
			const cSaveFileEntry &temp = *dateIt;

			tWString sFile = temp.msFile;

			gvSaveGameFileVec[i].push_back(sFile);

			sFile = cString::SetFileExtW(sFile,_W(""));
			sFile = cString::SubW(sFile,0, (int)sFile.length() - 3);
			sFile = cString::ReplaceCharToW(sFile,_W("_"),_W(" "));
			sFile = cString::ReplaceCharToW(sFile,_W("."),_W(":"));

			//TODO: PROBLEM!!!
			gpSaveGameList[i]->AddEntry(sFile);
			//gpSaveGameList[i]->AddEntry(sFile);
		}

		///////////////////////////
		//Save game buttons
		vPos.y += 170;
		vPos.x = vTextStart.x + 20;
		
		AddWidgetToState(state,hplNew( cMainMenuWidget_LoadSaveGame,(mpInit,vPos,kTranslate("MainMenu","Load"),17,eFontAlign_Left,sDir,(int)i)) ); 
		
		vPos.x += 70;
		if(i!=2)
			AddWidgetToState(state,hplNew( cMainMenuWidget_FavoriteSaveGame,(mpInit,vPos,kTranslate("MainMenu","Add To Favorites"),17,eFontAlign_Left,sDir,(int)i)) ); 
		
		vPos.x += 205;
		AddWidgetToState(state,hplNew( cMainMenuWidget_RemoveSaveGame,(mpInit,vPos,kTranslate("MainMenu","Remove"),17,eFontAlign_Left,sDir,(int)i)) ); 
		
		
        
	}

	///////////////////////////////////
	// Quit
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_Exit,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","SureQuit"),20,eFontAlign_Center)) ); 
	vPos.y += 34;
	AddWidgetToState(eMainMenuState_Exit,hplNew( cMainMenuWidget_Quit,(mpInit,vPos,kTranslate("MainMenu","Yes"),20,eFontAlign_Center)) ); 
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_Exit,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,20,eFontAlign_Center)) );

	///////////////////////////////////
	// Options
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Controls"),eMainMenuState_OptionsControls,25,eFontAlign_Center)) ); 
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Game"),eMainMenuState_OptionsGame,25,eFontAlign_Center)) );
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Sound"),eMainMenuState_OptionsSound,25,eFontAlign_Center)) );
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Graphics"),eMainMenuState_OptionsGraphics,25,eFontAlign_Center)) );
	vPos.y += 37;
	// The Dog's behaviour: only Overture has the Dog
	if(cInit::IsOverture())
	{
		AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,_W("AI"),eMainMenuState_OptionsAI,25,eFontAlign_Center)) );
		vPos.y += 37;
	}
	if(cInit::CanChangeGame())
	{
		AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,_W("Change Game"),eMainMenuState_OptionsChangeGame,25,eFontAlign_Center)) );
		vPos.y += 37;
	}
	AddWidgetToState(eMainMenuState_Options,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Start,25,eFontAlign_Center)) );

	///////////////////////////////////
	// Change game
	///////////////////////////////////
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,hplNew(cMainMenuWidget_Text,(mpInit,vPos,_W("Change Game"),25,eFontAlign_Center)));
	vPos.y += 45;
	for(int i=0; i<eGameId_LastEnum; ++i)
	{
		eGameId game = (eGameId)i;
		tWString sName = cString::To16Char(cInit::GetGameName(game));
		if(game == cInit::GetGameId())
			AddWidgetToState(eMainMenuState_OptionsChangeGame,hplNew(cMainMenuWidget_Text,(mpInit,vPos,sName + _W(" (playing)"),18,eFontAlign_Center)));
		else if(cInit::FindGameFolder(game) == "")
			AddWidgetToState(eMainMenuState_OptionsChangeGame,hplNew(cMainMenuWidget_Text,(mpInit,vPos,sName + _W(" (not found)"),18,eFontAlign_Center)));
		else
			AddWidgetToState(eMainMenuState_OptionsChangeGame,hplNew(cMainMenuWidget_ChangeGame,(mpInit,vPos,game)));
		vPos.y += 34;
	}
	vPos.y += 11;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,hplNew(cMainMenuWidget_Text,(mpInit,vPos,
		_W("The game closes and starts again as the other one. Anything not saved is lost."),16,eFontAlign_Center,nullptr,560)));
	vPos.y += 50;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,hplNew(cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Options,23,eFontAlign_Center)));

	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsAI,hplNew(cMainMenuWidget_Text,(mpInit,vPos,_W("AI - Dog behaviour"),25,eFontAlign_Center)));
	vPos.y += 45;
	AddWidgetToState(eMainMenuState_OptionsAI,hplNew(cMainMenuWidget_DogAI,(mpInit,vPos,false)));
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_OptionsAI,hplNew(cMainMenuWidget_DogAI,(mpInit,vPos,true)));
	vPos.y += 45;
	AddWidgetToState(eMainMenuState_OptionsAI,hplNew(cMainMenuWidget_Text,(mpInit,vPos,_W("Same original model and animations in both modes."),16,eFontAlign_Center)));
	vPos.y += 40;
	AddWidgetToState(eMainMenuState_OptionsAI,hplNew(cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Options,23,eFontAlign_Center)));

	///////////////////////////////////
	// Options Controls
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 260, 40);
	//Head
	AddWidgetToState(eMainMenuState_OptionsControls,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","Controls"),25,eFontAlign_Center)) ); 
	vPos.y += 37;
	//Buttons
	cMainMenuWidget *pWidgetInvertMouseY = hplNew( cMainMenuWidget_InvertMouseY,(mpInit,vPos,kTranslate("MainMenu","Invert Mouse Y:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsControls,pWidgetInvertMouseY); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetMouseSensitivity = hplNew( cMainMenuWidget_MouseSensitivity,(mpInit,vPos,kTranslate("MainMenu","Mouse Sensitivity:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsControls,pWidgetMouseSensitivity); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetToggleCrouch = hplNew( cMainMenuWidget_ToggleCrouch,(mpInit,vPos,kTranslate("MainMenu","Toggle Crouch:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsControls,pWidgetToggleCrouch); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetUseHaptics = nullptr;
	cMainMenuWidget *pWidgetInteractModeCameraSpeed = nullptr;
	cMainMenuWidget *pWidgetActionModeCameraSpeed = nullptr;
	cMainMenuWidget *pWidgetWeightForceScale = nullptr;
	if(mpInit->mbHapticsAvailable)
	{
		vPos.y+= 5;
		//Use haptics
		tWString sText = kTranslate("MainMenu","Use Haptics:");
		if(sText == _W("")) sText = _W("Use Haptics:");
		pWidgetUseHaptics = hplNew( cMainMenuWidget_UseHaptics,(mpInit,vPos,sText,20,eFontAlign_Right) );
		AddWidgetToState(eMainMenuState_OptionsControls,pWidgetUseHaptics); 
		vPos.y += 29;
		
		//Weight Force Scale
		sText = kTranslate("MainMenu","Weight Force Scale:");
		if(sText == _W("")) sText = _W("Weight Force Scale:");
		pWidgetWeightForceScale = hplNew( cMainMenuWidget_WeightForceScale,(mpInit,vPos,sText,20,eFontAlign_Right) );
		AddWidgetToState(eMainMenuState_OptionsControls,pWidgetWeightForceScale); 
		vPos.y += 29;

		//InteractMode camera speed
		sText = kTranslate("MainMenu","InteractMode Camera Speed:");
		if(sText == _W("")) sText = _W("InteractMode Camera Speed:");
		pWidgetInteractModeCameraSpeed = hplNew( cMainMenuWidget_InteractModeCameraSpeed,(mpInit,vPos,sText,20,eFontAlign_Right) );
		AddWidgetToState(eMainMenuState_OptionsControls,pWidgetInteractModeCameraSpeed); 
		vPos.y += 29;

		//ActionMode camera speed
		sText = kTranslate("MainMenu","ActionMode Camera Speed:");
		if(sText == _W("")) sText = _W("ActionMode Camera Speed:");
		pWidgetActionModeCameraSpeed = hplNew( cMainMenuWidget_ActionModeCameraSpeed,(mpInit,vPos,sText,20,eFontAlign_Right) );
		AddWidgetToState(eMainMenuState_OptionsControls,pWidgetActionModeCameraSpeed); 
		vPos.y += 29;
		
		vPos.y+= 5;
	}
	cMainMenuWidget *pWidgetChangeKeyConf = hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Change Key Mapping"),eMainMenuState_OptionsKeySetupMove,20,eFontAlign_Center) );
	AddWidgetToState(eMainMenuState_OptionsControls,pWidgetChangeKeyConf); 
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsControls,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Options,23,eFontAlign_Center) )); 

	//Text
	vPos = cVector3f(vTextStart.x+12, vTextStart.y+37, vTextStart.z);

	sText = mpInit->mpButtonHandler->mbInvertMouseY ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpInvertMouseYText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left,pWidgetInvertMouseY) );
	AddWidgetToState(eMainMenuState_OptionsControls,gpInvertMouseYText); 

	vPos.y += 29;
	sprintf(sTempVec,"%.1f",mpInit->mpButtonHandler->mfMouseSensitivity);
	sText = cString::To16Char(sTempVec);
	gpMouseSensitivityText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left,pWidgetMouseSensitivity) );
	AddWidgetToState(eMainMenuState_OptionsControls,gpMouseSensitivityText); 

	vPos.y += 29;
	sText = mpInit->mpButtonHandler->mbToggleCrouch ? kTranslate("MainMenu","On"): kTranslate("MainMenu","Off");
	gpToggleCrouchText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left,pWidgetToggleCrouch) );
	AddWidgetToState(eMainMenuState_OptionsControls,gpToggleCrouchText); 

	if(mpInit->mbHapticsAvailable)
	{
		vPos.y+= 5;
		vPos.y += 29;
		sText = mpInit->mbHasHapticsOnRestart ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
		gpUseHapticsText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsControls,gpUseHapticsText);
		gpUseHapticsText->SetExtraWidget(pWidgetUseHaptics);
		
		vPos.y += 29;
		sprintf(sTempVec,"%.1f",mpInit->mfHapticForceMul);
		sText = cString::To16Char(sTempVec);
		gpWidgetWeightForceScaleText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left,pWidgetWeightForceScale) );
		AddWidgetToState(eMainMenuState_OptionsControls,gpWidgetWeightForceScaleText); 

		vPos.y += 29;
		sprintf(sTempVec,"%.1f",mpInit->mpPlayer->GetHapticCamera()->GetInteractModeCameraSpeed());
		sText = cString::To16Char(sTempVec);
		gpWidgetInteractModeCameraSpeedText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left,pWidgetInteractModeCameraSpeed) );
		AddWidgetToState(eMainMenuState_OptionsControls,gpWidgetInteractModeCameraSpeedText); 

		vPos.y += 29;
		sprintf(sTempVec,"%.1f",mpInit->mpPlayer->GetHapticCamera()->GetActionModeCameraSpeed());
		sText = cString::To16Char(sTempVec);
		gpWidgetActionModeCameraSpeedText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left,pWidgetActionModeCameraSpeed) );
		AddWidgetToState(eMainMenuState_OptionsControls,gpWidgetActionModeCameraSpeedText); 
	}

	///////////////////////////////////
	// Options Key Setup General stuff
	///////////////////////////////////
	for(int i=0; i<3; ++i)
	{
		eMainMenuState state = (eMainMenuState)(i+ eMainMenuState_OptionsKeySetupMove);
		cVector3f vPos = vTextStart;//cVector3f(400, 260, 40);
		//Head
		AddWidgetToState(state,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","Configure Keys"),25,eFontAlign_Center)) ); 
		vPos.y += 42;
		vPos.x -= 110;
		//Buttons
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Movement"),eMainMenuState_OptionsKeySetupMove,25,eFontAlign_Center)) ); 
		vPos.y += 32;
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Actions"),eMainMenuState_OptionsKeySetupAction,25,eFontAlign_Center)) ); 
		vPos.y += 32;
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Misc"),eMainMenuState_OptionsKeySetupMisc,25,eFontAlign_Center)) ); 
		//Back
		vPos.y += 150;
		vPos.x += 130;
		AddWidgetToState(state,hplNew( cMainMenuWidget_KeyReset,(mpInit,vPos,kTranslate("MainMenu","Reset to defaults"),23,eFontAlign_Center)) );
		vPos.y += 32;
		AddWidgetToState(state,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_OptionsControls,23,eFontAlign_Center)) );
	}

	///////////////////////////////////
	// Options Key Setup Move
	///////////////////////////////////

	cMainMenuWidget *pWidgetKeyButton;
	float fKeyTextXAdd = 195;

	//Key buttons
	cInput *pInput = mpInit->mpGame->GetInput();
	vPos = vTextStart;//cVector3f(400, 260, 40);
	vPos.y += 46;
	vPos.x += 15;

	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Forward:"),
													18,eFontAlign_Left,pTempTextWidget,"Forward") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Backward:"),
												18,eFontAlign_Left,pTempTextWidget,"Backward") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Strafe Left:"),
					18,eFontAlign_Left,pTempTextWidget,"Left") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Strafe Right:"),
					18,eFontAlign_Left,pTempTextWidget,"Right") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Run:"),
					18,eFontAlign_Left,pTempTextWidget,"Run") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Crouch:"),
					18,eFontAlign_Left,pTempTextWidget,"Crouch") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Jump:"),
					18,eFontAlign_Left,pTempTextWidget,"Jump") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Lean Left:"),
					18,eFontAlign_Left,pTempTextWidget,"LeanLeft") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	
	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pTempTextWidget); 
	pWidgetKeyButton = hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Lean Right:"),
					18,eFontAlign_Left,pTempTextWidget,"LeanRight") );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMove,pWidgetKeyButton); 
	

	///////////////////////////////////
	// Options Key Setup Action
	///////////////////////////////////
	//Key buttons
	vPos = vTextStart;//cVector3f(400, 260, 40);
	vPos.y += 46;
	vPos.x += 15;

	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Interact:"),
		18,eFontAlign_Left,pTempTextWidget,"Interact")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Examine:"),
		18,eFontAlign_Left,pTempTextWidget,"Examine")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","InteractMode:"),
		18,eFontAlign_Left,pTempTextWidget,"InteractMode")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Holster:"),
		18,eFontAlign_Left,pTempTextWidget,"Holster")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupAction,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","LookMode:"),
		18,eFontAlign_Left,pTempTextWidget,"LookMode")) ); 

	if(!cInit::IsOverture())
	{
		vPos.y += 23;
		pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsKeySetupAction,pTempTextWidget);
		AddWidgetToState(eMainMenuState_OptionsKeySetupAction,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Rotate:"),
			18,eFontAlign_Left,pTempTextWidget,"Rotate")) );
	}


	///////////////////////////////////
	// Options Key Setup Misc
	///////////////////////////////////
	//Key buttons
	vPos = vTextStart;//cVector3f(400, 260, 40);
	vPos.y += 46;
	vPos.x += 15;

	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Inventory:"),
		18,eFontAlign_Left,pTempTextWidget,"Inventory")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Notebook:"),
		18,eFontAlign_Left,pTempTextWidget,"NoteBook")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Pers. Notes:"),
		18,eFontAlign_Left,pTempTextWidget,"PersonalNotes")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Flashlight:"),
		18,eFontAlign_Left,pTempTextWidget,"Flashlight")) ); 

	vPos.y += 23;
	pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget); 
	AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("MainMenu","Glowstick:"),
		18,eFontAlign_Left,pTempTextWidget,"GlowStick")) ); 

	// Requiem: quicksave and quickload
	if(cInit::GetGameId() == eGameId_Requiem)
	{
		vPos.y += 23;
		pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget);
		AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("RequiemMenu","Quicksave:"),
			18,eFontAlign_Left,pTempTextWidget,"SaveGame")) );

		vPos.y += 23;
		pTempTextWidget = hplNew( cMainMenuWidget_Text,(mpInit,vPos+cVector3f(fKeyTextXAdd,0,0),_W(""),18,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,pTempTextWidget);
		AddWidgetToState(eMainMenuState_OptionsKeySetupMisc,hplNew( cMainMenuWidget_KeyButton,(mpInit,vPos,kTranslate("RequiemMenu","Quickload:"),
			18,eFontAlign_Left,pTempTextWidget,"LoadGame")) );
	}

	///////////////////////////////////
	// Options Game
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 260, 40);
	//Head
	AddWidgetToState(eMainMenuState_OptionsGame,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","Game"),25,eFontAlign_Center)) ); 
	vPos.y += 37;
	//Buttons
	cMainMenuWidget *pWidgetLanguage = hplNew( cMainMenuWidget_Language,(mpInit,vPos,kTranslate("MainMenu","Language:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGame,pWidgetLanguage); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetSubtitles = hplNew( cMainMenuWidget_Subtitles,(mpInit,vPos,kTranslate("MainMenu","Subtitle:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGame,pWidgetSubtitles); 
	vPos.y += 50;
	// Requiem has no difficulty
	const bool bDifficultyOption = cInit::GetGameId() != eGameId_Requiem;
	cMainMenuWidget *pWidgetDifficulty = nullptr;
	if(bDifficultyOption)
	{
		pWidgetDifficulty = hplNew( cMainMenuWidget_Difficulty,(mpInit,vPos,kTranslate("MainMenu","Difficulty:"),20,eFontAlign_Right) );
		AddWidgetToState(eMainMenuState_OptionsGame,pWidgetDifficulty);
		vPos.y += 29;
	}
	if(mpInit->mbSimpleSwingInOptions)
	{
		AddWidgetToState(eMainMenuState_OptionsGame,hplNew( cMainMenuWidget_SimpleSwing,(mpInit,vPos,kTranslate("MainMenu","SimpleSwing:"),20,eFontAlign_Right)) ); 
		vPos.y += 29;
	}
	//AddWidgetToState(eMainMenuState_OptionsGame,hplNew( cMainMenuWidget_AllowQuickSave(mpInit,vPos,kTranslate("MainMenu","AllowQuickSave:"),20,eFontAlign_Right)); 
	//vPos.y += 29;
	tWString sCrosshairText = kTranslate("MainMenu","Show Crosshair:");
	if(sCrosshairText == _W("")) sCrosshairText = _W("Show Crosshair:");
	cMainMenuWidget *pWidgetShowCrossHair = hplNew( cMainMenuWidget_ShowCrossHair,(mpInit,vPos,sCrosshairText,20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGame,pWidgetShowCrossHair); 
	vPos.y += 29;
		
	cMainMenuWidget *pWidgetFlashItems = hplNew( cMainMenuWidget_FlashItems,(mpInit,vPos,kTranslate("MainMenu","FlashItems:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGame,pWidgetFlashItems); 
	vPos.y += 29;
	
	cMainMenuWidget *pWidgetDisablePersonal = hplNew( cMainMenuWidget_DisablePersonal,(mpInit,vPos,kTranslate("MainMenu","DisablePersonal:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGame,pWidgetDisablePersonal); 
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsGame,hplNew( cMainMenuWidget_GfxBack,(mpInit,vPos,kTranslate("MainMenu","Back"),23,eFontAlign_Center)) ); 

	//Text
	vPos = cVector3f(vTextStart.x+12, vTextStart.y+37, vTextStart.z);

	sText = cString::To16Char(cString::SetFileExt(mpInit->msLanguageFile,""));
	gpLanguageText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGame,gpLanguageText); 
	gpLanguageText->SetExtraWidget(pWidgetLanguage);

	vPos.y += 29;
	sText = mpInit->mbSubtitles ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpSubtitlesText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGame,gpSubtitlesText); 
	gpSubtitlesText->SetExtraWidget(pWidgetSubtitles);
	
	vPos.y +=25;
	AddWidgetToState(eMainMenuState_OptionsGame,hplNew( cMainMenuWidget_Text,(mpInit,cVector3f(vPos-cVector3f(12,0,0)),
										kTranslate("MainMenu","VoiceLanguange:"),12,eFontAlign_Right)) );
	AddWidgetToState(eMainMenuState_OptionsGame,hplNew( cMainMenuWidget_Text,(mpInit,cVector3f(vPos),
										kTranslate("MainMenu","SetThisToLanguageOfVoice"),12,eFontAlign_Left)) );
	vPos.y += 25;
	
	if(bDifficultyOption)
	{
		sText = kTranslate("MainMenu",gvDifficultyLevel[mpInit->mDifficulty]);
		gpDifficultyText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsGame,gpDifficultyText);
		gpDifficultyText->SetExtraWidget(pWidgetDifficulty);
		vPos.y += 29;
	}

	if(mpInit->mbSimpleSwingInOptions)
	{
		sText = mpInit->mbSimpleWeaponSwing ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
		gpSimpleSwingText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsGame,gpSimpleSwingText); 
		vPos.y += 29;
	}
	
	//sText = mpInit->mbAllowQuickSave ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	//gpAllowQuickSaveText = hplNew( cMainMenuWidget_Text, (mpInit,vPos,sText,20,eFontAlign_Left) );
	//AddWidgetToState(eMainMenuState_OptionsGame,gpAllowQuickSaveText); 
	//vPos.y += 29;
	sText = mpInit->mbShowCrossHair ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpShowCrossHairText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGame,gpShowCrossHairText);
	gpShowCrossHairText->SetExtraWidget(pWidgetShowCrossHair);
	vPos.y += 29;
	
	sText = mpInit->mbFlashItems ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpFlashItemsText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGame,gpFlashItemsText);
	gpFlashItemsText->SetExtraWidget(pWidgetFlashItems);
	
	vPos.y += 29;
	sText = mpInit->mbDisablePersonalNotes ? kTranslate("MainMenu","Off") : kTranslate("MainMenu","On");
	gpDisablePersonalText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGame,gpDisablePersonalText); 
	gpDisablePersonalText->SetExtraWidget(pWidgetDisablePersonal);

	///////////////////////////////////
	// Options Sound
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 230, 40);
	//Head
	AddWidgetToState(eMainMenuState_OptionsSound,hplNew( cMainMenuWidget_Text, (mpInit,vPos,kTranslate("MainMenu","Sound"),25,eFontAlign_Center)) ); 
	vPos.y += 37;
	
	//Buttons
	cMainMenuWidget *pWidgetSoundVolume = hplNew( cMainMenuWidget_SoundVolume, (mpInit,vPos,kTranslate("MainMenu","Sound Volume:"),20,eFontAlign_Right) );  
	AddWidgetToState(eMainMenuState_OptionsSound,pWidgetSoundVolume); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetSoundHardware = hplNew( cMainMenuWidget_SoundHardware, (mpInit,vPos,kTranslate("MainMenu","Use Hardware:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsSound,pWidgetSoundHardware); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetSoundOutputDevice = hplNew( cMainMenuWidget_SoundOutputDevice, (mpInit,vPos,kTranslate("MainMenu","Output Device:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsSound,pWidgetSoundOutputDevice);
	vPos.y += 29;
	cMainMenuWidget *pWidgetSoundEFX = hplNew( cMainMenuWidget_SoundEFX, (mpInit,vPos,_W("EFX:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsSound,pWidgetSoundEFX);
	vPos.y += 35;
    AddWidgetToState(eMainMenuState_OptionsSound,hplNew( cMainMenuWidget_GfxBack, (mpInit,vPos,kTranslate("MainMenu","Back"),23,eFontAlign_Center)) ); 


	//Text
	vPos = cVector3f(vTextStart.x+12, vTextStart.y+37, vTextStart.z);

	sprintf(sTempVec,"%.0f",mpInit->mpGame->GetSound()->GetLowLevel()->GetVolume()*100);
	sText = cString::To16Char(sTempVec);
	gpSoundVolumeText = hplNew( cMainMenuWidget_Text, (mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsSound,gpSoundVolumeText); 
	gpSoundVolumeText->SetExtraWidget(pWidgetSoundVolume);

	vPos.y += 29;
	sText = mpInit->mbUseSoundHardware ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpSoundHardwareText = hplNew( cMainMenuWidget_Text, (mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsSound,gpSoundHardwareText); 
	gpSoundHardwareText->SetExtraWidget(pWidgetSoundHardware);

	vPos.y += 29;
	// Set the default to what's really being used
	mpInit->msDeviceName = tString(OAL_Info_GetDeviceName());

	sText = cString::To16Char(mpInit->msDeviceName);
	gpSoundOutputDevice = hplNew( cMainMenuWidget_Text, (mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsSound,gpSoundOutputDevice);
	gpSoundOutputDevice->SetExtraWidget(pWidgetSoundOutputDevice);

	vPos.y += 29;
	sText = mpInit->mbUseEnvAudio ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpSoundEFXText = hplNew( cMainMenuWidget_Text, (mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsSound,gpSoundEFXText);
	gpSoundEFXText->SetExtraWidget(pWidgetSoundEFX);

	///////////////////////////////////
	// Options Graphics
	///////////////////////////////////
	vPos = vTextStart;//cVector3f(400, 230, 40);
	//Head
	AddWidgetToState(eMainMenuState_OptionsGraphics,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","Graphics"),25,eFontAlign_Center)) ); 
	vPos.y += 37;
	/*AddWidgetToState(eMainMenuState_OptionsGraphics,hplNew( cMainMenuWidget_Image, (mpInit,
													cVector3f(400,vPos.y,30),
													cVector2f(200,150),
													"menu_gamma.bmp",
													"diffalpha2d",
													cColor(1,1))) );
	
	//Buttons
	vPos.x -= 130;*/
	cMainMenuWidget *pWidgetPreset = hplNew( cMainMenuWidget_GraphicsPreset,(mpInit,vPos,_W("Preset:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetPreset);
	vPos.y += 29;
	cMainMenuWidget *pWidgetResolution = hplNew( cMainMenuWidget_Resolution,(mpInit,vPos,kTranslate("MainMenu","Resolution:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetResolution); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetFullscreen = hplNew( cMainMenuWidget_Fullscreen,(mpInit,vPos,_W("Fullscreen:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetFullscreen); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetDisplay = hplNew( cMainMenuWidget_Display,(mpInit,vPos,_W("Display:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetDisplay); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetNoiseFilter = hplNew( cMainMenuWidget_NoiseFilter,(mpInit,vPos,kTranslate("MainMenu","Noise Filter:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetNoiseFilter); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetBloom = hplNew( cMainMenuWidget_Bloom,(mpInit,vPos,kTranslate("MainMenu","Bloom:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetBloom); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetGamma = hplNew( cMainMenuWidget_Gamma,(mpInit,vPos,kTranslate("MainMenu","Gamma:"),20,eFontAlign_Right,0) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetGamma); 
	vPos.y += 29;
	cMainMenuWidget *pWidgetFOV = hplNew( cMainMenuWidget_FOV,(mpInit,vPos,_W("Field of View:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pWidgetFOV); 

	//vPos.x = 400;
	//vPos.y = 230 + 150;
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsGraphics,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Advanced"),eMainMenuState_OptionsGraphicsAdvanced,23,eFontAlign_Center)) );

	vPos.y += 31;
	AddWidgetToState(eMainMenuState_OptionsGraphics,hplNew( cMainMenuWidget_Button,(mpInit,vPos,_W("Lighting"),eMainMenuState_OptionsGraphicsLighting,23,eFontAlign_Center)) );

	vPos.y += 31;
	AddWidgetToState(eMainMenuState_OptionsGraphics,hplNew( cMainMenuWidget_GfxBack,(mpInit,vPos,kTranslate("MainMenu","Back"),23,eFontAlign_Center)) ); 


	//Text
	vPos = cVector3f(vTextStart.x+12, vTextStart.y+37, vTextStart.z);

	cMainMenuWidget_Text *pPresetText = hplNew( cMainMenuWidget_GraphicsPresetText,(mpInit,vPos,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,pPresetText);
	pPresetText->SetExtraWidget(pWidgetPreset);

	vPos.y += 29;
	sText = GetResolutionText(mpInit);
	gpResolutionText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpResolutionText); 
	gpResolutionText->SetExtraWidget(pWidgetResolution);

	vPos.y += 29;
	sText = mpInit->mbFullScreenSetting ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpFullscreenText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpFullscreenText); 
	gpFullscreenText->SetExtraWidget(pWidgetFullscreen);

	vPos.y += 29;
	sText = GetDisplayText(mpInit);
	gpDisplayText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpDisplayText); 
	gpDisplayText->SetExtraWidget(pWidgetDisplay);

	vPos.y += 29;
	sText = GetNoiseFilterText(mpInit);
	gpNoiseFilterText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left ));
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpNoiseFilterText); 
	gpNoiseFilterText->SetExtraWidget(pWidgetNoiseFilter);

	vPos.y += 29;
	sText = GetBloomText(mpInit);
	gpBloomText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpBloomText);
	gpBloomText->SetExtraWidget(pWidgetBloom);

	vPos.y += 29;
	sprintf(sTempVec,"%.1f",mpInit->mpGame->GetGraphics()->GetLowLevel()->GetGammaCorrection());
	sText = cString::To16Char(sTempVec);
	gpGammaText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpGammaText);
	gpGammaText->SetExtraWidget(pWidgetGamma);

	vPos.y += 29;
	sprintf(sTempVec,"%.0f",mpInit->mpPlayer->GetViewFOV());
	sText = cString::To16Char(sTempVec);
	gpFOVText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphics,gpFOVText);
	gpFOVText->SetExtraWidget(pWidgetFOV);


    
	///////////////////////////////////
	// Options Advanced Graphics
	///////////////////////////////////
	vPos = vTextStart + cVector3f(40,0,0);//cVector3f(400, 260, 40);
	//Head
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","Advanced Graphics"),25,eFontAlign_Center)) ); 
	vPos.y += 37;
	
	//Buttons
	cMainMenuWidget *pTextureQualityButton = hplNew( cMainMenuWidget_TextureQuality,(mpInit,vPos,kTranslate("MainMenu","Texture Quality:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pTextureQualityButton); 
	vPos.y += 22;
	cMainMenuWidget *pShadowsButton = hplNew( cMainMenuWidget_Shadows,(mpInit,vPos,kTranslate("MainMenu","Shadows:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pShadowsButton); 
	vPos.y += 22;
	cMainMenuWidget *pPostEffectsButton = hplNew( cMainMenuWidget_PostEffects,(mpInit,vPos,kTranslate("MainMenu","Post Effects:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pPostEffectsButton); 
	vPos.y += 22;
	cMainMenuWidget *pMotionBlurButton = hplNew( cMainMenuWidget_MotionBlur,(mpInit,vPos,kTranslate("MainMenu","Motion Blur:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pMotionBlurButton); 
	vPos.y += 22;
	cMainMenuWidget *pVSyncButton = hplNew( cMainMenuWidget_VSync,(mpInit,vPos,kTranslate("MainMenu","VSync:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pVSyncButton); 
	vPos.y += 22;
	cMainMenuWidget *pLimitFPSButton = hplNew( cMainMenuWidget_LimitFPS,(mpInit,vPos,_W("Limit to 60 FPS:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pLimitFPSButton); 
	vPos.y += 22;
	cMainMenuWidget *pTextureFilterButton = hplNew( cMainMenuWidget_TextureFilter,(mpInit,vPos,kTranslate("MainMenu","Texture Filter:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pTextureFilterButton); 
	vPos.y += 22;
	cMainMenuWidget *pFSAAButton = hplNew( cMainMenuWidget_FSAA,(mpInit,vPos,kTranslate("MainMenu","Anti-Aliasing:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pFSAAButton); 
	vPos.y += 22;
	cMainMenuWidget *pSmaaButton = hplNew( cMainMenuWidget_Smaa,(mpInit,vPos,_W("Edge Smoothing (SMAA):"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pSmaaButton); 
	vPos.y += 22;
	cMainMenuWidget *pRenderScaleButton = hplNew( cMainMenuWidget_RenderScale,(mpInit,vPos,_W("Render Scale:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pRenderScaleButton); 
	vPos.y += 22;
	cMainMenuWidget *pAmbientOcclusionButton = hplNew( cMainMenuWidget_AmbientOcclusion,(mpInit,vPos,_W("Ambient Occlusion:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pAmbientOcclusionButton); 
	vPos.y += 22;
	cMainMenuWidget *pDOFButton = hplNew( cMainMenuWidget_DOF,(mpInit,vPos,kTranslate("MainMenu","Depth of Field:"),20,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,pDOFButton); 
	vPos.y += 31;
    AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_OptionsGraphics,23,eFontAlign_Center)) );

	//Text
	vPos = cVector3f(vTextStart.x+12, vTextStart.y+37, vTextStart.z) + cVector3f(40,0,0);

	sText = kTranslate("MainMenu",gvTextureQuality[mpInit->mpGame->GetResources()->GetMaterialManager()->GetTextureSizeLevel()]);
	gpTextureQualityText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpTextureQualityText); 
	gpTextureQualityText->SetExtraWidget(pTextureQualityButton);

	vPos.y += 22;
	sText = GetShadowOptionText(mpInit, mpInit->mpGame->GetGraphics()->GetRenderer3D()->GetShowShadows());
	gpShadowsText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpShadowsText); 
	gpShadowsText->SetExtraWidget(pShadowsButton);

	vPos.y += 22;
	sText = mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetActive() ?	kTranslate("MainMenu","On") : 
																					kTranslate("MainMenu","Off");
	gpPostEffectsText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left ));
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpPostEffectsText);
	gpPostEffectsText->SetExtraWidget(pPostEffectsButton);

	vPos.y += 22;
	sText = GetMotionBlurText(mpInit);
	gpMotionBlurText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpMotionBlurText);
	gpMotionBlurText->SetExtraWidget(pMotionBlurButton);

	vPos.y += 22;
	sText = mpInit->mbVsync ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpVSyncText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpVSyncText);
	gpVSyncText->SetExtraWidget(pVSyncButton);

	vPos.y += 22;
	sText = mpInit->mpGame->GetLimitFPS() ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
	gpLimitFPSText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpLimitFPSText);
	gpLimitFPSText->SetExtraWidget(pLimitFPSButton);

	vPos.y += 22;
	gpTextureFilterText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,GetTextureFilterText(mpInit),20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpTextureFilterText);
	gpTextureFilterText->SetExtraWidget(pTextureFilterButton);

	vPos.y += 22;
	int lFSAA = mpInit->mlFSAA;
	if(lFSAA!=0)	sText = cString::To16Char(cString::ToString(lFSAA)+"x");
	else			sText = kTranslate("MainMenu","Off");
	gpFSAAText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpFSAAText);
	gpFSAAText->SetExtraWidget(pFSAAButton);

	vPos.y += 22;
	gpSmaaText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,GetSmaaText(mpInit),20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpSmaaText);
	gpSmaaText->SetExtraWidget(pSmaaButton);

	vPos.y += 22;
	gpRenderScaleText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,GetRenderScaleText(mpInit->mpGame->GetGraphics()->GetRenderScale()),20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpRenderScaleText);
	gpRenderScaleText->SetExtraWidget(pRenderScaleButton);


	vPos.y += 22;
	gpAmbientOcclusionText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,GetAmbientOcclusionText(mpInit),20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpAmbientOcclusionText);
	gpAmbientOcclusionText->SetExtraWidget(pAmbientOcclusionButton);

	vPos.y += 22;
	sText = GetDoFText(mpInit);

	gpDoFText = hplNew( cMainMenuWidget_Text,(mpInit,vPos,sText,20,eFontAlign_Left) );
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,gpDoFText);
	gpDoFText->SetExtraWidget(pDOFButton);


	///////////////////////////////////
	// Options Lighting
	///////////////////////////////////
	{
		vPos = vTextStart;
		AddWidgetToState(eMainMenuState_OptionsGraphicsLighting,hplNew( cMainMenuWidget_Text,(mpInit,vPos,_W("Lighting"),25,eFontAlign_Center)) );
		vPos.y += 37;

		cMainMenuWidget *pRefractionsButton = hplNew( cMainMenuWidget_Refractions,(mpInit,vPos,TranslateOr(mpInit,"Refractions:",_W("Refractions:")),20,eFontAlign_Right) );
		AddWidgetToState(eMainMenuState_OptionsGraphicsLighting,pRefractionsButton);
		sText = mpInit->mpGame->GetGraphics()->GetRenderer3D()->GetRefractionUsed() ?
					kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
		gpRefractionsText = hplNew( cMainMenuWidget_Text,(mpInit,vPos + cVector3f(12,0,0),sText,20,eFontAlign_Left) );
		AddWidgetToState(eMainMenuState_OptionsGraphicsLighting,gpRefractionsText);
		gpRefractionsText->SetExtraWidget(pRefractionsButton);
		vPos.y += 29;

		vPos.y += 6;
		AddWidgetToState(eMainMenuState_OptionsGraphicsLighting,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_OptionsGraphics,23,eFontAlign_Center)) );
	}

	///////////////////////////////////
	// Graphics Restart
	///////////////////////////////////

	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_GraphicsRestart,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","GraphicsRestart"),16,eFontAlign_Center,
						nullptr,400)) ); 
	vPos.y += 42;
	AddWidgetToState(eMainMenuState_GraphicsRestart,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","OK"),eMainMenuState_Options,22,eFontAlign_Center))); 

	///////////////////////////////////
	// Tutorial questions (Black Plague)
	///////////////////////////////////

	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_AskTutorial,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","AskTutorial"),16,eFontAlign_Center,
						nullptr,400)) );
	vPos.y += 42;
	AddWidgetToState(eMainMenuState_AskTutorial,hplNew( cMainMenuWidget_AskTutYes,(mpInit,vPos,kTranslate("MainMenu","Yes"),22,eFontAlign_Center)));
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_AskTutorial,hplNew( cMainMenuWidget_AskTutNo,(mpInit,vPos,kTranslate("MainMenu","No"),22,eFontAlign_Center)));

	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_AskStartTutorial,hplNew( cMainMenuWidget_Text,(mpInit,vPos,kTranslate("MainMenu","AskStartTutorial"),16,eFontAlign_Center,
						nullptr,400)) );
	vPos.y += 42;
	AddWidgetToState(eMainMenuState_AskStartTutorial,hplNew( cMainMenuWidget_AskTutYes,(mpInit,vPos,kTranslate("MainMenu","Yes"),22,eFontAlign_Center)));
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_AskStartTutorial,hplNew( cMainMenuWidget_Button,(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,22,eFontAlign_Center)));
	
}

//-----------------------------------------------------------------------

void cMainMenu::AddWidgetToState(eMainMenuState aState, cMainMenuWidget *apWidget)
{
	mlstWidgets.push_back(apWidget);
	mvStateWidgets[aState].push_back(apWidget);
}

//---------------------------------------------------------------------
