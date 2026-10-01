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
#include "systems/FadeHandler.h"

#include "core/Init.h"
#include "player/Player.h"
#include "entities/GameEntity.h"
#include "ui/Inventory.h"
#include "core/GraphicsHelper.h"



//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cFadeHandler::cFadeHandler(cInit *apInit)  : iUpdateable("FadeHandler")
{
	mpInit = apInit;
	
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
	
	mpBlackGfx = mpDrawer->CreateGfxObject("effect_black.bmp","diffalpha2d");

	Reset();
}

//-----------------------------------------------------------------------

cFadeHandler::~cFadeHandler(void)
{
	mpDrawer->DestroyGfxObject(mpBlackGfx);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cFadeHandler::FadeOut(float afTime)
{
	if(afTime <= 0)
		mfAlphaAdd = 1000000.0f;
	else
		mfAlphaAdd = (1.0f / afTime);

	mbActive = true;

	Update(1.0f/60.0f);
}

//-----------------------------------------------------------------------

void cFadeHandler::FadeIn(float afTime)
{
	if(afTime <= 0)
		mfAlphaAdd = -100000.0f;
	else
		mfAlphaAdd = -(1.0f / afTime);
	
	mbActive = true;

	Update(1.0f/60.0f);
}

//-----------------------------------------------------------------------

bool cFadeHandler::IsActive()
{
	return mbActive;
}


//-----------------------------------------------------------------------

void cFadeHandler::OnStart()
{

}

//-----------------------------------------------------------------------

void cFadeHandler::SetWideScreenActive(bool abX)
{
	mbWideScreenActive = abX;
}

//-----------------------------------------------------------------------

void cFadeHandler::Update(float afTimeStep)
{
	if(mbActive)
	{
		mfAlpha += mfAlphaAdd * afTimeStep;

		if(mfAlphaAdd<0)
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

	//////////////////////////////////
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

//-----------------------------------------------------------------------

void cFadeHandler::Reset()
{
	mfWideScreenAlpha =0;
	mbWideScreenActive = false;

	mfAlpha = 0;
	mfAlphaAdd = 0;
	mbActive = false;
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------
