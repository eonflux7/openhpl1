// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_FILESEARCHER_H
#define HPL_FILESEARCHER_H

#include <map>
#include <vector>
#include "resources/ResourcesTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class iLowLevelResources;

	// A file found in an added directory. Directories added earlier take
	// precedence: mlDirectory is the order in which its directory was added.
	class cFileSearcherEntry
	{
	public:
		tString msPath;
		int mlDirectory;
	};

	typedef std::map<tString, cFileSearcherEntry> tFilePathMap;
	typedef tFilePathMap::iterator tFilePathMapIt;

	class cFileSearcher
	{
	public:
		cFileSearcher(iLowLevelResources *apLowLevelResources);
		~cFileSearcher();

		/**
		 * Adds a directory that will be searched when looking for files.
		 * \param asMask What files that should be searched for, for example: "*.jpeg".
		 * \param asPath The path to the directory.
		 */
		void AddDirectory(tString asPath, tString asMask);

		/**
		 * Adds a directory and all its subdirectories (except hidden ones,
		 * whose names start with "."), the subdirectories sorted by path, each
		 * as with AddDirectory().
		 */
		void AddDirectoryTree(const tString &asPath, const tString &asMask);

		/**
		 * Finds the files again in the directories added so far, in the same
		 * order, with the subdirectories that AddDirectoryTree() directories
		 * have now. For files added to the directories since.
		 */
		void Rescan();

		/**
		 * Clears all directories
		 */
		void ClearDirectories();

		/**
		 * Gets a file pointer and searches through all added resources.
		 * \param asName Name of the file.
		 * \return Path to the file. "" if file is not found.
		 */
		tString GetFilePath(tString asName);

		/// Every file found, by its name in lower case
		const tFilePathMap& GetFiles() const { return m_mapFiles; }

		/**
		 * Lets a request for a file with extension asExt be answered by a file
		 * of the same name with one of the alternatives instead (e.g. a model
		 * converted from .dae to .glb). The file from the directory added first
		 * is used; within a directory the alternatives come first, in order.
		 */
		void SetAlternativeExtensions(const tString &asExt, const tStringVec &avAlternatives);

		/**
		 * The path of a file of exactly this name, without alternatives.
		 */
		tString GetExactFilePath(const tString &asName);

		/**
		 * The path of the first file of this name that a file in a directory
		 * added before it hides; "" if none.
		 */
		tString GetHiddenFilePath(const tString &asName);

	private:
		class cAddedDirectory
		{
		public:
			tString msPath;
			tString msMask;
			bool mbTree;
		};

		cFileSearcherEntry *FindEntry(const tString &asLowerName);
		bool ScanDirectory(tString asPath, const tString &asMask);
		void ScanDirectoryTree(const tString &asPath, const tString &asMask);

		std::vector<cAddedDirectory> mvAddedDirectories;

		tFilePathMap m_mapFiles;
		tStringSet m_setLoadedDirs;
		int mlDirectoryCount;
		std::map<tString, tStringVec> m_mapAlternatives;
		std::map<tString, tString> m_mapHidden;

		iLowLevelResources *mpLowLevelResources;
	};

};
#endif // HPL_FILESEARCHER_H
