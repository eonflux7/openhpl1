// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/SoundEntityManager.h"

#include "system/String.h"
#include "sound/Sound.h"
#include "resources/Resources.h"
#include "sound/SoundEntityData.h"
#include "system/LowLevelSystem.h"
#include "sound/SoundHandler.h"
#include "sound/SoundChannel.h"

namespace hpl {

	cSoundEntityManager::cSoundEntityManager(cSound* apSound,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpSound = apSound;
		mpResources = apResources;
	}

	cSoundEntityManager::~cSoundEntityManager()
	{
		DestroyAll();

		Log(" Done with sound entities\n");
	}

	void cSoundEntityManager::Preload(const tString& asFile)
	{
		cSoundEntityData *pData = CreateSoundEntity(asFile);
		if(pData == nullptr) {
			Warning("Couldn't preload sound '%s'\n",asFile.c_str());
			return;
		}

		if(pData->GetMainSoundName() != ""){
			iSoundChannel *pChannel = mpSound->GetSoundHandler()->CreateChannel(pData->GetMainSoundName(),0);
			if(pChannel) hplDelete(pChannel);
		}
		if(pData->GetStartSoundName() != ""){
			iSoundChannel *pChannel = mpSound->GetSoundHandler()->CreateChannel(pData->GetStartSoundName(),0);
			if(pChannel) hplDelete(pChannel);
		}
		if(pData->GetStopSoundName() != ""){
			iSoundChannel *pChannel = mpSound->GetSoundHandler()->CreateChannel(pData->GetStopSoundName(),0);
			if(pChannel) hplDelete(pChannel);
		}
	}

	cSoundEntityData* cSoundEntityManager::CreateSoundEntity(const tString& asName)
	{
		tString sPath;
		cSoundEntityData* pSoundEntity;
		tString asNewName;

		BeginLoad(asName);

		asNewName = cString::SetFileExt(asName,"snt");

		pSoundEntity = static_cast<cSoundEntityData*>(this->FindLoadedResource(asNewName,sPath));

		if(pSoundEntity==nullptr && sPath!="")
		{
			pSoundEntity = hplNew( cSoundEntityData, (asNewName) );

			if(pSoundEntity->CreateFromFile(sPath))
			{
				AddResource(pSoundEntity);
			}
			else
			{
				hplDelete(pSoundEntity);
				pSoundEntity =nullptr;
			}
		}

		if(pSoundEntity)pSoundEntity->IncUserCount();
		else Error("Couldn't create SoundEntity '%s'\n",asNewName.c_str());

		EndLoad();
		return pSoundEntity;
	}

	iResourceBase* cSoundEntityManager::Create(const tString& asName)
	{
		return CreateSoundEntity(asName);
	}

	void cSoundEntityManager::Unload(iResourceBase* apResource)
	{

	}

	void cSoundEntityManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}
}
