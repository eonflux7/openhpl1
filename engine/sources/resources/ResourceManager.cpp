// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/ResourceManager.h"
#include "system/String.h"

#include "resources/LowLevelResources.h"
#include "resources/FileSearcher.h"
#include "resources/ResourceBase.h"

#include "system/LowLevelSystem.h"

#include <algorithm>

namespace hpl {

	int iResourceManager::mlTabCount=0;

	iResourceManager::iResourceManager(cFileSearcher *apFileSearcher,
										iLowLevelResources *apLowLevelResources,
										iLowLevelSystem *apLowLevelSystem)
	{
		mpFileSearcher = apFileSearcher;
		mpLowLevelResources = apLowLevelResources;
		mpLowLevelSystem = apLowLevelSystem;
		mlHandleCount=0;
	}

	iResourceBase* iResourceManager::GetByName(const tString& asName)
	{
		tString sName = cString::ToLowerCase(asName);

		tResourceNameMapIt it = m_mapNameResources.find(sName);
		if(it == m_mapNameResources.end())return nullptr;

		return it->second;
	}

	iResourceBase* iResourceManager::GetByHandle(unsigned long alHandle)
	{
		tResourceHandleMapIt it = m_mapHandleResources.find(alHandle);
		if(it == m_mapHandleResources.end())return nullptr;

		return it->second;
	}

	cResourceBaseIterator iResourceManager::GetResourceBaseIterator()
	{
		return cResourceBaseIterator(&m_mapNameResources);
	}

	class cSortResources
	{
	public:
		bool operator()(iResourceBase* apResourceA, iResourceBase* apResourceB) const
		{
			if(apResourceA->GetUserCount() != apResourceB->GetUserCount())
			{
				return apResourceA->GetUserCount() > apResourceB->GetUserCount();
			}

			return apResourceA->GetTime() > apResourceB->GetTime();
		}
	};

	void iResourceManager::DestroyUnused(int alMaxToKeep)
	{
		//Check if there are too many resources.
		if((int)m_mapHandleResources.size() <= alMaxToKeep) return;

		//Add resources to a vector
		std::vector<iResourceBase*> vResources;
		vResources.reserve(m_mapHandleResources.size());

		tResourceHandleMapIt it = m_mapHandleResources.begin();
		for(;it != m_mapHandleResources.end();++it)
		{
			vResources.push_back(it->second);
		}

		//Sort the sounds according to num of users and then time.
		std::sort(vResources.begin(), vResources.end(), cSortResources());

		for(size_t i=alMaxToKeep; i<vResources.size(); ++i)
		{
			iResourceBase *pRes = vResources[i];

			if(pRes->HasUsers()==false)
			{
				RemoveResource(pRes);
				delete pRes;
			}
		}

	}

	void iResourceManager::DestroyAll()
	{
		tResourceHandleMapIt it = m_mapHandleResources.begin();
		while(it != m_mapHandleResources.end())
		{

			iResourceBase* pResource = it->second;

			while(pResource->HasUsers()) pResource->DecUserCount();

			Destroy(pResource);

			it = m_mapHandleResources.begin();

		}
	}

	void iResourceManager::BeginLoad(const tString& asFile)
	{
		mlTimeStart = GetApplicationTime();

		mlTabCount++;
	}

	void iResourceManager::EndLoad()
	{
		mlTabCount--;
	}

	iResourceBase* iResourceManager::FindLoadedResource(const tString &asName, tString &asFilePath)
	{
		iResourceBase* pResource = GetByName(asName);
		if(pResource==nullptr){
			asFilePath = mpFileSearcher->GetFilePath(asName);
		}
		else {
			asFilePath = "";
		}

		return pResource;
	}

	tString iResourceManager::LoadedFileName(const tString &asRequested, const tString &asPath)
	{
		tString sFile = cString::GetFileName(asPath);
		tString sExt = cString::GetFileExt(asRequested);
		if(sExt != "" && cString::ToLowerCase(sExt) != cString::ToLowerCase(cString::GetFileExt(sFile)))
			sFile = cString::SetFileExt(sFile, sExt);
		return sFile;
	}

	tString iResourceManager::GetTabs()
	{
		tString sTabs ="";
		for(int i=0; i<mlTabCount; ++i) sTabs+="  ";
		return sTabs;
	}

	void iResourceManager::AddResource(iResourceBase* apResource, bool abLog)
	{
		apResource->SetHandle(GetHandle());

		tString sName = cString::ToLowerCase(apResource->GetName());

		m_mapHandleResources.insert(tResourceHandleMap::value_type(
									apResource->GetHandle(), apResource));
		m_mapNameResources.insert(tResourceNameMap::value_type(
									sName, apResource));

		if(abLog && iResourceBase::GetLogCreateAndDelete())
		{
			unsigned long lTime = GetApplicationTime() - mlTimeStart;
			Log("%sLoaded resource %s in %d ms\n",GetTabs().c_str(), apResource->GetName().c_str(),lTime);
			apResource->SetLogDestruction(true);
		}

	}

	void iResourceManager::RemoveResource(iResourceBase* apResource)
	{
		m_mapHandleResources.erase(apResource->GetHandle());
		m_mapNameResources.erase(cString::ToLowerCase(apResource->GetName()));

	}

	unsigned long iResourceManager::GetHandle()
	{
		return mlHandleCount++;
	}
}
