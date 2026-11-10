/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "resources/ParticleManager.h"

#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "resources/FileSearcher.h"
#include "graphics/Graphics.h"
#include "graphics/ParticleSystem3D.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cParticleManager::cParticleManager(cGraphics* apGraphics,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpGraphics = apGraphics;
		mpResources = apResources;
		mbUseOriginal = false;
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

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iResourceBase* cParticleManager::Create(const tString& asName)
	{
		return nullptr;
	}

	//-----------------------------------------------------------------------

	iParticleSystem2D* cParticleManager::CreatePS2D(const tString& asName, cVector3f avSize)
	{
		/*tParticleSystemDataMapIt it = m_mapData.find(asName);
		if(it == m_mapData.end()) return NULL;

		iParticleSystemData *pData = it->second;

		//Is name needed?..nahh
		iParticleSystem* pPart = pData->Create("",avSize);
		pPart->SetDataName(asName);
		pPart->SetDataSize(avSize);

		if(pPart->GetType() == eParticleSystemType_2D)
			return static_cast<iParticleSystem2D*>(pPart);
		else
		{
			hplDelete(pPart);
			return NULL;
		}*/

		return nullptr;
	}

	//-----------------------------------------------------------------------

	cParticleSystem3D* cParticleManager::CreatePS3D(const tString& asName,const tString& asType,
												cVector3f avSize,const cMatrixf& a_mtxTransform)
	{
		cParticleSystemData3D *pData = LoadData(asType);
		if(pData == nullptr) return nullptr;

		pData->IncUserCount();
		cParticleSystem3D* pPS = pData->Create(asName,avSize,a_mtxTransform,mbUseOriginal);
		if(pPS == nullptr)
		{
			pData->DecUserCount();
			return nullptr;
		}
		pPS->SetDataName(asType);
		pPS->SetDataSize(avSize);
		pPS->SetParticleManager(this);

		return pPS;
	}

	//-----------------------------------------------------------------------

	cParticleSystemData3D* cParticleManager::LoadData(const tString& asType)
	{
		tString sTypeName = cString::SetFileExt(cString::ToLowerCase(asType),"");

		cParticleSystemData3D *pData = static_cast<cParticleSystemData3D*>(GetByName(sTypeName));
		if(pData) return pData;

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

		//An addon replaces the system or its materials: the game's own as well,
		//to compare them
		tString sHidden = mpFileSearcher->GetHiddenFilePath(sFile);
		if(sHidden != "" || pPSData->UsesReplacedMaterial())
		{
			cParticleSystemData3D *pOriginal = hplNew( cParticleSystemData3D, (sTypeName,
																		mpResources,mpGraphics) );
			if(pOriginal->LoadFromFile(sHidden != "" ? sHidden : sPath, true))
				pPSData->SetOriginal(pOriginal);
			else
				hplDelete(pOriginal);
		}

		AddData3D(pPSData);
		return pPSData;
	}

	//-----------------------------------------------------------------------

	void cParticleManager::AddData3D(cParticleSystemData3D *apData)
	{
		AddResource(apData);
		//m.insert(tParticleSystemData3DMap::value_type(cString::ToLowerCase(apData->GetName()),
		//														apData));
	}

	//-----------------------------------------------------------------------

	void cParticleManager::Preload(const tString& asFile)
	{
		LoadData(asFile);
	}

	//-----------------------------------------------------------------------

	void cParticleManager::Unload(iResourceBase* apResource)
	{

	}
	//-----------------------------------------------------------------------

	void cParticleManager::Destroy(iResourceBase* apResource)
	{
		if(apResource->HasUsers())
		{
			apResource->DecUserCount();
		}
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------


	//-----------------------------------------------------------------------
}
