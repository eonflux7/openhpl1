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
#include "tinyxml.h"

namespace hpl {


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cSDLFontData::cSDLFontData(const tString &asName,iLowLevelGraphics* apLowLevelGraphics)
		: iFontData(asName,apLowLevelGraphics)
	{


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

		mvGlyphs.resize(3000, NULL);

		mvSizeRatio.x = (float)lBase / (float)lLineHeight;
		mvSizeRatio.y = 1;

		////////////////////////////////////////////
		// Load bitmaps
		std::vector<cSDLBitmap2D *> vBitmaps;

		TiXmlElement *pPagesRootElem = pRootElem->FirstChildElement("pages");

		TiXmlElement *pPageElem = pPagesRootElem->FirstChildElement("page");
		for(; pPageElem != NULL; pPageElem = pPageElem->NextSiblingElement("page"))
		{
			tString sFileName = pPageElem->Attribute("file");
			tString sFilePath = cString::SetFilePath(sFileName,sPath);

			cSDLBitmap2D *pBitmap = static_cast<cSDLBitmap2D*>(mpLowLevelResources->LoadBitmap2D(sFilePath));
			if(pBitmap==NULL)
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
		for(; pCharElem != NULL; pCharElem = pCharElem->NextSiblingElement("char"))
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
			SDL_BlitSurface(pSourceBitmap->GetSurface(),&srcRect, pBmp->GetSurface(), NULL);

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

}
