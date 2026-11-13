// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GFX_OBJECT_H
#define HPL_GFX_OBJECT_H

#include <vector>
#include "graphics/GraphicsTypes.h"

namespace hpl {

	class iMaterial;

	class cGfxObject
	{
	public:
		cGfxObject(iMaterial* apMat,const tString& asFile, bool abIsImage);
		~cGfxObject();

		iMaterial* GetMaterial() const{ return mpMat;}
		cVertex* GetVtxPtr(int alNum){ return &mvVtx[alNum];}
		tVertexVec* GetVertexVec(){ return &mvVtx;}

		const tString& GetSourceFile(){ return msSourceFile;}

		bool IsImage(){ return mbIsImage;}
		//The area drawn, as a fraction of the size the object is drawn at; wider
		//than 0,0,1,1 for an image with a margin.
		const cRect2f& GetDrawRect()const{ return mDrawRect;}

	private:
		tVertexVec mvVtx;
		cRect2f mDrawRect;
		iMaterial* mpMat;
		float mfZ;
		bool mbIsImage;

		tString msSourceFile;
	};

	typedef std::vector<cGfxObject> tGfxObjectVec;
	typedef tGfxObjectVec::iterator tGfxObjectVecIt;
};
#endif // HPL_GFX_OBJECT_H
