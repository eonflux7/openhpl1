// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/ResourceImage.h"
#include "resources/FrameTexture.h"
#include "graphics/Texture.h"


namespace hpl {

#define kContractSize (0.001f)

	cResourceImage::cResourceImage(tString asName,cFrameTexture *apFrameTex,
		cFrameBitmap *apFrameBmp, cRect2l aRect,
		cVector2l avSrcSize, int alHandle) : iResourceBase(asName,0)
	{
		mpFrameTexture = apFrameTex;
		mpFrameBitmap = apFrameBmp;
		mRect = aRect;
		mvLogicalSize = cVector2l(aRect.w,aRect.h);
		mvMargin = cVector2l(0,0);
		mvSourceSize = avSrcSize;
		mlHandle = alHandle;

		cVector2f vTexSize = cVector2f((float)mRect.w,(float)mRect.h ) /
								cVector2f((float)mvSourceSize.x,(float)mvSourceSize.y);
		cVector2f vTexPos = cVector2f((float)mRect.x,(float)mRect.y ) /
								cVector2f((float)mvSourceSize.x,(float)mvSourceSize.y);

		mvVtx.push_back(cVertex(cVector3f(0,0,0),
						cVector3f(vTexPos.x+kContractSize, vTexPos.y+kContractSize,0), cColor(1)));

		mvVtx.push_back(cVertex(cVector3f((float)mRect.w,0,0),
						cVector3f(vTexPos.x+vTexSize.x-kContractSize, vTexPos.y+kContractSize,0),
						cColor(1)));

		mvVtx.push_back(cVertex(cVector3f((float)mRect.w,(float)mRect.h,0),
						cVector3f(vTexPos.x+vTexSize.x-kContractSize, vTexPos.y+vTexSize.y-kContractSize,0),
						cColor(1)));

		mvVtx.push_back(cVertex(cVector3f(0,(float)mRect.h,0),
						cVector3f(vTexPos.x+kContractSize, vTexPos.y+vTexSize.y-kContractSize,0),
						cColor(1)));
	}

	cResourceImage::~cResourceImage()
	{
		mvVtx.clear();
		mpFrameTexture = nullptr;
		mlHandle = -1;
	}

	void cResourceImage::SetLogicalSize(const cVector2l &avSize, const cVector2l &avMargin)
	{
		mvLogicalSize = avSize;
		mvMargin = avMargin;

		cRect2f rect = GetDrawRect(cVector2f((float)avSize.x,(float)avSize.y));
		mvVtx[0].pos = cVector3f(rect.x,rect.y,0);
		mvVtx[1].pos = cVector3f(rect.x+rect.w,rect.y,0);
		mvVtx[2].pos = cVector3f(rect.x+rect.w,rect.y+rect.h,0);
		mvVtx[3].pos = cVector3f(rect.x,rect.y+rect.h,0);
	}

	cRect2f cResourceImage::GetDrawRect(const cVector2f &avSize)const
	{
		cVector2f vMargin(avSize.x * (float)mvMargin.x / (float)mvLogicalSize.x,
						avSize.y * (float)mvMargin.y / (float)mvLogicalSize.y);
		return cRect2f(-vMargin.x, -vMargin.y, avSize.x + vMargin.x*2, avSize.y + vMargin.y*2);
	}

	iTexture *cResourceImage::GetTexture()const{return mpFrameTexture->GetTexture();}

	tVertexVec cResourceImage::GetVertexVecCopy(const cVector2f &avPos, const cVector2f &avSize)
	{
		tVertexVec vTmpVtx = mvVtx;

		cRect2f rect = GetDrawRect(avSize == cVector2f(-1,-1) ?
			cVector2f((float)mvLogicalSize.x,(float)mvLogicalSize.y) : avSize);
		vTmpVtx[0].pos = cVector3f(rect.x,rect.y,0);
		vTmpVtx[1].pos = cVector3f(rect.x+rect.w,rect.y,0);
		vTmpVtx[2].pos = cVector3f(rect.x+rect.w,rect.y+rect.h,0);
		vTmpVtx[3].pos = cVector3f(rect.x,rect.y+rect.h,0);

		for(int i=0;i<4;i++)
			vTmpVtx[i].pos+=avPos;

		return vTmpVtx;
	}

	bool cResourceImage::Reload()
	{
		return false;
	}

	void cResourceImage::Unload()
	{
	}

	void cResourceImage::Destroy()
	{
	}
}
