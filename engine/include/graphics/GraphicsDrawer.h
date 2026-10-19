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
#ifndef HPL_GRAPHICSDRAWER_H
#define HPL_GRAPHICSDRAWER_H

#include <set>
#include <vector>
#include "graphics/GraphicsTypes.h"

namespace hpl {

	class iLowLevelGraphics;
	class cResourceImage;
	class cGfxObject;
	class cMaterialHandler;
	class iMaterial;
	class iTexture;
	class iBitmap2D;

	class cGfxBufferObject
	{
	public:
		cGfxObject* mpObject;
		cVector3f mvTransform;

		bool mbIsColorAndSize;
		cColor mColor;
		cVector2f mvSize;
		bool mbFlipH = false;
		bool mbFlipV = false;
		float mfAngle = 0;
		//The part of the object drawn, as fractions of it
		cRect2f mPart = cRect2f(0,0,1,1);

		iMaterial* GetMaterial() const;
		float GetZ() const { return mvTransform.z;}
	};

	typedef std::vector<cGfxObject> tGfxObjectVec;
	typedef tGfxObjectVec::iterator tGfxObjectVecIt;

	class cGfxBufferCompare
	{
	public:
		bool operator()(const cGfxBufferObject& aObjectA,const cGfxBufferObject& aObjectB) const;
	};

	typedef std::multiset<cGfxBufferObject,cGfxBufferCompare> tGfxBufferSet;
	typedef tGfxBufferSet::iterator tGfxBufferSetIt;

	class cResources;

	typedef std::list<cGfxObject*> tGfxObjectList;
	typedef tGfxObjectList::iterator tGfxObjectListIt;

	class cGraphicsDrawer
	{
	public:
		cGraphicsDrawer(iLowLevelGraphics *apLowLevelGraphics,cMaterialHandler* apMaterialHandler,
						cResources* apResources);
		~cGraphicsDrawer();

		/**
		 * Draw Gfx object during next DrawAll call.
		 * \param apObject
		 * \param avPos
		 */
		void DrawGfxObject(cGfxObject* apObject, const cVector3f& avPos);

		/**
		 * Draw Gfx object during next DrawAll call.
		 * \param apObject
		 * \param avPos
		 * \param avSize Size of object
		 * \param aColor color to use
		 * \param abFlipH Flip image horisontally
		 * \param abFlipV Flip image verically
		 */
		void DrawGfxObject(cGfxObject* apObject, const cVector3f& avPos,
											const cVector2f& avSize, const cColor& aColor,
											bool abFlipH=false, bool abFlipV=false, float afAngle = 0);

		/**
		 * Draws only a part of a Gfx object, placed where it is when the whole
		 * object is drawn at avPos and avSize (e.g. to reveal it bit by bit).
		 * \param aPart The part, as fractions (0..1) of the object's width and height
		 */
		void DrawGfxObjectPart(cGfxObject* apObject, const cVector3f& avPos,
								const cVector2f& avSize, const cColor& aColor, const cRect2f& aPart);


		/**
		 * Draw all gfx obejcts, Called after world is rendered by cScene.
		 */
		void DrawAll();

		/**
		 * DrawAll() throws away what has been queued instead of drawing it.
		 */
		void SetSkipDrawing(bool abX){ mbSkipDrawing = abX;}
		bool GetSkipDrawing() const { return mbSkipDrawing; }

		/**
		 * Create Gfx object from file
		 * \param &asFileName Filename of image
		 * \param &asMaterialName material to use
		 * \param abAddToList if the engine should delete object at exit, this means DestroyGfxObject must be used. Should almost always be true.
		 * \return
		 */
		cGfxObject* CreateGfxObject(const tString &asFileName, const tString &asMaterialName,
									bool abAddToList=true);
		/**
		 * Create gfx object from Bitmap
		 * \param *apBmp bitmap
		 * \param &asMaterialName material to use
		 * \param abAddToList if the engine should delete object at exit, this means DestroyGfxObject must be used. Should almost always be true.
		 * \return
		 */
		cGfxObject* CreateGfxObject(iBitmap2D *apBmp, const tString &asMaterialName,
									bool abAddToList=true);


		cGfxObject* CreateGfxObjectFromTexture(const tString &asFileName, const tString &asMaterialName,
												bool abAddToList=true);
		/**
		 * Create gfx object showing a part of a texture that the caller owns.
		 * The object is not added to the list. Before deleting it, the texture
		 * must be taken out of its material (SetTexture(nullptr,...)), or the
		 * material hands it to the texture manager to destroy.
		 * \param *apTexture
		 * \param avUVStart texture coordinate of the upper left corner
		 * \param avUVEnd texture coordinate of the lower right corner
		 * \param &asMaterialName material to use
		 */
		cGfxObject* CreateGfxObject(iTexture *apTexture, const cVector2f &avUVStart,
									const cVector2f &avUVEnd, const tString &asMaterialName);
		/**
		 * Destroys a gfx object.
		 */
		void DestroyGfxObject(cGfxObject* apObject);

	private:
		bool mbSkipDrawing = false;
		iLowLevelGraphics *mpLowLevelGraphics;
		cMaterialHandler* mpMaterialHandler;
		cResources *mpResources;

		tGfxBufferSet m_setGfxBuffer;

		tGfxObjectList mlstGfxObjects;
	};

};
#endif // HPL_GRAPHICSDRAWER_H
