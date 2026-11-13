// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MESH_MANAGER_H
#define HPL_MESH_MANAGER_H

#include "resources/ResourceManager.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class cMesh;

	class cMeshManager : public iResourceManager
	{
	public:
		cMeshManager(cGraphics* apGraphics,cResources *apResources);
		~cMeshManager() override;

		iResourceBase* Create(const tString& asName) override;
		cMesh* CreateMesh(const tString& asName);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

	private:
		cGraphics* mpGraphics;
		cResources* mpResources;

	};

};
#endif // HPL_MESH_MANAGER_H
