// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MOUSE_H
#define HPL_MOUSE_H

#include "input/InputTypes.h"
#include "input/InputDevice.h"
#include "math/MathTypes.h"

namespace hpl {

	class iMouse : public iInputDevice
	{
	public:
		iMouse(tString asName);
		~iMouse() override{}

		/**
		 * Check if a mouse button is down
		 * \param eMButton the button to check
		 * \return
		 */
		virtual bool ButtonIsDown(eMButton)=0;
		/**
		 * Get the absolute pos of the mouse.
		 * \return
		 */
		virtual cVector2f GetAbsPosition()=0;
		/**
		 * Get the relative movement.
		 * \return
		 */
		virtual cVector2f GetRelPosition()=0;
		/**
		 * The movement that has arrived since the last input update, in the
		 * units of GetRelPosition(). It stays queued for that update; this
		 * is for showing it in frames drawn before then.
		 */
		virtual cVector2f GetPendingRelPosition()=0;

		/**
		 * Reset smoothing and relative movement.
		 */
		virtual void Reset()=0;
		/**
		 * Set parameters for mouse smoothing
		 * \param afMinPercent Influence of the oldest position.
		 * \param afMaxPercent Influence of the latest position.
		 * \param alBufferSize number of saved positions, 1 = no smoothing
		 */
		virtual void SetSmoothProperties(float afMinPercent,
										float afMaxPercent,unsigned int alBufferSize)=0;
	};

};

#endif // HPL_MOUSE_H
