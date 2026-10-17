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
#include "resources/GpuProgramManager.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/GPUProgram.h"


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------


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
				hplDelete(program);
				EndLoad();
				return nullptr;
			}
			AddResource(program);
			EndLoad();
		}
		program->IncUserCount();
		return program;
	}

	//-----------------------------------------------------------------------

	void cGpuProgramManager::Unload(iResourceBase* apResource)
	{

	}
	//-----------------------------------------------------------------------

	void cGpuProgramManager::Destroy(iResourceBase* apResource)
	{
		apResource->DecUserCount();

		if(apResource->HasUsers()==false){
			RemoveResource(apResource);
			hplDelete(apResource);
		}
	}

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------


	//-----------------------------------------------------------------------
}
