// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MainMenu.h"

#include "core/Init.h"
#include "core/GraphicsHelper.h"
#include "core/ButtonHandler.h"
#include "core/MapHandler.h"
#include "core/SaveHandler.h"
#include "core/LampAppearanceController.h"
#include "core/GameLauncher.h"
#include "core/RefView.h"
#include "ui/IntroStory.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/EffectHandler.h"
#include "ui/GraphicsPresets.h"


#include <algorithm>
#include <functional>

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

// WIDGET

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

// MAIN BUTTON

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

void cMainMenuWidget_MainButton::OnMouseOver(bool abOver)
{
	mbOver = abOver;

	if(abOver){
		mpInit->mpMainMenu->SetButtonTip(msTip);
	}
}

void cMainMenuWidget_MainButton::OnMouseDown(eMButton aButton)
{
	mpInit->mpMainMenu->SetState(mNextState);
	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);
}

void cMainMenuWidget_MainButton::OnDraw()
{
	mpFont->DrawPlain(mvWidgetPos,mvFontSize,cColor(0.62f + mfAlpha*0.3f,1),eFontAlign_Center,msText.c_str());

	float fAdd = sin(mfOverTimer) * 16.0f;

	if(mfAlpha >0)
	{
		mpFont->DrawPlain(mvWidgetPos + cVector3f(fAdd,0,-1),mvFontSize,cColor(0.56f,0.35f * mfAlpha),eFontAlign_Center,msText.c_str());
		mpFont->DrawPlain(mvWidgetPos + cVector3f(-fAdd,0,-1),mvFontSize,cColor(0.56f,0.35f * mfAlpha),eFontAlign_Center,msText.c_str());
	}
}

// BUTTON

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

void cMainMenuWidget_Button::OnMouseOver(bool abOver)
{
	mbOver = abOver;

	if(mbOver){
		mpInit->mpMainMenu->SetButtonTip(msTip);
	}
}

void cMainMenuWidget_Button::OnMouseDown(eMButton aButton)
{
	mpInit->mpMainMenu->SetState(mNextState);
	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);
}

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

// TEXT

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

void cMainMenuWidget_Text::OnMouseDown(eMButton aButton)
{
	if(mpExtra)
	{
		mpExtra->OnMouseDown(aButton);
	}
}

void cMainMenuWidget_Text::OnMouseOver(bool abOver)
{

	mbOver = abOver;

	if(mpExtra)
	{
		if(mpExtra->mbOver==false)
			mpExtra->OnMouseOver(abOver);
	}
}

// IMAGE

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

void cMainMenuWidget_Image::OnDraw()
{
	mpDrawer->DrawGfxObject(mpImage,mvWidgetPos,mvSize,mColor);
}

// LIST

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

void cMainMenuWidget_List::OnUpdate(float afTimeStep)
{
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

void cMainMenuWidget_List::OnMouseOver(bool abOver)
{
	if(abOver==false){
		mbSlideButtonPressed = false;
		mfSlideButtonMove =0;
	}
}

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
			mpFont->DrawPlain(vPos,mvFontSize,cColor(0.95f,1),eFontAlign_Left,mvEntries[i].c_str());
			mpDrawer->DrawGfxObject(mpBackGfx,vPos+cVector3f(0,2,-1),
									cVector2f(mvSize.x-5, mvFontSize.y), 
									cColor(0.0f,0.0f,0.73f,1));
		}
		else
			mpFont->DrawPlain(vPos,mvFontSize,cColor(0.7f,1),eFontAlign_Left,mvEntries[i].c_str());

		vPos.y += mvFontSize.y+2;
	}
}

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

void cMainMenuWidget_List::AddEntry(const tWString &asText)
{
	mvEntries.push_back(asText);
	if(mlSelected==-1 && mvEntries.size()==1) mlSelected =0;
}

const tWString& cMainMenuWidget_List::GetSelectedEntry()
{
	return mvEntries[0];
}

// NEW GAME

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

// TUTORIAL (Black Plague)

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

// CONTINUE

cMainMenuWidget_Continue::cMainMenuWidget_Continue(cInit *apInit, const cVector3f &avPos, 
												 const tWString& asText,cVector2f avFontSize, eFontAlign aAlignment)
												 : cMainMenuWidget_Button(apInit,avPos,asText,
												 eMainMenuState_LastEnum,avFontSize,aAlignment)
{

}

void cMainMenuWidget_Continue::OnMouseDown(eMButton aButton)
{
	
	mpInit->mpMainMenu->SetActive(false);

	mpInit->mpSaveHandler->AutoLoadLatestAutoOrSpot();
}

// QUIT

cMainMenuWidget_Quit::cMainMenuWidget_Quit(cInit *apInit, const cVector3f &avPos, 
												 const tWString& asText,
												 cVector2f avFontSize, eFontAlign aAlignment)
												 : cMainMenuWidget_Button(apInit,avPos,
												 asText,eMainMenuState_LastEnum,avFontSize,aAlignment)
{

}

void cMainMenuWidget_Quit::OnMouseDown(eMButton aButton)
{
	mpInit->mpGame->Exit();
}

// RESUME

cMainMenuWidget_Resume::cMainMenuWidget_Resume(cInit *apInit, const cVector3f &avPos, 
										   const tWString& asText)
										   : cMainMenuWidget_MainButton(apInit,avPos,asText,eMainMenuState_LastEnum)
{

}

void cMainMenuWidget_Resume::OnMouseDown(eMButton aButton)
{
	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_menu_click",false,1);

	mpInit->mpMainMenu->SetActive(false);
}

// LOADGAME

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

// OPTIONS

// A button for an option that steps through its values: forwards with the
// left button, back with the right.
class cMainMenuWidget_Option : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_Option(cInit *apInit, const cVector3f &avPos, const tWString& asText, const tWString& asTip,
							tMainMenuOptionStep aStep)
		: cMainMenuWidget_Button(apInit,avPos,asText,eMainMenuState_LastEnum,20,eFontAlign_Right),
		  mStep(std::move(aStep))
	{
		msTip = asTip;
	}

	void OnMouseDown(eMButton aButton) override
	{
		mStep(mpInit, aButton == eMButton_Right ? -1 : 1);
	}

private:
	tMainMenuOptionStep mStep;
};

// The value of an option, beside its button. It is read again each time it
// is drawn, as presets and the F2 panel change options too.
class cMainMenuWidget_OptionValue : public cMainMenuWidget_Text
{
public:
	cMainMenuWidget_OptionValue(cInit *apInit, const cVector3f &avPos, cMainMenuWidget *apOption,
								tMainMenuOptionText aText)
		: cMainMenuWidget_Text(apInit,avPos,aText(apInit),20,eFontAlign_Left,apOption),
		  mText(std::move(aText))
	{
	}

	void OnDraw() override
	{
		msText = mText(mpInit);
		cMainMenuWidget_Text::OnDraw();
	}

private:
	tMainMenuOptionText mText;
};

// The index alDir steps from among alNum, wrapping around
static int CycleIndex(int alIdx, int alNum, int alDir)
{
	return (alIdx + alDir + alNum) % alNum;
}

static tWString FormatText(const char *asFormat, float afX)
{
	char sTemp[64];
	snprintf(sTemp, sizeof(sTemp), asFormat, afX);
	return cString::To16Char(sTemp);
}

static tWString OnOffText(cInit *mpInit, bool abOn)
{
	return abOn ? kTranslate("MainMenu","On") : kTranslate("MainMenu","Off");
}

static tWString TranslateOr(cInit *mpInit, const tString &asEntry, const tWString &asFallback)
{
	tWString sText = kTranslate("MainMenu", asEntry);
	return sText == _W("") ? asFallback : sText;
}

// OPTIONS CONTROLS

static void StepMouseSensitivity(cInit *apInit, int alDir)
{
	float &fSensitivity = apInit->mpButtonHandler->mfMouseSensitivity;
	fSensitivity = cMath::Clamp(fSensitivity + 0.2f*alDir, 0.2f, 5.0f);
}

static tWString MouseSensitivityText(cInit *apInit)
{
	return FormatText("%.1f", apInit->mpButtonHandler->mfMouseSensitivity);
}

// OPTIONS SOUND

static void StepSoundVolume(cInit *apInit, int alDir)
{
	iLowLevelSound *pLowLevel = apInit->mpGame->GetSound()->GetLowLevel();
	pLowLevel->SetVolume(cMath::Clamp(pLowLevel->GetVolume() + 0.1f*alDir, 0.0f, 1.0f));
}

static tWString SoundVolumeText(cInit *apInit)
{
	return FormatText("%.0f", apInit->mpGame->GetSound()->GetLowLevel()->GetVolume()*100);
}

