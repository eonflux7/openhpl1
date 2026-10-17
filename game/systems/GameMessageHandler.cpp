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
#include "systems/GameMessageHandler.h"

#include "core/Init.h"
#include "player/Player.h"
#include "systems/EffectHandler.h"
#include "ui/Inventory.h"
#include "ui/Notebook.h"
#include "ui/NumericalPanel.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameMessageHandler::cGameMessageHandler(cInit *apInit)  : iUpdateable("GameMessageHandler")
{
	mpInit = apInit;

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");

	msOverCallback = "";
	
	mbDepthOfFieldUsed = false;

	mbBlackText = false;
}

//-----------------------------------------------------------------------

cGameMessageHandler::~cGameMessageHandler(void)
{
	STLDeleteAll(mlstMessages);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// GAME MESSAGE
//////////////////////////////////////////////////////////////////////////

cGameMessage::cGameMessage(const tWString &asText, cGameMessageHandler *apMessageHandler)
{
	mpMessageHandler = apMessageHandler;

	msText = asText;
	mfTextAlpha =0;
	mbActive =false;
	mfTextAlphaRatePerSecond =1.3f;
}

//-----------------------------------------------------------------------

void cGameMessage::Update(float afTimeStep)
{
	if(mbActive)
	{
		mfTextAlpha += mfTextAlphaRatePerSecond*afTimeStep;

		if(mfTextAlphaRatePerSecond<0)
		{
			if(mfTextAlpha<=0)
			{
				mfTextAlpha =0;
				mbActive =0;
			}
		}
		else
		{
			if(mfTextAlpha>1)
			{
				mfTextAlpha = 1;
			}
		}
	}
}

//-----------------------------------------------------------------------

void cGameMessage::Draw(iFontData *apFont)
{
	if(mbActive == false) return;
	
	float fSide=30;
	if(mpMessageHandler->mbBlackText)
	{
		apFont->DrawWordWrap(cVector3f(fSide, 300,152),800 - fSide*2,16,14,cColor(0,mfTextAlpha),
			eFontAlign_Left,msText);
	}
	else
	{
		apFont->DrawWordWrap(cVector3f(fSide, 300,152),800 - fSide*2,16,14,cColor(1,1,1,mfTextAlpha),
							eFontAlign_Left,msText);
		apFont->DrawWordWrap(cVector3f(fSide, 300,150),800 - fSide*2,16,14,cColor(0,0,0,mfTextAlpha),
							eFontAlign_Left,msText,eFontEffect_Outline,1);
	}
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameMessageHandler::Add(const tWString& asText)
{
	if(mpInit->mpPlayer->GetHealth() <=0) return;

	cGameMessage *pMessage = hplNew( cGameMessage, (asText,this) );
	mlstMessages.push_back(pMessage);

	if(mpInit->mpPlayer->GetState() != ePlayerState_Message)
	{
		if(mpInit->mpInventory->IsActive()) mpInit->mpInventory->SetActive(false);
		if(mpInit->mpNotebook->IsActive()) mpInit->mpNotebook->SetActive(false);
		if(mpInit->mpNumericalPanel->IsActive()) mpInit->mpNumericalPanel->SetActive(false);

		mLastState = mpInit->mpPlayer->GetState();
		mpInit->mpPlayer->ChangeState(ePlayerState_Message);

		pMessage->mbActive = true;
	}
}

//-----------------------------------------------------------------------

void cGameMessageHandler::ShowNext()
{
	bool bFoundNextMessage = false;
	cGameMessage *pPreviousMessage = nullptr;
	
	// Allow advancing once the current message reaches 20% opacity.
	if(mlstMessages.empty() == false)
	{
		cGameMessage *pMessage = mlstMessages.front();
		if(pMessage->mfTextAlpha <0.2f && pMessage->mfTextAlphaRatePerSecond>0) return;
	}

	tGameMessageListIt it = mlstMessages.begin();
	for(;it != mlstMessages.end(); ++it)
	{
		cGameMessage *pMessage = *it;
		if(pMessage->mbActive==false)
		{
			pMessage->mbActive=true;
			bFoundNextMessage = true;
			if(pPreviousMessage) pPreviousMessage->mfTextAlphaRatePerSecond = -pPreviousMessage->mfTextAlphaRatePerSecond;
			break;
		}
		pPreviousMessage = pMessage;
	}

	//Check if this is the last message.
	if(bFoundNextMessage==false)
	{
		if(pPreviousMessage) pPreviousMessage->mfTextAlphaRatePerSecond = -pPreviousMessage->mfTextAlphaRatePerSecond;
		
		if(mLastState != ePlayerState_Grab &&
			mLastState != ePlayerState_Push &&
			mLastState != ePlayerState_Move &&
			mLastState != ePlayerState_UseItem)
		{
			mpInit->mpPlayer->ChangeState(mLastState);
		}
		else
		{
			mpInit->mpPlayer->ChangeState(ePlayerState_Normal);
		}

		if(mbDepthOfFieldUsed)
		{
			mbDepthOfFieldUsed = false;
			mpInit->mpEffectHandler->GetDepthOfField()->SetActive(false,2.0f);
		}
		
		
		//Check if there is a script callback
		if(msOverCallback != "")
		{
			tString sCommand = msOverCallback + "()";
			msOverCallback ="";

			mpInit->RunScriptCommand(sCommand);
		}
		else
		{
		}
	}

}

//-----------------------------------------------------------------------

void cGameMessageHandler::Update(float afTimeStep)
{
	if(mpInit->mpPlayer->IsDead())
	{
		STLDeleteAll(mlstMessages);
		mlstMessages.clear();
		return;
	}
	
	int lCount=0;
	tGameMessageListIt it = mlstMessages.begin();
	for(;it != mlstMessages.end(); ++lCount)
	{
		cGameMessage *pMessage = *it;
		
		pMessage->Update(afTimeStep);
		
		if(lCount==0 && pMessage->mbActive==false)
		{
			hplDelete( pMessage );
			it = mlstMessages.erase(it);
		}
		else
		{
			++it;
		}
	}
}

//-----------------------------------------------------------------------

void cGameMessageHandler::OnWorldLoad()
{

}

//-----------------------------------------------------------------------

void cGameMessageHandler::OnWorldExit()
{
	Reset();
	mpInit->mpEffectHandler->GetDepthOfField()->SetActive(false,1.0f);
}

//-----------------------------------------------------------------------

void cGameMessageHandler::OnDraw()
{
	tGameMessageListIt it = mlstMessages.begin();
	for(;it != mlstMessages.end(); ++it)
	{
		cGameMessage *pMessage = *it;
		pMessage->Draw(mpFont);
	}
}

//-----------------------------------------------------------------------

void cGameMessageHandler::SetOnMessagesOverCallback(const tString& asFunction)
{
	msOverCallback = asFunction;
}

//-----------------------------------------------------------------------

void cGameMessageHandler::Reset()
{
	STLDeleteAll(mlstMessages);
	mlstMessages.clear();

	mbDepthOfFieldUsed = false;
}

//-----------------------------------------------------------------------
