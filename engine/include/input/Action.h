// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ACTION_H
#define HPL_ACTION_H

#include "system/SystemTypes.h"

namespace hpl {

	class iAction
	{
	public:
		iAction(tString asName);
		virtual ~iAction(){}

		/**
		 *
		 * \return True once on release after an observed press.
		 */
		bool WasTriggered();

		/**
		 *
		 * \return True once on press until the action is released.
		 */
		bool BecameTriggered();

		/**
		*
		* \return True on a repeated press within afMaxIntervalSeconds; time advances only while released.
		*/
		bool DoubleTriggered(float afMaxIntervalSeconds);

		/**
		 *Update the Action, called by cInput
		 */
		void Update(float afTimeStep);

		/**
		 *
		 * \return the name of the action.
		 */
		tString GetName();

		/**
		 * Update special logic for the action. Normally empty
		 */
		virtual void UpdateLogic(float afTimeStep);


		/**
		 * Filled in by the class that inherits from Action.
		 * \return true if the action is being triggered
		 */
		virtual bool IsTriggered()=0;

		/**
		 *
		 * \return A value from the input, ie the relative mouse x position.
		 */
		virtual float GetValue()=0;

		/**
		 * The name of the input, ie for keyboard the name of the key is returned.
		 */
		virtual tString GetInputName()=0;

		/**
		 * The name of the input type.
		 */
		virtual tString GetInputType()=0;

	private:
		tString msName;

		bool mbBecameTriggered;
		bool mbIsTriggered;

		bool mbTriggerDown;

		double mfDoubleTriggerElapsedSeconds;
	};

};
#endif // HPL_ACTION_H