// Volume buses under the master volume. The labels and tips are English
// only; the language files don't have them.
static void StepSoundBus(cInit *apInit, eSoundBus aBus, int alDir)
{
	cSound *pSound = apInit->mpGame->GetSound();
	float fVolume = cMath::Clamp(pSound->GetBusVolume(aBus) + 0.1f*alDir, 0.0f, 1.0f);
	// Snap to tenths so repeated steps don't drift.
	pSound->SetBusVolume(aBus, std::round(fVolume * 10) / 10);
}

static tWString SoundBusText(cInit *apInit, eSoundBus aBus)
{
	return FormatText("%.0f", apInit->mpGame->GetSound()->GetBusVolume(aBus)*100);
}

static void StepSoundOutputDevice(cInit *apInit, int alDir)
{
	iLowLevelSound *pLowLevel = apInit->mpGame->GetSound()->GetLowLevel();
	// Devices can be plugged in while the game runs.
	tStringVec vDevices = pLowLevel->GetOutputDevices();
	if(vDevices.empty()) return;

	int lCurrent = (int)(std::find(vDevices.begin(), vDevices.end(), apInit->msDeviceName) - vDevices.begin());
	if(lCurrent == (int)vDevices.size()) lCurrent = 0;
	apInit->msDeviceName = vDevices[CycleIndex(lCurrent, (int)vDevices.size(), alDir)];

	// Switches at once where the backend can; otherwise on restart.
	if(pLowLevel->SetOutputDevice(apInit->msDeviceName))
		apInit->msDeviceName = pLowLevel->GetDeviceName();
	else
		gbMustRestart = true;
}

static tWString SoundOutputDeviceText(cInit *apInit)
{
	return cString::To16Char(apInit->msDeviceName);
}

static void StepSoundHRTF(cInit *apInit, int alDir)
{
	iLowLevelSound *pLowLevel = apInit->mpGame->GetSound()->GetLowLevel();
	// Auto -> On -> Off with the left button, the reverse with the right.
	const int vNext[] = { eSoundHRTF_On, eSoundHRTF_Auto, eSoundHRTF_Off };
	const int vPrev[] = { eSoundHRTF_Off, eSoundHRTF_On, eSoundHRTF_Auto };
	int lMode = (int)pLowLevel->GetHRTF();
	pLowLevel->SetHRTF((eSoundHRTF)(alDir < 0 ? vPrev[lMode] : vNext[lMode]));
}

static tWString SoundHRTFText(cInit *mpInit)
{
	iLowLevelSound *pLowLevel = mpInit->mpGame->GetSound()->GetLowLevel();
	tWString sMode = pLowLevel->GetHRTF() == eSoundHRTF_On ? kTranslate("MainMenu","On")
					: pLowLevel->GetHRTF() == eSoundHRTF_Off ? kTranslate("MainMenu","Off") : _W("Auto");
	tString sStatus = pLowLevel->GetHRTFStatus();
	if(pLowLevel->GetHRTF() == eSoundHRTF_Auto && sStatus != "")
		sMode += _W(" (") + cString::To16Char(sStatus.substr(0, sStatus.find(' '))) + _W(")");
	return sMode;
}

// OPTIONS GAME

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

// Whether the game chooser opens at every start (games.cfg)
class cMainMenuWidget_AskGameAtStart : public cMainMenuWidget_Button
{
public:
	cMainMenuWidget_AskGameAtStart(cInit *init, const cVector3f &pos)
		: cMainMenuWidget_Button(init,pos,Label(),eMainMenuState_LastEnum,18,eFontAlign_Center)
	{
		msTip = _W("Without it the last game played starts, unless it can't be found.");
	}
	void OnMouseDown(eMButton button) override
	{
		if(button != eMButton_Left) return;
		cGameLauncher::SetAskAtStart(!cGameLauncher::GetAskAtStart());
		msText = Label();
	}
private:
	static tWString Label()
	{
		return cGameLauncher::GetAskAtStart() ? _W("Choose the game at every start: On")
											  : _W("Choose the game at every start: Off");
	}
};

static const tString gvDifficultyLevel[] = {"Easy","Normal","Hard"};

static void StepDifficulty(cInit *apInit, int alDir)
{
	apInit->mDifficulty = (eGameDifficulty)CycleIndex((int)apInit->mDifficulty, 3, alDir);
}

static tWString DifficultyText(cInit *mpInit)
{
	return kTranslate("MainMenu",gvDifficultyLevel[mpInit->mDifficulty]);
}

static void StepLanguage(cInit *apInit, int alDir)
{
	tWStringList lstFiles;
	apInit->mpGame->GetResources()->GetLowLevel()->FindFilesInDir(lstFiles,_W("config/"),_W("*.lang"));
	std::vector<tWString> vFiles(lstFiles.begin(), lstFiles.end());
	if(vFiles.empty()) return;

	int lCurrent = 0;
	for(size_t i=0; i<vFiles.size(); ++i)
	{
		if(cString::To16Char(cString::ToLowerCase(apInit->msLanguageFile)) == cString::ToLowerCaseW(vFiles[i]))
			lCurrent = (int)i;
	}
	apInit->msLanguageFile = cString::To8Char(vFiles[CycleIndex(lCurrent, (int)vFiles.size(), alDir)]);

	if(apInit->mpMapHandler->GetCurrentMapName() != "")
	{
		gbMustRestart = true;
		return;
	}

	cResources *pResources = apInit->mpGame->GetResources();
	pResources->ClearResourceDirs();
	pResources->AddResourceDir("core/programs");
	pResources->AddResourceDir("core/textures");
	pResources->LoadResourceDirsFile("resources.cfg");
	pResources->SetLanguageFile(apInit->msLanguageFile);

	apInit->mpMainMenu->UpdateWidgets();
}

static tWString LanguageText(cInit *apInit)
{
	return cString::To16Char(cString::SetFileExt(apInit->msLanguageFile,""));
}

// OPTIONS GRAPHICS

static const cVector2l gvResolutions[] = {cVector2l(640,480), cVector2l(800,600), cVector2l(1024, 768),
							cVector2l(1152,864),cVector2l(1280,720),cVector2l(1280,768),
							cVector2l(1280,800),cVector2l(1280,960),cVector2l(1280,1024),
							cVector2l(1360,768),cVector2l(1360,1024),cVector2l(1400,1050),
							cVector2l(1440,900),cVector2l(1680,1050),cVector2l(1600,1200),
							cVector2l(1920,1080),cVector2l(1920,1200)
						};

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
	for(const cVector2l &vSize : gvResolutions)
	{
		if(vDesktop.x > 0 && (vSize.x > vDesktop.x || vSize.y > vDesktop.y)) continue;
		if(std::find(avList.begin(), avList.end(), vSize) == avList.end())
			avList.push_back(vSize);
	}
	std::sort(avList.begin(), avList.end(), [](const cVector2l &a, const cVector2l &b)
				{ return a.x != b.x ? a.x < b.x : a.y < b.y; });

	avList.insert(avList.begin(), cVector2l(-1,-1));
}

static void StepResolution(cInit *apInit, int alDir)
{
	std::vector<cVector2l> vResolutions;
	GetResolutionList(apInit, vResolutions);

	// A size that is not in the list (set in the config file) counts as
	// the desktop size.
	int lCurrent = (int)(std::find(vResolutions.begin()+1, vResolutions.end(), apInit->mvScreenSize) - vResolutions.begin());
	if(lCurrent == (int)vResolutions.size()) lCurrent = 0;

	apInit->mvScreenSize = vResolutions[CycleIndex(lCurrent, (int)vResolutions.size(), alDir)];
	gbMustRestart = true;
}

static tWString ResolutionText(cInit *apInit)
{
	// Not in the game's language files
	if(apInit->mvScreenSize.x <= 0 || apInit->mvScreenSize.y <= 0) return _W("Desktop");
	return cString::To16Char(cString::ToString(apInit->mvScreenSize.x) + " x " + cString::ToString(apInit->mvScreenSize.y));
}

// -1 is automatic
static void StepDisplay(cInit *apInit, int alDir)
{
	int lNum = apInit->mpGame->GetGraphics()->GetLowLevel()->GetDisplayNum();
	int lDisplay = apInit->mlDisplay < lNum ? apInit->mlDisplay : -1;
	apInit->mlDisplay = CycleIndex(lDisplay + 1, lNum + 1, alDir) - 1;
	gbMustRestart = true;
}

