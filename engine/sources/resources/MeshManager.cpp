// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/MeshManager.h"
#include "system/String.h"
#include "system/System.h"
#include "resources/Resources.h"
#include "graphics/Mesh.h"
#include "system/LowLevelSystem.h"
#include "resources/MeshLoaderHandler.h"
#include "resources/FileSearcher.h"


namespace hpl {

	cMeshManager::cMeshManager(cGraphics* apGraphic,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphic;
		mpResources = apResources;
	}

	cMeshManager::~cMeshManager()
	{
		DestroyAll();

		Log(" Done with meshes\n");
	}

	cMesh* cMeshManager::CreateMesh(const tString& asName)
	{
		tString sPath;
		cMesh* pMesh;
		tString asNewName;

		BeginLoad(asName);

		asNewName = asName;//cString::SetFileExt(asName,"mesh");

		//If the file is missing an extension, search for an existing file.
		if(cString::GetFileExt(asNewName) == "")
		{
			bool bFound = false;
			tStringVec *pTypes = mpResources->GetMeshLoaderHandler()->GetSupportedTypes();
			for(size_t i=0; i< pTypes->size(); i++)
			{
				asNewName = cString::SetFileExt(asNewName, (*pTypes)[i]);
				tString sPath = mpResources->GetFileSearcher()->GetFilePath(asNewName);
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

		pMesh = static_cast<cMesh*>(this->FindLoadedResource(asNewName,sPath));

		if(pMesh==nullptr && sPath!="")
		{
			pMesh = mpResources->GetMeshLoaderHandler()->LoadMesh(sPath,0);
			if(pMesh == nullptr)
			{
				EndLoad();
				return nullptr;
			}
			//A .glb used for a .dae keeps the .dae name, so that it is found
			//again and saves name the same file
			pMesh->SetName(LoadedFileName(asNewName, sPath));

			AddResource(pMesh);
		}

		if(pMesh)pMesh->IncUserCount();
		else Error("Couldn't create mesh '%s'\n",asNewName.c_str());

		EndLoad();
		return pMesh;
	}

	iResourceBase* cMeshManager::Create(const tString& asName)
	{
		return CreateMesh(asName);
	}

	void cMeshManager::Unload(iResourceBase* apResource)
	{

	}

	void cMeshManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			delete apResource;
		}
	}
}
