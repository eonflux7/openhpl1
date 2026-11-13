// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LANGUAGE_FILE_H
#define HPL_LANGUAGE_FILE_H

#include <map>
#include "system/SystemTypes.h"

namespace hpl {

	class cResources;

	class cLanguageEntry
	{
	public:
		tWString mwsText;
	};

	typedef std::map<tString, cLanguageEntry*> tLanguageEntryMap;
	typedef tLanguageEntryMap::iterator tLanguageEntryMapIt;

	class cLanguageCategory
	{
	public:
		~cLanguageCategory(){
			STLMapDeleteAll(m_mapEntries);
		}

		tLanguageEntryMap m_mapEntries;
	};

	typedef std::map<tString, cLanguageCategory*> tLanguageCategoryMap;
	typedef tLanguageCategoryMap::iterator tLanguageCategoryMapIt;

	class cLanguageFile
	{
	public:
		cLanguageFile(cResources *apResources);
		~cLanguageFile();

		bool LoadFromFile(const tString asFile);

		const tWString& Translate(const tString& asCat, const tString& asName);

	private:
		tLanguageCategoryMap m_mapCategories;
		tWString mwsEmpty;

		cResources *mpResources;
	};

};
#endif // HPL_LANGUAGE_FILE_H