static tWString DisplayText(cInit *apInit)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();

	// Not in the game's language files
	if(apInit->mlDisplay < 0 || apInit->mlDisplay >= pLowLevel->GetDisplayNum()) return _W("Automatic");

	tString sName = pLowLevel->GetDisplayName(apInit->mlDisplay);
	if(sName.size() > 20) sName = sName.substr(0,20);

	return cString::To16Char(cString::ToString(apInit->mlDisplay + 1) + ": " + sName);
}

// Off, the original noise tiles, or the grain made by a shader: plain, or as
// film has it.
static void StepNoiseFilter(cInit *apInit, int alDir)
{
	cPlayerNoiseFilter *pFilter = apInit->mpPlayer->GetNoiseFilter();
	int lIdx = pFilter->IsActive() ? 1 + (int)pFilter->GetType() : 0;
	lIdx = CycleIndex(lIdx, 1 + eNoiseFilterType_LastEnum, alDir);

	pFilter->SetActive(lIdx > 0);
	if(lIdx > 0) pFilter->SetType((eNoiseFilterType)(lIdx - 1));
}

// The last three names are not in the game's language files.
static tWString NoiseFilterText(cInit *mpInit)
{
	cPlayerNoiseFilter *pFilter = mpInit->mpPlayer->GetNoiseFilter();
	if(pFilter->IsActive()==false) return kTranslate("MainMenu","Off");
	static const tWString vNames[eNoiseFilterType_LastEnum] = {_W("Original"), _W("Grain"), _W("Film Grain")};
	return vNames[pFilter->GetType()];
}

static void StepGamma(cInit *apInit, int alDir)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();
	pLowLevel->SetGammaCorrection(cMath::Clamp(pLowLevel->GetGammaCorrection() + 0.1f*alDir, 0.1f, 3.0f));
}

static tWString GammaText(cInit *apInit)
{
	return FormatText("%.1f", apInit->mpGame->GetGraphics()->GetLowLevel()->GetGammaCorrection());
}

static void StepFOV(cInit *apInit, int alDir)
{
	apInit->mpPlayer->SetViewFOV(cMath::Clamp(apInit->mpPlayer->GetViewFOV() + 5.0f*alDir, 60.0f, 100.0f));
}

static tWString FOVText(cInit *apInit)
{
	return FormatText("%.0f", apInit->mpPlayer->GetViewFOV());
}

// Graphics presets. The names are not in the game's language files.

// The presets in turn; from Custom (options changed by hand) the first or
// last
static void StepGraphicsPreset(cInit *apInit, int alDir)
{
	int lCurrent = (int)GetGraphicsPreset(apInit);
	const int lNum = eGraphicsPreset_Custom;
	if(lCurrent == eGraphicsPreset_Custom)	lCurrent = alDir < 0 ? lNum - 1 : 0;
	else									lCurrent = CycleIndex(lCurrent, lNum, alDir);

	ApplyGraphicsPreset(apInit, (eGraphicsPreset)lCurrent);
}

static tWString GraphicsPresetText(cInit *apInit)
{
	return GetGraphicsPresetName(GetGraphicsPreset(apInit));
}

// OPTIONS ADVANCED GRAPHICS

static const tString gvTextureQuality[] = {"High","Medium","Low"};

// The left button goes to higher quality, the lower level
static void StepTextureQuality(cInit *apInit, int alDir)
{
	cMaterialManager *pMaterialManager = apInit->mpGame->GetResources()->GetMaterialManager();
	pMaterialManager->SetTextureSizeLevel(CycleIndex(pMaterialManager->GetTextureSizeLevel(), 3, -alDir));
	gbMustRestart = true;
}

static tWString TextureQualityText(cInit *mpInit)
{
	return kTranslate("MainMenu",gvTextureQuality[mpInit->mpGame->GetResources()->GetMaterialManager()->GetTextureSizeLevel()]);
}

// The F2 panel changes this too
static void StepShadows(cInit *apInit, int alDir)
{
	cRenderer3D *pRenderer = apInit->mpGame->GetGraphics()->GetRenderer3D();
	pRenderer->SetShowShadows((eRendererShowShadows)CycleIndex((int)pRenderer->GetShowShadows(), eRendererShowShadows_LastEnum, alDir));
}

// In the order of eRendererShowShadows
static tWString ShadowsText(cInit *mpInit)
{
	switch(mpInit->mpGame->GetGraphics()->GetRenderer3D()->GetShowShadows())
	{
	case 0: return kTranslate("MainMenu","On");
	case 1: return kTranslate("MainMenu","Only Static");
	default: return kTranslate("MainMenu","Off");
	}
}

// The caps offered for the frame rate with the 60 FPS limit off; 0 is none.
// The settings file can hold another.
static const int gvMaxFPS[] = {0, 75, 90, 100, 120, 144, 165, 240, 360};
static const int glMaxFPSNum = sizeof(gvMaxFPS) / sizeof(gvMaxFPS[0]);

// The next cap above (left button) or below (right) the current one
static void StepMaxFPS(cInit *apInit, int alDir)
{
	int lMax = apInit->mpGame->GetMaxFPS();
	int lIdx = -1;
	if(alDir < 0)
	{
		for(int i=glMaxFPSNum-1; i>=0; --i) if(gvMaxFPS[i] < lMax){ lIdx = i; break; }
		if(lIdx < 0) lIdx = glMaxFPSNum-1;
	}
	else
	{
		for(int i=0; i<glMaxFPSNum; ++i) if(gvMaxFPS[i] > lMax){ lIdx = i; break; }
		if(lIdx < 0) lIdx = 0;
	}
	apInit->mpGame->SetMaxFPS(gvMaxFPS[lIdx]);
}

static tWString MaxFPSText(cInit *mpInit)
{
	int lMax = mpInit->mpGame->GetMaxFPS();
	if(lMax <= 0) return kTranslate("MainMenu","Off");
	return cString::To16Char(cString::ToString(lMax));
}

// One option for how textures are filtered: trilinear, or that with 2 to
// 16 samples along surfaces seen at a slant, as far as the graphics card
// goes. Bilinear is only shown when the settings file asks for it.
static void StepTextureFilter(cInit *apInit, int alDir)
{
	int lMax = cMath::Max(apInit->mpGame->GetGraphics()->GetLowLevel()->GetCaps(eGraphicCaps_MaxAnisotropicFiltering), 1);
	cMaterialManager *pMaterialManager = apInit->mpGame->GetResources()->GetMaterialManager();
	int lX = cMath::Max((int)pMaterialManager->GetTextureAnisotropy(), 1);

	if(alDir > 0)	lX = lX*2 > lMax ? 1 : lX*2;
	else			lX = lX/2 < 1 ? lMax : lX/2;

	pMaterialManager->SetTextureFilter(eTextureFilter_Trilinear);
	pMaterialManager->SetTextureAnisotropy((float)lX);
}

// The last names are not in the game's language files.
static tWString TextureFilterText(cInit *mpInit)
{
	cMaterialManager *pMaterialManager = mpInit->mpGame->GetResources()->GetMaterialManager();
	int lAnisotropy = (int)pMaterialManager->GetTextureAnisotropy();
	if(lAnisotropy > 1) return cString::To16Char("Anisotropic " + cString::ToString(lAnisotropy) + "x");
	return kTranslate("MainMenu", pMaterialManager->GetTextureFilter() == eTextureFilter_Bilinear ? "Bilinear" : "Trilinear");
}

// Samples per pixel of the 3D scene: off, 2, 4, 8, as far as the graphics
// card goes.
static void StepFSAA(cInit *apInit, int alDir)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();
	int lMax = cMath::Min(pLowLevel->GetCaps(eGraphicCaps_MaxMultisampling), 8);
	int lX = pLowLevel->GetMultisampling();

	if(alDir > 0)
	{
		lX = lX == 0 ? 2 : lX*2;
		if(lX > lMax) lX = 0;
	}
	else
	{
		if(lX == 2)			lX = 0;
		else if(lX == 0)	lX = lMax;
		else				lX /= 2;
	}

	pLowLevel->SetMultisampling(lX);
	apInit->mlFSAA = pLowLevel->GetMultisampling();
}

static tWString FSAAText(cInit *mpInit)
{
	if(mpInit->mlFSAA == 0) return kTranslate("MainMenu","Off");
	return cString::To16Char(cString::ToString(mpInit->mlFSAA)+"x");
}

// Off, then the qualities. Settings made by hand (F2 panel) count as the
// first.
static void StepSmaa(cInit *apInit, int alDir)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();
	cSmaaSettings settings = pLowLevel->GetSmaa();

	int lIdx = settings.mbActive ? 1 + settings.GetQuality() % cSmaaSettings::klQualityNum : 0;
	lIdx = CycleIndex(lIdx, 1 + cSmaaSettings::klQualityNum, alDir);

	settings.mbActive = lIdx > 0;
	if(lIdx > 0) settings.SetQuality(lIdx - 1);
	pLowLevel->SetSmaa(settings);
}

