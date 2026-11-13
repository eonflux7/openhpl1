// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MESH_LOADER_MSH_H
#define HPL_MESH_LOADER_MSH_H



#include "math/MathTypes.h"
#include "resources/MeshLoader.h"
#include "graphics/VertexBuffer.h"

class TiXmlElement;

namespace hpl {

	class cMesh;
	class cNode3D;
	class iVertexBuffer;

	class cMeshLoaderMSH : public iMeshLoader
	{
	public:
		cMeshLoaderMSH(iLowLevelGraphics *apLowLevelGraphics);
		~cMeshLoaderMSH() override;

		cMesh* LoadMesh(const tString& asFile, tMeshLoadFlag aFlags) override;
		bool SaveMesh(cMesh* apMesh,const tString& asFile) override;

		cWorld3D* LoadWorld(const tString& asFile, cScene* apScene,tWorldLoadFlag aFlags) override;

		cAnimation* LoadAnimation(const tString& asFile) override{ return nullptr;}

		bool IsSupported(const tString asFileType) override;

		void AddSupportedTypes(tStringVec* avFileTypes) override;


	private:
		//Saving
		void SaveFloatData(TiXmlElement* apRoot, int alSize, const char *asName, float* apData);
		void SaveIntData(TiXmlElement* apRoot, int alSize, unsigned int* apData);

		//Loading
		void FillVtxArray(float *apArray, const char* apString,int alSize);
		void FillIdxArray(unsigned int *apArray, const char* apString,int alSize);

		//Common
		const char* GetVertexName(tVertexFlag aFlag);


	};

};
#endif // HPL_MESH_LOADER_MSH_H
