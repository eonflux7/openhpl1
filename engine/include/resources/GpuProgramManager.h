// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GPU_PROGRAM_MANAGER_H
#define HPL_GPU_PROGRAM_MANAGER_H

#include "resources/ResourceManager.h"

#include "graphics/GPUProgram.h"

namespace hpl {

	class iLowLevelGraphics;

	class cGpuProgramManager : public iResourceManager
	{
	public:
		cGpuProgramManager(cFileSearcher *apFileSearcher, iLowLevelGraphics *apLowLevelGraphics,
			iLowLevelResources *apLowLevelResources,iLowLevelSystem *apLowLevelSystem);
		~cGpuProgramManager() override;

		iResourceBase* Create(const tString& asName) override;
		// GLSL base name and space-separated defines for a linked pair.
		// asVertex selects a different vertex file; otherwise both stages
		// use asName. The cache includes both filenames and the defines.
		iGpuProgram* CreateProgram(const tString& asName, const tString& asDefines="", const tString& asVertex="");

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

	private:
		iLowLevelGraphics *mpLowLevelGraphics;
	};

};
#endif // HPL_GPU_PROGRAM_MANAGER_H
