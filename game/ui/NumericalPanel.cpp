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
#include "ui/NumericalPanel.h"

#include "core/Init.h"
#include "player/Player.h"
#include "player/HapticGameCamera.h"
#include "core/GraphicsHelper.h"

//////////////////////////////////////////////////////////////////////////
// BUTTON
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cNumericalButton::cNumericalButton(cInit *apInit, cNumericalPanel* apPanel, cVector2f avPos, 
								cVector2f avSize, int alNum)
{
	mpInit = apInit;
	mpPanel = apPanel;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

    mvPosition = cVector3f(avPos.x, avPos.y, 40);

	mRect.x = avPos.x;
	mRect.y = avPos.y;
	mRect.w = avSize.x;
	mRect.h = avSize.y;

	mpGfxUp = mpDrawer->CreateGfxObject("numpanel_button_up","diffalpha2d");
	mpGfxDown = mpDrawer->CreateGfxObject("numpanel_button_down","diffalpha2d");

	mlNum = alNum;

	mfAlpha = 0; 
	mbOver = false;  
}

cNumericalButton::~cNumericalButton()
{

}

//-----------------------------------------------------------------------

void cNumericalButton::OnUpdate(float afTimeStep)
{
	if(mbOver)
	{
		mfAlpha += 0.8f * afTimeStep;
		if(mfAlpha > 1)mfAlpha = 1;
	}
	else 
	{
		mfAlpha -= 1.6f * afTimeStep;
		if(mfAlpha < 0)mfAlpha = 0;
	}
}

//-----------------------------------------------------------------------

void cNumericalButton::OnDraw()
{
	if(mbOver && mpPanel->mbMouseIsDown)
		mpDrawer->DrawGfxObject(mpGfxDown,mvPosition,cVector2f(mRect.w,mRect.h),cColor(1,mpPanel->mfAlpha));
	else
		mpDrawer->DrawGfxObject(mpGfxUp,mvPosition,cVector2f(mRect.w,mRect.h),cColor(1,mpPanel->mfAlpha));

	cVector3f vFontPos = mvPosition + cVector3f(mRect.w/2,1,1);

	tWString sChar;
	if(mlNum==-1) sChar =_W("*");
	else if(mlNum==-2) sChar =_W("#");
	else sChar = cString::To16Char(cString::ToString(mlNum));
	
	if(mbOver && mpPanel->mbMouseIsDown)
	{
		mpInit->mpDefaultFont->Draw(vFontPos + cVector3f(1,1,0), 17, cColor(0,0,0,0.9f * mpPanel->mfAlpha),eFontAlign_Center,				
									sChar.c_str());
	}
	else
	{
		mpInit->mpDefaultFont->Draw(vFontPos, 17, cColor(0,0,0,0.9f * mpPanel->mfAlpha),eFontAlign_Center,				
									sChar.c_str());
	}

}

//-----------------------------------------------------------------------

void cNumericalButton::OnMouseDown()
{
	mpPanel->AddDigit(mlNum);

	mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_numpanel_button",false,1);
}

//-----------------------------------------------------------------------

void cNumericalButton::OnMouseUp()
{
}

//-----------------------------------------------------------------------

void cNumericalButton::OnMouseOver(bool abOver)
{
	mbOver = abOver;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cNumericalPanel::cNumericalPanel(cInit *apInit)  : iUpdateable("NumericalPanel")
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	//Load graphics (use notebook background for now).
	mpGfxBackground = mpDrawer->CreateGfxObject("notebook_background.bmp","diffalpha2d");

	mpGfxPanel = mpDrawer->CreateGfxObject("numpanel_panel.bmp","diffalpha2d");
	
	cVector2f vPos(307, 205);

	for(int i=1; i<=12; ++i)
	{
		int lNum = i;
		if(i==10) lNum=-1;
		else if(i==11)lNum=0;
		else if(i==12) lNum=-2;

		mlstButtons.push_back(hplNew( cNumericalButton, (mpInit,this,vPos,cVector2f(62,30),lNum)) );
        
		if(i%3 ==0)
		{
			vPos.y += (30 + 16);
			vPos.x = 307;
		}
		else
		{
			vPos.x += 62 + 16;
		}
	}

	Reset();
}

//-----------------------------------------------------------------------

