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
#include "resources/FontManager.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "graphics/Graphics.h"
#include "graphics/LowLevelGraphics.h"
#include "resources/ImageManager.h"

#include "graphics/FontData.h"

#include "tinyxml.h"


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iFontData* cFontManager::CreateFontData(const tString& asName, int alSize,unsigned short alFirstChar,
											unsigned short alLastChar)
	{
		tString sPath;
		iFontData* pFont;
		tString asNewName = cString::ToLowerCase(asName);

		BeginLoad(asName);

		//asNewName = cString::SetFileExt(asName,"ttf");

		pFont = static_cast<iFontData*>(this->FindLoadedResource(asNewName,sPath));

		if(pFont==nullptr && sPath!="")
		{
			pFont = mpGraphics->GetLowLevel()->CreateFontData(asNewName);
			pFont->SetUp(mpGraphics->GetDrawer(),mpLowLevelResources,mpGui);

			tString sExt = cString::ToLowerCase(cString::GetFileExt(asName));

			//True Type Font
			if(sExt == "ttf")
			{
				if(pFont->CreateFromFontFile(sPath,alSize,alFirstChar,alLastChar)==false){
					hplDelete(pFont);
					EndLoad();
					return nullptr;
				}
			}
			//Angel code font type
			else if(sExt == "fnt")
			{
				if(pFont->CreateFromBitmapFile(sPath)==false){
					hplDelete(pFont);
					EndLoad();
					return nullptr;
				}

				auto vectorIt = m_mapVectorFonts.find(cString::GetFileName(asNewName));
				if(vectorIt != m_mapVectorFonts.end())
				{
					const cVectorFont &vectorFont = vectorIt->second;
					if(pFont->SetVectorFont(vectorFont.msFile,vectorFont.mfSize,vectorFont.mfWidth)==false)
						Warning("Font '%s' uses its bitmaps\n",asNewName.c_str());
				}
			}
			else
			{
				Error("Font '%s' has an unkown extension!\n",asName.c_str());
				hplDelete(pFont);
				EndLoad();
				return nullptr;
			}

			//mpResources->GetImageManager()->FlushAll();
			AddResource(pFont);
		}

		if(pFont)pFont->IncUserCount();
		else Error("Couldn't create font '%s'\n",asNewName.c_str());

		EndLoad();
		return pFont;
	}

	//-----------------------------------------------------------------------

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
			vectorFont.mfSize = cString::ToFloat(pFontElem->Attribute("Size"),1);
			vectorFont.mfWidth = cString::ToFloat(pFontElem->Attribute("Width"),1);

			m_mapVectorFonts[sName] = vectorFont;
		}

		return true;
	}

	//-----------------------------------------------------------------------

	iResourceBase* cFontManager::Create(const tString& asName)
	{
		return CreateFontData(asName, 16, 32, 255);
	}

	//-----------------------------------------------------------------------

	void cFontManager::Unload(iResourceBase* apResource)
	{

	}
	//-----------------------------------------------------------------------

	void cFontManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------


	//-----------------------------------------------------------------------
}
