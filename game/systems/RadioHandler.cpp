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
#include "systems/RadioHandler.h"

#include "core/Init.h"
#include "player/Player.h"
#include "entities/GameEntity.h"
#include "systems/EffectHandler.h"
#include "ui/GameGui.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cRadioHandler::cRadioHandler(cInit *apInit)  : iUpdateable("RadioHandler")
{
	mpInit = apInit;
	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");

	mpSoundHandler = mpInit->mpGame->GetSound()->GetSoundHandler();

	Reset();
}

//-----------------------------------------------------------------------

cRadioHandler::~cRadioHandler(void)
{
	STLDeleteAll(mlstMessages);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// GAME MESSAGE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cRadioMessage::cRadioMessage(const tWString &asText, const tString &asSound)
{
	msText = asText;
	msSound = asSound;

	mpChannel = nullptr;
	mpEntity = nullptr;
	mfMinDist = 0;
	mfMaxDist = 0;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cRadioHandler::Add(const tWString& asText, const tString& asSound)
{
	if(mlstMessages.empty())msPrevText = _W("");

	cRadioMessage *pMess = hplNew( cRadioMessage, (asText, asSound) );
	mlstMessages.push_back(pMess);
}

//-----------------------------------------------------------------------

void cRadioHandler::Add(const tWString& asText, const tString& asSound, iGameEntity *apEntity,
						float afMinDist, float afMaxDist)
{
	if(mlstMessages.empty())msPrevText = _W("");

	cRadioMessage *pMess = hplNew( cRadioMessage, (asText, asSound) );
	pMess->mpEntity = apEntity;
	pMess->mfMinDist = afMinDist;
	pMess->mfMaxDist = afMaxDist;
	mlstMessages.push_back(pMess);
}

//-----------------------------------------------------------------------

void cRadioHandler::StopAll(float afFadeSpeed)
{
	STLDeleteAll(mlstMessages);
	mlstMessages.clear();

	msCurrentText = _W("");
	msPrevText = _W("");
	mfAlpha = 0;
	msOnEndCallback = "";
	mbGuiCallback = false;

	if(mpCurrentMessage)
	{
		iSoundChannel *pChannel = mpCurrentMessage->mpChannel;
		cSoundEntry *pEntry = mpSoundHandler->GetEntryFromSound(pChannel);
		if(pEntry)
		{
			pEntry->mfNormalVolumeFadeDest = 0;
			pEntry->mfNormalVolumeFadeSpeed = -afFadeSpeed;
		}
		else if(mpSoundHandler->IsValid(pChannel))
		{
			pChannel->Stop();
		}

		hplDelete( mpCurrentMessage );
		mpCurrentMessage = nullptr;
	}
}

//-----------------------------------------------------------------------

void cRadioHandler::OnWorldExit()
{
	//The messages heard from a place of the map end with the map
	if(mpCurrentMessage && mpCurrentMessage->mpEntity)
	{
		if(mpSoundHandler->IsValid(mpCurrentMessage->mpChannel))
			mpCurrentMessage->mpChannel->Stop();

		hplDelete( mpCurrentMessage );
		mpCurrentMessage = nullptr;

		msCurrentText = _W("");
		msPrevText = _W("");
		mfAlpha = 0;
	}

	for(tRadioMessageListIt it = mlstMessages.begin(); it != mlstMessages.end(); )
	{
		cRadioMessage *pMess = *it;
		if(pMess->mpEntity)
		{
			hplDelete( pMess );
			it = mlstMessages.erase(it);
		}
		else
		{
			++it;
		}
	}
}

//-----------------------------------------------------------------------

void cRadioHandler::StartNextMessage()
{
	//Get newer message
	mpCurrentMessage = mlstMessages.front();
	mlstMessages.pop_front();

	msCurrentText = mpCurrentMessage->msText;

	mfAlpha =0;

	//Start newer message
	mpCurrentMessage->mpChannel = mpSoundHandler->PlayStream(mpCurrentMessage->msSound,false,1);

	if(mpCurrentMessage->mpEntity)
	{
		cSoundEntry *pEntry = mpSoundHandler->GetEntryFromSound(mpCurrentMessage->mpChannel);
		if(pEntry) pEntry->mfNormalVolume = GetVolumeAtPlayer(mpCurrentMessage);
	}
}

//-----------------------------------------------------------------------

void cRadioHandler::RunOnEndCallback()
{
	if(msOnEndCallback == "") return;

	// Cleared first: the function may set the next one
	tString sCommand = msOnEndCallback + "()";
	msOnEndCallback = "";

	if(mbGuiCallback)
	{
		cGameGui *pGui = mpInit->mpGameGuiHandler->GetScreenGui();
		if(pGui) pGui->RunScriptCommand(sCommand);
	}
	else
	{
		mpInit->RunScriptCommand(sCommand);
	}
}

//-----------------------------------------------------------------------

void cRadioHandler::NextMessage()
{
	if(mpCurrentMessage)
	{
		if(mpSoundHandler->IsValid(mpCurrentMessage->mpChannel)) mpCurrentMessage->mpChannel->Stop();

		msPrevText = mpCurrentMessage->msText;
		hplDelete( mpCurrentMessage );
		mpCurrentMessage = nullptr;
		msCurrentText = _W("");
	}

	if(mlstMessages.empty()==false)
		StartNextMessage();
	else
		RunOnEndCallback();
}

//-----------------------------------------------------------------------

bool cRadioHandler::IsShowingText()
{
	return IsActive() && mpInit->mbSubtitles && mbShowText;
}

//-----------------------------------------------------------------------

float cRadioHandler::GetVolumeAtPlayer(cRadioMessage *apMessage)
{
	cVector3f vPos = apMessage->mpEntity->GetBody(0)->GetLocalPosition();
	float fDist = cMath::Vector3Dist(mpInit->mpPlayer->GetCamera()->GetPosition(), vPos);

	if(fDist > apMessage->mfMaxDist) return 0;
	if(fDist <= apMessage->mfMinDist) return 1;

	return 1 - (fDist - apMessage->mfMinDist) / (apMessage->mfMaxDist - apMessage->mfMinDist);
}

//-----------------------------------------------------------------------

void cRadioHandler::Update(float afTimeStep)
{
	if(mpInit->mpPlayer->IsDead()) {
		STLDeleteAll(mlstMessages);
		mlstMessages.clear();
		return;
	}
	
	/////////////////////////////
	//Update current message
	mbShowText = true;
	if(mpCurrentMessage)
	{
		//A message from a place: not read when too far away to be heard
		cSoundEntry *pEntry = mpSoundHandler->GetEntryFromSound(mpCurrentMessage->mpChannel);
		if(pEntry && mpCurrentMessage->mpEntity)
		{
			pEntry->mfNormalVolume = GetVolumeAtPlayer(mpCurrentMessage);
			if(pEntry->mfNormalVolume <= 0) mbShowText = false;
		}

		if(mpSoundHandler->IsValid(mpCurrentMessage->mpChannel)==false)
		{
			msCurrentText = _W("");
			msPrevText = mpCurrentMessage->msText;
			hplDelete( mpCurrentMessage );
			mpCurrentMessage = nullptr;

			if(mlstMessages.empty()) RunOnEndCallback();
		}
	}

	////////////////////////////
	// Get next message
    if(mpCurrentMessage==nullptr && mlstMessages.empty()==false)
	{
		StartNextMessage();
	}
	
	////////////////////////
	//Alpha
	if(mfAlpha <1)
	{
		mfAlpha += afTimeStep*2;
		if(mfAlpha >1){
			mfAlpha =1;
			msPrevText = msCurrentText;
		}
	}
}

//-----------------------------------------------------------------------

bool cRadioHandler::IsActive()
{
	if(mpCurrentMessage) return true;

	//Black Plague: also while messages wait
	return cInit::IsOverture()==false && mlstMessages.empty()==false;
}

//-----------------------------------------------------------------------

void cRadioHandler::OnDraw()
{
	float fAlpha = mfAlpha;

	if(mpInit->mbSubtitles==false || mbShowText==false) return;

	// Over an open computer terminal the text is drawn by the gui, which is on top
	if(mpInit->mpGameGuiHandler->HasOpenScreenGui())
	{
		mpInit->mpGameGuiHandler->DrawText(mpFont, msCurrentText, fAlpha);

		if(msPrevText != msCurrentText && mfAlpha <1)
			mpInit->mpGameGuiHandler->DrawText(mpFont, msPrevText, 1-fAlpha);

		return;
	}

	if(msCurrentText !=_W(""))
	{
		mpFont->DrawWordWrap(cVector3f(25, 500,47),750,16,15,cColor(1,fAlpha),
							eFontAlign_Left,msCurrentText);
		mpFont->DrawWordWrap(cVector3f(25, 500,46) + cVector3f(2,2,0),750,16,15,cColor(0,fAlpha),
							eFontAlign_Left,msCurrentText);
	}

	if(msPrevText !=_W("") && msPrevText != msCurrentText && mfAlpha <1)
	{
		mpFont->DrawWordWrap(cVector3f(25, 500,47),750,16,15,cColor(1,1-fAlpha),
			eFontAlign_Left,msPrevText);
		mpFont->DrawWordWrap(cVector3f(25, 500,46) + cVector3f(2,2,0),750,16,15,cColor(0,1-fAlpha),
			eFontAlign_Left,msPrevText);
	}
}


//-----------------------------------------------------------------------

void cRadioHandler::Reset()
{
	STLDeleteAll(mlstMessages);
	mlstMessages.clear();

	msCurrentText = _W("");
	msPrevText = _W("");
	
	mfAlpha =0;
	mbShowText = true;

	mpCurrentMessage = nullptr;

	msOnEndCallback = "";
	mbGuiCallback = false;
}

//-----------------------------------------------------------------------
