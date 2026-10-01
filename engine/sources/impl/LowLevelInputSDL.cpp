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
#include "impl/LowLevelInputSDL.h"

#include "impl/MouseSDL.h"
#include "impl/KeyboardSDL.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelInputSDL::cLowLevelInputSDL(iLowLevelGraphics *apLowLevelGraphics)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		mbLockInput = false;
		LockInput(true);
	}

	//-----------------------------------------------------------------------

	cLowLevelInputSDL::~cLowLevelInputSDL()
	{
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHOD
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cLowLevelInputSDL::LockInput(bool abX)
	{
		mbLockInput = abX;
		ApplyInputLock();
	}

	//-----------------------------------------------------------------------

	void cLowLevelInputSDL::BeginInputUpdate()
	{
		// The window may not have existed yet when the lock was requested.
		ApplyInputLock();


		SDL_Event sdlEvent;

		while(SDL_PollEvent(&sdlEvent))
		{
			mlstEvents.push_back(sdlEvent);
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelInputSDL::EndInputUpdate()
	{
		mlstEvents.clear();
	}

	//-----------------------------------------------------------------------

	void cLowLevelInputSDL::ApplyInputLock()
	{
		// Locked input means a captured, hidden pointer with raw relative
		// motion (SDL 1.2's SDL_WM_GrabInput).
		SDL_Window *pWindow = SDL_GL_GetCurrentWindow();
		if(pWindow==NULL) return;

		if(SDL_GetWindowRelativeMouseMode(pWindow) != mbLockInput)
			SDL_SetWindowRelativeMouseMode(pWindow, mbLockInput);
	}

	//-----------------------------------------------------------------------

	iMouse* cLowLevelInputSDL::CreateMouse()
	{
		return hplNew( cMouseSDL,(this,mpLowLevelGraphics));
	}

	//-----------------------------------------------------------------------

	iKeyboard* cLowLevelInputSDL::CreateKeyboard()
	{
		return hplNew( cKeyboardSDL,(this) );
	}

	//-----------------------------------------------------------------------

}
