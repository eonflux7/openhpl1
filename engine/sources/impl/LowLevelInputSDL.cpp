// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/LowLevelInputSDL.h"

#include "impl/MouseSDL.h"
#include "impl/KeyboardSDL.h"
#include "impl/DebugOverlay.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	cLowLevelInputSDL::cLowLevelInputSDL(iLowLevelGraphics *apLowLevelGraphics)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		mbLockInput = false;
		mbQuitRequested = false;
		LockInput(true);
	}

	cLowLevelInputSDL::~cLowLevelInputSDL()
	{
	}

	void cLowLevelInputSDL::LockInput(bool abX)
	{
		mbLockInput = abX;
		ApplyInputLock();
	}

	void cLowLevelInputSDL::BeginInputUpdate()
	{
		// The window may not have existed yet when the lock was requested.
		ApplyInputLock();


		SDL_Event sdlEvent;

		while(SDL_PollEvent(&sdlEvent))
		{
			if(sdlEvent.type == SDL_EVENT_QUIT) mbQuitRequested = true;

			if(cDebugOverlay::HasInput())
			{
				cDebugOverlay::ProcessEvent(&sdlEvent);
				if(IsForDebugOverlay(sdlEvent)) continue;
			}

			mlstEvents.push_back(sdlEvent);
		}
	}

	// While the debug overlay has the input the game gets no mouse motion,
	// wheel or presses. Releases still reach it, so that nothing held when
	// the overlay took over stays down, and so do the function keys: they
	// toggle the overlay's windows.
	bool cLowLevelInputSDL::IsForDebugOverlay(const SDL_Event& aEvent)
	{
		switch(aEvent.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_WHEEL:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			return true;
		case SDL_EVENT_KEY_DOWN:
			return aEvent.key.key < SDLK_F1 || aEvent.key.key > SDLK_F12;
		default:
			return false;
		}
	}

	void cLowLevelInputSDL::EndInputUpdate()
	{
		mlstEvents.clear();
	}

	void cLowLevelInputSDL::ApplyInputLock()
	{
		// Locked input means a captured, hidden pointer with raw relative
		// motion (SDL 1.2's SDL_WM_GrabInput).
		SDL_Window *pWindow = SDL_GL_GetCurrentWindow();
		if(pWindow==nullptr) return;

		bool bLock = (mbLockInput && cDebugOverlay::HasInput()==false) || cDebugOverlay::IsMouseCaptured();
		if(SDL_GetWindowRelativeMouseMode(pWindow) != bLock)
			SDL_SetWindowRelativeMouseMode(pWindow, bLock);
	}

	iMouse* cLowLevelInputSDL::CreateMouse()
	{
		return new cMouseSDL(this,mpLowLevelGraphics);
	}

	iKeyboard* cLowLevelInputSDL::CreateKeyboard()
	{
		return new cKeyboardSDL(this);
	}
}
