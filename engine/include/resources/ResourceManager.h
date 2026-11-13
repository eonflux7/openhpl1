// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_RESOURCEMANAGER_H
#define HPL_RESOURCEMANAGER_H

#include <map>
#include "system/SystemTypes.h"

namespace hpl {

	class iLowLevelResources;
	class iLowLevelSystem;
	class cFileSearcher;
	class iResourceBase;

	typedef std::map<unsigned long, iResourceBase*> tResourceHandleMap;
	typedef tResourceHandleMap::iterator tResourceHandleMapIt;

	typedef std::map<tString, iResourceBase*> tResourceNameMap;
	typedef tResourceNameMap::iterator tResourceNameMapIt;

	typedef std::list<iResourceBase*> tResourceBaseList;
	typedef tResourceBaseList::iterator tResourceBaseListIt;

	typedef cSTLMapIterator<iResourceBase*, tResourceNameMap, tResourceNameMapIt> cResourceBaseIterator;


	class iResourceManager
	{
	public:
		/**
		 * The name of a resource loaded from asPath when asRequested was asked
		 * for: the file's name, with the requested extension when a file of
		 * another type was used in its place (a .glb model for a .dae one).
		 */
		static tString LoadedFileName(const tString &asRequested, const tString &asPath);

		iResourceManager(cFileSearcher *apFileSearcher, iLowLevelResources *apLowLevelResources,
						iLowLevelSystem *apLowLevelSystem);
		virtual ~iResourceManager(){}

		virtual iResourceBase* Create(const tString& asName)=0;

		iResourceBase* GetByName(const tString& asName);
		iResourceBase* GetByHandle(unsigned long alHandle);

		cResourceBaseIterator GetResourceBaseIterator();

		void DestroyUnused(int alMaxToKeep);

		virtual void Destroy(iResourceBase* apResource)=0;
		virtual void DestroyAll();

		virtual void Unload(iResourceBase* apResource)=0;

		virtual void Update(float afTimeStep){}

	protected:
		unsigned long mlHandleCount;
		tResourceNameMap m_mapNameResources;
		tResourceHandleMap m_mapHandleResources;

		cFileSearcher *mpFileSearcher;
		iLowLevelResources *mpLowLevelResources;
		iLowLevelSystem *mpLowLevelSystem;

		void BeginLoad(const tString& asFile);
		void EndLoad();

		unsigned long mlTimeStart;

		/**
		 * Checks if a resource alllready is in the manager, else searches the resources.
		 * \param &asName Name of the resource.
		 * \param &asFilePath If the file is not in the manager, the path is put here. "" if there is no such file.
		 * \return A pointer to the resource. NULL if not in manager.
		 */
		iResourceBase* FindLoadedResource(const tString &asName, tString &asFilePath);
		void AddResource(iResourceBase* apResource, bool abLog=true);
		void RemoveResource(iResourceBase* apResource);

		unsigned long GetHandle();

		tString GetTabs();
		static int mlTabCount;

	};

};
#endif // HPL_RESOURCEMANAGER_H
