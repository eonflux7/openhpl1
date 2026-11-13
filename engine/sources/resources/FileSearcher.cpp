// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/FileSearcher.h"
#include "system/String.h"
#include "resources/LowLevelResources.h"
#include "system/LowLevelSystem.h"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;


namespace hpl {

	cFileSearcher::cFileSearcher(iLowLevelResources *apLowLevelResources)
	{
		mpLowLevelResources = apLowLevelResources;
		mlDirectoryCount = 0;
	}

	cFileSearcher::~cFileSearcher()
	{
	}

	void cFileSearcher::AddDirectory(tString asPath, tString asMask)
	{
		if(ScanDirectory(asPath, asMask))
		{
			cAddedDirectory dir = {asPath, asMask, false};
			mvAddedDirectories.push_back(dir);
		}
	}

	void cFileSearcher::AddDirectoryTree(const tString &asPath, const tString &asMask)
	{
		cAddedDirectory dir = {asPath, asMask, true};
		mvAddedDirectories.push_back(dir);
		ScanDirectoryTree(asPath, asMask);
	}

	void cFileSearcher::Rescan()
	{
		std::vector<cAddedDirectory> vDirectories;
		vDirectories.swap(mvAddedDirectories);
		ClearDirectories();
		for(const cAddedDirectory &dir : vDirectories)
		{
			if(dir.mbTree)
				AddDirectoryTree(dir.msPath, dir.msMask);
			else
				AddDirectory(dir.msPath, dir.msMask);
		}
	}

	void cFileSearcher::ScanDirectoryTree(const tString &asPath, const tString &asMask)
	{
		std::vector<fs::path> vDirs;
		std::error_code err;
		const fs::path root = fs::u8path(asPath);
		for(fs::recursive_directory_iterator it(root, err), end; it != end; it.increment(err))
		{
			tString sName = it->path().filename().u8string();
			bool bDirectory = it->is_directory(err);
			if(sName.empty() || sName[0] == '.')
			{
				if(bDirectory) it.disable_recursion_pending();
				continue;
			}
			if(bDirectory) vDirs.push_back(it->path());
		}
		std::sort(vDirs.begin(), vDirs.end());

		ScanDirectory(asPath, asMask);
		for(const fs::path &dir : vDirs) ScanDirectory(dir.u8string(), asMask);
	}

	bool cFileSearcher::ScanDirectory(tString asPath, const tString &asMask)
	{
		tWStringList lstFileNames;
		//Make the path with only "/". The game's own directories are lower
		//case on disk, but absolute paths (e.g. next to the executable) may not be.
		asPath = cString::ReplaceCharTo(asPath,"\\","/");
		tString sLowerPath = cString::ToLowerCase(asPath);

		tStringSetIt it = m_setLoadedDirs.find(sLowerPath);
		//If the path is not allready added, add it!
		if(it==m_setLoadedDirs.end())
		{
			m_setLoadedDirs.insert(sLowerPath);

			mpLowLevelResources->FindFilesInDir(lstFileNames,cString::To16Char(asPath),
												cString::To16Char(asMask));
			if(lstFileNames.empty() && sLowerPath != asPath)
			{
				asPath = sLowerPath;
				mpLowLevelResources->FindFilesInDir(lstFileNames,cString::To16Char(asPath),
													cString::To16Char(asMask));
			}

			//A name already found in an earlier directory keeps that file.
			int lDirectory = mlDirectoryCount++;
			for(tWStringListIt it = lstFileNames.begin();it!=lstFileNames.end();it++)
			{
				tString sFile = cString::To8Char(*it);
				cFileSearcherEntry entry;
				entry.msPath = cString::SetFilePath(sFile, asPath);
				entry.mlDirectory = lDirectory;
				tString sLowerFile = cString::ToLowerCase(sFile);
				if(m_mapFiles.insert(tFilePathMap::value_type(sLowerFile, entry)).second == false)
					m_mapHidden.insert(std::pair<tString, tString>(sLowerFile, entry.msPath));
			}
			return true;
		}
		return false;
	}

	void cFileSearcher::ClearDirectories()
	{
		mvAddedDirectories.clear();
		m_mapFiles.clear();
		m_setLoadedDirs.clear();
		m_mapHidden.clear();
		mlDirectoryCount = 0;
	}

	tString cFileSearcher::GetExactFilePath(const tString &asName)
	{
		cFileSearcherEntry *pEntry = FindEntry(cString::ToLowerCase(asName));
		return pEntry ? pEntry->msPath : "";
	}

	tString cFileSearcher::GetHiddenFilePath(const tString &asName)
	{
		std::map<tString, tString>::iterator it = m_mapHidden.find(cString::ToLowerCase(asName));
		return it == m_mapHidden.end() ? "" : it->second;
	}

	void cFileSearcher::SetAlternativeExtensions(const tString &asExt, const tStringVec &avAlternatives)
	{
		tStringVec vLower;
		for(const tString &sAlt : avAlternatives) vLower.push_back(cString::ToLowerCase(sAlt));
		m_mapAlternatives[cString::ToLowerCase(asExt)] = vLower;
	}

	cFileSearcherEntry *cFileSearcher::FindEntry(const tString &asLowerName)
	{
		tFilePathMapIt it = m_mapFiles.find(asLowerName);
		return it == m_mapFiles.end() ? nullptr : &it->second;
	}

	tString cFileSearcher::GetFilePath(tString asName)
	{
		tString sLowerName = cString::ToLowerCase(asName);
		cFileSearcherEntry *pBest = nullptr;

		std::map<tString, tStringVec>::iterator altIt = m_mapAlternatives.find(cString::GetFileExt(sLowerName));
		if(altIt != m_mapAlternatives.end())
		{
			for(const tString &sAlt : altIt->second)
			{
				cFileSearcherEntry *pEntry = FindEntry(cString::SetFileExt(sLowerName, sAlt));
				if(pEntry && (pBest == nullptr || pEntry->mlDirectory < pBest->mlDirectory)) pBest = pEntry;
			}
		}

		cFileSearcherEntry *pEntry = FindEntry(sLowerName);
		if(pEntry && (pBest == nullptr || pEntry->mlDirectory < pBest->mlDirectory)) pBest = pEntry;

		return pBest ? pBest->msPath : "";
	}
}
