// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MOUSE_SDL_H
#define HPL_MOUSE_SDL_H

#include <vector>
#include "input/Mouse.h"


namespace hpl {

	class iLowLevelGraphics;
	class cLowLevelInputSDL;

	class cMouseSDL : public iMouse
	{
	public:
		cMouseSDL(cLowLevelInputSDL *apLowLevelInputSDL, iLowLevelGraphics *apLowLevelGraphics);

		bool ButtonIsDown(eMButton) override;

		void Update() override;

		/**
		* \todo Fix so it works and handles screen size
		* \return
		*/
		cVector2f GetAbsPosition() override;

		/**
		 * \todo Fix so it works and handles screen size
		 * \return
		 */
		cVector2f GetRelPosition() override;
		cVector2f GetPendingRelPosition() override;

		void Reset() override;

		/**
		 * Sets how much smoothening there will be in the RelPosition.
		 * The percentages are just ratios, so min/max 1/10 equals 0.1/1
		 * \param afMinPercent The influence of the oldest value
		 * \param afMaxPercent The influence of the newest value
		 * \param alBufferSize The number of values recorded
		 */
		void SetSmoothProperties(float afMinPercent,
			float afMaxPercent,unsigned int alBufferSize) override;

	private:
		cVector2f mvMouseAbsPos;
		cVector2f mvMouseRelPos;

		std::vector<bool> mvMButtonArray;

		tVector2fList mlstMouseCoord;

		float mfMaxPercent;
		float mfMinPercent;
		int mlBufferSize;

		cLowLevelInputSDL *mpLowLevelInputSDL;
		iLowLevelGraphics *mpLowLevelGraphics;

		bool mbWheelUpMoved;
		bool mbWheelDownMoved;
	};

};

#endif // HPL_MOUSE_SDL_H
