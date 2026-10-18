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
