// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_KEYBOARD_SDL_H
#define HPL_KEYBOARD_SDL_H

#include <vector>
#include <list>
#include "system/SystemTypes.h"
#include "input/Keyboard.h"

namespace hpl {

#define MAX_KEY_PRESSES (20)

	class cLowLevelInputSDL;

	class cKeyboardSDL : public iKeyboard
	{
	public:
		cKeyboardSDL(cLowLevelInputSDL *apLowLevelInputSDL);

		void Update() override;

		//Keyboard specific
		bool KeyIsDown(eKey aKey) override;
		cKeyPress GetKey() override;
		bool KeyIsPressed() override;
		eKeyModifier GetModifier() override;
		tString KeyToString(eKey) override;
		eKey StringToKey(tString) override;

	private:
		eKey SDLToKey(int alKey);
		void ClearKeyList();
		eKey AsciiToKey(int alChar);
		eKeyModifier mModifier;

		std::vector<bool> mvKeyArray;

		std::list<cKeyPress> mlstKeysPressed;

		cLowLevelInputSDL *mpLowLevelInputSDL;
	};

};

#endif // HPL_KEYBOARD_SDL_H
