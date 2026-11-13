// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/LanguageFile.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/FileSearcher.h"
#include "tinyxml.h"

#include "resources/Resources.h"
#include "resources/FileSearcher.h"

namespace hpl {

	cLanguageFile::cLanguageFile(cResources *apResources)
	{
		mpResources = apResources;
	}

	cLanguageFile::~cLanguageFile()
	{
		STLMapDeleteAll(m_mapCategories);
	}

	bool cLanguageFile::LoadFromFile(const tString asFile)
	{
		TiXmlDocument *pDoc = hplNew(TiXmlDocument,(asFile.c_str()) );
		if(pDoc->LoadFile()==false)
		{
			hplDelete(pDoc);
			Error("Couldn't find language file '%s'\n",asFile.c_str());
			return false;
		}

		TiXmlElement *pRootElem = pDoc->FirstChildElement();

		//Iterate the resources
		TiXmlElement *pResourceElem = pRootElem->FirstChildElement("RESOURCES");
		if(pResourceElem)
		{
			TiXmlElement *pDirElem = pResourceElem->FirstChildElement("Directory");
			for(; pDirElem != nullptr; pDirElem = pDirElem->NextSiblingElement("Directory"))
			{
					tString sPath = pDirElem->Attribute("Path");
					mpResources->AddResourceDir(sPath);
			}
		}
		else
		{
			Warning("No resources element found in '%s'\n",asFile.c_str());
		}

		//Iterate the categories
		TiXmlElement *pCatElem = pRootElem->FirstChildElement("CATEGORY");
		for(; pCatElem != nullptr; pCatElem = pCatElem->NextSiblingElement("CATEGORY"))
		{
			cLanguageCategory *pCategory = hplNew( cLanguageCategory, () );
			tString sCatName = pCatElem->Attribute("Name");

			m_mapCategories.insert(tLanguageCategoryMap::value_type(sCatName, pCategory));

			//Iterate the entries
			TiXmlElement *pEntryElem = pCatElem->FirstChildElement("Entry");
			for(; pEntryElem != nullptr; pEntryElem = pEntryElem->NextSiblingElement("Entry"))
			{
				cLanguageEntry *pEntry = hplNew( cLanguageEntry, () );
				tString sEntryName = pEntryElem->Attribute("Name");

				if(pEntryElem->FirstChild()==nullptr)
				{
					pEntry->mwsText = _W("");
				}
				else
				{
					TiXmlText *pTextNode = pEntryElem->FirstChild()->ToText();
					if(pTextNode)
					{

						tString sString = pTextNode->Value();
						pEntry->mwsText = _W("");

						for(size_t i=0; i< sString.length(); ++i)
						{
							unsigned char c = sString[i];
							if(c=='[')
							{
								bool bFoundCommand = true;
								tString sCommand = "";
								int lCount =1;

								while(sString[i+lCount] != ']' && i+lCount<sString.length() && lCount < 16)
								{
									sCommand += sString[i+lCount];
									lCount++;
								}

								if(sCommand=="br")
								{
									pEntry->mwsText += _W('\n');
								}
								else if(sCommand[0]=='u')
								{
									int lNum = cString::ToInt(sCommand.substr(1).c_str(),0);
									pEntry->mwsText += (wchar_t)lNum;
								}
								else
								{
									bFoundCommand = false;
								}

								//Go forward or add [ to string
								if(bFoundCommand)
								{
									i += lCount;
								}
								else
								{
									pEntry->mwsText += sString[i];
								}
							}
							//Decode UTF-8!
							else if(c >= 128)
							{
								unsigned char c2 = sString[i+1];

								int lNum = c & 0x1f; // c AND 0001 1111
								lNum = lNum << 6;
								lNum = lNum | (c2 & 0x3f);// c AND 0011 1111

								pEntry->mwsText += (wchar_t)lNum;
								++i;
							}
							else
							{
								pEntry->mwsText += c;
							}
						}

					}
				}

				std::pair<tLanguageEntryMap::iterator,bool> ret = pCategory->m_mapEntries.insert(tLanguageEntryMap::value_type(sEntryName,pEntry));
				if(ret.second==false){
					Warning("Language entry '%s' in category '%s' already exists!\n",sEntryName.c_str(), sCatName.c_str());
					hplDelete(pEntry);
				}
			}
		}

		hplDelete(pDoc);


		return true;
	}

	const tWString& cLanguageFile::Translate(const tString& asCat, const tString& asName)
	{
		tLanguageCategoryMapIt CatIt = m_mapCategories.find(asCat);
		if(CatIt == m_mapCategories.end())
		{
			Warning("Could not find language file category '%s'\n",asCat.c_str());
			return mwsEmpty;
		}

		cLanguageCategory *pCategory = CatIt->second;
		tLanguageEntryMapIt EntryIt = pCategory->m_mapEntries.find(asName);
		if(EntryIt == pCategory->m_mapEntries.end())
		{
			Warning("Could not find language file entry '%s'\n",asName.c_str());
			return mwsEmpty;
		}

		cLanguageEntry *pEntry = EntryIt->second;

		return pEntry->mwsText;
	}
}
