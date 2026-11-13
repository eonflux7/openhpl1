// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/OpenALSoundEnvironment.h"
#include "system/String.h"
#include "tinyxml.h"

namespace hpl
{

	bool cOpenALSoundEnvironment::CreateFromFile(const tString &asFile)
	{
		TiXmlDocument doc;
		if (!doc.LoadFile(asFile.c_str()))
			return false;

		TiXmlElement* pRoot = doc.FirstChildElement("SoundEnvironment");
		if (pRoot == nullptr)
			return false;

		TiXmlElement* pMain = pRoot->FirstChildElement("Main");
		const char* pType = pMain ? pMain->Attribute("Type") : nullptr;
		const char* pName = pMain ? pMain->Attribute("Name") : nullptr;
		if (pName)
			mstrName = pName;

		TiXmlElement* pParams = pRoot->FirstChildElement("Parameters");
		if ( (pParams == nullptr) || (pType == nullptr) || (tString(pType) != "OpenAL") )
			return false;

		mfDensity = cString::ToFloat(pParams->Attribute("Density"),0);
		mfDiffusion = cString::ToFloat(pParams->Attribute("Diffusion"),0);
		mfGain = cString::ToFloat(pParams->Attribute("Gain"),0);
		mfGainHF = cString::ToFloat(pParams->Attribute("GainHF"),0);
		mfGainLF = cString::ToFloat(pParams->Attribute("GainLF"),0);
		mfDecayTime = cString::ToFloat(pParams->Attribute("DecayTime"),0);
		mfDecayHFRatio = cString::ToFloat (pParams->Attribute("DecayHFRatio"),0);
		mfDecayLFRatio = cString::ToFloat (pParams->Attribute("DecayLFRatio"),0);
		mfReflectionsGain = cString::ToFloat(pParams->Attribute("ReflectionsGain"),0);
		mfReflectionsDelay = cString::ToFloat(pParams->Attribute("ReflectionsDelay"),0);
		cVector3f vTemp = cString::ToVector3f(pParams->Attribute("ReflectionsPan"),cVector3f(0));
		mfReflectionsPan[0] = vTemp.x;
		mfReflectionsPan[1] = vTemp.y;
		mfReflectionsPan[2] = vTemp.z;
		mfLateReverbGain = cString::ToFloat(pParams->Attribute("LateReverbGain"),0);
		mfLateReverbDelay = cString::ToFloat(pParams->Attribute("LateReverbDelay"),0);
		vTemp = cString::ToVector3f(pParams->Attribute("LateReverbPan"),cVector3f(0));
		mfLateReverbPan[0] = vTemp.x;
		mfLateReverbPan[1] = vTemp.y;
		mfLateReverbPan[2] = vTemp.z;
		mfEchoTime = cString::ToFloat(pParams->Attribute("EchoTime"),0);
		mfEchoDepth = cString::ToFloat(pParams->Attribute("EchoDepth"),0);
		mfModulationTime = cString::ToFloat(pParams->Attribute("ModulationTime"),0);
		mfModulationDepth = cString::ToFloat(pParams->Attribute("ModulationDepth"),0);
		mfAirAbsorptionGainHF = cString::ToFloat(pParams->Attribute("AirAbsorptionGainHF"),0);
		mfHFReference = cString::ToFloat(pParams->Attribute("HFReference"),0);
		mfLFReference = cString::ToFloat(pParams->Attribute("LFReference"),0);
		mfRoomRolloffFactor =cString::ToFloat(pParams->Attribute("RoomRolloffFactor"),0);
		mbDecayHFLimit = cString::ToInt(pParams->Attribute("DecayHFLimit"),0);

		return true;
	}

}
