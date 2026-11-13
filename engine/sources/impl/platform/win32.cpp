// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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
