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
