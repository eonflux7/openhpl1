// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/VideoManager.h"

#include "system/LowLevelSystem.h"
#include "system/String.h"
#include "resources/Resources.h"
#include "resources/FileSearcher.h"
#include "graphics/VideoStream.h"

namespace hpl {

	cVideoManager::cVideoManager(cGraphics* apGraphics,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphics;
		mpResources = apResources;
	}

	cVideoManager::~cVideoManager()
	{
		STLDeleteAll(mlstVideoLoaders);
	}

	iResourceBase* cVideoManager::Create(const tString& asName)
	{
		return CreateVideo(asName);
	}

	iVideoStream* cVideoManager::CreateVideo(const tString& asName)
	{
		BeginLoad(asName);

		tString sPath = mpFileSearcher->GetFilePath(asName);
		if(sPath == "")
		{
			EndLoad();
			Error("Video file '%s' could not be found!\n",asName.c_str());
			return nullptr;
		}

		iVideoStreamLoader *pLoader = GetLoader(asName);
		if(pLoader==nullptr)
		{
			Error("Could not find a loader for '%s'\n",asName.c_str());
			return nullptr;
		}

		iVideoStream *pVideo = pLoader->Create(asName);

		if(pVideo->LoadFromFile(sPath)==false)
		{
			EndLoad();
			hplDelete(pVideo);
			Error("Could not load video '%s'\n",asName.c_str());
			return nullptr;
		}

		AddResource(pVideo);

		EndLoad();
		return pVideo;
	}

	void cVideoManager::AddVideoLoader(iVideoStreamLoader *apLoader)
	{
		mlstVideoLoaders.push_back(apLoader);
	}

	void cVideoManager::Unload(iResourceBase* apResource)
	{

	}

	void cVideoManager::Destroy(iResourceBase* apResource)
	{
		if(apResource)
		{
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}

	void cVideoManager::Update(float afTimeStep)
	{
		tResourceHandleMapIt it = m_mapHandleResources.begin();
		for(; it != m_mapHandleResources.end(); ++it)
		{
			iResourceBase *pBase = it->second;
			iVideoStream *pVideo = static_cast<iVideoStream*>(pBase);

			pVideo->Update(afTimeStep);
		}

	}

	iVideoStreamLoader * cVideoManager::GetLoader(const tString& asFileName)
	{
		tString sExt = cString::ToLowerCase(cString::GetFileExt(asFileName));

		tVideoStreamLoaderListIt it = mlstVideoLoaders.begin();
		for(; it != mlstVideoLoaders.end(); ++it)
		{
			iVideoStreamLoader *pLoader = *it;

			tStringVec& vExt = pLoader->GetExtensions();
			for(size_t i=0; i< vExt.size(); ++i)
			{
				if(vExt[i] == sExt){
					return pLoader;
				}
			}
		}

		return nullptr;
	}
}
