// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELINPUT_H
#define HPL_LOWLEVELINPUT_H

namespace hpl {

	class iMouse;
	class iKeyboard;

	class iLowLevelInput
	{
	public:
		virtual ~iLowLevelInput(){}

		/**
		 * Lock all input to current window.
		 * \param abX
		 * \return
		 */
		virtual void LockInput(bool abX)=0;
		/**
		 * Called by cInput
		 */
		virtual void BeginInputUpdate()=0;
		/**
		 * called by cInput
		 */
		virtual void EndInputUpdate()=0;

		/**
		 * True once the OS asked the application to quit (window closed, SIGTERM...).
		 */
		virtual bool IsQuitRequested(){ return false; }

		virtual iMouse* CreateMouse()=0;
		virtual iKeyboard* CreateKeyboard()=0;
	};
};
#endif // HPL_LOWLEVELINPUT_H
