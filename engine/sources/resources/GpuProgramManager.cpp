// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/GpuProgramManager.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/GPUProgram.h"


namespace hpl {

	cGpuProgramManager::cGpuProgramManager(cFileSearcher *apFileSearcher,iLowLevelGraphics *apLowLevelGraphics,
		iLowLevelResources *apLowLevelResources,iLowLevelSystem *apLowLevelSystem)
		: iResourceManager(apFileSearcher, apLowLevelResources,apLowLevelSystem)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
	}

	cGpuProgramManager::~cGpuProgramManager()
	{
		DestroyAll();

		Log(" Done with Gpu programs\n");
	}

	/**
	 * Since further parameters are needed for the gpu prog this does not work...
	 * For now at least.
	 * \param asName
	 * \return
	 */
	iResourceBase* cGpuProgramManager::Create(const tString& asName)
	{
		return nullptr;
	}

	iGpuProgram* cGpuProgramManager::CreateProgram(const tString& asName, const tString& asDefines, const tString& asVertex)
	{
		const tString vertex = asVertex.empty() ? asName : asVertex;
		// The general resource manager folds keys to lowercase. Encode the
		// complete request so GLSL names and preprocessor tokens keep case.
		const tString request = vertex + ":" + asName + ":" + asDefines;
		const char* hex = "0123456789abcdef";
		tString key = asName + ":";
		for(unsigned char value : request){
			key += hex[value >> 4]; key += hex[value & 15];
		}
		iGpuProgram* program = static_cast<iGpuProgram*>(GetByName(key));
		if(!program){
			BeginLoad(asName);
			program = mpLowLevelGraphics->CreateGpuProgram(key);
			if(!program->CreateFromFile(vertex + ":" + asName, asDefines)){
				delete program;
				EndLoad();
				return nullptr;
			}
			AddResource(program);
			EndLoad();
		}
		program->IncUserCount();
		return program;
	}

	void cGpuProgramManager::Unload(iResourceBase* apResource)
	{

	}

	void cGpuProgramManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			delete apResource;
		}
	}
}