// The names are not in the game's language files.
static tWString SmaaText(cInit *mpInit)
{
	const cSmaaSettings &settings = mpInit->mpGame->GetGraphics()->GetLowLevel()->GetSmaa();
	if(settings.mbActive==false) return kTranslate("MainMenu","Off");

	static const wchar_t *vNames[] = {L"Low", L"Medium", L"High", L"Ultra", L"Custom"};
	return vNames[settings.GetQuality()];
}

static const float gvRenderScales[] = {0.5f, 0.75f, 1.0f, 1.5f, 2.0f};
static const int glRenderScaleNum = sizeof(gvRenderScales) / sizeof(gvRenderScales[0]);

static void StepRenderScale(cInit *apInit, int alDir)
{
	cGraphics *pGraphics = apInit->mpGame->GetGraphics();

	// From the closest step to the current scale, which may come from the
	// settings file
	int lIdx = 0;
	for(int i=1; i<glRenderScaleNum; ++i)
	{
		if(fabs(gvRenderScales[i] - pGraphics->GetRenderScale()) < fabs(gvRenderScales[lIdx] - pGraphics->GetRenderScale()))
			lIdx = i;
	}
	pGraphics->SetRenderScale(gvRenderScales[CycleIndex(lIdx, glRenderScaleNum, alDir)]);
}

static tWString RenderScaleText(cInit *apInit)
{
	return cString::To16Char(cString::ToString((int)(apInit->mpGame->GetGraphics()->GetRenderScale() * 100.0f + 0.5f))+"%");
}

// Off, then the qualities. Samples set by hand (F2 panel) count as the first.
static void StepAmbientOcclusion(cInit *apInit, int alDir)
{
	cRendererPostEffects *pPostEffects = apInit->mpGame->GetGraphics()->GetRendererPostEffects();
	cAmbientOcclusionSettings settings = pPostEffects->GetAmbientOcclusionSettings();

	int lIdx = pPostEffects->GetAmbientOcclusionActive() ? 1 + settings.GetQuality() % cAmbientOcclusionSettings::klQualityNum : 0;
	lIdx = CycleIndex(lIdx, 1 + cAmbientOcclusionSettings::klQualityNum, alDir);

	pPostEffects->SetAmbientOcclusionActive(lIdx > 0);
	if(lIdx > 0)
	{
		settings.SetQuality(lIdx - 1);
		pPostEffects->SetAmbientOcclusionSettings(settings);
	}
}

// The names are not in the game's language files.
static tWString AmbientOcclusionText(cInit *mpInit)
{
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	if(pPostEffects->GetAmbientOcclusionActive()==false) return kTranslate("MainMenu","Off");

	static const wchar_t *vNames[] = {L"Low", L"Medium", L"High", L"Very High", L"Ultra", L"Custom"};
	return vNames[pPostEffects->GetAmbientOcclusionSettings().GetQuality()];
}

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

// OPTIONS KEY CONFIG

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
			// The same short names as tips and the inventory
			mpKeyWidget->msText = mpInit->mpButtonHandler->GetKeyLabel(pAction->GetInputName());

		}
		else
		{
			mpKeyWidget->msText = kTranslate("MainMenu","Empty");
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
			// The same short names as tips and the inventory
			mpKeyWidget->msText = mpInit->mpButtonHandler->GetKeyLabel(pAction->GetInputName());

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
		mpInit->mpButtonHandler->SetDefaultKeys();
		mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupMove);
		mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupAction);
		mpInit->mpMainMenu->ResetWidgets(eMainMenuState_OptionsKeySetupMisc);
	}
private:
	cMainMenuWidget_Text *mpKeyWidget;
	tString msActionName;
};

cMainMenu::cMainMenu(cInit *apInit)  : iUpdateable("MainMenu")
{
	mState = eMainMenuState_Start;

	mLogo = cScreenImage();
	mBackground = cScreenImage();

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

	//Init widgets
	
	mvStateWidgets.resize(eMainMenuState_LastEnum);
	
	Reset();
}

cMainMenu::~cMainMenu(void)
{
	STLDeleteAll(mlstWidgets);

    mpDrawer->DestroyGfxObject(mpGfxBlackQuad);
	mpDrawer->DestroyGfxObject(mpGfxMouse);
	mpDrawer->DestroyGfxObject(mpGfxRainDrop);
	mpDrawer->DestroyGfxObject(mpGfxRainSplash);
	mpDrawer->DestroyGfxObject(mpGfxSnowFlake);

	mpInit->mpGraphicsHelper->DestroyScreenImage(mLogo);
	mpInit->mpGraphicsHelper->DestroyScreenImage(mBackground);
	
}

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

void cMainMenu::OnPostSceneDraw()
{
	mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));
	
	mpInit->mpGraphicsHelper->DrawScreenImage(mLogo,0,cVector2f(800,180));
	mpInit->mpGraphicsHelper->DrawScreenImage(mBackground,cVector3f(0,180,0),cVector2f(800,420));

	// Fade in
	if (mbFadeIn)
	{
		mpDrawer->DrawGfxObject(mpGfxBlackQuad,mpInit->mpGraphicsHelper->GetScreenPos(120),mpInit->mpGraphicsHelper->GetScreenSize(), cColor(1,1-mfFadeAmount));
	}
}

void cMainMenu::OnDraw()
{
	// Draw widgets
	tMainMenuWidgetListIt it = mlstWidgets.begin();
	for(; it != mlstWidgets.end(); ++it)
	{
		cMainMenuWidget* pWidget = *it;
		
		if(pWidget->IsActive()) pWidget->OnDraw();
	}

	DrawBackground();

	//Draw tip
	if(msButtonTip != _W(""))
	{
		mpTipFont->DrawWordWrap(cVector3f(10,570,150),780,13,12,cColor(1,1),
								eFontAlign_Left,msButtonTip.c_str());
	}

	// Draw mouse
	if(mpCurrentActionText) return;
	cResourceImage *pImage = mpGfxMouse->GetMaterial()->GetImage(eMaterialTexture_Diffuse);
	cVector2l vSize = pImage->GetSize();
	cVector2f vPosAdd(((float)vSize.x) / 2.0f, ((float)vSize.y) / 2.0f);
	cVector2f vMousePos = mvMousePos + mpInit->mpButtonHandler->GetCursorDrawOffset(true);
	mpInit->mpGraphicsHelper->ClampToScreen(vMousePos);
	mpDrawer->DrawGfxObject(mpGfxMouse,cVector3f(0,0,100)+(vMousePos - vPosAdd));
}

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
			iAction *pAction = pInput->InputToAction(msCurrentActionName);
			
			mpCurrentActionText->msText = mpInit->mpButtonHandler->GetKeyLabel(pAction->GetInputName());

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

void cMainMenu::OnMouseDown(eMButton aButton)
{
	if(mpCurrentActionText) return;

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

void cMainMenu::OnMouseDoubleClick(eMButton aButton)
{
	if(mpCurrentActionText) return;

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

void cMainMenu::SetActive(bool abActive)
{
	if(mbActive == abActive) return;

	
	mbActive = abActive;

	if(mbActive)
	{
		if (!mpInit->mbFullScreen) {
			mpInit->mpGame->GetInput()->GetLowLevel()->LockInput(false);
		}

		mpInit->mpGame->GetUpdater()->SetContainer("MainMenu");
		mpInit->mpGame->GetScene()->SetDrawScene(false);
		mpInit->mpGame->GetScene()->SetUpdateMap(false);
		
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_MainMenu);
				
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

		mLogo = mpInit->mpGraphicsHelper->LoadScreenImage("menu_logo.jpg");
		
		if(mbGameActive)
			mBackground = mpInit->mpGraphicsHelper->LoadScreenImage("menu_background_ingame.jpg");
		else if(cInit::GetGameId() == eGameId_Requiem)
			mBackground = mpInit->mpGraphicsHelper->LoadScreenImage("requiem_menu_background.jpg");
		else
			mBackground = mpInit->mpGraphicsHelper->LoadScreenImage("menu_background.jpg");

	}
	else
	{
		if (!mpInit->mbFullScreen) {
			mpInit->mpGame->GetInput()->GetLowLevel()->LockInput(true);
		}

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
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_Game);

		mpInit->mpGraphicsHelper->DestroyScreenImage(mLogo);
		mpInit->mpGraphicsHelper->DestroyScreenImage(mBackground);
	}
}

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

void cMainMenu::OnExit()
{
	SetActive(false);
}

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

