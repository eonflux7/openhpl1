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
#include "core/StdAfx.h"
#include "ui/Ending.h"

#include "core/Init.h"
#include "core/ButtonHandler.h"
#include "core/GraphicsHelper.h"
#include "systems/FadeHandler.h"
#include "ui/Credits.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEnding::cEnding(cInit *apInit)  : iUpdateable("Ending")
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_computer.fnt");

	Reset();
}

//-----------------------------------------------------------------------

cEnding::~cEnding()
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cEnding::Reset()
{
	mbActive = false;
	mbTextActive = false;
	mlState = 0;

	mlTextLength = 0;
	mlShownChars = 0;
	mfCharacterDelaySeconds = 0;
	mfTypeSoundDelaySeconds = 0;

	mfTextColor = 1;
	mfWaitElapsedSeconds = 0;
	mfAlpha = 1;

	mfBlinkMin = 0;
	mfBlinkMax = 1;
	mfBlinkAlpha = 0;
	mfBlinkUpSpeed = 1;
	mfBlinkDownSpeed = -1;
	mfBlinkSpeed = 1;
}

//-----------------------------------------------------------------------

void cEnding::OnPostSceneDraw()
{
	if(mbActive==false) return;

	mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));
}

//-----------------------------------------------------------------------

void cEnding::OnDraw()
{
	if(mbActive==false) return;

	DrawText();

	if(mlState == 5 && mlShownChars >= mlTextLength && mfWaitElapsedSeconds >= 3)
	{
		mpFont->Draw(cVector3f(400,550,0),15,cColor(1,mfBlinkAlpha * mfAlpha),eFontAlign_Center,
					_W("%ls"),kTranslate("Ending","MessageSent").c_str());
	}
}

//-----------------------------------------------------------------------

void cEnding::Update(float afTimeStep)
{
	if(mbActive==false) return;

	bool bTextDone = mlShownChars >= mlTextLength;

	switch(mlState)
	{
	//Ending 1: the first mail, then the second
	case 0:
		if(bTextDone)
		{
			mfWaitElapsedSeconds += afTimeStep;
			if(mfWaitElapsedSeconds >= 3)
			{
				mfWaitElapsedSeconds = 0;
				mlState = 1;
				SetCurrentText("Mail02");
			}
		}
		break;
	//The screen fades
	case 1:
		mfWaitElapsedSeconds += afTimeStep;
		if(mfWaitElapsedSeconds >= 3)
		{
			mfAlpha -= 0.5f * afTimeStep;
			if(mfAlpha <= 0)
			{
				mfAlpha = 0;
				mfWaitElapsedSeconds = 0;
				mlState = 2;
			}
		}
		break;
	//Back to the game
	case 2:
		mfWaitElapsedSeconds += afTimeStep;
		if(mfWaitElapsedSeconds >= 3)
		{
			mfAlpha = 1;
			mfWaitElapsedSeconds = 0;
			SetActive(false, 1);
			mpInit->mpFadeHandler->FadeIn(5);
			return;
		}
		break;
	//Ending 2: three mails
	case 3:
		if(bTextDone)
		{
			mfWaitElapsedSeconds += afTimeStep;
			if(mfWaitElapsedSeconds >= 3)
			{
				mfWaitElapsedSeconds = 0;
				mlState = 4;
				SetCurrentText("Mail04");
			}
		}
		break;
	case 4:
		if(bTextDone)
		{
			mfWaitElapsedSeconds += afTimeStep;
			if(mfWaitElapsedSeconds >= 3)
			{
				mfWaitElapsedSeconds = 0;
				mlState = 5;
				SetCurrentText("Mail05");
			}
		}
		break;
	//"Message sent" blinks, with a beep each time, then the credits
	case 5:
		if(bTextDone)
		{
			mfWaitElapsedSeconds += afTimeStep;
			if(mfWaitElapsedSeconds >= 3)
			{
				mfBlinkAlpha += afTimeStep * mfBlinkSpeed;
				if(mfBlinkSpeed > 0)
				{
					if(mfBlinkAlpha >= mfBlinkMax)
					{
						mfBlinkAlpha = mfBlinkMax;
						mfBlinkSpeed = mfBlinkDownSpeed;
					}
				}
				else if(mfBlinkAlpha <= mfBlinkMin)
				{
					mfBlinkAlpha = mfBlinkMin;
					mfBlinkSpeed = mfBlinkUpSpeed;
				}

				if(mfBlinkAlpha == mfBlinkMax)
				{
					mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("22_message_beep",false,1);
				}
			}

			if(mfWaitElapsedSeconds >= 9)
			{
				mlState++;
				mfWaitElapsedSeconds = 0;
				SetActive(false, 0);
				return;
			}
		}
		break;
	}

	UpdateText(afTimeStep);
}

