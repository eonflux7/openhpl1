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
