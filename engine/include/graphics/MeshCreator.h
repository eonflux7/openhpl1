// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MESHCREATOR_H
#define HPL_MESHCREATOR_H

#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "graphics/GraphicsTypes.h"

namespace hpl {

	class iVertexBuffer;
	class iLowLevelGraphics;
	class cResources;
	class cMesh;

	class cMeshCreator
	{
	public:
		cMeshCreator(iLowLevelGraphics *apLowLevelGraphics, cResources *apResources);
		~cMeshCreator();


		cMesh* CreateBox(const tString &asName, cVector3f avSize, const tString &asMaterial);

		iVertexBuffer* CreateSkyBoxVertexBuffer(float afSize);
		iVertexBuffer* CreateBoxVertexBuffer(cVector3f avSize);

	private:
		cVector3f GetBoxTex(int i,int x, int y, int z, cVector3f *vAdd);
		int GetBoxIdx(int i,int x, int y, int z);

		iLowLevelGraphics *mpLowLevelGraphics;
		cResources *mpResources;
	};

};
#endif // HPL_MESHCREATOR_H