cNumericalPanel::~cNumericalPanel(void)
{
	STLDeleteAll(mlstButtons);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------

void cNumericalPanel::Reset()
{
	mbActive = false;
	mfAlpha =0;
	mbMouseIsDown = false;
}

//-----------------------------------------------------------------------

void cNumericalPanel::OnDraw()
{
	if(mfAlpha == 0) return;
	
	mpDrawer->DrawGfxObject(mpGfxBackground,mpInit->mpGraphicsHelper->GetScreenPos(),mpInit->mpGraphicsHelper->GetScreenSize(),cColor(1,mfAlpha));

	
	mpDrawer->DrawGfxObject(mpGfxPanel,cVector3f(280,170,10),cVector2f(270,300),cColor(1,mfAlpha));

	////////////////////////////////
	// Draw buttons
	tNumericalButtonListIt it = mlstButtons.begin();
	for(; it != mlstButtons.end(); ++it)
	{
		cNumericalButton *pButton = *it;

		pButton->OnDraw();
	}
}

//-----------------------------------------------------------------------

void cNumericalPanel::Update(float afTimeStep)
{
	////////////////////////////////
	// Check active and fade
	if(mbActive==false)
	{
		mfAlpha -=2.5f * afTimeStep;
		if(mfAlpha < 0)mfAlpha =0;
		return;
	}
	else
	{
		//Bad hack fix... owell
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Pointer);

		mfAlpha += 2.3f * afTimeStep;
		if(mfAlpha >1)mfAlpha =1;
	}
	
	////////////////////////////////
	// Update buttons
	tNumericalButtonListIt it = mlstButtons.begin();
	for(; it != mlstButtons.end(); ++it)
	{
		cNumericalButton *pButton = *it;

		pButton->OnUpdate(afTimeStep);

		if(cMath::PointBoxCollision(mvMousePos,pButton->GetRect()))
		{
			pButton->OnMouseOver(true);
		}
		else
		{
			pButton->OnMouseOver(false);
		}
	}
}

//-----------------------------------------------------------------------

void cNumericalPanel::SetMousePos(const cVector2f &avPos)
{
	mvMousePos = avPos;
	mpInit->mpPlayer->SetCrossHairPos(mvMousePos);
}

void cNumericalPanel::AddMousePos(const cVector2f &avRel)
{
	mvMousePos += avRel;

	mpInit->mpGraphicsHelper->ClampToScreen(mvMousePos);

	mpInit->mpPlayer->SetCrossHairPos(mvMousePos);
}

//-----------------------------------------------------------------------

void cNumericalPanel::OnMouseDown(eMButton aButton)
{
	////////////////////////////////
	// Dispatch button presses
	tNumericalButtonListIt it = mlstButtons.begin();
	for(; it != mlstButtons.end(); ++it)
	{
		cNumericalButton *pButton = *it;

		if(cMath::PointBoxCollision(mvMousePos,pButton->GetRect()))
		{
			pButton->OnMouseDown();
		}
	}

	mbMouseIsDown = true;
}

void cNumericalPanel::OnMouseUp(eMButton aButton)
{
	////////////////////////////////
	// Dispatch button releases
	tNumericalButtonListIt it = mlstButtons.begin();
	for(; it != mlstButtons.end(); ++it)
	{
		cNumericalButton *pButton = *it;

		if(cMath::PointBoxCollision(mvMousePos,pButton->GetRect()))
		{
			pButton->OnMouseUp();
		}
	}

	mbMouseIsDown = false;
}

//-----------------------------------------------------------------------

void cNumericalPanel::SetActive(bool abActive)
{
	if(mbActive == abActive) return;

	mbActive = abActive;

	if(mbActive)
	{
		if(mpInit->mbHasHaptics)
			mpInit->mpPlayer->GetHapticCamera()->SetActive(false);

		mpInit->mpGame->GetInput()->BecameTriggerd("RightClick");
		mpInit->mpGame->GetInput()->BecameTriggerd("LeftClick");
		

		mLastCrossHairState = mpInit->mpPlayer->GetCrossHairState();

		mpInit->mpPlayer->SetCrossHairPos(mvMousePos);
		mpInit->mpPlayer->SetCrossHairState(eCrossHairState_Pointer);

		mvDigits.clear();
	}
	else
	{
		if(mpInit->mbHasHaptics)
			mpInit->mpPlayer->GetHapticCamera()->SetActive(true);

		mpInit->mpPlayer->SetCrossHairState(mLastCrossHairState);
		mpInit->mpPlayer->SetCrossHairPos(cVector2f(400,300));
	}
}

//-----------------------------------------------------------------------

void cNumericalPanel::OnExit()
{
	SetActive(false);
}

//-----------------------------------------------------------------------

void cNumericalPanel::AddDigit(int alNum)
{
	mvDigits.push_back(alNum);

	if(mvDigits.size() == 4)
	{
		bool bCorrect = true;
		for(int i=0; i<4; ++i)
		{
			if(mvCode[i] != mvDigits[i])
			{
				bCorrect = false;
				break;
			}
		}

		tString sCorrect = bCorrect ? "true" : "false";

		tString sCommand = msCallback + "(\""+msName+"\", "+sCorrect+")";
		mpInit->RunScriptCommand(sCommand);

		SetActive(false);
	}
}

//-----------------------------------------------------------------------

void cNumericalPanel::SetUp(const tString& asName, const tString& asCallback)
{
	msName = asName;
	msCallback = asCallback;
}

void cNumericalPanel::SetCode(tIntVec &avCode)
{
	mvCode = avCode;
}

//-----------------------------------------------------------------------
