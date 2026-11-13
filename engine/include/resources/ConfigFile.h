// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_CONFIGFILE_H
#define HPL_CONFIGFILE_H

#include "system/SystemTypes.h"

class TiXmlDocument;

namespace hpl {

	class cFileSearcher;

	class cConfigFile
	{
	public:
		cConfigFile(tWString asFile);
		~cConfigFile();

		/**
		 * Loads data from file
		 * \return true if loading ws ok, else false
		 */
		bool Load();
		/**
		 * Saves the data to file
		 * \return true if loading ws ok, else false
		 */
		bool Save();

		void SetString(tString asLevel, tString asName, tString asVal);
		void SetInt(tString asLevel, tString asName, int alVal);
		void SetFloat(tString asLevel, tString asName, float afVal);
		void SetBool(tString asLevel, tString asName, bool abVal);

		tString GetString(tString asLevel, tString asName, tString asDefault);
		int GetInt(tString asLevel, tString asName, int alDefault);
		float GetFloat(tString asLevel, tString asName, float afDefault);
		bool GetBool(tString asLevel, tString asName, bool abDefault);

	private:
		tWString msFile;
		cFileSearcher *mpFileSearcher;

		TiXmlDocument *mpXmlDoc;

		const char* GetCharArray(tString asLevel, tString asName);
	};

};
#endif // HPL_CONFIGFILE_H
