// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/ParticleManager.h"

#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "resources/FileSearcher.h"
#include "graphics/Graphics.h"
#include "graphics/ParticleSystem3D.h"

namespace hpl {

	cParticleManager::cParticleManager(cGraphics* apGraphics,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphics;
		mpResources = apResources;
	}

	cParticleManager::~cParticleManager()
	{
		tResourceHandleMapIt it = m_mapHandleResources.begin();
		for(;it != m_mapHandleResources.end(); ++it)
		{
			iResourceBase* pResource = it->second;
			while(pResource->HasUsers()) pResource->DecUserCount();
		}

		DestroyUnused(0);

		Log(" Done with particles\n");


	}

	iResourceBase* cParticleManager::Create(const tString& asName)
	{
		return nullptr;
	}

	iParticleSystem2D* cParticleManager::CreatePS2D(const tString& asName, cVector3f avSize)
	{

		return nullptr;
	}

	cParticleSystem3D* cParticleManager::CreatePS3D(const tString& asName,const tString& asType,
												cVector3f avSize,const cMatrixf& a_mtxTransform)
	{
		cParticleSystemData3D *pData = nullptr;

		tString sTypeName = cString::SetFileExt(cString::ToLowerCase(asType),"");


		pData = static_cast<cParticleSystemData3D*>(GetByName(sTypeName));
		if(pData == nullptr)
		{
			tString sFile = cString::SetFileExt(asType,"ps");

			tString sPath = mpFileSearcher->GetFilePath(sFile);

			if(sPath == "")
			{
				Error("Couldn't find particle system file '%s'\n",sFile.c_str());
				return nullptr;
			}

			cParticleSystemData3D *pPSData = hplNew( cParticleSystemData3D, (sTypeName,
																		mpResources,mpGraphics) );

			if(pPSData->LoadFromFile(sPath)==false)
			{
				Error("Can't load data from particle system file '%s'\n",sTypeName.c_str());
				hplDelete(pPSData);
				return nullptr;
			}

			AddData3D(pPSData);

			pData = pPSData;
		}


		pData->IncUserCount();
		cParticleSystem3D* pPS = pData->Create(asName,avSize,a_mtxTransform);
		pPS->SetDataName(asType);
		pPS->SetDataSize(avSize);
		pPS->SetParticleManager(this);

		return pPS;
	}

	void cParticleManager::AddData3D(cParticleSystemData3D *apData)
	{
		AddResource(apData);
	}

	void cParticleManager::Preload(const tString& asFile)
	{
		tString sTypeName = cString::SetFileExt(cString::ToLowerCase(asFile),"");

		cParticleSystemData3D *pData = static_cast<cParticleSystemData3D*>(GetByName(sTypeName));
		if(pData == nullptr)
		{
			tString sFile = cString::SetFileExt(asFile,"ps");
			tString sPath = mpFileSearcher->GetFilePath(sFile);
			if(sPath == "")
			{
				Error("Couldn't find particle system file '%s'\n",sFile.c_str());
				return;
			}

			cParticleSystemData3D *pPSData = hplNew( cParticleSystemData3D, (sTypeName,
																	mpResources,mpGraphics) );

			if(pPSData->LoadFromFile(sPath)==false)
			{
				Error("Can't load data from particle system file '%s'\n",sTypeName.c_str());
				hplDelete(pPSData);
				return;
			}

			AddData3D(pPSData);
		}
	}

	void cParticleManager::Unload(iResourceBase* apResource)
	{

	}

	void cParticleManager::Destroy(iResourceBase* apResource)
	{
		if(apResource->HasUsers())
		{
			apResource->DecUserCount();
		}
	}
}
