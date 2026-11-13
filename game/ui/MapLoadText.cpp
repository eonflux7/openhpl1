// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/MapLoadText.h"

#include "core/Init.h"
#include "core/GraphicsHelper.h"
#include "core/ButtonHandler.h"

cMapLoadText::cMapLoadText(cInit *apInit)  : iUpdateable("MapLoadText")
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	//Load graphics
	//mpGfxMouse = mpDrawer->CreateGfxObject("player_crosshair_pointer.bmp","diffalpha2d");
	
	//load fonts
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");
	mpTextFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_computer.fnt");

	Reset();
}

cMapLoadText::~cMapLoadText(void)
{
}

void cMapLoadText::Reset()
{
	mBackground = cScreenImage();
	mbActive = false;
	mfPulsePhase = 0;
	mfPulseRatePerSecond = 1;
}

void cMapLoadText::OnPostSceneDraw()
{
	mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));
	mpInit->mpGraphicsHelper->DrawFullScreenImage(mBackground);
}

void cMapLoadText::OnDraw()
{
	DrawText(false);
}

void cMapLoadText::DrawText(bool abLoading)
{
	//mpTextFont->DrawWordWrap(	cVector3f(25,25,25),750,17,15,cColor(0.75f, 1, 0.75f, 1),
	//							eFontAlign_Left,kTranslate(msTextCat, msTextEntry));

	cVector3f vStart(25,250.0f - 17.0f* (float)mvRows.size()/2.0f,15);
	for(size_t i=0; i< mvRows.size(); ++i)
	{
		mpTextFont->DrawPlain(vStart + cVector3f(0,17.0f* (float)i,0),15,cColor(1,1),//cColor(0.75f, 1, 0.75f, 1),
						eFontAlign_Left,mvRows[i].c_str());
	}

	if(abLoading)
	{
		mpTextFont->DrawPlain(cVector3f(400,550,25),17,cColor(0.75f, 0.75f, 0.75f, 1),
			eFontAlign_Center,kTranslate("LoadTexts", "Loading").c_str());
	}
	else
	{
		tWString wsText = kTranslate("LoadTexts", "ClickToContinue");

		mpTextFont->DrawPlain(cVector3f(400,550,25),
			17,cColor(0.75f*mfPulsePhase, 1, 0.75f*mfPulsePhase, 1),
			eFontAlign_Center,wsText.c_str());

		mpTextFont->DrawPlain(cVector3f(401+ 10*sin(mfPulsePhase*kPi2f),551,23),
			17,cColor(0.1f, 0.1f, 0.1f, 0.7f),
			eFontAlign_Center,wsText.c_str());
		mpTextFont->DrawPlain(cVector3f(399+ -10*sin(mfPulsePhase*kPi2f),549,23),
			17,cColor(0.1f, 0.1f, 0.1f, 0.7f),
			eFontAlign_Center,wsText.c_str());

	}
}

void cMapLoadText::Update(float afTimeStep)
{
	mfPulsePhase += mfPulseRatePerSecond*afTimeStep;
	if(mfPulseRatePerSecond <0)
	{
		if(mfPulsePhase <=0){
			mfPulsePhase =0;
			mfPulseRatePerSecond = -mfPulseRatePerSecond;
		}
	}
	else
	{
		if(mfPulsePhase >=1){
			mfPulsePhase =1;
			mfPulseRatePerSecond = -mfPulseRatePerSecond;
		}
	}
}

void cMapLoadText::OnMouseDown(eMButton aButton)
{
}

void cMapLoadText::OnMouseUp(eMButton aButton)
{
}

void cMapLoadText::SetActive(bool abActive)
{
	if(mbActive == abActive) return;

	mbActive = abActive;

	if(mbActive)
	{
		mvRows.clear();
		mpTextFont->GetWordWrapRows(750,17,15,kTranslate(msTextCat, msTextEntry),&mvRows);

		mpInit->mpGame->GetUpdater()->SetContainer("MapLoadText");
		mpInit->mpGame->GetScene()->SetDrawScene(false);
		mpInit->mpGame->GetScene()->SetUpdateMap(false);
		if(mpInit->mbHasHaptics)
			mpInit->mpGame->GetHaptic()->GetLowLevel()->SetUpdateShapes(false);
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_MapLoadText);

		tString sBack = cInit::IsOverture() || msBackground == "" ? tString("other_load_text_back.jpg") : msBackground;
		mBackground = mpInit->mpGraphicsHelper->LoadScreenImage(sBack);
		
		mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));
		mpInit->mpGraphicsHelper->DrawFullScreenImage(mBackground);
		DrawText(true);
		mpDrawer->DrawAll();
		mpInit->mpGraphicsHelper->SwapBuffers();

		mpInit->mpGame->GetGraphics()->GetRenderer3D()->GetRenderList()->Clear();
	}
	else
	{
		mpInit->mpGraphicsHelper->DestroyScreenImage(mBackground);

		mpInit->mpGame->GetUpdater()->SetContainer("Default");
		mpInit->mpGame->GetScene()->SetDrawScene(true);
		mpInit->mpGame->GetScene()->SetUpdateMap(true);
		if(mpInit->mbHasHaptics)
			mpInit->mpGame->GetHaptic()->GetLowLevel()->SetUpdateShapes(true);
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_Game);
	}
}

void cMapLoadText::OnExit()
{
	SetActive(false);
}
