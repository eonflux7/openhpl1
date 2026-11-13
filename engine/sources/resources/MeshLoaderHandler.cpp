// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/MeshLoaderHandler.h"

#include "resources/MeshLoader.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "scene/Scene.h"

namespace hpl {

	bool iMeshLoader::mbRestrictStaticLightToSector = false;
	bool iMeshLoader::mbUseFastMaterial = false;
	tString iMeshLoader::msFastMaterialFile = "";
	tString iMeshLoader::msCacheDir = "core/cache/";
	tString iMeshLoader::msCacheWriteDir = "";

	cMeshLoaderHandler::cMeshLoaderHandler(cResources* apResources, cScene *apScene)
	{
		mpResources = apResources;
		mpScene = apScene;
	}

	cMeshLoaderHandler::~cMeshLoaderHandler()
	{
		tMeshLoaderListIt it = mlstLoaders.begin();
		for(;it != mlstLoaders.end();it++)
		{
			hplDelete(*it);
		}

		mlstLoaders.clear();
	}

	cMesh* cMeshLoaderHandler::LoadMesh(const tString& asFile,tMeshLoadFlag aFlags)
	{
		tString sType = cString::ToLowerCase(cString::GetFileExt(asFile));

		tMeshLoaderListIt it = mlstLoaders.begin();
		for(;it != mlstLoaders.end();it++)
		{
			iMeshLoader *pLoader = *it;

			if(pLoader->IsSupported(sType))
			{
				return pLoader->LoadMesh(asFile,aFlags);
			}
		}

		Log("No loader for '%s' found!\n", sType.c_str());
		return nullptr;
	}

	cWorld3D* cMeshLoaderHandler::LoadWorld(const tString& asFile, tWorldLoadFlag aFlags)
	{
		tString sType = cString::ToLowerCase(cString::GetFileExt(asFile));

		tMeshLoaderListIt it = mlstLoaders.begin();
		for(;it != mlstLoaders.end();it++)
		{
			iMeshLoader *pLoader = *it;

			if(pLoader->IsSupported(sType))
			{
				return pLoader->LoadWorld(asFile,mpScene, aFlags);
			}
		}

		Log("No loader for '%s' found!\n", sType.c_str());
		return nullptr;
	}

	cAnimation* cMeshLoaderHandler::LoadAnimation(const tString& asFile)
	{
		tString sType = cString::ToLowerCase(cString::GetFileExt(asFile));

		tMeshLoaderListIt it = mlstLoaders.begin();
		for(;it != mlstLoaders.end();it++)
		{
			iMeshLoader *pLoader = *it;

			if(pLoader->IsSupported(sType))
			{
				return pLoader->LoadAnimation(asFile);
			}
		}

		Log("No loader for '%s' found!\n", sType.c_str());
		return nullptr;
	}

	void cMeshLoaderHandler::AddLoader(iMeshLoader *apLoader)
	{
		mlstLoaders.push_back(apLoader);

		apLoader->mpMaterialManager = mpResources->GetMaterialManager();
		apLoader->mpMeshManager = mpResources->GetMeshManager();
		apLoader->mpAnimationManager = mpResources->GetAnimationManager();
		apLoader->mpSystem = mpScene->GetSystem();

		apLoader->AddSupportedTypes(&mvSupportedTypes);
	}
}
