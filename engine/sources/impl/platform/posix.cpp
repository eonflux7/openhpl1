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
#include "impl/Platform.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <wctype.h>
#include "system/LowLevelSystem.h"

namespace hpl {

	long Platform::FileLength(const tWString& asFileName)
	{
		struct stat statbuf;
		if (stat(cString::To8Char(asFileName).c_str(), &statbuf) == -1) {
			return 0;
		};
		return ((long)statbuf.st_size);
	}

	long Platform::FileLength(FILE *pFile)
	{
		struct stat statbuf;
		if (fstat(fileno(pFile), &statbuf) == -1) {
			return 0;
		};
		return ((long)statbuf.st_size);
	}

	static inline int patiMatch (const wchar_t *pattern, const wchar_t *string) {
	  switch (pattern[0])
	  {
	  case _W('\0'):
			return !string[0];

	  case _W('*') :
			return patiMatch(pattern+1, string) || (string[0] && patiMatch(pattern, string+1));

	  case _W('?') :
			return string[0] && patiMatch(pattern+1, string+1);

	  default  :
			return (towupper(pattern[0]) == towupper(string[0])) && patiMatch(pattern+1, string+1);
	  }
	}

	void Platform::FindFileInDir(tWStringList &alstStrings,tWString asDir, tWString asMask)
	{
		if(asDir.empty()) return;
		tString sDirectory = cString::To8Char(asDir);
		if(sDirectory.empty()) return;
		DIR *dirhandle = opendir(sDirectory.c_str());
		if(!dirhandle) return;
		if(sDirectory.back() != '/') sDirectory += '/';

		while(dirent *entry = readdir(dirhandle))
		{
			tWString fileentry = cString::To16Char(entry->d_name);
			if(fileentry.empty()) continue;
			struct stat statbuff;
			tString sPath = sDirectory + entry->d_name;
			if(stat(sPath.c_str(), &statbuff) == -1 || S_ISDIR(statbuff.st_mode)) continue;
			if(patiMatch(asMask.c_str(), fileentry.c_str())) alstStrings.push_back(fileentry);
		}
		closedir(dirhandle);
	}

}