//-----------------------------------------------------------------------

void cEnding::OnExit()
{
}

//-----------------------------------------------------------------------

// A key or click shows the whole text, or skips the wait after it
void cEnding::OnMouseDown(eMButton aButton)
{
	OnButtonDown();
}

void cEnding::OnButtonDown()
{
	if(mlState == 1)
	{
		if(mfWaitElapsedSeconds < 3)	mfWaitElapsedSeconds = 3;
		else			mfAlpha = 0;
		return;
	}
	if(mlState == 2)
	{
		mfWaitElapsedSeconds = 3;
		return;
	}

	if(mlShownChars < mlTextLength)
		mlShownChars = mlTextLength;
	else
		mfWaitElapsedSeconds = 1000;
}

//-----------------------------------------------------------------------

void cEnding::SetActive(bool abActive, int alType)
{
	if(mbActive == abActive) return;

	mbActive = abActive;

	if(mbActive)
	{
		mpInit->mpGame->GetUpdater()->SetContainer("Ending");
		mpInit->mpGame->GetScene()->SetDrawScene(false);
		mpInit->mpGame->GetScene()->SetUpdateMap(false);
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_Ending);

		mlState = alType;
		if(alType == 0)
		{
			SetCurrentText("Mail01");
		}
		else
		{
			SetCurrentText("Mail03");
			if(mpInit->mpGame->GetScene()->GetWorld3D())
				mpInit->mpGame->GetScene()->GetWorld3D()->DestroyAllSoundEntities();
		}

		mbTextActive = true;
		mfWaitElapsedSeconds = 0;
	}
	else if(alType == 1)
	{
		mpInit->mpGame->GetUpdater()->SetContainer("Default");
		mpInit->mpGame->GetScene()->SetDrawScene(true);
		mpInit->mpGame->GetScene()->SetUpdateMap(true);
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_Game);
	}
	else
	{
		mpInit->mpCredits->SetActive(true);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cEnding::SetCurrentText(const tString &asEntry)
{
	mvRows.clear();

	tWString sText = kTranslate("Ending", asEntry);
	mpFont->GetWordWrapRows(750, 17, 15, sText, &mvRows);

	mlTextLength = 0;
	for(size_t i=0; i<mvRows.size(); ++i) mlTextLength += (int)mvRows[i].size();

	mlShownChars = 0;
	mfTypeSoundDelaySeconds = 0;
	mfCharacterDelaySeconds = 0;
}

//-----------------------------------------------------------------------

// Type a character every 1/19 s, pausing before an empty row, with occasional key sounds.
void cEnding::UpdateText(float afTimeStep)
{
	if(mbTextActive==false || mlShownChars >= mlTextLength) return;

	mfCharacterDelaySeconds -= afTimeStep;
	mfTypeSoundDelaySeconds -= afTimeStep;

	if(mfCharacterDelaySeconds > 0) return;

	if(mfTypeSoundDelaySeconds <= 0)
	{
		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_type",false,1);
		mfTypeSoundDelaySeconds = 0.125f + cMath::RandRectf(-0.05f, 0.05f);
	}

	mlShownChars++;
	mfCharacterDelaySeconds = 0.0526f;

	// Pause before an empty row.
	int lCount = 0;
	for(size_t i=0; i<mvRows.size(); ++i)
	{
		lCount += (int)mvRows[i].size();
		if(lCount > mlShownChars) break;
		if(lCount == mlShownChars && i+1 < mvRows.size() && mvRows[i+1].size() == 0)
		{
			mfCharacterDelaySeconds = 1;
			break;
		}
	}
}

//-----------------------------------------------------------------------

void cEnding::DrawText()
{
	if(mbTextActive==false) return;

	cColor col(mfTextColor, mfAlpha);

	int lCount = 0;
	for(size_t i=0; i<mvRows.size(); ++i)
	{
		cVector3f vPos(25, 90 + 17.0f*(float)i, 10);
		int lRowLength = (int)mvRows[i].size();

		if(lCount + lRowLength >= mlShownChars)
		{
			tWString sPart = mvRows[i].substr(0, mlShownChars - lCount);
			mpFont->Draw(vPos,15,col,eFontAlign_Left,_W("%ls"),sPart.c_str());
			break;
		}

		mpFont->Draw(vPos,15,col,eFontAlign_Left,_W("%ls"),mvRows[i].c_str());
		lCount += lRowLength;
	}
}

//-----------------------------------------------------------------------
