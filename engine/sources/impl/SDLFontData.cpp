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
#include "impl/SDLFontData.h"
#include "graphics/GraphicsDrawer.h"
#include "system/LowLevelSystem.h"
#include "impl/SDLBitmap2D.h"
#include "impl/SDLTexture.h"
#include "graphics/GfxObject.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/Material.h"
#include "math/Math.h"
#include "tinyxml.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include FT_GLYPH_H
#include FT_STROKER_H

#include <glad/gl.h>

#include <math.h>
#include <vector>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// VECTOR FONT ATLAS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	static FT_Library gpFTLibrary = nullptr;
	static int glVectorFontCount = 0;

	// The rendered glyphs of all vector fonts share these textures. They are
	// filled row by row and freed when the last vector font is destroyed.
	static const int glAtlasSize = 1024;

	class cFontAtlasPage
	{
	public:
		iTexture *mpTexture;
		int mlX;
		int mlY;
		int mlRowHeight;
	};

	static std::vector<cFontAtlasPage> gvAtlasPages;

	//-----------------------------------------------------------------------

	// Glyphs are rendered while the frame is being drawn, so the texture
	// state the engine keeps track of must be left as it was.
	class cFontTextureStateKeeper
	{
	public:
		cFontTextureStateKeeper()
		{
			glGetIntegerv(GL_ACTIVE_TEXTURE, &mlActiveUnit);
			glActiveTexture(GL_TEXTURE0);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &mlBinding);
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &mlAlignment);
		}
		~cFontTextureStateKeeper()
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, mlAlignment);
			glBindTexture(GL_TEXTURE_2D, (GLuint)mlBinding);
			glActiveTexture((GLenum)mlActiveUnit);
		}
	private:
		GLint mlActiveUnit;
		GLint mlBinding;
		GLint mlAlignment;
	};

	//-----------------------------------------------------------------------

	/**
	 * Puts a bitmap of coverage values in the atlas, as white with the
	 * coverage as alpha, with at least one transparent texel around it.
	 */
	static bool AddToFontAtlas(iLowLevelGraphics *apLowLevelGraphics, int alW, int alH,
								const unsigned char *apCoverage,
								iTexture **apTexture, cVector2l *apPos)
	{
		if(alW+2 > glAtlasSize || alH+2 > glAtlasSize) return false;

		cFontTextureStateKeeper stateKeeper;

		cFontAtlasPage *pPage = gvAtlasPages.empty() ? nullptr : &gvAtlasPages.back();

		//Next row
		if(pPage && pPage->mlX + alW + 1 > glAtlasSize)
		{
			pPage->mlX = 1;
			pPage->mlY += pPage->mlRowHeight + 1;
			pPage->mlRowHeight = 0;
		}

		//Next page
		if(pPage==nullptr || pPage->mlY + alH + 1 > glAtlasSize)
		{
			iTexture *pTexture = apLowLevelGraphics->CreateTexture(false,eTextureType_Normal,eTextureTarget_2D);

			std::vector<unsigned char> vPixels((size_t)glAtlasSize*glAtlasSize*4, 255);
			for(size_t i=3; i<vPixels.size(); i+=4) vPixels[i] = 0;

			if(pTexture->CreateFromArray(vPixels.data(),4,cVector3l(glAtlasSize,glAtlasSize,1))==false)
			{
				hplDelete(pTexture);
				return false;
			}

			cFontAtlasPage page;
			page.mpTexture = pTexture;
			page.mlX = 1;
			page.mlY = 1;
			page.mlRowHeight = 0;
			gvAtlasPages.push_back(page);

			pPage = &gvAtlasPages.back();
		}

		std::vector<unsigned char> vPixels((size_t)alW*alH*4, 255);
		for(int i=0; i<alW*alH; i++) vPixels[(size_t)i*4+3] = apCoverage[i];

		cSDLTexture *pSDLTexture = static_cast<cSDLTexture*>(pPage->mpTexture);
		glBindTexture(GL_TEXTURE_2D, pSDLTexture->GetTextureHandle());
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTexSubImage2D(GL_TEXTURE_2D,0,pPage->mlX,pPage->mlY,alW,alH,
						GL_RGBA,GL_UNSIGNED_BYTE,vPixels.data());

		*apTexture = pPage->mpTexture;
		*apPos = cVector2l(pPage->mlX,pPage->mlY);

		pPage->mlX += alW + 1;
		if(alH > pPage->mlRowHeight) pPage->mlRowHeight = alH;

		return true;
	}

	//-----------------------------------------------------------------------

	static void BoxBlurRows(std::vector<float> &avSrc, std::vector<float> &avDest,
							int alW, int alH, int alRadius)
	{
		float fMul = 1.0f / (float)(alRadius*2 + 1);
		for(int y=0; y<alH; y++)
		{
			const float *pSrc = &avSrc[(size_t)y*alW];
			float *pDest = &avDest[(size_t)y*alW];

			float fSum =0;
			for(int x=0; x<=alRadius && x<alW; x++) fSum += pSrc[x];

			for(int x=0; x<alW; x++)
			{
				pDest[x] = fSum * fMul;
				if(x+alRadius+1 < alW) fSum += pSrc[x+alRadius+1];
				if(x-alRadius >= 0) fSum -= pSrc[x-alRadius];
			}
		}
	}

	static void Transpose(std::vector<float> &avSrc, std::vector<float> &avDest, int alW, int alH)
	{
		for(int y=0; y<alH; y++)
			for(int x=0; x<alW; x++)
				avDest[(size_t)x*alH + y] = avSrc[(size_t)y*alW + x];
	}

	/**
	 * Blurs coverage values in place, close to a gaussian (three box blurs).
	 * The bitmap needs alRadius*3 empty texels on each side.
	 */
	static void BlurCoverage(std::vector<unsigned char> &avCoverage, int alW, int alH,
							int alRadius, float afGain)
	{
		std::vector<float> vA(avCoverage.begin(), avCoverage.end());
		std::vector<float> vB(vA.size());

		for(int i=0;i<3;i++){
			BoxBlurRows(vA,vB,alW,alH,alRadius);
			vA.swap(vB);
		}
		Transpose(vA,vB,alW,alH);
		for(int i=0;i<3;i++){
			BoxBlurRows(vB,vA,alH,alW,alRadius);
			vB.swap(vA);
		}
		Transpose(vB,vA,alH,alW);

		for(size_t i=0;i<vA.size();i++)
		{
			float fVal = vA[i] * afGain;
			avCoverage[i] = fVal > 255.0f ? 255 : (unsigned char)(fVal + 0.5f);
		}
	}

	//-----------------------------------------------------------------------

	bool cSDLFontSizedGlyphKey::operator<(const cSDLFontSizedGlyphKey &aKey) const
	{
		if(mlChar != aKey.mlChar) return mlChar < aKey.mlChar;
		if(mlLineHeight != aKey.mlLineHeight) return mlLineHeight < aKey.mlLineHeight;
		if(mlStretch != aKey.mlStretch) return mlStretch < aKey.mlStretch;
		if(mlEffect != aKey.mlEffect) return mlEffect < aKey.mlEffect;
		return mlEffectWidth < aKey.mlEffectWidth;
	}

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cSDLFontData::cSDLFontData(const tString &asName,iLowLevelGraphics* apLowLevelGraphics)
		: iFontData(asName,apLowLevelGraphics)
	{
		mpFace = nullptr;
		mfVectorSize = 1;
		mfVectorWidth = 1;
	}

	//-----------------------------------------------------------------------

	cSDLFontData::~cSDLFontData()
	{
		for(auto &glyphPair : m_mapSizedGlyphs)
		{
			cFontSizedGlyph *pGlyph = glyphPair.second;
			if(pGlyph->mpGfxObject)
			{
				//The atlas owns the texture.
				pGlyph->mpGfxObject->GetMaterial()->SetTexture(nullptr,eMaterialTexture_Diffuse);
				hplDelete(pGlyph->mpGfxObject);
			}
			hplDelete(pGlyph);
		}

		if(mpFace)
		{
			FT_Done_Face(mpFace);

			glVectorFontCount--;
			if(glVectorFontCount <= 0)
			{
				for(cFontAtlasPage &page : gvAtlasPages) hplDelete(page.mpTexture);
				gvAtlasPages.clear();

				FT_Done_FreeType(gpFTLibrary);
				gpFTLibrary = nullptr;
				glVectorFontCount = 0;
			}
		}
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cSDLFontData::CreateFromBitmapFile(const tString &asFileName)
	{
		tString sPath = cString::GetFilePath(asFileName);

		////////////////////////////////////////////
		// Load xml file
		TiXmlDocument *pXmlDoc = hplNew( TiXmlDocument,(asFileName.c_str()) );

		if(pXmlDoc->LoadFile()==false)
		{
			Error("Couldn't load angle code font file '%s'\n",asFileName.c_str());
			hplDelete(pXmlDoc);
			return false;
		}

		TiXmlElement *pRootElem = pXmlDoc->RootElement();

		////////////////////////////////////////////
		// Load Common info
		TiXmlElement *pCommonElem = pRootElem->FirstChildElement("common");

		int lLineHeight = cString::ToInt(pCommonElem->Attribute("lineHeight"),0);
		int lBase = cString::ToInt(pCommonElem->Attribute("base"),0);

		mlFirstChar =0;
		mlLastChar = 3000; //Get this num from something.

		mfHeight = (float)lLineHeight;

		mvGlyphs.resize(3000, nullptr);

		mvSizeRatio.x = (float)lBase / (float)lLineHeight;
		mvSizeRatio.y = 1;

		mfBaseLine = (float)lBase / (float)lLineHeight;

		////////////////////////////////////////////
		// Load bitmaps
		std::vector<cSDLBitmap2D *> vBitmaps;

		TiXmlElement *pPagesRootElem = pRootElem->FirstChildElement("pages");

		TiXmlElement *pPageElem = pPagesRootElem->FirstChildElement("page");
		for(; pPageElem != nullptr; pPageElem = pPageElem->NextSiblingElement("page"))
		{
			tString sFileName = pPageElem->Attribute("file");
			tString sFilePath = cString::SetFilePath(sFileName,sPath);

			cSDLBitmap2D *pBitmap = static_cast<cSDLBitmap2D*>(mpLowLevelResources->LoadBitmap2D(sFilePath));
			if(pBitmap==nullptr)
			{
				Error("Couldn't load bitmap %s for FNT file '%s'\n",sFilePath.c_str(),asFileName.c_str());
				hplDelete(pXmlDoc);
				return false;
			}

			vBitmaps.push_back(pBitmap);
		}

		////////////////////////////////////////////
		// Load glyphs
		TiXmlElement *pCharsRootElem = pRootElem->FirstChildElement("chars");

		TiXmlElement *pCharElem = pCharsRootElem->FirstChildElement("char");
		for(; pCharElem != nullptr; pCharElem = pCharElem->NextSiblingElement("char"))
		{
			//Get the info on the character
			int lId = cString::ToInt(pCharElem->Attribute("id"),0);
			int lX = cString::ToInt(pCharElem->Attribute("x"),0);
			int lY = cString::ToInt(pCharElem->Attribute("y"),0);

			int lW = cString::ToInt(pCharElem->Attribute("width"),0);
			int lH = cString::ToInt(pCharElem->Attribute("height"),0);

			int lXOffset = cString::ToInt(pCharElem->Attribute("xoffset"),0);
			int lYOffset = cString::ToInt(pCharElem->Attribute("yoffset"),0);

			int lAdvance = cString::ToInt(pCharElem->Attribute("xadvance"),0);

			int lPage = cString::ToInt(pCharElem->Attribute("page"),0);

			//Get the bitmap where the character graphics is
			cSDLBitmap2D *pSourceBitmap = vBitmaps[lPage];

			//Create a bitmap for the character.
			cVector2l vSize(lW, lH);
			cSDLBitmap2D *pBmp = static_cast<cSDLBitmap2D*>(mpLowLevelGraphics->CreateBitmap2D(vSize,32));

			//Copy from source to character bitmap
			SDL_Rect srcRect;
			srcRect.x = lX; srcRect.y = lY;
			srcRect.w = lW; srcRect.h = lH;
			SDL_BlitSurface(pSourceBitmap->GetSurface(),&srcRect, pBmp->GetSurface(), nullptr);

			int lBmpSize = SDL_BYTESPERPIXEL(pBmp->GetSurface()->format);
			unsigned char* PixBuffer = (unsigned char*)pBmp->GetSurface()->pixels;

			//Set proper alpha (dunno if this is needed)
			for(unsigned int y=0;y<pBmp->GetHeight();y++)
				for(unsigned int x=0;x<pBmp->GetWidth();x++) {
					unsigned char* Pix = &PixBuffer[y*pBmp->GetWidth()*lBmpSize + x*lBmpSize];
					Pix[3] = Pix[0];
				}

			//Create glyph and place it correctly.
			cGlyph *pGlyph = CreateGlyph(pBmp,cVector2l(lXOffset,lYOffset),cVector2l(lW,lH),
										cVector2l(lBase,lLineHeight),lAdvance);

			mvGlyphs[lId] = pGlyph;

			hplDelete(pBmp);
		}



		//Destroy bitmaps
		STLDeleteAll(vBitmaps);

		//Destroy XML
		hplDelete(pXmlDoc);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cSDLFontData::CreateFromFontFile(const tString &asFileName, int alSize, unsigned short alFirstChar,
		unsigned short alLastChar)
	{
		// TrueType fonts are not used by the game; all fonts are bitmap .fnt files.
		Error("TrueType fonts are not supported: '%s'\n",asFileName.c_str());
		return false;
	}

	//-----------------------------------------------------------------------

	bool cSDLFontData::SetVectorFont(const tString &asFileName, float afSize, float afWidth)
	{
		if(mpFace)
		{
			Error("Font '%s' already has a vector font\n",GetName().c_str());
			return false;
		}
		if(afSize <= 0 || afWidth <= 0)
		{
			Error("Bad size %f or width %f for vector font '%s'\n",afSize,afWidth,asFileName.c_str());
			return false;
		}

		if(gpFTLibrary==nullptr && FT_Init_FreeType(&gpFTLibrary) != 0)
		{
			Error("Couldn't init FreeType\n");
			gpFTLibrary = nullptr;
			return false;
		}

		if(FT_New_Face(gpFTLibrary,asFileName.c_str(),0,&mpFace) != 0 || mpFace->units_per_EM == 0)
		{
			Error("Couldn't load vector font file '%s'\n",asFileName.c_str());
			if(mpFace) FT_Done_Face(mpFace);
			mpFace = nullptr;
			if(glVectorFontCount == 0){
				FT_Done_FreeType(gpFTLibrary);
				gpFTLibrary = nullptr;
			}
			return false;
		}
		glVectorFontCount++;

		mfVectorSize = afSize;
		mfVectorWidth = afWidth;

		////////////////////////////////////////////
		// Advances, unhinted so that they are the same at every size.
		mvVectorAdvance.assign(mvGlyphs.size(), -1.0f);
		for(size_t lChar=32; lChar<mvVectorAdvance.size(); lChar++)
		{
			FT_UInt lIndex = FT_Get_Char_Index(mpFace,(FT_ULong)lChar);
			if(lIndex == 0) continue;

			FT_Fixed lAdvance =0;
			if(FT_Get_Advance(mpFace,lIndex,FT_LOAD_NO_SCALE,&lAdvance) != 0) continue;

			mvVectorAdvance[lChar] = (float)lAdvance / (float)mpFace->units_per_EM * afSize * afWidth;
		}

		mbVectorFont = true;
		return true;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PROTECTED METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cFontSizedGlyph* cSDLFontData::GetSizedGlyph(wchar_t alChar, int alLineHeight, float afStretch,
												eFontEffect aEffect, int alEffectWidth)
	{
		if(mpFace==nullptr) return nullptr;

		cSDLFontSizedGlyphKey key;
		key.mlChar = alChar;
		key.mlLineHeight = alLineHeight;
		key.mlStretch = (int)lroundf(afStretch * 256.0f);
		key.mlEffect = (int)aEffect;
		key.mlEffectWidth = alEffectWidth;

		tSDLFontSizedGlyphMap::iterator it = m_mapSizedGlyphs.find(key);
		if(it != m_mapSizedGlyphs.end()) return it->second;

		//A glyph that can't be made is stored empty, so it is only tried once.
		cFontSizedGlyph *pGlyph = CreateSizedGlyph(key);
		if(pGlyph==nullptr) pGlyph = hplNew( cFontSizedGlyph, () );

		m_mapSizedGlyphs.insert(tSDLFontSizedGlyphMap::value_type(key,pGlyph));
		return pGlyph;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	// Copies a rendered glyph into a coverage bitmap, keeping the highest value.
	static void AddGlyphCoverage(FT_BitmapGlyph apBitmap, std::vector<unsigned char> &avCoverage,
								int alLeft, int alTop, int alW, int alH)
	{
		const FT_Bitmap &bitmap = apBitmap->bitmap;
		if(bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) return;

		for(int y=0; y<(int)bitmap.rows; y++)
		{
			int lDestY = alTop - apBitmap->top + y;
			if(lDestY<0 || lDestY>=alH) continue;

			const unsigned char *pSrc = bitmap.buffer + (ptrdiff_t)y * bitmap.pitch;
			for(int x=0; x<(int)bitmap.width; x++)
			{
				int lDestX = apBitmap->left - alLeft + x;
				if(lDestX<0 || lDestX>=alW) continue;

				unsigned char &dest = avCoverage[(size_t)lDestY*alW + lDestX];
				if(pSrc[x] > dest) dest = pSrc[x];
			}
		}
	}

	//-----------------------------------------------------------------------

	cFontSizedGlyph* cSDLFontData::CreateSizedGlyph(const cSDLFontSizedGlyphKey &aKey)
	{
		FT_UInt lIndex = FT_Get_Char_Index(mpFace,(FT_ULong)aKey.mlChar);
		if(lIndex == 0) return nullptr;

		////////////////////////////////////////////
		// Render the glyph. Light hinting only fits it to the pixels
		// vertically, the widths stay as designed.
		float fEm = (float)aKey.mlLineHeight * mfVectorSize;
		float fEmWidth = fEm * mfVectorWidth * ((float)aKey.mlStretch / 256.0f);

		if(FT_Set_Char_Size(mpFace,(FT_F26Dot6)(fEmWidth*64.0f + 0.5f),(FT_F26Dot6)(fEm*64.0f + 0.5f),72,72) != 0)
			return nullptr;
		if(FT_Load_Glyph(mpFace,lIndex,FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP) != 0)
			return nullptr;
		if(mpFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
			return nullptr;

		FT_Glyph pFill = nullptr;
		if(FT_Get_Glyph(mpFace->glyph,&pFill) != 0) return nullptr;

		float fEffectWidth = (float)aKey.mlEffectWidth / 4.0f;

		//The outline is the glyph's contour drawn with a round pen, joined with the glyph.
		FT_Glyph pStroke = nullptr;
		if(aKey.mlEffect == eFontEffect_Outline)
		{
			FT_Stroker pStroker = nullptr;
			if(FT_Stroker_New(gpFTLibrary,&pStroker) == 0)
			{
				FT_Stroker_Set(pStroker,(FT_Fixed)(fEffectWidth*64.0f),
								FT_STROKER_LINECAP_ROUND,FT_STROKER_LINEJOIN_ROUND,0);

				if(FT_Glyph_Copy(pFill,&pStroke) == 0)
				{
					if(FT_Glyph_Stroke(&pStroke,pStroker,1) != 0 ||
						FT_Glyph_To_Bitmap(&pStroke,FT_RENDER_MODE_NORMAL,nullptr,1) != 0)
					{
						FT_Done_Glyph(pStroke);
						pStroke = nullptr;
					}
				}
				FT_Stroker_Done(pStroker);
			}
		}

		if(FT_Glyph_To_Bitmap(&pFill,FT_RENDER_MODE_NORMAL,nullptr,1) != 0)
		{
			FT_Done_Glyph(pFill);
			if(pStroke) FT_Done_Glyph(pStroke);
			return nullptr;
		}

		FT_BitmapGlyph pFillBitmap = (FT_BitmapGlyph)pFill;
		FT_BitmapGlyph pStrokeBitmap = (FT_BitmapGlyph)pStroke;

		////////////////////////////////////////////
		// The area the glyph and its effect cover, in pixels with y going
		// down and the pen position on the baseline at 0,0.
		int lLeft = pFillBitmap->left;
		int lTop = pFillBitmap->top;
		int lRight = lLeft + (int)pFillBitmap->bitmap.width;
		int lBottom = lTop - (int)pFillBitmap->bitmap.rows;

		if(pStrokeBitmap)
		{
			lLeft = cMath::Min(lLeft, pStrokeBitmap->left);
			lTop = cMath::Max(lTop, pStrokeBitmap->top);
			lRight = cMath::Max(lRight, pStrokeBitmap->left + (int)pStrokeBitmap->bitmap.width);
			lBottom = cMath::Min(lBottom, pStrokeBitmap->top - (int)pStrokeBitmap->bitmap.rows);
		}

		//The glow is the glyph blurred, about a gaussian with the width as 2 sigma.
		int lBlurRadius = 0;
		if(aKey.mlEffect == eFontEffect_Glow)
		{
			float fSigma = fEffectWidth * 0.5f;
			lBlurRadius = (int)lroundf((sqrtf(4.0f*fSigma*fSigma + 1.0f) - 1.0f) * 0.5f);
			if(lBlurRadius < 1) lBlurRadius = 1;

			lLeft -= lBlurRadius*3;
			lTop += lBlurRadius*3;
			lRight += lBlurRadius*3;
			lBottom -= lBlurRadius*3;
		}

		int lW = lRight - lLeft;
		int lH = lTop - lBottom;

		cFontSizedGlyph *pGlyph = nullptr;

		if(lW > 0 && lH > 0 && pFillBitmap->bitmap.width > 0 && pFillBitmap->bitmap.rows > 0)
		{
			std::vector<unsigned char> vCoverage((size_t)lW*lH, 0);

			AddGlyphCoverage(pFillBitmap,vCoverage,lLeft,lTop,lW,lH);
			if(pStrokeBitmap) AddGlyphCoverage(pStrokeBitmap,vCoverage,lLeft,lTop,lW,lH);

			if(lBlurRadius > 0) BlurCoverage(vCoverage,lW,lH,lBlurRadius,2.0f);

			iTexture *pTexture = nullptr;
			cVector2l vPos;
			if(AddToFontAtlas(mpLowLevelGraphics,lW,lH,vCoverage.data(),&pTexture,&vPos))
			{
				float fAtlasSize = (float)glAtlasSize;

				pGlyph = hplNew( cFontSizedGlyph, () );
				pGlyph->mpGfxObject = mpGraphicsDrawer->CreateGfxObject(pTexture,
										cVector2f((float)vPos.x / fAtlasSize, (float)vPos.y / fAtlasSize),
										cVector2f((float)(vPos.x+lW) / fAtlasSize, (float)(vPos.y+lH) / fAtlasSize),
										"fontnormal");
				pGlyph->mvOffset = cVector2l(lLeft,-lTop);
				pGlyph->mvSize = cVector2l(lW,lH);
			}
		}

		FT_Done_Glyph(pFill);
		if(pStroke) FT_Done_Glyph(pStroke);

		return pGlyph;
	}

	//-----------------------------------------------------------------------

}
