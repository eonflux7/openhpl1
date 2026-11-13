// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sound/LowLevelSound.h"

#include "sound/SoundEnvironment.h"

namespace hpl {

	eSoundHRTF SoundHRTFFromString(const tString& asValue)
	{
		tString sValue = cString::ToLowerCase(asValue);
		if(sValue == "on" || sValue == "true" || sValue == "1") return eSoundHRTF_On;
		if(sValue == "off" || sValue == "false" || sValue == "0") return eSoundHRTF_Off;
		return eSoundHRTF_Auto;
	}

	const char* SoundHRTFToString(eSoundHRTF aMode)
	{
		if(aMode == eSoundHRTF_On) return "on";
		if(aMode == eSoundHRTF_Off) return "off";
		return "auto";
	}

	iLowLevelSound::iLowLevelSound()
	{
		mfVolume=1;
		mfEnvVolume=0;
		mbListenerAttenuation=true;
		mbHardwareAcc = false;
		mbEnvAudioEnabled = false;
		mHRTF = eSoundHRTF_Auto;
	}

	iLowLevelSound::~iLowLevelSound()
	{
		STLDeleteAll(mlstSoundEnv);
	}

	iSoundEnvironment* iLowLevelSound::GetSoundEnvironmentFromFileName (const tString& asName)
	{
		tString sLowName = cString::ToLowerCase(asName);
		for (tSoundEnvListIt SEIt = mlstSoundEnv.begin(); SEIt != mlstSoundEnv.end(); ++SEIt)
		{
			iSoundEnvironment* pSoundEnv = *SEIt;

			if(sLowName == pSoundEnv->GetFileName()) return pSoundEnv;
		}
		return nullptr;
	}

	/*void iLowLevelSound::DestroySoundEnvironment( iSoundEnvironment* apSoundEnv)
	{
		Log(" Destroy %d\n",apSoundEnv);

		STLFindAndDelete(mlstSoundEnv,apSoundEnv);
	}*/
}