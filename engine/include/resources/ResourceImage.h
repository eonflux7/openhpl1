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
	typedef tResourceImageVec::iterator tResourceImageVecIt;

};

#endif // HPL_RESOURCE_IMAGE_H
