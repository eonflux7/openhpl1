/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "impl/MouseSDL.h"

#include <SDL3/SDL.h>

#include "graphics/LowLevelGraphics.h"
#include "impl/LowLevelInputSDL.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cMouseSDL::cMouseSDL(cLowLevelInputSDL *apLowLevelInputSDL,iLowLevelGraphics *apLowLevelGraphics) : iMouse("SDL Portable Mouse")
	{
		mfMaxPercent = 0.7f;
		mfMinPercent = 0.1f;
		mlBufferSize = 6;

		mvMButtonArray.resize(eMButton_LastEnum);
		mvMButtonArray.assign(mvMButtonArray.size(),false);

		mpLowLevelInputSDL = apLowLevelInputSDL;
		mpLowLevelGraphics = apLowLevelGraphics;

		mvMouseRelPos = cVector2f(0,0);
		mvMouseAbsPos = cVector2f(0,0);

		mbWheelUpMoved = false;
		mbWheelDownMoved = false;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cMouseSDL::Update()
	{
		cVector2f vScreenSize = mpLowLevelGraphics->GetScreenSize();
		cVector2f vVirtualOffset = mpLowLevelGraphics->GetVirtualOffset();
		//Virtual units per pixel, the same on both axes.
		float fVirtualScale = (mpLowLevelGraphics->GetVirtualSize().x + vVirtualOffset.x*2) / vScreenSize.x;

		//mvMouseRelPos = cVector2f(0,0);

		//Log("Input start\n");
		mbWheelUpMoved = false;
		mbWheelDownMoved = false;

		cVector2f vRelMotion(0,0);

		std::list<SDL_Event>::iterator it = mpLowLevelInputSDL->mlstEvents.begin();
		for(; it != mpLowLevelInputSDL->mlstEvents.end(); ++it)
		{
			SDL_Event *pEvent = &(*it);

			if(pEvent->type == SDL_EVENT_MOUSE_MOTION)
			{
				mvMouseAbsPos = cVector2f(pEvent->motion.x,pEvent->motion.y);
				mvMouseAbsPos = mvMouseAbsPos*fVirtualScale - vVirtualOffset;

				vRelMotion.x += pEvent->motion.xrel;
				vRelMotion.y += pEvent->motion.yrel;
			}
			else if(pEvent->type == SDL_EVENT_MOUSE_WHEEL)
			{
				float fY = pEvent->wheel.y;
				if(pEvent->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) fY = -fY;

				if(fY > 0) mbWheelUpMoved = true;
				else if(fY < 0) mbWheelDownMoved = true;
			}
			else if(pEvent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
					pEvent->type == SDL_EVENT_MOUSE_BUTTON_UP)
			{
				bool bButtonIsDown = pEvent->button.down;

				switch(pEvent->button.button)
				{
					case SDL_BUTTON_LEFT: mvMButtonArray[eMButton_Left] = bButtonIsDown;break;
					case SDL_BUTTON_MIDDLE: mvMButtonArray[eMButton_Middle] = bButtonIsDown;break;
					case SDL_BUTTON_RIGHT: mvMButtonArray[eMButton_Right] = bButtonIsDown;break;
				}
			}
		}

		mvMButtonArray[eMButton_WheelDown] = mbWheelDownMoved;
		mvMButtonArray[eMButton_WheelUp] = mbWheelUpMoved;

		mvMouseRelPos += vRelMotion*fVirtualScale;
	}

	//-----------------------------------------------------------------------

	bool cMouseSDL::ButtonIsDown(eMButton mButton)
	{
		return mvMButtonArray[mButton];
	}

	//-----------------------------------------------------------------------

	cVector2f cMouseSDL::GetAbsPosition()
	{
		// Do a transform with the screen-size to the the float coordinates.
		cVector2f vPos = mvMouseAbsPos;

		return vPos;
	}

	//-----------------------------------------------------------------------

	cVector2f cMouseSDL::GetRelPosition()
	{
		// Do a transform with the screen-size to the the float coordinates.
		cVector2f vPos = mvMouseRelPos;
		//Ok this is?
		mvMouseRelPos = cVector2f(0,0);

		return vPos;
		/*cVector2f vNew;

		if((int)mlstMouseCoord.size() >= mlBufferSize)
			mlstMouseCoord.erase(mlstMouseCoord.begin());

		mlstMouseCoord.push_back(vPos);

		int lBufferSize = (int) mlstMouseCoord.size();

		cVector2f vSum(0,0);
		float fPercent = mfMinPercent;
		float fPercentAdd =  (mfMaxPercent - mfMinPercent)/((float)lBufferSize);
		float fTotalPercent=0;

		tVector2fListIt It = mlstMouseCoord.begin();
		while(It != mlstMouseCoord.end())
		{
			vSum.x +=It->x*fPercent;
			vSum.y +=It->y*fPercent;
			fTotalPercent+=fPercent;
			fPercent+=fPercentAdd;

			It++;
		}
		vNew.x = vSum.x/fTotalPercent;
		vNew.y = vSum.y/fTotalPercent;

		return vNew;*/
	}

	//-----------------------------------------------------------------------

	cVector2f cMouseSDL::GetPendingRelPosition()
	{
		static const int kMaxEvents = 256;
		SDL_Event vEvents[kMaxEvents];

		SDL_PumpEvents();
		int lNum = SDL_PeepEvents(vEvents, kMaxEvents, SDL_PEEKEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_MOTION);

		cVector2f vRelMotion(0,0);
		for(int i=0; i<lNum; ++i)
		{
			vRelMotion.x += vEvents[i].motion.xrel;
			vRelMotion.y += vEvents[i].motion.yrel;
		}

		cVector2f vScreenSize = mpLowLevelGraphics->GetScreenSize();
		cVector2f vVirtualOffset = mpLowLevelGraphics->GetVirtualOffset();
		float fVirtualScale = (mpLowLevelGraphics->GetVirtualSize().x + vVirtualOffset.x*2) / vScreenSize.x;

		return vRelMotion*fVirtualScale;
	}

	//-----------------------------------------------------------------------

	void cMouseSDL::Reset()
	{
		mlstMouseCoord.clear();
		mvMouseRelPos = cVector2f(0,0);

		float fX,fY; //Just to clear the rel pos.

		SDL_PumpEvents();
		SDL_GetRelativeMouseState(&fX, &fY);
	}

	//-----------------------------------------------------------------------

	void cMouseSDL::SetSmoothProperties(float afMinPercent,
		float afMaxPercent,unsigned int alBufferSize)
	{
		mfMaxPercent = afMaxPercent;
		mfMinPercent = afMinPercent;
		mlBufferSize = alBufferSize;
	}

	//-----------------------------------------------------------------------

	/////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	/////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

}
