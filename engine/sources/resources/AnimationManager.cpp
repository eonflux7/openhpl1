// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/AnimationManager.h"
#include "system/String.h"
#include "system/System.h"
#include "resources/Resources.h"
#include "graphics/Mesh.h"
#include "graphics/Animation.h"
#include "system/LowLevelSystem.h"
#include "resources/MeshLoaderHandler.h"
#include "resources/FileSearcher.h"


namespace hpl {

	cAnimationManager::cAnimationManager(cGraphics* apGraphic,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphic;
		mpResources = apResources;
	}

	cAnimationManager::~cAnimationManager()
	{
		DestroyAll();

		Log(" Done with animations\n");
	}

	cAnimation* cAnimationManager::CreateAnimation(const tString& asName)
	{
		tString sPath;
		cAnimation *pAnimation=nullptr;
		tString asNewName;

		BeginLoad(asName);

		asNewName = asName;

		//If the file is missing an extension, search for an existing file.
		if(cString::GetFileExt(asNewName) == "")
		{
			bool bFound = false;
			tStringVec *pTypes = mpResources->GetMeshLoaderHandler()->GetSupportedTypes();
			for(size_t i=0; i< pTypes->size(); i++)
			{
				asNewName = cString::SetFileExt(asNewName, (*pTypes)[i]);
				sPath = mpResources->GetFileSearcher()->GetFilePath(asNewName);
				if(sPath != "")
				{
					bFound = true;
					break;
				}
			}

			if(bFound == false){
				Error("Couldn't create mesh '%s'\n",asName.c_str());
				EndLoad();
				return nullptr;
			}
		}

		pAnimation = static_cast<cAnimation*>(this->FindLoadedResource(asNewName,sPath));

		if(pAnimation==nullptr && sPath!="")
		{
			cMeshLoaderHandler *pMeshLoadHandler = mpResources->GetMeshLoaderHandler();

			//try to load animation from mesh
			cMesh *pTempMesh = pMeshLoadHandler->LoadMesh(sPath,0);
			if(pTempMesh==nullptr){
				Error("Couldn't load animation from '%s'\n",sPath.c_str());
				EndLoad();
				return nullptr;
			}

			if(pTempMesh->GetAnimationNum()<=0)
			{
				Error("No animations found in '%s'\n",sPath.c_str());
				hplDelete(pTempMesh);
				EndLoad();
				return nullptr;
			}

			pAnimation = pTempMesh->GetAnimation(0);
			pTempMesh->ClearAnimations(false);
			if(pAnimation->GetName() == pTempMesh->GetName()) pAnimation->SetName(LoadedFileName(asNewName, sPath));

			hplDelete(pTempMesh);

			AddResource(pAnimation);
		}

		if(pAnimation) pAnimation->IncUserCount();
		else Error("Couldn't create animation '%s'\n",asNewName.c_str());

		EndLoad();
		return pAnimation;
	}

	iResourceBase* cAnimationManager::Create(const tString& asName)
	{
		return CreateAnimation(asName);
	}

	void cAnimationManager::Unload(iResourceBase* apResource)
	{

	}

	void cAnimationManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}
}
