// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sound/SoundEntityData.h"

#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "system/String.h"

#include "tinyxml.h"


namespace hpl {

	cSoundEntityData::cSoundEntityData(tString asName) : iResourceBase(asName,0)
	{
		msMainSound = "";
		msStartSound = "";
		msStopSound = "";

		mbFadeStart = false;
		mbFadeStop = false;

		mfVolume = 1;
		mfMaxDistance =0;
		mfMinDistance=0;

		mbStream  = false;
		mbLoop = false;
		mbUse3D = true;

		mfRandom = 1;
		mfInterval =0;
	}

	cSoundEntityData::~cSoundEntityData()
	{

	}

	bool cSoundEntityData::CreateFromFile(const tString &asFile)
	{
		TiXmlDocument *pDoc = new TiXmlDocument();
		if(pDoc->LoadFile(asFile.c_str())==false)
		{
			Error("Couldn't load '%s'!\n",asFile.c_str());
			delete pDoc;
			return false;
		}

		TiXmlElement *pRootElem = pDoc->FirstChildElement();

		// MAIN
		TiXmlElement *pMainElem = pRootElem->FirstChildElement("MAIN");
		if(pMainElem==nullptr){
			Error("Couldn't find MAIN element in '%s'!\n",asFile.c_str());
			delete pDoc;
			return false;
		}

		msMainSound = cString::ToString(pMainElem->Attribute("MainSound"),"");
		msStartSound = cString::ToString(pMainElem->Attribute("StartSound"),"");
		msStopSound = cString::ToString(pMainElem->Attribute("StopSound"),"");

		// PROPERTIES
		TiXmlElement *pPropElem = pRootElem->FirstChildElement("PROPERTIES");
		if(pPropElem==nullptr){
			Error("Couldn't find PROPERTIES element in '%s'!\n",asFile.c_str());
			delete pDoc;
			return false;
		}

		mbUse3D = cString::ToBool(pPropElem->Attribute("Use3D"),true);
		mbLoop = cString::ToBool(pPropElem->Attribute("Loop"),true);
		mbStream = cString::ToBool(pPropElem->Attribute("Stream"),true);

		mbBlockable = cString::ToBool(pPropElem->Attribute("Blockable"),false);
		mfBlockVolumeMul = cString::ToFloat(pPropElem->Attribute("BlockVolumeMul"),0.6f);

		mfVolume = cString::ToFloat(pPropElem->Attribute("Volume"),1);
		mfMaxDistance = cString::ToFloat(pPropElem->Attribute("MaxDistance"),1);
		mfMinDistance = cString::ToFloat(pPropElem->Attribute("MinDistance"),1);

		mbFadeStart = cString::ToBool(pPropElem->Attribute("FadeStart"),true);
		mbFadeStop = cString::ToBool(pPropElem->Attribute("FadeStop"),true);

		mfRandom = cString::ToFloat(pPropElem->Attribute("Random"),1);
		mfInterval = cString::ToFloat(pPropElem->Attribute("Interval"),0);

		mlPriority = cString::ToInt(pPropElem->Attribute("Priority"),0);

		delete pDoc;

		return true;
	}
}
