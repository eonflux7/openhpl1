// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_KEYBOARD_H
#define HPL_KEYBOARD_H

#include "input/InputTypes.h"
#include "input/InputDevice.h"

namespace hpl {

	class iKeyboard : public iInputDevice
	{
	public:
		iKeyboard(tString asName);
		~iKeyboard() override{}

		/**
		 *
		 * \param aKey The key to check
		 * \return true if pressed else false
		 */
		virtual bool KeyIsDown(eKey aKey)=0;
		/**
		 * Can be checked many times to see all key presses
		 * \return key that is currently pressed. eKey_NONE is no key.
		 */
		virtual cKeyPress GetKey()=0;
		/**
		 *
		 * \return If ANY key is pressed
		 */
		virtual bool KeyIsPressed()=0;
		/**
		 * \return The current modifiers.
		 */
		virtual eKeyModifier GetModifier()=0;
		/**
		 * \todo Implement!
		 * \param eKey The key to change to string.
		 * \return The name of the key as a string.
		 */
		virtual tString KeyToString(eKey)=0;
		/**
		 * \todo Implement!
		 * \param tString NAme of the key
		 * \return enum of the key.
		 */
		virtual eKey StringToKey(tString)=0;
	};

};

#endif // HPL_KEYBOARD_H
