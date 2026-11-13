// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_INPUT_H
#define HPL_INPUT_H

#include <map>
#include <list>

#include "system/SystemTypes.h"
#include "game/Updateable.h"
#include "input/InputTypes.h"

namespace hpl {

	class iKeyboard;
	class iMouse;
	class iLowLevelInput;
	class iInputDevice;
	class iAction;

	typedef std::map<tString, iAction*> tActionMap;
	typedef tActionMap::iterator tActionMapIt;

	typedef std::multimap<tString, iAction*> tActionMultiMap;
	typedef tActionMultiMap::iterator tActionMultiMapIt;

	typedef std::list<iInputDevice*> tInputDeviceList;
	typedef tInputDeviceList::iterator tInputDeviceListIt;

	class cInput : public iUpdateable
	{
	public:
		cInput(iLowLevelInput *apLowLevelInput);
		~cInput() override;

		/**
		 * Updates the input, called by cGame
		 */
		void Update(float afTimeStep) override;

		/**
		 * Add a new action.
		 * \param *apAction The action to add.
		 */
		void AddAction(iAction *apAction);

		/**
		 * Check if an action is triggered.
		 * \param asName
		 * \return
		 */
		bool IsTriggered(tString asName);
		/**
		 *
		 * \param asName name of the action.
		 * \return
		 */
		bool WasTriggered(tString asName);

		/**
		 *
		 * \param asName name of the action.
		 * \return
		 */
		bool BecameTriggered(tString asName);

		/**
		*
		* \param asName name of the action.
		* \return
		*/
		bool DoubleTriggered(tString asName, float afMaxIntervalSeconds);

		/**
		 *
		 * \return currently used keyboard
		 */
		iKeyboard * GetKeyboard();

		/**
		*
		* \return currently used mouse
		*/
		iMouse * GetMouse();

		/**
		* Get action from map.
		* \param asName name of action.
		* \return Pointer to action, if not found NULL.
		*/
		iAction* GetAction(tString asName);

		/**
		 * Destroys an action if it exists.
		 * \param asName
		 */
		void DestroyAction(tString asName);

		/**
		 * Checks if any input is given.
		 * \return true if any input is given, else false.
		 */
		bool CheckForInput();

		/**
		 * Creates an action from the latest input made. Any action with the same name is destroyed.
		 * \param &asName Name of action be be created.
		 * \return NULL if no input was given.
		 */
		iAction* InputToAction(const tString &asName);

		iLowLevelInput* GetLowLevel(){ return mpLowLevelInput;}

	private:


		tActionMap m_mapActions;
		tInputDeviceList mlstInputDevices;

		iLowLevelInput *mpLowLevelInput;

		iMouse* mpMouse;
		iKeyboard* mpKeyboard;
	};
};

#endif // HPL_INPUT_H
