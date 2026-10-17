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
#include "ui/Credits.h"

#include "core/Init.h"
#include "core/GraphicsHelper.h"
#include "core/ButtonHandler.h"
#include "ui/MainMenu.h"
#include "core/MapHandler.h"
#include "player/Player.h"
#include "player/PlayerHands.h"
#include "systems/GameMusicHandler.h"
#include "systems/GameMessageHandler.h"
#include "player/HapticGameCamera.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cCredits::cCredits(cInit *apInit)  : iUpdateable("Credits")
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	//Load fonts
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");
	
	//Load text
	tWString sText = cInit::GetGameId() == eGameId_Requiem ? kTranslate("RequiemMenu", "CreditsText") :
															kTranslate("MainMenu", "CreditsText");
	mpFont->GetWordWrapRows(750, 19,17,sText,&mvTextRows);
	
	Reset();
}

//-----------------------------------------------------------------------

cCredits::~cCredits(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cCredits::Reset()
{
	mbActive = false;
	
	mfYPos =600;
	mlPage = 0;
	mfPageElapsedSeconds = 0;

	mfBlink = 0;
	mfBlinkSpeed = 1;
	mlSecretTextIndex = 0;
	mvSecretRows.clear();
	mlSecretChars = 0;
	mlSecretCharsShown = 0;
	mfCharacterDelaySeconds = 0;
	mfTypeSoundDelaySeconds = 0;
}

//-----------------------------------------------------------------------

void cCredits::OnPostSceneDraw()
{
	mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));
	
}

//-----------------------------------------------------------------------


void cCredits::OnDraw()
{
	if(mlPage > 0)
	{
		DrawStatistics();
		return;
	}

	float fSize[2] = {17,19};
	float fY = mfYPos;

	// The text scrolls less than a pixel per frame.
	mpFont->SetVerticalPixelSnap(false);

	for(size_t i=0; i< mvTextRows.size(); ++i)
	{
		int lSize =0;
		if(mvTextRows[i][0] == _W('*'))
		{
			lSize=1;
		}
		if(mvTextRows[i].size()<=1)
		{
			fY += fSize[lSize];
		}

		if(fY >= -fSize[lSize])
		{
			if(fY > 600) continue;

			float fAlpha = fY/300;
			if(fAlpha > 1) fAlpha = (2 - fAlpha);
			
			if(lSize==0)
				mpFont->Draw(cVector3f(400,fY,10),fSize[lSize],cColor(1,fAlpha),eFontAlign_Center,
							mvTextRows[i].c_str());
			else
				mpFont->Draw(cVector3f(400,fY,10),fSize[lSize],cColor(0.8f,fAlpha),eFontAlign_Center,
							mvTextRows[i].substr(1).c_str());
		}
		fY += fSize[lSize];
	}

	mpFont->SetVerticalPixelSnap(true);
}

//-----------------------------------------------------------------------

void cCredits::Update(float afTimeStep)
{
	//Make sure haptics is off
	if(mpInit->mbHasHaptics)
	{
		mpInit->mpGame->GetHaptic()->GetLowLevel()->StopAllForces();
	}

	if(mlPage > 0)
	{
		mfPageElapsedSeconds += afTimeStep;
		if(cInit::GetGameId() == eGameId_Requiem) UpdateRequiemSecret(afTimeStep);
		return;
	}

	mfYPos -= afTimeStep * 30;
	cMusicHandler *pMusicHandler = mpInit->mpGame->GetSound()->GetMusicHandler();

	//Check if the credits are over.
	float fSize[2] = {17,19};
	float fY = mfYPos;
	for(size_t i=0; i< mvTextRows.size(); ++i)
	{
		int lSize =0;
		if(mvTextRows[i][0] == _W('*')) lSize=1;
		if(mvTextRows[i].size()<=1) fY += fSize[lSize];
		fY += fSize[lSize];
	}
	if(fY < -120 && pMusicHandler->GetCurrentSong() != nullptr){
		SetActive(false);
		pMusicHandler->Stop(0.1f);
	}
	if(cInit::IsOverture())
	{
		if(fY < -340){
			SetActive(false);
		}
	}
	// Black Plague and Requiem go on to the statistics
	else if(fY < -280)
	{
		++mlPage;
		mfPageElapsedSeconds = 0;
	}
}

//-----------------------------------------------------------------------

void cCredits::OnMouseDown(eMButton aButton)
{
	OnButtonDown();
}

//-----------------------------------------------------------------------

