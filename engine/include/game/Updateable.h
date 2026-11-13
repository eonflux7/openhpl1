// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_UPDATEABLE_H
#define HPL_UPDATEABLE_H

#include "system/SystemTypes.h"

namespace hpl {

	class iUpdateable
	{
	public:
		virtual ~iUpdateable(){}
		iUpdateable(const tString& asName) : msName(asName){}

		/**
		 * Called before a frame is drawn between two logic steps, after the
		 * engine has moved the cameras and physics objects to where they are
		 * at that time. afT is the position between the steps, 0 - 1.
		 * Changes made to them here last for this frame only.
		 */
		virtual void OnInterpolate(float afT){}

		virtual void OnDraw(){}

		virtual void OnPostSceneDraw(){}

		virtual void OnPostGUIDraw(){}

		virtual void OnPostBufferSwap(){}

		virtual void OnStart(){}

		virtual void Update(float afTimeStep){}

		virtual void OnExit(){}

		virtual void Reset(){}

		const tString& GetName(){ return msName;}

	private:
		tString msName;
	};
};

#endif // HPL_UPDATEABLE_H
