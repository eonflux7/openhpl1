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
#include "ui/DemoEndText.h"

#include "core/Init.h"
#include "core/GraphicsHelper.h"
#include "core/ButtonHandler.h"
#include "ui/MainMenu.h"
#include "core/MapHandler.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cDemoEndText::cDemoEndText(cInit *apInit)  : iUpdateable("PreMenu")
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	//Load fonts
	//mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");
	
	Reset();
}

//-----------------------------------------------------------------------

cDemoEndText::~cDemoEndText(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cDemoEndText::Reset()
{
	mbActive = false;

	mfImageBrightness = 0;
	mfBrightnessRatePerSecond = 0.6f;

	mlCurrentImage = 0;
}

//-----------------------------------------------------------------------

void cDemoEndText::OnPostSceneDraw()
{
	if(mbActive==false) return;
	if(mvTextures.empty()) return;

	mpInit->mpGraphicsHelper->ClearScreen(cColor(0,0));

	mpInit->mpGraphicsHelper->DrawTexture(mvTextures[mlCurrentImage],0,
											cVector2f(800,600),cColor(mfImageBrightness,1));
	
}

//-----------------------------------------------------------------------


void cDemoEndText::OnDraw()
{
}

//-----------------------------------------------------------------------

void cDemoEndText::Update(float afTimeStep)
{
	mfImageBrightness += mfBrightnessRatePerSecond * afTimeStep;

	if(mfBrightnessRatePerSecond >0)
	{
		if(mfImageBrightness > 1) mfImageBrightness =1;
	}
	else
	{
		if(mfImageBrightness < 0){
			mfImageBrightness =0;
			mfBrightnessRatePerSecond = - mfBrightnessRatePerSecond;
			if(mlCurrentImage >= (int)mvTextures.size()-1)
			{
				SetActive(false);
			}
			else
			{
				mlCurrentImage++;
			}
		}
	}
}

//-----------------------------------------------------------------------

void cDemoEndText::OnMouseDown(eMButton aButton)
{
	OnButtonDown();
}

//-----------------------------------------------------------------------

void cDemoEndText::OnButtonDown()
{
	if(mfBrightnessRatePerSecond > 0 && mfImageBrightness == 1)
	{
		mfBrightnessRatePerSecond = - mfBrightnessRatePerSecond;
	}
}

//-----------------------------------------------------------------------

void cDemoEndText::SetActive(bool abActive)
{
	if(mbActive == abActive) return;

	mbActive = abActive;

	if(mbActive)
	{
		mpInit->mpGame->GetScene()->GetWorld3D()->DestroyAllSoundEntities();

		mpInit->mpGame->GetUpdater()->SetContainer("DemoEndText");
		mpInit->mpGame->GetScene()->SetDrawScene(false);
		mpInit->mpGame->GetScene()->SetUpdateMap(false);
		if(mpInit->mbHasHaptics)
			mpInit->mpGame->GetHaptic()->GetLowLevel()->SetUpdateShapes(false);
		mpInit->mpButtonHandler->ChangeState(eButtonHandlerState_DemoEndText);

		for(int i=0; i<3; ++i)
		{
			iTexture *pTex = mpInit->mpGame->GetResources()->GetTextureManager()->Create2D(
											"demo_end0"+cString::ToString(i)+".jpg",false);
			if(pTex) mvTextures.push_back(pTex);
		}
		

		mfImageBrightness =0;
	}
	else
	{
		for(size_t i=0; i<mvTextures.size(); ++i)
		{
			mpInit->mpGame->GetResources()->GetTextureManager()->Destroy(mvTextures[i]);
		}
        mvTextures.clear();

		mpInit->mpGame->Exit();
	}
}

//-----------------------------------------------------------------------

void cDemoEndText::OnExit()
{
	//SetActive(false);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