void cCredits::OnButtonDown()
{
	if(cInit::IsOverture())
	{
		SetActive(false);
		return;
	}

	// Requiem: a click shows the whole text being typed, the next one goes
	// on to the next text
	if(cInit::GetGameId() == eGameId_Requiem && mlPage == 3 && mlSecretTextIndex <= 3)
	{
		if(mlSecretCharsShown != mlSecretChars)
		{
			mlSecretCharsShown = mlSecretChars;
			return;
		}
		SetSecretText(mlSecretTextIndex + 1);
		if(mlSecretTextIndex <= 3) return;
	}

	// The statistics are kept three seconds, the secret pages two
	if(mlPage == 1 && mfPageElapsedSeconds < 3) return;
	if(mlPage > 1 && mfPageElapsedSeconds < 2) return;

	NextPage();
}

//-----------------------------------------------------------------------

void cCredits::NextPage()
{
	++mlPage;
	mfPageElapsedSeconds = 0;

	if(mlPage == 2)
	{
		cPlayer *pPlayer = mpInit->mpPlayer;
		if(cInit::GetGameId() == eGameId_Requiem)
		{
			mpInit->mpGame->GetSound()->GetMusicHandler()->Stop(1.8f);
			mfBlink = 0;
			mfBlinkSpeed = 1;
		}
		// The secret only for all artefacts found
		if(pPlayer->mlStat_Collectables < pPlayer->mlStat_MaxCollectables)
		{
			SetActive(false);
			return;
		}
	}
	if(cInit::GetGameId() == eGameId_Requiem)
	{
		if(mlPage == 3)
		{
			mpInit->mpGame->GetSound()->GetMusicHandler()->Play("Penumbra_BP_T1.ogg",1,1.3f,false);
			SetSecretText(0);
		}
		if(mlPage == 5) SetActive(false);
		return;
	}
	if(mlPage == 4) SetActive(false);
}

//-----------------------------------------------------------------------

void cCredits::SetSecretText(int alTextIndex)
{
	mlSecretTextIndex = alTextIndex;
	mvSecretRows.clear();

	// There are four texts; Requiem asks for a fifth that is not there
	if(alTextIndex <= 3)
		mpFont->GetWordWrapRows(750,17,15,kTranslate("GUI_exp","9_10" + cString::ToString(alTextIndex)),&mvSecretRows);

	mlSecretChars = 0;
	for(size_t i=0; i<mvSecretRows.size(); ++i) mlSecretChars += (int)mvSecretRows[i].size();
	mlSecretCharsShown = 0;
	mfCharacterDelaySeconds = 0;
	mfTypeSoundDelaySeconds = 0;
}

//-----------------------------------------------------------------------

void cCredits::UpdateRequiemSecret(float afTimeStep)
{
	if(mlPage == 2)
	{
		// The transmission's text fades in and out, a beep at its brightest
		mfBlink += afTimeStep * mfBlinkSpeed;
		if(mfBlinkSpeed > 0 && mfBlink >= 1)
		{
			mfBlink = 1;
			mfBlinkSpeed = -1;
		}
		else if(mfBlinkSpeed <= 0 && mfBlink <= 0)
		{
			mfBlink = 0;
			mfBlinkSpeed = 1;
		}
		if(mfBlink == 1)
			mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("22_message_beep",false,1);
	}
	else if(mlPage == 3)
	{
		if(mlSecretCharsShown >= mlSecretChars) return;

		mfCharacterDelaySeconds -= afTimeStep;
		mfTypeSoundDelaySeconds -= afTimeStep;
		if(mfCharacterDelaySeconds > 0) return;

		if(mfTypeSoundDelaySeconds <= 0)
		{
			mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_type",false,1);
			mfTypeSoundDelaySeconds = 0.125f + cMath::RandRectf(-0.05f,0.05f);
		}

		++mlSecretCharsShown;
		mfCharacterDelaySeconds = 0.025f;

		// A second's pause where a row ends before an empty one
		int lCount = 0;
		for(size_t i=1; i<mvSecretRows.size(); ++i)
		{
			lCount += (int)mvSecretRows[i-1].size();
			if(lCount > mlSecretCharsShown) break;
			if(lCount == mlSecretCharsShown && mvSecretRows[i].empty()) mfCharacterDelaySeconds = 1;
		}
	}
}

//-----------------------------------------------------------------------

void cCredits::DrawSecretText()
{
	int lCount = 0;
	for(size_t i=0; i<mvSecretRows.size(); ++i)
	{
		cVector3f vPos(25, 90 + (float)i*17, 10);
		int lRowSize = (int)mvSecretRows[i].size();
		if(lCount + lRowSize >= mlSecretCharsShown)
		{
			mpFont->Draw(vPos,15,cColor(1,1),eFontAlign_Left,_W("%ls"),
						mvSecretRows[i].substr(0, mlSecretCharsShown - lCount).c_str());
			return;
		}
		mpFont->Draw(vPos,15,cColor(1,1),eFontAlign_Left,_W("%ls"),mvSecretRows[i].c_str());
		lCount += lRowSize;
	}
}