void cMainMenu::SetInputToAction(const tString &asActionName,cMainMenuWidget_Text *apText)
{
	msCurrentActionName = asActionName;
	mpCurrentActionText = apText;
	InitCheckInput();
}

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

bool cMainMenu::CheckForInput()
{
	cInput *pInput = mpInit->mpGame->GetInput();

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

void cMainMenu::ResetWidgets(eMainMenuState aState)
{
	tMainMenuWidgetListIt it = mvStateWidgets[aState].begin();
	for(; it != mvStateWidgets[aState].end(); ++it)
	{
		cMainMenuWidget *pWidget = *it;
		pWidget->Reset();
	}
}

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

void cMainMenu::CreateWidgets()
{
	cVector3f vPos;


	cVector3f vTextStart(220, 230, 40);

	//Erase all previous
	STLDeleteAll(mlstWidgets);
	for(size_t i=0; i< eMainMenuState_LastEnum; ++i) mvStateWidgets[i].clear();

	// First start
	bool bFirstStart = mpInit->mpConfig->GetBool("Game","FirstStart",true);
	if(bFirstStart)
	{
		vPos = cVector3f(40, 190, 40);
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu", "Welcome"),15,eFontAlign_Left) ); 
		vPos.y += 18;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu", "Too Improve"),15,eFontAlign_Left) ); 
		vPos.y += 28;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu", "StartTip1"),15,eFontAlign_Left) ); 
		vPos.y += 18;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu", "StartTip2"),15,eFontAlign_Left) ); 
		vPos.y += 18;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu", "StartTip3"),15,eFontAlign_Left) ); 
		vPos.y += 28;
		vPos.x = 395;
		cMainMenuWidget *pGammaFirstButton = new cMainMenuWidget_Option(mpInit,vPos,kTranslate("MainMenu","Gamma:"),
																		kTranslate("MainMenu","TipGraphicsGamma"),StepGamma);
		AddWidgetToState(eMainMenuState_FirstStart,pGammaFirstButton);
		vPos.x = 405;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_OptionValue(mpInit,vPos,pGammaFirstButton,GammaText));

		vPos.y += 31;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Image(mpInit,
																			cVector3f(250,vPos.y,30),
																			cVector2f(300,200),
																			"menu_gamma.bmp",
																			"diffalpha2d",
																			cColor(1,1)) );
		vPos.y+=205;
		AddWidgetToState(eMainMenuState_FirstStart,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","OK"),eMainMenuState_Start,20,eFontAlign_Center) );
	}
	


	// Start menu:
	vPos = vTextStart;//cVector3f(400, 260, 40);

	if(mpInit->mpMapHandler->GetCurrentMapName() != "")
	{
		AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_Resume(mpInit,vPos,kTranslate("MainMenu","Resume")) );
		vPos.y += 60;
	}
	else
	{
		tWString sAuto = mpInit->mpSaveHandler->GetLatest(_W("save/auto/"),_W("*.sav"));
		tWString sSpot = mpInit->mpSaveHandler->GetLatest(_W("save/spot/"),_W("*.sav"));

		if(sAuto != _W("") || sSpot != _W(""))
		{
			AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","Continue"),eMainMenuState_Continue) ); 
			vPos.y += 51;
		}
	}

	if(cInit::GetGameId() == eGameId_BlackPlague)
	{
		// Asked once; after that the tutorial has its own button
		if(mpInit->mbAskTutorial)
		{
			AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","New Game"),eMainMenuState_AskTutorial) );
			vPos.y += 51;
		}
		else
		{
			AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","Tutorial"),eMainMenuState_AskStartTutorial) );
			vPos.y += 51;
			AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","New Game"),eMainMenuState_NewGame) );
			vPos.y += 51;
		}
	}
	else
	{
		AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","New Game"),eMainMenuState_NewGame) );
		vPos.y += 51;
	}
	AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","Load Game"),eMainMenuState_LoadGameSpot) ); 
	vPos.y += 51;
	AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","Options"),eMainMenuState_Options) ); 
	vPos.y += 51;
	AddWidgetToState(eMainMenuState_Start,new cMainMenuWidget_MainButton(mpInit,vPos,kTranslate("MainMenu","Exit"),eMainMenuState_Exit) );
	
	
	// New Game

	vPos = vTextStart;//cVector3f(400, 260, 40);
	if(cInit::GetGameId() == eGameId_Requiem)
	{
		// Requiem: no difficulty, only whether to start
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("RequiemMenu","SureNewGame"),20,eFontAlign_Center) );
		vPos.y += 34;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_NewGame(mpInit,vPos,kTranslate("MainMenu","Yes"),20,eFontAlign_Center,eGameDifficulty_Normal) );
		vPos.y += 29;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,20,eFontAlign_Center) );
	}
	else
	{
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_NewGame(mpInit,vPos,kTranslate("MainMenu","Easy"),24,eFontAlign_Center,eGameDifficulty_Easy) );
		vPos.y += 30;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","EasyDesc"),16,eFontAlign_Center) );
		vPos.y += 42;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_NewGame(mpInit,vPos,kTranslate("MainMenu","Normal"),24,eFontAlign_Center,eGameDifficulty_Normal) );
		vPos.y += 30;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","NormalDesc"),16,eFontAlign_Center) );
		vPos.y += 42;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_NewGame(mpInit,vPos,kTranslate("MainMenu","Hard"),24,eFontAlign_Center,eGameDifficulty_Hard) );
		vPos.y += 30;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","HardDesc"),16,eFontAlign_Center) );
		vPos.y +=46;
		AddWidgetToState(eMainMenuState_NewGame,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Start,22,eFontAlign_Center) );
	}

	// Continue
	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_Continue,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","ContinueLastSave"),20,eFontAlign_Center) ); 
	vPos.y += 34;
	AddWidgetToState(eMainMenuState_Continue,new cMainMenuWidget_Continue(mpInit,vPos,kTranslate("MainMenu","Yes"),20,eFontAlign_Center) ); 
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_Continue,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,20,eFontAlign_Center) );

	// Load Game

	for(size_t i=0; i< 3; ++i)
	{
		vPos = vTextStart;//cVector3f(400, 260, 40);
		
		eMainMenuState state = (eMainMenuState)(eMainMenuState_LoadGameSpot + i);
		
		//Head
		AddWidgetToState(state,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Load Game"),27,eFontAlign_Center) ); 
		vPos.y += 42;
		vPos.x -= 110;
		
		//Buttons
		// Requiem's auto saves are its quick saves; its names are in a
		// category of their own
		const bool bRequiem = cInit::GetGameId() == eGameId_Requiem;
		const tString sLoadCategory = bRequiem ? "RequiemMenu" : "MainMenu";
		const tString sAutoName = bRequiem ? "Quick Saves" : "Auto Saves";
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate(sLoadCategory,"Saved Games"),eMainMenuState_LoadGameSpot,25,eFontAlign_Center) );
		vPos.y += 32;
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate(sLoadCategory,sAutoName),eMainMenuState_LoadGameAuto,25,eFontAlign_Center) );
		vPos.y += 32;
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate(sLoadCategory,"Favorites"),eMainMenuState_LoadGameFavorite,25,eFontAlign_Center) );
		
		//Back
		vPos.y += 150;
		vPos.x += 130;
		vPos.y += 32;
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Start,23,eFontAlign_Center) );
        
		//Load type
		vPos = vTextStart;//cVector3f(400, 260, 40);
		vPos.y += 42;
		vPos.x += 185;
		tString sLoadType = "Saved Games";
		if(i==1)sLoadType = sAutoName;
		if(i==2)sLoadType = "Favorites";
		AddWidgetToState(state,new cMainMenuWidget_Text(mpInit,vPos,kTranslate(sLoadCategory,sLoadType)+_W(":"),21,eFontAlign_Center) );
		
		//Saved games list
		
		//Set up
		vPos = vTextStart;//cVector3f(400, 260 , 40);
		vPos.y += 46 + 30;
		vPos.x += 15;
		
		tWString sDir = _W("save/spot");
		if(i == 1)		sDir = _W("save/auto");
		else if(i == 2) sDir = _W("save/favorite");
		
		gpSaveGameList[i] = new cMainMenuWidget_SaveGameList(
														mpInit,vPos,cVector2f(355,170),15,sDir,(int)i);
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
		}

		//Save game buttons
		vPos.y += 170;
		vPos.x = vTextStart.x + 20;
		
		AddWidgetToState(state,new cMainMenuWidget_LoadSaveGame(mpInit,vPos,kTranslate("MainMenu","Load"),17,eFontAlign_Left,sDir,(int)i) ); 
		
		vPos.x += 70;
		if(i!=2)
			AddWidgetToState(state,new cMainMenuWidget_FavoriteSaveGame(mpInit,vPos,kTranslate("MainMenu","Add To Favorites"),17,eFontAlign_Left,sDir,(int)i) ); 
		
		vPos.x += 205;
		AddWidgetToState(state,new cMainMenuWidget_RemoveSaveGame(mpInit,vPos,kTranslate("MainMenu","Remove"),17,eFontAlign_Left,sDir,(int)i) ); 
		
		
        
	}

	// Quit
	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_Exit,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","SureQuit"),20,eFontAlign_Center) ); 
	vPos.y += 34;
	AddWidgetToState(eMainMenuState_Exit,new cMainMenuWidget_Quit(mpInit,vPos,kTranslate("MainMenu","Yes"),20,eFontAlign_Center) ); 
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_Exit,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,20,eFontAlign_Center) );

	// Options
	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Controls"),eMainMenuState_OptionsControls,25,eFontAlign_Center) ); 
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Game"),eMainMenuState_OptionsGame,25,eFontAlign_Center) );
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Sound"),eMainMenuState_OptionsSound,25,eFontAlign_Center) );
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Graphics"),eMainMenuState_OptionsGraphics,25,eFontAlign_Center) );
	vPos.y += 37;
	// The Dog's behaviour: only Overture has the Dog
	if(cInit::IsOverture())
	{
		AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,_W("AI"),eMainMenuState_OptionsAI,25,eFontAlign_Center) );
		vPos.y += 37;
	}
	if(cInit::CanChangeGame())
	{
		AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,_W("Change Game"),eMainMenuState_OptionsChangeGame,25,eFontAlign_Center) );
		vPos.y += 37;
	}
	AddWidgetToState(eMainMenuState_Options,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Start,25,eFontAlign_Center) );

	// Change game
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_Text(mpInit,vPos,_W("Change Game"),25,eFontAlign_Center));
	vPos.y += 45;
	for(int i=0; i<eGameId_LastEnum; ++i)
	{
		eGameId game = (eGameId)i;
		tWString sName = cString::To16Char(cInit::GetGameName(game));
		if(game == cInit::GetGameId())
			AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_Text(mpInit,vPos,sName + _W(" (playing)"),18,eFontAlign_Center));
		else if(cInit::FindGameFolder(game) == "")
			AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_Text(mpInit,vPos,sName + _W(" (not found)"),18,eFontAlign_Center));
		else
			AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_ChangeGame(mpInit,vPos,game));
		vPos.y += 34;
	}
	vPos.y += 11;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_AskGameAtStart(mpInit,vPos));
	vPos.y += 40;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_Text(mpInit,vPos,
		_W("The game closes and starts again as the other one. Anything not saved is lost."),16,eFontAlign_Center,nullptr,560));
	vPos.y += 50;
	AddWidgetToState(eMainMenuState_OptionsChangeGame,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Options,23,eFontAlign_Center));

	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsAI,new cMainMenuWidget_Text(mpInit,vPos,_W("AI - Dog behaviour"),25,eFontAlign_Center));
	vPos.y += 45;
	AddWidgetToState(eMainMenuState_OptionsAI,new cMainMenuWidget_DogAI(mpInit,vPos,false));
	vPos.y += 37;
	AddWidgetToState(eMainMenuState_OptionsAI,new cMainMenuWidget_DogAI(mpInit,vPos,true));
	vPos.y += 45;
	AddWidgetToState(eMainMenuState_OptionsAI,new cMainMenuWidget_Text(mpInit,vPos,_W("Same original model and animations in both modes."),16,eFontAlign_Center));
	vPos.y += 40;
	AddWidgetToState(eMainMenuState_OptionsAI,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Options,23,eFontAlign_Center));

	// Options Controls
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsControls,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Controls"),25,eFontAlign_Center) );
	vPos.y += 37;
	AddToggle(eMainMenuState_OptionsControls,vPos,kTranslate("MainMenu","Invert Mouse Y:"),kTranslate("MainMenu","TipControlsInvertMouseY"),
		mpInit->mpButtonHandler->mbInvertMouseY);
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsControls,vPos,kTranslate("MainMenu","Mouse Sensitivity:"),kTranslate("MainMenu","TipControlsMouseSensitivity"),
		StepMouseSensitivity,MouseSensitivityText);
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsControls,vPos,kTranslate("MainMenu","Toggle Crouch:"),kTranslate("MainMenu","TipControlsToggleCrouch"),
		mpInit->mpButtonHandler->mbToggleCrouch);
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_OptionsControls,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Change Key Mapping"),eMainMenuState_OptionsKeySetupMove,20,eFontAlign_Center));
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsControls,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_Options,23,eFontAlign_Center));

	// Options Key Setup General stuff
	for(int i=0; i<3; ++i)
	{
		eMainMenuState state = (eMainMenuState)(i+ eMainMenuState_OptionsKeySetupMove);
		cVector3f vPos = vTextStart;//cVector3f(400, 260, 40);
		//Head
		AddWidgetToState(state,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Configure Keys"),25,eFontAlign_Center) ); 
		vPos.y += 42;
		vPos.x -= 110;
		//Buttons
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Movement"),eMainMenuState_OptionsKeySetupMove,25,eFontAlign_Center) ); 
		vPos.y += 32;
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Actions"),eMainMenuState_OptionsKeySetupAction,25,eFontAlign_Center) ); 
		vPos.y += 32;
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Misc"),eMainMenuState_OptionsKeySetupMisc,25,eFontAlign_Center) ); 
		//Back
		vPos.y += 150;
		vPos.x += 130;
		AddWidgetToState(state,new cMainMenuWidget_KeyReset(mpInit,vPos,kTranslate("MainMenu","Reset to defaults"),23,eFontAlign_Center) );
		vPos.y += 32;
		AddWidgetToState(state,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_OptionsControls,23,eFontAlign_Center) );
	}

	// Options Key Setup: the button of each action, and its key beside it
	struct cKeyRow
	{
		eMainMenuState mState;
		const char *msCategory;
		const char *msLabel;
		const char *msAction;
		bool mbShown;
	};
	const bool bRequiem = cInit::GetGameId() == eGameId_Requiem;
	const cKeyRow vKeyRows[] = {
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Forward:", "Forward", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Backward:", "Backward", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Strafe Left:", "Left", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Strafe Right:", "Right", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Run:", "Run", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Crouch:", "Crouch", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Jump:", "Jump", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Lean Left:", "LeanLeft", true},
		{eMainMenuState_OptionsKeySetupMove, "MainMenu", "Lean Right:", "LeanRight", true},
		{eMainMenuState_OptionsKeySetupAction, "MainMenu", "Interact:", "Interact", true},
		{eMainMenuState_OptionsKeySetupAction, "MainMenu", "Examine:", "Examine", true},
		{eMainMenuState_OptionsKeySetupAction, "MainMenu", "InteractMode:", "InteractMode", true},
		{eMainMenuState_OptionsKeySetupAction, "MainMenu", "Holster:", "Holster", true},
		{eMainMenuState_OptionsKeySetupAction, "MainMenu", "LookMode:", "LookMode", true},
		{eMainMenuState_OptionsKeySetupAction, "MainMenu", "Rotate:", "Rotate", !cInit::IsOverture()},
		{eMainMenuState_OptionsKeySetupMisc, "MainMenu", "Inventory:", "Inventory", true},
		{eMainMenuState_OptionsKeySetupMisc, "MainMenu", "Notebook:", "NoteBook", true},
		{eMainMenuState_OptionsKeySetupMisc, "MainMenu", "Pers. Notes:", "PersonalNotes", true},
		{eMainMenuState_OptionsKeySetupMisc, "MainMenu", "Flashlight:", "Flashlight", true},
		{eMainMenuState_OptionsKeySetupMisc, "MainMenu", "Glowstick:", "GlowStick", true},
		{eMainMenuState_OptionsKeySetupMisc, "RequiemMenu", "Quicksave:", "SaveGame", bRequiem},
		{eMainMenuState_OptionsKeySetupMisc, "RequiemMenu", "Quickload:", "LoadGame", bRequiem},
	};
	eMainMenuState lastKeyState = eMainMenuState_LastEnum;
	for(const cKeyRow &row : vKeyRows)
	{
		if(row.mbShown==false) continue;
		if(row.mState != lastKeyState)	vPos = vTextStart + cVector3f(15,46,0);
		else							vPos.y += 23;
		lastKeyState = row.mState;

		cMainMenuWidget_Text *pKeyText = new cMainMenuWidget_Text(mpInit,vPos+cVector3f(195,0,0),_W(""),18,eFontAlign_Left);
		AddWidgetToState(row.mState,pKeyText);
		AddWidgetToState(row.mState,new cMainMenuWidget_KeyButton(mpInit,vPos,kTranslate(row.msCategory,row.msLabel),
																	18,eFontAlign_Left,pKeyText,row.msAction));
	}

	// Options Game
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsGame,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Game"),25,eFontAlign_Center) );
	vPos.y += 37;
	AddOption(eMainMenuState_OptionsGame,vPos,kTranslate("MainMenu","Language:"),kTranslate("MainMenu","TipGameLanguage"),
		StepLanguage,LanguageText);
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsGame,vPos,kTranslate("MainMenu","Subtitle:"),kTranslate("MainMenu","TipGameSubtitles"),
		mpInit->mbSubtitles);
	vPos.y += 25;
	AddWidgetToState(eMainMenuState_OptionsGame,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","VoiceLanguange:"),12,eFontAlign_Right) );
	AddWidgetToState(eMainMenuState_OptionsGame,new cMainMenuWidget_Text(mpInit,vPos+cVector3f(12,0,0),
										kTranslate("MainMenu","SetThisToLanguageOfVoice"),12,eFontAlign_Left) );
	vPos.y += 25;
	// Requiem has no difficulty
	if(cInit::GetGameId() != eGameId_Requiem)
	{
		AddOption(eMainMenuState_OptionsGame,vPos,kTranslate("MainMenu","Difficulty:"),kTranslate("MainMenu","TipGameDifficulty"),
			StepDifficulty,DifficultyText);
		vPos.y += 29;
	}
	if(mpInit->mbSimpleSwingInOptions)
	{
		AddToggle(eMainMenuState_OptionsGame,vPos,kTranslate("MainMenu","SimpleSwing:"),_W(""),mpInit->mbSimpleWeaponSwing);
		vPos.y += 29;
	}
	AddToggle(eMainMenuState_OptionsGame,vPos,TranslateOr(mpInit,"Show Crosshair:",_W("Show Crosshair:")),
		kTranslate("MainMenu","TipGameShowCrossHair"),mpInit->mbShowCrossHair);
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsGame,vPos,kTranslate("MainMenu","FlashItems:"),kTranslate("MainMenu","TipGameFlashText"),
		mpInit->mbFlashItems);
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsGame,vPos,kTranslate("MainMenu","DisablePersonal:"),kTranslate("MainMenu","TipGameDisablePersonal"),
		[](cInit *apInit){ return !apInit->mbDisablePersonalNotes; },
		[](cInit *apInit, bool abX){ apInit->mbDisablePersonalNotes = !abX; });
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsGame,vPos,_W("Developer Mode:"),_W("Debug keys: F2 renderer tools, F3 performance overlay, L logs the rendering of a frame."),
		[](cInit *apInit){ return apInit->mbDeveloperMode; },
		[](cInit *apInit, bool abX)
		{
			apInit->mbDeveloperMode = abX;
			if(apInit->IsDeveloperMode()==false)
			{
				apInit->mpGame->GetGraphics()->GetLowLevel()->SetDebugOverlayActive(false);
				if(apInit->mpRefView) apInit->mpRefView->SetOpen(false);
			}
		});
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsGame,new cMainMenuWidget_GfxBack(mpInit,vPos,kTranslate("MainMenu","Back"),23,eFontAlign_Center) );

	// Options Sound
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsSound,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Sound"),25,eFontAlign_Center) );
	vPos.y += 37;
	AddOption(eMainMenuState_OptionsSound,vPos,kTranslate("MainMenu","Sound Volume:"),kTranslate("MainMenu","TipSoundVolume"),
		StepSoundVolume,SoundVolumeText);
	vPos.y += 29;
	const eSoundBus vMenuBuses[] = { eSoundBus_Music, eSoundBus_Effects, eSoundBus_Voice, eSoundBus_Ambience };
	const wchar_t *vBusLabels[] = { L"Music:", L"Effects:", L"Voice:", L"Ambience:" };
	const wchar_t *vBusTips[] = {
		L"Volume of the music, relative to the sound volume.",
		L"Volume of sound effects, relative to the sound volume.",
		L"Volume of spoken lines and radio messages, relative to the sound volume.",
		L"Volume of looping background sounds such as machinery, wind and water, relative to the sound volume." };
	for(int i=0; i<4; ++i)
	{
		eSoundBus bus = vMenuBuses[i];
		AddOption(eMainMenuState_OptionsSound,vPos,vBusLabels[i],vBusTips[i],
			[bus](cInit *apInit, int alDir){ StepSoundBus(apInit, bus, alDir); },
			[bus](cInit *apInit){ return SoundBusText(apInit, bus); });
		vPos.y += 29;
	}
	AddToggle(eMainMenuState_OptionsSound,vPos,kTranslate("MainMenu","Use Hardware:"),kTranslate("MainMenu","TipSoundHardware"),
		[](cInit *apInit){ return apInit->mbUseSoundHardware; },
		[](cInit *apInit, bool abX){ apInit->mbUseSoundHardware = abX; gbMustRestart = true; });
	vPos.y += 29;
	// Show the device that is really being used
	mpInit->msDeviceName = mpInit->mpGame->GetSound()->GetLowLevel()->GetDeviceName();
	AddOption(eMainMenuState_OptionsSound,vPos,kTranslate("MainMenu","Output Device:"),kTranslate("MainMenu","TipSoundOutputDevice"),
		StepSoundOutputDevice,SoundOutputDeviceText);
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsSound,vPos,_W("HRTF:"),_W("3D sound for headphones. Auto enables it when headphones are detected."),
		StepSoundHRTF,SoundHRTFText);
	vPos.y += 29;
	// Black Plague: reverb and echo of the rooms. The label and tip are not
	// in the language files; the game has them in English only.
	AddToggle(eMainMenuState_OptionsSound,vPos,_W("EFX:"),_W("Enables environmental effects such as reverb and echo. Requires restart."),
		[](cInit *apInit){ return apInit->mbUseEnvAudio; },
		[](cInit *apInit, bool abX){ apInit->mbUseEnvAudio = abX; gbMustRestart = true; });
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsSound,new cMainMenuWidget_GfxBack(mpInit,vPos,kTranslate("MainMenu","Back"),23,eFontAlign_Center) );

	// Options Graphics
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsGraphics,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Graphics"),25,eFontAlign_Center) );
	vPos.y += 37;
	AddOption(eMainMenuState_OptionsGraphics,vPos,_W("Preset:"),
		_W("Sets graphics quality to High, Medium or Low. Custom means the options have been changed by hand."),
		StepGraphicsPreset,GraphicsPresetText);
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsGraphics,vPos,kTranslate("MainMenu","Resolution:"),kTranslate("MainMenu","TipGraphicsResolution"),
		StepResolution,ResolutionText);
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsGraphics,vPos,_W("Fullscreen:"),_W("Fill the screen or run in a window. Requires restart."),
		[](cInit *apInit){ return apInit->mbFullScreenSetting; },
		[](cInit *apInit, bool abX){ apInit->mbFullScreenSetting = abX; gbMustRestart = true; });
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsGraphics,vPos,_W("Display:"),
		_W("The monitor to use. Automatic picks the primary one (the largest on Wayland). Requires restart."),
		StepDisplay,DisplayText);
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsGraphics,vPos,kTranslate("MainMenu","Noise Filter:"),kTranslate("MainMenu","TipGraphicsNoiseFilter"),
		StepNoiseFilter,NoiseFilterText);
	vPos.y += 29;
	AddToggle(eMainMenuState_OptionsGraphics,vPos,kTranslate("MainMenu","Bloom:"),kTranslate("MainMenu","TipGraphicsBloom"),
		[](cInit *apInit){ return apInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetBloomActive(); },
		[](cInit *apInit, bool abX){ apInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetBloomActive(abX); });
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsGraphics,vPos,kTranslate("MainMenu","Gamma:"),kTranslate("MainMenu","TipGraphicsGamma"),
		StepGamma,GammaText);
	vPos.y += 29;
	AddOption(eMainMenuState_OptionsGraphics,vPos,_W("Field of View:"),_W("Vertical viewing angle in degrees. The original is 70."),
		StepFOV,FOVText);
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsGraphics,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Advanced"),eMainMenuState_OptionsGraphicsAdvanced,23,eFontAlign_Center) );
	vPos.y += 31;
	AddWidgetToState(eMainMenuState_OptionsGraphics,new cMainMenuWidget_Button(mpInit,vPos,_W("Lighting"),eMainMenuState_OptionsGraphicsLighting,23,eFontAlign_Center) );
	vPos.y += 31;
	AddWidgetToState(eMainMenuState_OptionsGraphics,new cMainMenuWidget_GfxBack(mpInit,vPos,kTranslate("MainMenu","Back"),23,eFontAlign_Center) );

	// Options Advanced Graphics. Labels and tips given as text are not in the
	// game's language files.
	vPos = vTextStart + cVector3f(40,0,0);
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","Advanced Graphics"),25,eFontAlign_Center) );
	vPos.y += 37;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Texture Quality:"),kTranslate("MainMenu","TipGraphicsTextureQuality"),
		StepTextureQuality,TextureQualityText);
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Shadows:"),kTranslate("MainMenu","TipGraphicsShadows"),
		StepShadows,ShadowsText);
	vPos.y += 21;
	AddToggle(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Post Effects:"),kTranslate("MainMenu","TipGraphicsPostEffects"),
		[](cInit *apInit){ return apInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetActive(); },
		[](cInit *apInit, bool abX){ apInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetActive(abX); apInit->mbPostEffects = abX; });
	vPos.y += 21;
	AddToggle(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Motion Blur:"),kTranslate("MainMenu","TipGraphicsMotionBlur"),
		[](cInit *apInit){ return apInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetMotionBlurActive(); },
		[](cInit *apInit, bool abX){ apInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetMotionBlurActive(abX); });
	vPos.y += 21;
	AddToggle(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","VSync:"),kTranslate("MainMenu","TipGraphicsVSync"),
		[](cInit *apInit){ return apInit->mbVsync; },
		[](cInit *apInit, bool abX){ apInit->mbVsync = abX; apInit->mpGame->GetGraphics()->GetLowLevel()->SetVsyncActive(abX); });
	vPos.y += 21;
	AddToggle(eMainMenuState_OptionsGraphicsAdvanced,vPos,_W("Limit to 60 FPS:"),
		_W("Off: frames are drawn as fast as possible (up to the frame rate cap), or at the display's rate with VSync on. The game itself still runs at 60 steps per second."),
		[](cInit *apInit){ return apInit->mpGame->GetLimitFPS(); },
		[](cInit *apInit, bool abX){ apInit->mpGame->SetLimitFPS(abX); });
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,_W("Frame Rate Cap:"),
		_W("The most frames drawn per second when Limit to 60 FPS is off. Off: as many as can be drawn, or the display's rate with VSync on."),
		StepMaxFPS,MaxFPSText);
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Texture Filter:"),kTranslate("MainMenu","TipGraphicsTextureAnisotropy"),
		StepTextureFilter,TextureFilterText);
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Anti-Aliasing:"),
		_W("Multisampling: several samples a pixel along the edges of surfaces, and of textures with cut-out parts. Smooths their outlines, at a cost that grows with the samples."),
		StepFSAA,FSAAText);
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,_W("Edge Smoothing (SMAA):"),
		_W("Smooths the stair steps of edges in the finished picture, also those of cut-out textures and shadows. Cheap, and works along with Anti-Aliasing."),
		StepSmaa,SmaaText);
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,_W("Render Scale:"),
		_W("Resolution of the 3D view relative to the screen. Lower is faster, higher is sharper. The menus and HUD stay sharp."),
		StepRenderScale,RenderScaleText);
	vPos.y += 21;
	AddOption(eMainMenuState_OptionsGraphicsAdvanced,vPos,_W("Ambient Occlusion:"),
		_W("Darkens corners, creases and where things touch, which light from all around reaches least. Off is the original look."),
		StepAmbientOcclusion,AmbientOcclusionText);
	vPos.y += 21;
	AddToggle(eMainMenuState_OptionsGraphicsAdvanced,vPos,kTranslate("MainMenu","Depth of Field:"),kTranslate("MainMenu","TipGraphicsDOF"),
		[](cInit *apInit){ return !apInit->mpEffectHandler->GetDepthOfField()->IsDisabled(); },
		[](cInit *apInit, bool abX){ apInit->mpEffectHandler->GetDepthOfField()->SetDisabled(!abX); });
	vPos.y += 28;
	AddWidgetToState(eMainMenuState_OptionsGraphicsAdvanced,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_OptionsGraphics,23,eFontAlign_Center) );

	// Options Lighting
	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_OptionsGraphicsLighting,new cMainMenuWidget_Text(mpInit,vPos,_W("Lighting"),25,eFontAlign_Center) );
	vPos.y += 37;
	// Water, some windows and particles drawn through what is behind them.
	// Black Plague's option; Overture's language file lacks its texts.
	AddToggle(eMainMenuState_OptionsGraphicsLighting,vPos,TranslateOr(mpInit,"Refractions:",_W("Refractions:")),
		TranslateOr(mpInit,"TipGraphicsRefractions",_W("Adds more realism to water, some windows and particle effects. Quite large impact on framerate.")),
		[](cInit *apInit){ return apInit->mpGame->GetGraphics()->GetRenderer3D()->GetRefractionUsed(); },
		[](cInit *apInit, bool abX){ apInit->mpGame->GetGraphics()->GetRenderer3D()->SetRefractionUsed(abX); });
	vPos.y += 35;
	AddWidgetToState(eMainMenuState_OptionsGraphicsLighting,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","Back"),eMainMenuState_OptionsGraphics,23,eFontAlign_Center) );

	// Graphics Restart

	vPos = vTextStart;//cVector3f(400, 260, 40);
	AddWidgetToState(eMainMenuState_GraphicsRestart,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","GraphicsRestart"),16,eFontAlign_Center,
						nullptr,400) ); 
	vPos.y += 42;
	AddWidgetToState(eMainMenuState_GraphicsRestart,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","OK"),eMainMenuState_Options,22,eFontAlign_Center)); 

	// Tutorial questions (Black Plague)

	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_AskTutorial,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","AskTutorial"),16,eFontAlign_Center,
						nullptr,400) );
	vPos.y += 42;
	AddWidgetToState(eMainMenuState_AskTutorial,new cMainMenuWidget_AskTutYes(mpInit,vPos,kTranslate("MainMenu","Yes"),22,eFontAlign_Center));
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_AskTutorial,new cMainMenuWidget_AskTutNo(mpInit,vPos,kTranslate("MainMenu","No"),22,eFontAlign_Center));

	vPos = vTextStart;
	AddWidgetToState(eMainMenuState_AskStartTutorial,new cMainMenuWidget_Text(mpInit,vPos,kTranslate("MainMenu","AskStartTutorial"),16,eFontAlign_Center,
						nullptr,400) );
	vPos.y += 42;
	AddWidgetToState(eMainMenuState_AskStartTutorial,new cMainMenuWidget_AskTutYes(mpInit,vPos,kTranslate("MainMenu","Yes"),22,eFontAlign_Center));
	vPos.y += 29;
	AddWidgetToState(eMainMenuState_AskStartTutorial,new cMainMenuWidget_Button(mpInit,vPos,kTranslate("MainMenu","No"),eMainMenuState_Start,22,eFontAlign_Center));
	
}

