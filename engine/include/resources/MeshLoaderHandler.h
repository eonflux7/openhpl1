// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MESH_LOADER_HANDLER_H
#define HPL_MESH_LOADER_HANDLER_H

#include <list>

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

#include "resources/MeshLoader.h"

namespace hpl {

	class cMesh;
	class iMeshLoader;
	class cResources;
	class cWorld3D;
	class cScene;
	class cAnimation;

	typedef std::list<iMeshLoader*> tMeshLoaderList;
	typedef tMeshLoaderList::iterator tMeshLoaderListIt;

	class cMeshLoaderHandler
	{
	public:
		cMeshLoaderHandler(cResources* apResources, cScene *apScene);
		~cMeshLoaderHandler();

		cMesh* LoadMesh(const tString& asFile,tMeshLoadFlag aFlags);
		bool SaveMesh(cMesh* apMesh,const tString& asFile);

		cWorld3D* LoadWorld(const tString& asFile, tWorldLoadFlag aFlags);

		cAnimation *LoadAnimation(const tString& asFile);

		void AddLoader(iMeshLoader *apLoader);

		tStringVec* GetSupportedTypes(){ return &mvSupportedTypes;}

	private:
		tStringVec mvSupportedTypes;

		tMeshLoaderList mlstLoaders;

		cResources* mpResources;
		cScene* mpScene;
	};

};
#endif // HPL_MESH_LOADER_HANDLER_H
