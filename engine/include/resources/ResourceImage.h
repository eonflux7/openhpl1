// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_RESOURCE_IMAGE_H
#define HPL_RESOURCE_IMAGE_H

#include <vector>

#include "resources/ResourceBase.h"

#include "graphics/GraphicsTypes.h"
#include "math/MathTypes.h"

namespace hpl {

	class cFrameTexture;
	class cFrameBitmap;
	class iTexture;

	class cResourceImage : public iResourceBase
	{
	friend class cImageManager;
	public:
		cResourceImage(tString asName, cFrameTexture *apFrameTex,
						cFrameBitmap *apFrameBmp,
						cRect2l aRect,
						cVector2l avSrcSize, int alHandle);

		bool Reload() override;
		void Unload() override;
		void Destroy() override;

		//Image specific
		//The size the image is drawn at. A high-resolution replacement keeps the
		//size of the image it replaces while its texels cover more pixels.
		int GetHeight()const{return mvLogicalSize.y;}
		int GetWidth()const{return mvLogicalSize.x;}
		cVector2l GetSize()const{return mvLogicalSize;}
		//A replacement may also have a transparent margin around that size (in
		//its units), for edges and shadows that fade out past the original.
		void SetLogicalSize(const cVector2l &avSize, const cVector2l &avMargin=cVector2l(0,0));
		cVector2l GetMargin()const{return mvMargin;}
		//The area the texture covers when the image is drawn at avSize from 0,0
		cRect2f GetDrawRect(const cVector2f &avSize)const;
		cVector2l GetTexelSize()const{return cVector2l(mRect.w,mRect.h);}
		cVector2l GetPosition()const{return cVector2l(mRect.x,mRect.y);}

		int GetSourceWidth()const{return mvSourceSize.x;}
		int GetSourceHeight()const{return mvSourceSize.y;}

		iTexture *GetTexture()const;

		cFrameTexture *GetFrameTexture()const{return mpFrameTexture;}
		cFrameBitmap *GetFrameBitmap()const{return mpFrameBitmap;}

		tVertexVec GetVertexVecCopy(const cVector2f &avPos, const cVector2f &avSize);
		const tVertexVec& GetVertexVec(){return mvVtx;}

	private:
		~cResourceImage() override;

		cFrameTexture *mpFrameTexture;
		cFrameBitmap *mpFrameBitmap;

		cVector2l mvSourceSize;
		cRect2l mRect;
		cVector2l mvLogicalSize;
		cVector2l mvMargin;
		tVertexVec mvVtx;

		int mlHandle;
	};

	typedef std::vector<cResourceImage*> tResourceImageVec;

};

#endif // HPL_RESOURCE_IMAGE_H
