// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/ScriptManager.h"
#include "system/String.h"
#include "system/System.h"
#include "resources/Resources.h"
#include "system/Script.h"
#include "system/LowLevelSystem.h"



namespace hpl {

	cScriptManager::cScriptManager(cSystem* apSystem,cResources *apResources)
		: iResourceManager(apResources->GetFileSearcher(), apResources->GetLowLevel(),
							apResources->GetLowLevelSystem())
	{
		mpSystem = apSystem;
		mpResources = apResources;
	}

	cScriptManager::~cScriptManager()
	{
		DestroyAll();
		Log(" Done with scripts\n");
	}

	iScript* cScriptManager::CreateScript(const tString& asName)
	{
		tString sPath;
		iScript* pScript;
		tString asNewName;

		BeginLoad(asName);

		asNewName = cString::SetFileExt(asName,"hps");

		pScript = static_cast<iScript*>(this->FindLoadedResource(asNewName,sPath));

		if(pScript==nullptr && sPath!="")
		{
			pScript = mpSystem->GetLowLevel()->CreateScript(asNewName);

			if(pScript->CreateFromFile(sPath)==false){
				hplDelete(pScript);
				EndLoad();
				return nullptr;
			}

			AddResource(pScript);
		}

		if(pScript)pScript->IncUserCount();
		else Error("Couldn't create script '%s'\n",asNewName.c_str());

		EndLoad();
		return pScript;
	}

	iResourceBase* cScriptManager::Create(const tString& asName)
	{
		return CreateScript(asName);
	}

	void cScriptManager::Unload(iResourceBase* apResource)
	{

	}

	void cScriptManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}
}
