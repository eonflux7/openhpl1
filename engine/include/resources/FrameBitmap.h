// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_FRAMEBITMAP_H
#define HPL_FRAMEBITMAP_H

#include "resources/FrameBase.h"
#include "system/BinTree.h"
#include "math/MathTypes.h"

namespace hpl {

	class cFrameTexture;
	class cResourceImage;
	class iBitmap2D;

	//The frames bitmap + rect class
	class cFBitmapRect
	{
	public:
		cFBitmapRect(){mlHandle=-1;}
		cFBitmapRect(int x,int y,int w,int h, int alHandle){
			mRect = cRect2l(x,y,w,h);
			mlHandle = alHandle;
		}

		cRect2l mRect;
		int mlHandle;
	};

	typedef BinTree<cFBitmapRect> tRectTree;
	typedef BinTreeNode<cFBitmapRect> tRectTreeNode;
	typedef std::list<tRectTreeNode*> tRectTreeNodeList;
	typedef tRectTreeNodeList::iterator tRectTreeNodeListIt;

	class cFrameBitmap : public iFrameBase
	{
	public:
		cFrameBitmap(iBitmap2D *apBitmap, cFrameTexture *apFrmTex, int alHandle);
		~cFrameBitmap();

		cResourceImage * AddBitmap(iBitmap2D *apSrc);
		bool MinimumFit(cRect2l aSrc,cRect2l aDest);
		bool IsFull();
		bool IsUpdated();

		bool IsLocked()const { return mbIsLocked;}
		void SetLocked(bool abX){ mbIsLocked = abX; }

		bool FlushToTexture();

		cFrameTexture* GetFrameTexture(){ return mpFrameTexture;}

		int GetHandle()const{ return mlHandle; }
	private:
		iBitmap2D* mpBitmap;
		cFrameTexture* mpFrameTexture;
		tRectTree mRects;
		int mlMinHole;
		int mlHandle;
		bool mbIsFull;
		bool mbIsUpdated;
		bool mbIsLocked;
	};

};
#endif // HPL_FRAMEBITMAP_H
