// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELINPUT_SDL_H
#define HPL_LOWLEVELINPUT_SDL_H

#include <list>
#include "input/LowLevelInput.h"

#include <SDL3/SDL.h>

namespace hpl {

	class iLowLevelGraphics;

	class cLowLevelInputSDL : public iLowLevelInput
	{
	public:
		cLowLevelInputSDL(iLowLevelGraphics *apLowLevelGraphics);
		~cLowLevelInputSDL() override;

		void LockInput(bool abX) override;

		void BeginInputUpdate() override;
		void EndInputUpdate() override;

		bool IsQuitRequested() override{ return mbQuitRequested; }

		iMouse* CreateMouse() override;
		iKeyboard* CreateKeyboard() override;
	public:
		std::list<SDL_Event> mlstEvents;

	private:
		void ApplyInputLock();
		bool IsForDebugOverlay(const SDL_Event& aEvent);

		iLowLevelGraphics *mpLowLevelGraphics;
		bool mbLockInput;
		bool mbQuitRequested;
	};
};
#endif // HPL_LOWLEVELINPUT_SDL_H
