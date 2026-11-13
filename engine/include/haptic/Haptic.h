// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_HAPTIC_H
#define HPL_HAPTIC_H

#include "game/Updateable.h"
#include "haptic/HapticTypes.h"

namespace hpl {

	class iLowLevelHaptic;
	class cResources;

	class cHaptic : public iUpdateable
	{
	public:
		cHaptic(iLowLevelHaptic *apLowLevelHaptic);
		~cHaptic() override;

		static void SetIsUsed(bool abX){ cHaptic::mbIsUsed = abX;}
		static bool GetIsUsed(){ return cHaptic::mbIsUsed;}

		void Init(cResources *apResources);

		void Update(float afTimeStep) override;

		iLowLevelHaptic * GetLowLevel(){ return mpLowLevelHaptic;}

	private:
		iLowLevelHaptic *mpLowLevelHaptic;

		static bool mbIsUsed;
	};

};
#endif // HPL_HAPTIC_H
