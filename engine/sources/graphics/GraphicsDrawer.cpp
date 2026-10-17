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
#include "graphics/GraphicsDrawer.h"

#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "resources/ResourceImage.h"
#include "resources/FrameBitmap.h"
#include "graphics/GfxObject.h"
#include "graphics/MaterialHandler.h"
#include "graphics/Material.h"
#include "resources/TextureManager.h"


#include "math/Math.h"

#include "resources/Resources.h"
#include "resources/ImageManager.h"

namespace hpl {


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cGraphicsDrawer::cGraphicsDrawer(iLowLevelGraphics *apLowLevelGraphics, cMaterialHandler* apMaterialHandler,
		cResources* apResources)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		mpMaterialHandler = apMaterialHandler;
		mpResources = apResources;
	}

	//-----------------------------------------------------------------------

	cGraphicsDrawer::~cGraphicsDrawer()
	{
		STLDeleteAll(mlstGfxObjects);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iMaterial* cGfxBufferObject::GetMaterial() const
	{
		return mpObject->GetMaterial();
	}

	//-----------------------------------------------------------------------

	bool cGfxBufferCompare::operator()(const cGfxBufferObject& aObjectA,const cGfxBufferObject& aObjectB) const
	{
		// Equivalent keys retain submission order; texture addresses must not
		// decide which of two overlapping translucent sprites covers the other.
		return aObjectA.GetZ() < aObjectB.GetZ();
	}

	//-----------------------------------------------------------------------

	static void FlushImage(cGfxObject* apObject)
	{
		if(apObject->IsImage())
		{
			cResourceImage *pImage = apObject->GetMaterial()->GetImage(eMaterialTexture_Diffuse);
			pImage->GetFrameBitmap()->FlushToTexture();
		}
	}

	void cGraphicsDrawer::DrawGfxObject(cGfxObject* apObject, const cVector3f& avPos,
										const cVector2f& avSize, const cColor& aColor,
										bool abFlipH, bool abFlipV, float afAngle)
	{
		FlushImage(apObject);

		cGfxBufferObject BuffObj;
		BuffObj.mpObject = apObject;
		BuffObj.mvTransform = avPos;
		BuffObj.mvSize= avSize;
		BuffObj.mColor = aColor;
		BuffObj.mbFlipH = abFlipH;
		BuffObj.mbFlipV = abFlipV;
		BuffObj.mfAngle = afAngle;

		BuffObj.mbIsColorAndSize = true;

		m_setGfxBuffer.insert(BuffObj);
	}

	//-----------------------------------------------------------------------

	void cGraphicsDrawer::DrawGfxObject(cGfxObject* apObject, const cVector3f& avPos)
	{
		FlushImage(apObject);

		cGfxBufferObject BuffObj;
		BuffObj.mpObject = apObject;
		BuffObj.mvTransform = avPos;
		BuffObj.mbIsColorAndSize = false;

		m_setGfxBuffer.insert(BuffObj);
	}
	//-----------------------------------------------------------------------

	void cGraphicsDrawer::DrawAll()
	{
		if(mbSkipDrawing)
		{
			m_setGfxBuffer.clear();
			return;
		}

		//Set all states
		mpLowLevelGraphics->SetDepthTestActive(false);
		mpLowLevelGraphics->SetDrawMatrices(cMatrixf::Identity, cMatrixf::Identity,
			mpLowLevelGraphics->CreateVirtualOrthoProjection(-1000, 1000));

		cSpriteBatch batch;
		for(const cGfxBufferObject& object : m_setGfxBuffer)
		{
			std::array<cSpriteVertex,4> corners;
			for(int i=0; i<4; ++i) corners[i] = cSpriteVertex(*object.mpObject->GetVtxPtr(i));
			if(object.mbIsColorAndSize)
			{
				const float w = object.mvSize.x * 0.5f, h = object.mvSize.y * 0.5f;
				const cMatrixf translate = cMath::MatrixTranslate(object.mvTransform + cVector3f(w,h,0));
				const cMatrixf rotate = cMath::MatrixRotateZ(object.mfAngle);
				const cVector3f positions[] = {{-w,-h,0}, {w,-h,0}, {w,h,0}, {-w,h,0}};
				for(int i=0; i<4; ++i)
				{
					cVector3f pos = positions[i];
					if(object.mfAngle != 0) pos = cMath::MatrixMul(rotate, pos);
					corners[i].pos = cMath::MatrixMul(translate, pos);
					corners[i].col = object.mColor;
				}
				// Preserve the existing sized-draw geometry, including its flip-H offset.
				if(object.mbFlipH) corners[3].pos += cVector3f(object.mvSize.x,object.mvSize.y,0);
			}
			else
			{
				for(auto& vertex : corners) vertex.pos += object.mvTransform;
			}
			iMaterial* material = object.GetMaterial();
			const auto blend = material->GetSpriteBlend();
			if(blend) batch.Add(material->GetTexture(eMaterialTexture_Diffuse), *blend, corners);
		}
		batch.Flush(mpLowLevelGraphics);

		//Clear the buffer of objects.
		m_setGfxBuffer.clear();

		//Reset all states
		mpLowLevelGraphics->SetDepthTestActive(true);
	}

	//-----------------------------------------------------------------------

	cGfxObject* cGraphicsDrawer::CreateGfxObject(const tString &asFileName, const tString &asMaterialName,
												bool abAddToList)
	{
		cResourceImage* pImage = mpResources->GetImageManager()->CreateImage(asFileName);
		if(pImage==nullptr){
			FatalError("Couldn't load image '%s'!\n", asFileName.c_str());
			return nullptr;
		}

		iMaterial* pMat = mpMaterialHandler->Create(asMaterialName, eMaterialPicture_Image);
		if(pMat==nullptr){
			FatalError("Couldn't create material '%s'!\n", asMaterialName.c_str());
			return nullptr;
		}

		pMat->SetImage(pImage, eMaterialTexture_Diffuse);

		cGfxObject* pObject = hplNew( cGfxObject,(pMat,asFileName,true));

		if(abAddToList) mlstGfxObjects.push_back(pObject);

		return pObject;
	}

	//-----------------------------------------------------------------------

	cGfxObject* cGraphicsDrawer::CreateGfxObject(iBitmap2D *apBmp, const tString &asMaterialName,
												bool abAddToList)
	{
		cResourceImage* pImage = mpResources->GetImageManager()->CreateFromBitmap("",apBmp);
		if(pImage==nullptr){
			FatalError("Couldn't create image\n");
			return nullptr;
		}

		iMaterial* pMat = mpMaterialHandler->Create(asMaterialName, eMaterialPicture_Image);
		if(pMat==nullptr){
			FatalError("Couldn't create material '%s'!\n", asMaterialName.c_str());
			return nullptr;
		}

		pMat->SetImage(pImage, eMaterialTexture_Diffuse);

		cGfxObject* pObject = hplNew( cGfxObject,(pMat,"",true));

		if(abAddToList) mlstGfxObjects.push_back(pObject);

		return pObject;
	}

	//-----------------------------------------------------------------------

	cGfxObject* cGraphicsDrawer::CreateGfxObjectFromTexture(	const tString &asFileName, const tString &asMaterialName,
																bool abAddToList)
	{
		iTexture *pTex = mpResources->GetTextureManager()->Create2D(asFileName,false);
		if(pTex==nullptr)
		{
			FatalError("Couldn't create texture '%s'!\n", asFileName.c_str());
			return nullptr;
		}

		iMaterial* pMat = mpMaterialHandler->Create(asMaterialName, eMaterialPicture_Texture);
		if(pMat==nullptr){
			FatalError("Couldn't create material '%s'!\n", asMaterialName.c_str());
			return nullptr;
		}

		pMat->SetTexture(pTex, eMaterialTexture_Diffuse);

		cGfxObject* pObject = hplNew( cGfxObject,(pMat,asFileName,false));

		if(abAddToList) mlstGfxObjects.push_back(pObject);

		return pObject;
	}

	//-----------------------------------------------------------------------

	cGfxObject* cGraphicsDrawer::CreateGfxObject(iTexture *apTexture, const cVector2f &avUVStart,
												const cVector2f &avUVEnd, const tString &asMaterialName)
	{
		iMaterial* pMat = mpMaterialHandler->Create(asMaterialName, eMaterialPicture_Texture);
		if(pMat==nullptr){
			FatalError("Couldn't create material '%s'!\n", asMaterialName.c_str());
			return nullptr;
		}

		pMat->SetTexture(apTexture, eMaterialTexture_Diffuse);

		cGfxObject* pObject = hplNew( cGfxObject,(pMat,"",false));

		pObject->GetVtxPtr(0)->tex = cVector3f(avUVStart.x,avUVStart.y,0);
		pObject->GetVtxPtr(1)->tex = cVector3f(avUVEnd.x,avUVStart.y,0);
		pObject->GetVtxPtr(2)->tex = cVector3f(avUVEnd.x,avUVEnd.y,0);
		pObject->GetVtxPtr(3)->tex = cVector3f(avUVStart.x,avUVEnd.y,0);

		return pObject;
	}

	//-----------------------------------------------------------------------

	void cGraphicsDrawer::DestroyGfxObject(cGfxObject* apObject)
	{
		STLFindAndDelete(mlstGfxObjects,apObject);
	}

	//-----------------------------------------------------------------------

}
