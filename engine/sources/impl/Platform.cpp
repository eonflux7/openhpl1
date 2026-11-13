// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/Platform.h"
#include <filesystem>
#include <wctype.h>

namespace hpl {

	// Case-insensitive '*' and '?' matching, the same on every platform
	// (Windows' own search also matched 8.3 short names and let "*.*"
	// match names without a dot).
	static bool MaskMatches(const wchar_t *apMask, const wchar_t *apName)
	{
		switch(apMask[0])
		{
		case _W('\0'):
			return !apName[0];
		case _W('*'):
			return MaskMatches(apMask+1, apName) || (apName[0] && MaskMatches(apMask, apName+1));
		case _W('?'):
			return apName[0] && MaskMatches(apMask+1, apName+1);
		default:
			return towupper(apMask[0]) == towupper(apName[0]) && MaskMatches(apMask+1, apName+1);
		}
	}

	void Platform::FindFileInDir(tWStringList &alstStrings,tWString asDir, tWString asMask)
	{
		namespace fs = std::filesystem;
		if(asDir.empty()) return;

		std::error_code err;
		fs::directory_iterator it(fs::u8path(cString::To8Char(asDir)), err), end;
		for(; !err && it != end; it.increment(err))
		{
			// Like stat(), this follows links and skips broken ones
			std::error_code statErr;
			fs::file_status status = it->status(statErr);
			if(statErr || !fs::exists(status) || fs::is_directory(status)) continue;
			// A name that is not valid UTF-8 converts to nothing and is skipped
			tWString sName = cString::To16Char(it->path().filename().u8string());
			if(sName.empty()) continue;
			if(MaskMatches(asMask.c_str(), sName.c_str())) alstStrings.push_back(sName);
		}
	}
}
