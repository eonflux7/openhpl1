// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/GfxObject.h"

#include "graphics/Material.h"
#include "resources/ResourceImage.h"


namespace hpl {

	cGfxObject::cGfxObject(iMaterial* apMat,const tString& asFile, bool abIsImage)
	{
		mpMat = apMat;

		msSourceFile = asFile;

		mbIsImage = abIsImage;
		mDrawRect = cRect2f(0,0,1,1);

		if(mbIsImage)
		{
			cResourceImage *pImage = apMat->GetImage(eMaterialTexture_Diffuse);
			mvVtx = pImage->GetVertexVecCopy(0,-1);
			mDrawRect = pImage->GetDrawRect(cVector2f(1,1));
		}
		else
		{
			mvVtx.push_back(cVertex(cVector3f(0,0,0),cVector2f(0,0),cColor(1,1)) );
			mvVtx.push_back(cVertex(cVector3f(1,0,0),cVector2f(1,0),cColor(1,1)) );
			mvVtx.push_back(cVertex(cVector3f(1,1,0),cVector2f(1,1),cColor(1,1)) );
			mvVtx.push_back(cVertex(cVector3f(0,1,0),cVector2f(0,1),cColor(1,1)) );
		}
	}

	cGfxObject::~cGfxObject()
	{
		delete mpMat;
	}
}
