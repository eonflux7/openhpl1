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
#include "impl/KeyboardSDL.h"

#include <SDL3/SDL.h>

#include "impl/LowLevelInputSDL.h"

#ifdef WIN32
#include <conio.h>
#endif

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cKeyboardSDL::cKeyboardSDL(cLowLevelInputSDL *apLowLevelInputSDL) : iKeyboard("SDL Portable Keyboard")
	{
		mpLowLevelInputSDL = apLowLevelInputSDL;

		mvKeyArray.resize(eKey_LastEnum);
		ClearKeyList();


	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	void cKeyboardSDL::Update()
	{
		mlstKeysPressed.clear();
		std::list<SDL_Event>::iterator it = mpLowLevelInputSDL->mlstEvents.begin();
		for(; it != mpLowLevelInputSDL->mlstEvents.end(); ++it)
		{
			SDL_Event *pEvent = &(*it);

			if(pEvent->type != SDL_EVENT_KEY_DOWN && pEvent->type != SDL_EVENT_KEY_UP)
			{
				continue;
			}

			eKey key = SDLToKey(pEvent->key.key);

			mvKeyArray[key] = pEvent->type == SDL_EVENT_KEY_DOWN;

			if(pEvent->type == SDL_EVENT_KEY_DOWN)
			{
				SDL_Keymod sdl_mod = pEvent->key.mod;
				mModifier = eKeyModifier_NONE;

				if(sdl_mod & SDL_KMOD_CTRL)		mModifier |= eKeyModifier_CTRL;
				if(sdl_mod & SDL_KMOD_SHIFT)	mModifier |= eKeyModifier_SHIFT;
				if(sdl_mod & SDL_KMOD_ALT)		mModifier |= eKeyModifier_ALT;
				if(sdl_mod & SDL_KMOD_GUI)		mModifier |= eKeyModifier_META;

				// SDL 1.2 attached the typed character to key events. Derive it
				// from the layout and modifiers; non-character keys give 0.
				SDL_Keycode charKey = SDL_GetKeyFromScancode(pEvent->key.scancode, sdl_mod, false);
				int lUnicode = (charKey & SDLK_SCANCODE_MASK) ? 0 : (int)charKey;
				if(lUnicode > 0xFFFF) lUnicode = 0;

				mlstKeysPressed.push_back(cKeyPress(key,lUnicode,mModifier));
			}
		}
	}

	//-----------------------------------------------------------------------

	bool cKeyboardSDL::KeyIsDown(eKey aKey)
	{
		return mvKeyArray[aKey];
	}

	//-----------------------------------------------------------------------


	cKeyPress cKeyboardSDL::GetKey()
	{
		cKeyPress key = mlstKeysPressed.front();
		mlstKeysPressed.pop_front();
		return key;
	}

	//-----------------------------------------------------------------------

	bool cKeyboardSDL::KeyIsPressed()
	{
		return mlstKeysPressed.empty()==false;
	}

	//-----------------------------------------------------------------------

	eKeyModifier cKeyboardSDL::GetModifier()
	{
		return mModifier;
	}

	//-----------------------------------------------------------------------

	tString cKeyboardSDL::KeyToString(eKey)
	{
		return "None";
	}

	//-----------------------------------------------------------------------

	eKey cKeyboardSDL::StringToKey(tString)
	{
		return eKey_NONE;
	}

	//-----------------------------------------------------------------------

	/////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	/////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	eKey cKeyboardSDL::SDLToKey(int alKey)
	{
		switch(alKey)
		{
			case	SDLK_BACKSPACE: return eKey_BACKSPACE;
			case	SDLK_TAB:return  eKey_TAB;
			case 	SDLK_CLEAR: return eKey_CLEAR;
			case 	SDLK_RETURN: return eKey_RETURN;
			case 	SDLK_PAUSE: return eKey_PAUSE;
			case 	SDLK_ESCAPE: return eKey_ESCAPE;
			case 	SDLK_SPACE: return eKey_SPACE;
			case 	SDLK_EXCLAIM: return eKey_EXCLAIM;
			case 	SDLK_DBLAPOSTROPHE: return eKey_QUOTEDBL;
			case 	SDLK_HASH: return eKey_HASH;
			case 	SDLK_DOLLAR: return eKey_DOLLAR;
			case 	SDLK_AMPERSAND: return eKey_AMPERSAND;
			case 	SDLK_APOSTROPHE: return eKey_QUOTE;
			case 	SDLK_LEFTPAREN: return eKey_LEFTPAREN;
			case 	SDLK_RIGHTPAREN: return eKey_RIGHTPAREN;
			case 	SDLK_ASTERISK: return eKey_ASTERISK;
			case 	SDLK_PLUS: return eKey_PLUS;
			case 	SDLK_COMMA: return eKey_COMMA;
			case 	SDLK_MINUS: return eKey_MINUS;
			case 	SDLK_PERIOD: return eKey_PERIOD;
			case 	SDLK_SLASH: return eKey_SLASH;
			case 	SDLK_0: return eKey_0;
			case 	SDLK_1: return eKey_1;
			case 	SDLK_2: return eKey_2;
			case 	SDLK_3: return eKey_3;
			case 	SDLK_4: return eKey_4;
			case 	SDLK_5: return eKey_5;
			case 	SDLK_6: return eKey_6;
			case 	SDLK_7: return eKey_7;
			case 	SDLK_8: return eKey_8;
			case 	SDLK_9: return eKey_9;
			case 	SDLK_COLON: return eKey_COLON;
			case 	SDLK_SEMICOLON: return eKey_SEMICOLON;
			case 	SDLK_LESS: return eKey_LESS;
			case 	SDLK_EQUALS: return eKey_EQUALS;
			case 	SDLK_GREATER: return eKey_GREATER;
			case 	SDLK_QUESTION: return eKey_QUESTION;
			case 	SDLK_AT: return eKey_AT;
			case 	SDLK_LEFTBRACKET: return eKey_LEFTBRACKET;
			case 	SDLK_BACKSLASH: return eKey_BACKSLASH;
			case 	SDLK_RIGHTBRACKET: return eKey_RIGHTBRACKET;
			case 	SDLK_CARET: return eKey_CARET;
			case 	SDLK_UNDERSCORE: return eKey_UNDERSCORE;
			case 	SDLK_GRAVE: return eKey_BACKQUOTE;
			case 	SDLK_A: return eKey_a;
			case 	SDLK_B: return eKey_b;
			case 	SDLK_C: return eKey_c;
			case 	SDLK_D: return eKey_d;
			case 	SDLK_E: return eKey_e;
			case 	SDLK_F: return eKey_f;
			case 	SDLK_G: return eKey_g;
			case 	SDLK_H: return eKey_h;
			case 	SDLK_I: return eKey_i;
			case 	SDLK_J: return eKey_j;
			case 	SDLK_K: return eKey_k;
			case 	SDLK_L: return eKey_l;
			case 	SDLK_M: return eKey_m;
			case 	SDLK_N: return eKey_n;
			case 	SDLK_O: return eKey_o;
			case 	SDLK_P: return eKey_p;
			case 	SDLK_Q: return eKey_q;
			case 	SDLK_R: return eKey_r;
			case 	SDLK_S: return eKey_s;
			case 	SDLK_T: return eKey_t;
			case 	SDLK_U: return eKey_u;
			case 	SDLK_V: return eKey_v;
			case 	SDLK_W: return eKey_w;
			case 	SDLK_X: return eKey_x;
			case 	SDLK_Y: return eKey_y;
			case 	SDLK_Z: return eKey_z;
			case 	SDLK_DELETE: return eKey_DELETE;
			case 	SDLK_KP_0: return eKey_KP0;
			case 	SDLK_KP_1: return eKey_KP1;
			case 	SDLK_KP_2: return eKey_KP2;
			case 	SDLK_KP_3: return eKey_KP3;
			case 	SDLK_KP_4: return eKey_KP4;
			case 	SDLK_KP_5: return eKey_KP5;
			case 	SDLK_KP_6: return eKey_KP6;
			case 	SDLK_KP_7: return eKey_KP7;
			case 	SDLK_KP_8: return eKey_KP8;
			case 	SDLK_KP_9: return eKey_KP9;
			case 	SDLK_KP_PERIOD: return eKey_KP_PERIOD;
			case 	SDLK_KP_DIVIDE: return eKey_KP_DIVIDE;
			case 	SDLK_KP_MULTIPLY: return eKey_KP_MULTIPLY;
			case 	SDLK_KP_MINUS: return eKey_KP_MINUS;
			case 	SDLK_KP_PLUS: return eKey_KP_PLUS;
			case 	SDLK_KP_ENTER: return eKey_KP_ENTER;
			case 	SDLK_KP_EQUALS: return eKey_KP_EQUALS;
			case 	SDLK_UP: return eKey_UP;
			case 	SDLK_DOWN: return eKey_DOWN;
			case 	SDLK_RIGHT: return eKey_RIGHT;
			case 	SDLK_LEFT: return eKey_LEFT;
			case 	SDLK_INSERT: return eKey_INSERT;
			case 	SDLK_HOME: return eKey_HOME;
			case 	SDLK_END: return eKey_END;
			case 	SDLK_PAGEUP: return eKey_PAGEUP;
			case 	SDLK_PAGEDOWN: return eKey_PAGEDOWN;
			case 	SDLK_F1: return eKey_F1;
			case 	SDLK_F2: return eKey_F2;
			case 	SDLK_F3: return eKey_F3;
			case 	SDLK_F4: return eKey_F4;
			case 	SDLK_F5: return eKey_F5;
			case 	SDLK_F6: return eKey_F6;
			case 	SDLK_F7: return eKey_F7;
			case 	SDLK_F8: return eKey_F8;
			case 	SDLK_F9: return eKey_F9;
			case 	SDLK_F10: return eKey_F10;
			case 	SDLK_F11: return eKey_F11;
			case 	SDLK_F12: return eKey_F12;
			case 	SDLK_F13: return eKey_F13;
			case 	SDLK_F14: return eKey_F14;
			case 	SDLK_F15: return eKey_F15;
			case 	SDLK_NUMLOCKCLEAR: return eKey_NUMLOCK;
			case 	SDLK_CAPSLOCK: return eKey_CAPSLOCK;
			case 	SDLK_SCROLLLOCK: return eKey_SCROLLOCK;
			case 	SDLK_RSHIFT: return eKey_RSHIFT;
			case 	SDLK_LSHIFT: return eKey_LSHIFT;
			case 	SDLK_RCTRL: return eKey_RCTRL;
			case 	SDLK_LCTRL: return eKey_LCTRL;
			case 	SDLK_RALT: return eKey_RALT;
			case 	SDLK_LALT: return eKey_LALT;
			case 	SDLK_RMETA: return eKey_RMETA;
			case 	SDLK_LMETA: return eKey_LMETA;
			case 	SDLK_LGUI: return eKey_LSUPER;
			case 	SDLK_RGUI: return eKey_RSUPER;
			case 	SDLK_MODE: return eKey_MODE;
			case 	SDLK_HELP: return eKey_HELP;
			case 	SDLK_PRINTSCREEN: return eKey_PRINT;
			case 	SDLK_SYSREQ: return eKey_SYSREQ;
			case 	SDLK_MENU: return eKey_MENU;
			case 	SDLK_POWER: return eKey_POWER;
		}

		return eKey_NONE;
	}

	//-----------------------------------------------------------------------

	void cKeyboardSDL::ClearKeyList()
	{
		mvKeyArray.assign(mvKeyArray.size(),false);
	}

	//-----------------------------------------------------------------------

}
