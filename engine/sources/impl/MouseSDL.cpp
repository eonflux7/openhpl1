// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/MouseSDL.h"

#include <SDL3/SDL.h>

#include "graphics/LowLevelGraphics.h"
#include "impl/LowLevelInputSDL.h"
#include "impl/DebugOverlay.h"

namespace hpl {

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

	void cMouseSDL::Update()
	{
		cVector2f vScreenSize = mpLowLevelGraphics->GetScreenSize();
		cVector2f vVirtualOffset = mpLowLevelGraphics->GetVirtualOffset();
		//Virtual units per pixel, the same on both axes.
		float fVirtualScale = (mpLowLevelGraphics->GetVirtualSize().x + vVirtualOffset.x*2) / vScreenSize.x;

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

	bool cMouseSDL::ButtonIsDown(eMButton mButton)
	{
		return mvMButtonArray[mButton];
	}

	cVector2f cMouseSDL::GetAbsPosition()
	{
		// Do a transform with the screen-size to the the float coordinates.
		cVector2f vPos = mvMouseAbsPos;

		return vPos;
	}

	cVector2f cMouseSDL::GetRelPosition()
	{
		// Do a transform with the screen-size to the the float coordinates.
		cVector2f vPos = mvMouseRelPos;
		//Ok this is?
		mvMouseRelPos = cVector2f(0,0);

		return vPos;
	}

	cVector2f cMouseSDL::GetPendingRelPosition()
	{
		// The motion is the debug overlay's
		if(cDebugOverlay::HasInput()) return cVector2f(0,0);

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

	void cMouseSDL::Reset()
	{
		mlstMouseCoord.clear();
		mvMouseRelPos = cVector2f(0,0);

		float fX,fY; //Just to clear the rel pos.

		SDL_PumpEvents();
		SDL_GetRelativeMouseState(&fX, &fY);
	}

	void cMouseSDL::SetSmoothProperties(float afMinPercent,
		float afMaxPercent,unsigned int alBufferSize)
	{
		mfMaxPercent = afMaxPercent;
		mfMinPercent = afMinPercent;
		mlBufferSize = alBufferSize;
	}
}
