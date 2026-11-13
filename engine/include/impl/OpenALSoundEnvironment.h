// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_OPENALSOUNDENVIRONMENT_H
#define HPL_OPENALSOUNDENVIRONMENT_H

#include "sound/SoundEnvironment.h"

namespace hpl
{
	/** Reverb parameters read from a .snt sound environment file. A
	 * default-constructed environment has every parameter at zero. */
	class cOpenALSoundEnvironment : public iSoundEnvironment
	{
	public:
		cOpenALSoundEnvironment() {}
		~cOpenALSoundEnvironment() override {}

		bool CreateFromFile(const tString &asFile) override;

		float	mfDensity = 0;
		float	mfDiffusion = 0;
		float	mfGain = 0;
		float	mfGainHF = 0;
		float	mfGainLF = 0;
		float	mfDecayTime = 0;
		float	mfDecayHFRatio = 0;
		float	mfDecayLFRatio = 0;
		float	mfReflectionsGain = 0;
		float	mfReflectionsDelay = 0;
		float	mfReflectionsPan[3] = {0,0,0};
		float	mfLateReverbGain = 0;
		float	mfLateReverbDelay = 0;
		float	mfLateReverbPan[3] = {0,0,0};
		float	mfEchoTime = 0;
		float	mfEchoDepth = 0;
		float	mfModulationTime = 0;
		float	mfModulationDepth = 0;
		float	mfAirAbsorptionGainHF = 0;
		float	mfHFReference = 0;
		float	mfLFReference = 0;
		float	mfRoomRolloffFactor = 0;
		int		mbDecayHFLimit = 0;
	};

}
#endif	// HPL_OPENALSOUNDENVIRONMENT_H
