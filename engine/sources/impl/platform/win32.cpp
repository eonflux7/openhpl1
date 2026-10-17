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
#include <io.h>

#include "system/LowLevelSystem.h"

namespace hpl {

	long Platform::FileLength(const tWString& asFileName)
	{
		FILE *pFile = _wfopen(asFileName.c_str(), _W("rb"));
		if(pFile == nullptr) return 0;
		long ret = Platform::FileLength(pFile);
		fclose(pFile);
		return ret;
	}

	long Platform::FileLength(FILE *pFile)
	{
		return (long)_filelength(_fileno(pFile));
	}

	void Platform::FindFileInDir(tWStringList &alstStrings,tWString asDir, tWString asMask)
	{
		if(asDir.empty()) return;

		// The search functions only accept '\\' separators
		tWString sSpec = cString::ReplaceCharToW(asDir,_W("/"),_W("\\"));
		if(sSpec.back() != _W('\\')) sSpec += _W('\\');
		sSpec += asMask;

		struct _wfinddata_t FileInfo;
		intptr_t lHandle = _wfindfirst(sSpec.c_str(), &FileInfo);
		if(lHandle == -1) return;

		do
		{
			// Skip subdirectories
			if((FileInfo.attrib & _A_SUBDIR)==0)
				alstStrings.push_back(FileInfo.name);
		}
		while(_wfindnext(lHandle, &FileInfo) == 0);

		_findclose(lHandle);
	}
}
