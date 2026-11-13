// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MESH_LOADER_H
#define HPL_MESH_LOADER_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	typedef tFlag tWorldLoadFlag;

	#define eWorldLoadFlag_NoLights			(0x00000001)
	#define eWorldLoadFlag_NoEntities		(0x00000002)
	#define eWorldLoadFlag_NoGameEntities	(0x00000004)

	typedef tFlag tMeshLoadFlag;

	#define eMeshLoadFlag_NoGeometry		(0x00000001)

	class cMesh;

	class cResources;
	class cMaterialManager;
	class cMeshManager;
	class cAnimationManager;
	class cMeshLoaderHandler;
	class iLowLevelGraphics;
	class cWorld3D;
	class cScene;
	class cAnimation;
	class cSystem;

	class iMeshLoader
	{
	friend class cMeshLoaderHandler;
	public:
		virtual ~iMeshLoader(){}
		iMeshLoader(iLowLevelGraphics *apLowLevelGraphics)
					: mpLowLevelGraphics(apLowLevelGraphics){}

		virtual cMesh* LoadMesh(const tString& asFile, tMeshLoadFlag aFlags)=0;
		virtual bool SaveMesh(cMesh* apMesh,const tString& asFile)=0;

		virtual cWorld3D* LoadWorld(const tString& asFile, cScene* apScene,tWorldLoadFlag aFlags)=0;

		virtual cAnimation* LoadAnimation(const tString& asFile)=0;

		virtual bool IsSupported(const tString asFileType)=0;

		virtual void AddSupportedTypes(tStringVec* avFileTypes)=0;

		static void SetRestrictStaticLightToSector(bool abX){ mbRestrictStaticLightToSector = abX;}
		static void SetUseFastMaterial(bool abX) {mbUseFastMaterial = abX;}
		static void SetFastMaterialFile(const tString& asFile) { msFastMaterialFile = asFile;}
		static void SetCacheDir(const tString& asDir) { msCacheDir = asDir;}
		// Where caches that had to be rebuilt are written, and looked for
		// first. Empty: next to the ones that came with the game.
		static void SetCacheWriteDir(const tString& asDir) { msCacheWriteDir = asDir;}
		static const tString& GetCacheWriteDir() { return msCacheWriteDir;}
	protected:
		cMaterialManager *mpMaterialManager;
		cMeshManager *mpMeshManager;
		cAnimationManager *mpAnimationManager;
		iLowLevelGraphics *mpLowLevelGraphics;
		cSystem *mpSystem;

		static bool mbRestrictStaticLightToSector;
		static bool mbUseFastMaterial;
		static tString msFastMaterialFile;
		static tString msCacheDir;
		static tString msCacheWriteDir;

	};

};
#endif // HPL_MESH_LOADER_H
