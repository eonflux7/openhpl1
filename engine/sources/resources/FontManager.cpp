// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/FontManager.h"
#include "resources/FileSearcher.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "graphics/Graphics.h"
#include "graphics/LowLevelGraphics.h"
#include "resources/ImageManager.h"

#include "graphics/FontData.h"

#include "tinyxml.h"


namespace hpl {

	cFontManager::cFontManager(cGraphics* apGraphics,cGui *apGui,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphics;
		mpResources = apResources;
		mpGui = apGui;
	}

	cFontManager::~cFontManager()
	{
		DestroyAll();
		Log(" Done with fonts\n");
	}

	iFontData* cFontManager::CreateFontData(const tString& asName, int alSize,unsigned short alFirstChar,
											unsigned short alLastChar)
	{
		tString sPath;
		iFontData* pFont;
		tString asNewName = cString::ToLowerCase(asName);

		BeginLoad(asName);

		pFont = static_cast<iFontData*>(this->FindLoadedResource(asNewName,sPath));

		//A vector font with a base: the base's .fnt under this name
		auto vectorIt = m_mapVectorFonts.find(cString::GetFileName(asNewName));
		if(pFont==nullptr && sPath=="" && vectorIt != m_mapVectorFonts.end() && vectorIt->second.msBase != "")
			sPath = mpFileSearcher->GetFilePath(vectorIt->second.msBase);

		if(pFont==nullptr && sPath!="")
		{
			pFont = mpGraphics->GetLowLevel()->CreateFontData(asNewName);
			pFont->SetUp(mpGraphics->GetDrawer(),mpLowLevelResources,mpGui);

			tString sExt = cString::ToLowerCase(cString::GetFileExt(asName));

			//True Type Font
			if(sExt == "ttf")
			{
				if(pFont->CreateFromFontFile(sPath,alSize,alFirstChar,alLastChar)==false){
					delete pFont;
					EndLoad();
					return nullptr;
				}
			}
			//Angel code font type
			else if(sExt == "fnt")
			{
				if(pFont->CreateFromBitmapFile(sPath)==false){
					delete pFont;
					EndLoad();
					return nullptr;
				}

				if(vectorIt != m_mapVectorFonts.end())
				{
					const cVectorFont &vectorFont = vectorIt->second;
					if(pFont->SetVectorFont(vectorFont.msFile,vectorFont.mfSize,vectorFont.mfWidth,vectorFont.mfPixels)==false)
						Warning("Font '%s' uses its bitmaps\n",asNewName.c_str());
				}
			}
			else
			{
				Error("Font '%s' has an unkown extension!\n",asName.c_str());
				delete pFont;
				EndLoad();
				return nullptr;
			}

			AddResource(pFont);
		}

		if(pFont)pFont->IncUserCount();
		else Error("Couldn't create font '%s'\n",asNewName.c_str());

		EndLoad();
		return pFont;
	}

	bool cFontManager::LoadVectorFonts(const tString& asDir)
	{
		tString sFile = asDir + "fonts.cfg";

		TiXmlDocument xmlDoc(sFile.c_str());
		if(xmlDoc.LoadFile()==false || xmlDoc.RootElement()==nullptr)
		{
			Log("No vector font list '%s', using bitmap fonts\n",sFile.c_str());
			return false;
		}

		TiXmlElement *pFontElem = xmlDoc.RootElement()->FirstChildElement("Font");
		for(; pFontElem != nullptr; pFontElem = pFontElem->NextSiblingElement("Font"))
		{
			tString sName = cString::ToLowerCase(cString::ToString(pFontElem->Attribute("Name"),""));
			tString sFontFile = cString::ToString(pFontElem->Attribute("File"),"");
			if(sName=="" || sFontFile=="") continue;

			cVectorFont vectorFont;
			vectorFont.msFile = asDir + sFontFile;
			vectorFont.msBase = cString::ToLowerCase(cString::ToString(pFontElem->Attribute("Base"),""));
			vectorFont.mfSize = cString::ToFloat(pFontElem->Attribute("Size"),1);
			vectorFont.mfWidth = cString::ToFloat(pFontElem->Attribute("Width"),1);
			vectorFont.mfPixels = cString::ToFloat(pFontElem->Attribute("Pixels"),0);

			m_mapVectorFonts[sName] = vectorFont;
		}

		return true;
	}

	bool cFontManager::HasVectorFont(const tString& asName) const
	{
		return m_mapVectorFonts.count(cString::ToLowerCase(cString::GetFileName(asName))) > 0;
	}

	iResourceBase* cFontManager::Create(const tString& asName)
	{
		return CreateFontData(asName, 16, 32, 255);
	}

	void cFontManager::Unload(iResourceBase* apResource)
	{

	}

	void cFontManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			delete apResource;
		}
	}
}
