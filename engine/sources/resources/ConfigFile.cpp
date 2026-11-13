// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/ConfigFile.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/FileSearcher.h"
#include "tinyxml.h"

#include <stdio.h>

namespace hpl {

	cConfigFile::cConfigFile(tWString asFile)
	{
		msFile = asFile;
		mpXmlDoc = hplNew( TiXmlDocument,() );
	}

	cConfigFile::~cConfigFile()
	{
		hplDelete(mpXmlDoc);
	}

	bool cConfigFile::Load()
	{
		#ifdef WIN32
				FILE *pFile = _wfopen(msFile.c_str(),_W("rb"));
		#else
				FILE *pFile = fopen(cString::To8Char(msFile).c_str(),"rb");
		#endif

		bool bRet = mpXmlDoc->LoadFile(pFile);

		if(pFile) fclose(pFile);

		return bRet;
	}

	bool cConfigFile::Save()
	{
		#ifdef WIN32
				FILE *pFile = _wfopen(msFile.c_str(),_W("w+"));
		#else
				FILE *pFile = fopen(cString::To8Char(msFile).c_str(),"w+");
		#endif

		bool bRet = mpXmlDoc->SaveFile(pFile);

		if(pFile) fclose(pFile);

		return bRet;
	}

	void cConfigFile::SetString(tString asLevel, tString asName, tString asVal)
	{
		TiXmlElement *pLevelElem = mpXmlDoc->FirstChildElement(asLevel.c_str());

		if(pLevelElem==nullptr){
			TiXmlElement *pNodeChild = hplNew( TiXmlElement, (asLevel.c_str()) );
			pLevelElem = static_cast<TiXmlElement*>(mpXmlDoc->InsertEndChild(*pNodeChild));
			hplDelete(pNodeChild);
		}

		pLevelElem->SetAttribute(asName.c_str(),asVal.c_str());
	}

	void cConfigFile::SetInt(tString asLevel, tString asName, int alVal)
	{
		char sBuffer[40];
		sprintf(sBuffer,"%d",alVal);

		SetString(asLevel,asName,sBuffer);
	}

	void cConfigFile::SetFloat(tString asLevel, tString asName, float afVal)
	{
		char sBuffer[40];
		sprintf(sBuffer,"%f",afVal);

		SetString(asLevel,asName,sBuffer);
	}

	void cConfigFile::SetBool(tString asLevel, tString asName, bool abVal)
	{
		SetString(asLevel,asName,abVal?"true":"false");
	}

	tString cConfigFile::GetString(tString asLevel, tString asName, tString asDefault)
	{
		const char *sVal = GetCharArray(asLevel,asName);
		if(sVal==nullptr){
			return asDefault;
		}

		return sVal;
	}

	int cConfigFile::GetInt(tString asLevel, tString asName, int alDefault)
	{
		const char *sVal = GetCharArray(asLevel,asName);
		if(sVal==nullptr) return alDefault;

		return cString::ToInt(sVal,alDefault);
	}

	float cConfigFile::GetFloat(tString asLevel, tString asName,float afDefault)
	{
		const char *sVal = GetCharArray(asLevel,asName);
		if(sVal==nullptr) return afDefault;

		return cString::ToFloat(sVal,afDefault);
	}

	bool cConfigFile::GetBool(tString asLevel, tString asName, bool abDefault)
	{
		const char *sVal = GetCharArray(asLevel,asName);
		if(sVal==nullptr) return abDefault;

		return cString::ToBool(sVal,abDefault);
	}

	const char* cConfigFile::GetCharArray(tString asLevel, tString asName)
	{
		TiXmlElement *pLevelElem = mpXmlDoc->FirstChildElement(asLevel.c_str());
		if(pLevelElem==nullptr){
			return nullptr;
		}

		return pLevelElem->Attribute(asName.c_str());
	}
}
