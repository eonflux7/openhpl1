// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/SoundManager.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "sound/Sound.h"
#include "sound/SoundData.h"
#include "sound/LowLevelSound.h"

namespace hpl {

	cSoundManager::cSoundManager(cSound* apSound,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpSound = apSound;
		mpResources = apResources;

		mpSound->GetLowLevel()->GetSupportedFormats(mlstFileFormats);
	}

	cSoundManager::~cSoundManager()
	{
		DestroyAll();
		Log(" Done with sounds\n");
	}

	iSoundData* cSoundManager::CreateSoundData(const tString& asName, bool abStream,bool abLoopStream)
	{
		tString sPath;
		iSoundData* pSound=nullptr;

		BeginLoad(asName);

		pSound = FindData(asName, sPath);

		if(pSound==nullptr && sPath!="")
		{
			pSound = mpSound->GetLowLevel()->LoadSoundData(cString::GetFileName(sPath),sPath,"",abStream,
															abLoopStream);
			if(pSound)
			{
				pSound->SetFilePath(sPath);
				AddResource(pSound);
				pSound->SetSoundManager(mpResources->GetSoundManager());
			}

		}
		else
		{

		}


		EndLoad();
		return pSound;
	}

	iResourceBase* cSoundManager::Create(const tString& asName)
	{
		return	CreateSoundData(asName,false);
	}

	void cSoundManager::Unload(iResourceBase* apResource)
	{

	}

	void cSoundManager::Destroy(iResourceBase* apResource)
	{
		if(apResource->HasUsers())
		{
			apResource->DecUserCount();

			iSoundData *pData = static_cast<iSoundData *>(apResource);
			if(pData->IsStream() && pData->HasUsers()==false)
			{
				RemoveResource(pData);
				hplDelete(pData);
			}
		}
	}

	void cSoundManager::DestroyAll()
	{
		//FIX FIX FIX
		//Fix what? :P
		tResourceHandleMapIt it = m_mapHandleResources.begin();
		while(it != m_mapHandleResources.end())
		{
			iResourceBase* pData = it->second;
			RemoveResource(pData);
			hplDelete(pData);

			it= m_mapHandleResources.begin();
		}
	}

	iSoundData *cSoundManager::FindData(const tString &asName, tString &asFilePath)
	{
		iSoundData *pData=nullptr;

		if(cString::GetFileExt(asName)=="")
		{
			for(tStringListIt it = mlstFileFormats.begin();it!=mlstFileFormats.end();++it)
			{
				tString sNewName = cString::SetFileExt(asName,*it);
				pData = static_cast<iSoundData*> (FindLoadedResource(sNewName, asFilePath));

				if((pData==nullptr && asFilePath!="") || pData!=nullptr)break;
			}
		}
		else
		{
			pData = static_cast<iSoundData*> (FindLoadedResource(asName, asFilePath));
		}

		return pData;
	}
}