void cMainMenu::AddWidgetToState(eMainMenuState aState, cMainMenuWidget *apWidget)
{
	mlstWidgets.push_back(apWidget);
	mvStateWidgets[aState].push_back(apWidget);
}

// The button of an option and, beside it, its value
void cMainMenu::AddOption(eMainMenuState aState, const cVector3f &avPos, const tWString &asLabel,
						const tWString &asTip, tMainMenuOptionStep aStep, tMainMenuOptionText aText)
{
	cMainMenuWidget *pButton = new cMainMenuWidget_Option(mpInit,avPos,asLabel,asTip,std::move(aStep));
	AddWidgetToState(aState,pButton);
	AddWidgetToState(aState,new cMainMenuWidget_OptionValue(mpInit,avPos+cVector3f(12,0,0),pButton,std::move(aText)));
}

void cMainMenu::AddToggle(eMainMenuState aState, const cVector3f &avPos, const tWString &asLabel,
						const tWString &asTip, std::function<bool(cInit*)> aGet,
						std::function<void(cInit*, bool)> aSet)
{
	AddOption(aState,avPos,asLabel,asTip,
		[aGet,aSet](cInit *apInit, int){ aSet(apInit, !aGet(apInit)); },
		[aGet](cInit *apInit){ return OnOffText(apInit, aGet(apInit)); });
}

void cMainMenu::AddToggle(eMainMenuState aState, const cVector3f &avPos, const tWString &asLabel,
						const tWString &asTip, bool &abValue)
{
	AddToggle(aState,avPos,asLabel,asTip,
		[&abValue](cInit*){ return abValue; }, [&abValue](cInit*, bool abX){ abValue = abX; });
}