//-----------------------------------------------------------------------

void cCredits::DrawStatRow(float afY, const tString &asLabel, const tWString &asValue)
{
	mpFont->Draw(cVector3f(380,afY,10),14,cColor(1,1),eFontAlign_Right,
				kTranslate("Stats",asLabel).c_str());
	mpFont->Draw(cVector3f(420,afY,10),14,cColor(1,1),eFontAlign_Left,asValue.c_str());
}

void cCredits::DrawStatistics()
{
	cPlayer *pPlayer = mpInit->mpPlayer;

	if(mlPage == 1)
	{
		mpFont->Draw(cVector3f(400,100,10),24,cColor(1,1),eFontAlign_Center,
					kTranslate("Stats","Statistics").c_str());

		// Counted in player updates
		int lUpdatesPerSec = cMath::Max(mpInit->mpGame->GetUpdatesPerSec(), 1);
		int lSecs = pPlayer->mlStat_PlayTime / lUpdatesPerSec;
		wchar_t sBuffer[64];
		swprintf(sBuffer,64,_W("%dh %dm %ds"),lSecs/3600, (lSecs/60)%60, lSecs%60);

		float fY = 184;
		DrawStatRow(fY,"Completion Time:",sBuffer);
		fY += 34;

		if(cInit::GetGameId() != eGameId_Requiem)
		{
			swprintf(sBuffer,64,_W("%d"),pPlayer->mlStat_NumOfSaves);
			DrawStatRow(fY,"Save artifacts found:",sBuffer);
			fY += 34;

			swprintf(sBuffer,64,_W("%d / %d"),pPlayer->mlStat_Notes,pPlayer->mlStat_MaxNotes);
			DrawStatRow(fY,"Notes found:",sBuffer);
			fY += 34;
		}

		swprintf(sBuffer,64,_W("%d / %d"),pPlayer->mlStat_Collectables,pPlayer->mlStat_MaxCollectables);
		DrawStatRow(fY,"Collectables found:",sBuffer);
	}
	else if(cInit::GetGameId() == eGameId_Requiem)
	{
		if(mlPage == 2)
			mpFont->DrawWordWrap(cVector3f(400,200,10),580,18,17,cColor(1,mfBlink),eFontAlign_Center,
								kTranslate("GUI_exp","10_aaa"));
		else if(mlPage == 3)
			DrawSecretText();
		else if(mlPage == 4)
			mpFont->Draw(cVector3f(400,300,10),25,cColor(1,1),eFontAlign_Center,_W("ikj765"));
	}
	else if(mlPage == 2)
	{
		mpFont->DrawWordWrap(cVector3f(400,200,10),580,18,17,cColor(1,1),eFontAlign_Center,
							kTranslate("Ending","SecretFound"));
	}
	else if(mlPage == 3)
	{
		mpFont->Draw(cVector3f(400,300,10),25,cColor(1,1),eFontAlign_Center,_W("xor451"));
	}
}

//-----------------------------------------------------------------------

void cCredits::SetActive(bool abActive)
{
	if(mbActive == abActive) return;

	mbActive = abActive;

	if(mbActive)
	{
		mpInit->mpGame->GetScene()->GetWorld3D()->DestroyAllSoundEntities();

		mpInit->mpGame->GetUpdater()->SetContainer("Credits");
		mpInit->mpGame->GetScene()->SetDrawScene(false);
		mpInit->mpGame->GetScene()->SetUpdateMap(false);
		if(mpInit->mbHasHaptics)
		{
			mpInit->mpGame->GetHaptic()->GetLowLevel()->SetUpdateShapes(false);
			mpInit->mpGame->GetHaptic()->GetLowLevel()->StopAllForces();
			mpInit->mpPlayer->GetHapticCamera()->SetActive(false);
		}
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_Credits);

		cMusicHandler *pMusic = mpInit->mpGame->GetSound()->GetMusicHandler();
		switch(cInit::GetGameId())
		{
		case eGameId_BlackPlague:	pMusic->Play("Penumbra_BP_E1.ogg",1,1.3f,false); break;
		case eGameId_Requiem:		pMusic->Play("Penumbra_Requiem_E.ogg",1,1.3f,false); break;
		default:					pMusic->Play("penumbra_music_E1_E",1,0.3f,false); break;
		}
		mlPage = 0;
		mfPageElapsedSeconds = 0;

		Log("Set Active!\n");
	}
	else
	{
		mpInit->mpGame->GetSound()->GetMusicHandler()->Stop(0.5f);

		mpInit->ResetGame(true);
		mpInit->mpMainMenu->SetActive(true);
	}
}

//-----------------------------------------------------------------------

void cCredits::OnExit()
{
	SetActive(false);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

