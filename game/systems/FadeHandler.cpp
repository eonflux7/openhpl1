// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "systems/FadeHandler.h"

#include "core/Init.h"
#include "player/Player.h"
#include "entities/GameEntity.h"
#include "ui/Inventory.h"
#include "core/GraphicsHelper.h"

cFadeHandler::cFadeHandler(cInit *apInit)  : iUpdateable("FadeHandler")
{
	mpInit = apInit;
	
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
	
	mpBlackGfx = mpDrawer->CreateGfxObject("effect_black.bmp","diffalpha2d");

	Reset();
}

cFadeHandler::~cFadeHandler(void)
{
	mpDrawer->DestroyGfxObject(mpBlackGfx);
}

void cFadeHandler::FadeOut(float afDurationSeconds)
{
	if(afDurationSeconds <= 0)
		mfAlphaRatePerSecond = 1000000.0f;
	else
		mfAlphaRatePerSecond = (1.0f / afDurationSeconds);

	mbActive = true;

	Update(1.0f/60.0f);
}

void cFadeHandler::FadeIn(float afDurationSeconds)
{
	if(afDurationSeconds <= 0)
		mfAlphaRatePerSecond = -100000.0f;
	else
		mfAlphaRatePerSecond = -(1.0f / afDurationSeconds);
	
	mbActive = true;

	Update(1.0f/60.0f);
}

bool cFadeHandler::IsActive()
{
	return mbActive;
}

void cFadeHandler::OnStart()
{

}

void cFadeHandler::SetWideScreenActive(bool abActive)
{
	mbWideScreenActive = abActive;
}

void cFadeHandler::Update(float afTimeStep)
{
	if(mbActive)
	{
		mfAlpha += mfAlphaRatePerSecond * afTimeStep;

		if(mfAlphaRatePerSecond<0)
		{
			if(mfAlpha<0)
			{
				mbActive = false;
				mfAlpha =0;
			}
		}
		else
		{
			if(mfAlpha>1)
			{
				mbActive = false;
				mfAlpha =1;
			}
		}
	}

	// Wide Screen
	if(mbWideScreenActive)
	{
		mfWideScreenAlpha += 0.8f*afTimeStep;
		if(mfWideScreenAlpha > 1) mfWideScreenAlpha = 1;
	}
	else
	{
		mfWideScreenAlpha -= 0.7f*afTimeStep;
		if(mfWideScreenAlpha < 0) mfWideScreenAlpha = 0;
	}
}

void cFadeHandler::Reset()
{
	mfWideScreenAlpha =0;
	mbWideScreenActive = false;

	mfAlpha = 0;
	mfAlphaRatePerSecond = 0;
	mbActive = false;
}

void cFadeHandler::OnDraw()
{
	if(mfAlpha !=0)
		mpDrawer->DrawGfxObject(mpBlackGfx,mpInit->mpGraphicsHelper->GetScreenPos(150),mpInit->mpGraphicsHelper->GetScreenSize(),cColor(1,mfAlpha));

	if(mfWideScreenAlpha != 0)
	{
		//The bars reach from the screen edges to where they end in the 800x600 layout.
		cVector3f vPos = mpInit->mpGraphicsHelper->GetScreenPos(40);
		cVector2f vSize = mpInit->mpGraphicsHelper->GetScreenSize();
		float fBarHeight = 75 - vPos.y;

		mpDrawer->DrawGfxObject(mpBlackGfx,vPos,cVector2f(vSize.x,fBarHeight),cColor(1,mfWideScreenAlpha));
		mpDrawer->DrawGfxObject(mpBlackGfx,cVector3f(vPos.x,525,40),cVector2f(vSize.x,fBarHeight),cColor(1,mfWideScreenAlpha));
	}
}
