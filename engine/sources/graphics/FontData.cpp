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
#include "graphics/FontData.h"
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <wchar.h>

#include "system/LowLevelSystem.h"
#include "math/Math.h"
#include "graphics/LowLevelGraphics.h"

#include "graphics/GraphicsDrawer.h"
#include "graphics/GfxObject.h"
#include "graphics/Material.h"

#include "resources/ResourceImage.h"

#include "gui/GuiGfxElement.h"
#include "gui/Gui.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// KEY CAPS
	//////////////////////////////////////////////////////////////////////////

	static cGfxObject *gpKeyCapGfx = nullptr;
	static float gfKeyCapEdge = 0.25f;
	static float gfKeyCapAspect = 2;

	// Sizes as parts of the text's height: the key's height, the name's size
	// on it, the space between the name and the key's sides, and between the
	// key and the text beside it.
	static const float kfKeyCapHeight = 1.2f;
	static const float kfKeyCapName = 0.72f;
	static const float kfKeyCapPadding = 0.45f;
	static const float kfKeyCapGap = 0.25f;
	// Height of the middle of capital letters above the baseline, where the
	// key is centred
	static const float kfKeyCapCentre = 0.36f;
	// Middle of the key's top face, where the name is centred, as a part of
	// the key's height from its top
	static const float kfKeyCapFace = 0.45f;

	// The key's name between apText and the end mark (or the end of the
	// text), with its non-breaking spaces made spaces. Returns where the
	// marked text ends: at the end mark, or at the end of the text.
	static const wchar_t* ReadKeyCapName(const wchar_t* apText, tWString *apName)
	{
		const wchar_t* pChar = apText;
		for(; *pChar != 0 && *pChar != kwcKeyCapEnd; ++pChar)
			apName->push_back(*pChar == 0xA0 ? _W(' ') : *pChar);
		return pChar;
	}

	void iFontData::SetKeyCapGfx(cGfxObject *apGfx, float afEdge, float afAspect)
	{
		gpKeyCapGfx = apGfx;
		gfKeyCapEdge = afEdge;
		gfKeyCapAspect = afAspect;
	}


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////


	//-----------------------------------------------------------------------

	cGlyph::cGlyph(	cGfxObject *apObject, cGuiGfxElement *apGuiGfx,const cVector2f &avOffset,
					const cVector2f &avSize, float afAdvance)
	{
		mpGfxObject = apObject;
		mpGuiGfx = apGuiGfx;
		mvOffset = avOffset;
		mvSize = avSize;
		mfAdvance = afAdvance;
	}


	cGlyph::~cGlyph()
	{
		if(mpGfxObject) hplDelete(mpGfxObject);
		if(mpGuiGfx) hplDelete(mpGuiGfx);

	}

	//-----------------------------------------------------------------------

	iFontData::iFontData(const tString &asName,iLowLevelGraphics* apLowLevelGraphics)
	: iResourceBase(asName,0)
	{
		mpLowLevelGraphics = apLowLevelGraphics;

		mfBaseLine = 1;
		mbVectorFont = false;
		mfVectorPixel = 0;
		mbVerticalPixelSnap = true;
	}

	//-----------------------------------------------------------------------

	iFontData::~iFontData()
	{
		for(int i=0;i<(int)mvGlyphs.size();i++)
		{
			if(mvGlyphs[i]) hplDelete(mvGlyphs[i]);
		}
	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void iFontData::Draw(const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
						eFontAlign aAlign,	const wchar_t* fmt,...)
	{
		wchar_t sText[256];
		va_list ap;
		if (fmt == nullptr) return;
		va_start(ap, fmt);
		vswprintf(sText, 255, fmt, ap);
		va_end(ap);


		DrawString(avPos,avSize,aCol,aAlign,sText,eFontEffect_None,0);
	}

	//-----------------------------------------------------------------------

	void iFontData::DrawEffect(eFontEffect aEffect, float afEffectWidth,
								const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
								eFontAlign aAlign, const tWString &asString)
	{
		DrawString(avPos,avSize,aCol,aAlign,asString.c_str(),aEffect,afEffectWidth);
	}

	//-----------------------------------------------------------------------

	int iFontData::DrawWordWrap(cVector3f avPos,float afLength,float afFontHeight,cVector2f avSize,const cColor& aCol,
								eFontAlign aAlign,	const tWString &asString,
								eFontEffect aEffect, float afEffectWidth)
	{
		int rows = 0;

		unsigned int pos;
		unsigned int first_letter=0;
		unsigned int last_space=0;

		tUIntList RowLengthList;

		float fTextLength;

		for(pos = 0; pos < asString.size();pos++)
		{
			if(asString[pos] == _W(' ') || asString[pos] == _W('\n'))
			{
				tWString temp = asString.substr(first_letter, pos-first_letter);
				fTextLength =  GetLength(avSize,temp.c_str());

				bool nothing = true;
				if(fTextLength > afLength)
				{
					rows++;
					RowLengthList.push_back(last_space);
					first_letter=last_space+1;
					last_space = pos;
					nothing = false;
				}
				if(asString[pos] == _W('\n'))
				{
					last_space = pos;
					first_letter=last_space+1;
					RowLengthList.push_back(last_space-1);
					rows++;
					nothing = false;
				}
				if(nothing)
				{
					last_space = pos;
				}
			}
		}
		tWString temp =  asString.substr(first_letter, pos-first_letter);
		fTextLength = GetLength(avSize,temp.c_str());
		if(fTextLength > afLength)
		{
			rows++;
			RowLengthList.push_back(last_space);
		}

		if(rows==0)
		{
			DrawString(avPos,avSize,aCol,aAlign,asString.c_str(),aEffect,afEffectWidth);
		}
		else
		{
			first_letter=0;
			unsigned int i=0;

			for(tUIntListIt it = RowLengthList.begin();it != RowLengthList.end();++it)
			{
				DrawString(cVector3f(avPos.x,avPos.y + i*afFontHeight,avPos.z),avSize,aCol,aAlign,
								asString.substr(first_letter,*it-first_letter).c_str(),aEffect,afEffectWidth);
				i++;
				first_letter = *it+1;
			}
			DrawString(cVector3f(avPos.x,avPos.y + i*afFontHeight,avPos.z),avSize,aCol,aAlign,
				asString.substr(first_letter).c_str(),aEffect,afEffectWidth);

		}

		return rows;
	}

	//-----------------------------------------------------------------------

	void iFontData::GetWordWrapRows(float afLength,float afFontHeight,cVector2f avSize,
							const tWString& asString,tWStringVec *apRowVec)
	{
		int rows = 0;

		unsigned int pos;
		unsigned int first_letter=0;
		unsigned int last_space=0;

		tUIntList RowLengthList;

		float fTextLength;

		for(pos = 0; pos < asString.size();pos++)
		{
			if(asString[pos] == _W(' ') || asString[pos] == _W('\n'))
			{
				tWString temp = asString.substr(first_letter, pos-first_letter);
				fTextLength =  GetLength(avSize,temp.c_str());

				bool nothing = true;
				if(fTextLength > afLength)
				{
					rows++;
					RowLengthList.push_back(last_space);
					first_letter=last_space+1;
					last_space = pos;
					nothing = false;
				}
				if(asString[pos] == _W('\n'))
				{
					last_space = pos;
					first_letter=last_space+1;
					RowLengthList.push_back(last_space);
					rows++;
					nothing = false;
				}
				if(nothing)
				{
					last_space = pos;
				}
			}
		}
		tWString temp =  asString.substr(first_letter, pos-first_letter);
		fTextLength = GetLength(avSize,temp.c_str());
		if(fTextLength > afLength)
		{
			rows++;
			RowLengthList.push_back(last_space);
		}

		if(rows==0)
		{
			apRowVec->push_back(asString.c_str());
		}
		else
		{
			first_letter=0;
			unsigned int i=0;

			for(tUIntListIt it = RowLengthList.begin();it != RowLengthList.end();++it)
			{
				apRowVec->push_back(asString.substr(first_letter,*it-first_letter).c_str());
				i++;
				first_letter = *it+1;
			}
			apRowVec->push_back(asString.substr(first_letter).c_str());

		}
	}

	//-----------------------------------------------------------------------

	float iFontData::GetLength(const cVector2f& avSize,const wchar_t* sText)
	{
		float fLength =0;
		for(int lCount=0; sText[lCount] != 0; lCount++)
		{
			wchar_t lChar = sText[lCount];

			if(lChar == kwcKeyCapStart)
			{
				tWString sName;
				const wchar_t* pEnd = ReadKeyCapName(&sText[lCount+1], &sName);
				fLength += GetKeyCapLength(avSize, sName);
				if(*pEnd == 0) break;
				lCount = (int)(pEnd - sText);
				continue;
			}

			if(HasVectorGlyph(lChar))
			{
				fLength += mvVectorAdvance[lChar]*avSize.x;
				continue;
			}

			cGlyph *pGlyph = GetBitmapGlyph(lChar);
			if(pGlyph) fLength += pGlyph->mfAdvance*avSize.x;
		}

		return fLength;
	}

	//-----------------------------------------------------------------------


	float iFontData::GetLengthFmt(const cVector2f& avSize,const wchar_t* fmt,...)
	{
		wchar_t sText[256];
		va_list ap;
		if (fmt == nullptr) return 0;
		va_start(ap, fmt);
		vswprintf(sText, 255, fmt, ap);
		va_end(ap);

		return GetLength(avSize, sText);
	}
	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cGlyph* iFontData::GetBitmapGlyph(wchar_t alChar)
	{
		if(alChar<mlFirstChar || alChar>mlLastChar) return nullptr;

		size_t lGlyphNum = (size_t)(alChar - mlFirstChar);
		if(lGlyphNum >= mvGlyphs.size()) return nullptr;

		return mvGlyphs[lGlyphNum];
	}

	//-----------------------------------------------------------------------

	bool iFontData::HasVectorGlyph(wchar_t alChar)
	{
		return mbVectorFont && alChar>=0 && (size_t)alChar < mvVectorAdvance.size() &&
				mvVectorAdvance[alChar] >= 0;
	}

	//-----------------------------------------------------------------------

	int iFontData::GetVectorPixelScale(int alLineHeight, float afStretch)
	{
		if(mfVectorPixel <= 0) return 0;

		int lScale = (int)floorf((float)alLineHeight * mfVectorPixel * afStretch + 0.001f);
		return lScale < 1 ? 1 : lScale;
	}

	//-----------------------------------------------------------------------

	void iFontData::DrawBitmapString(cVector3f avPos,const cVector2f& avSize, const cColor& aCol,
									const wchar_t* apText)
	{
		for(int lCount=0; apText[lCount] != 0; lCount++)
		{
			cGlyph *pGlyph = GetBitmapGlyph(apText[lCount]);
			if(pGlyph==nullptr) continue;

			cVector2f vOffset(pGlyph->mvOffset * avSize);
			cVector2f vSize(pGlyph->mvSize * avSize);

			mpGraphicsDrawer->DrawGfxObject(pGlyph->mpGfxObject,avPos + vOffset,vSize,aCol);

			avPos.x += pGlyph->mfAdvance*avSize.x;
		}
	}

	//-----------------------------------------------------------------------

	void iFontData::DrawString(const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
								eFontAlign aAlign, const wchar_t* apText,
								eFontEffect aEffect, float afEffectWidth)
	{
		if(wcschr(apText, kwcKeyCapStart)==nullptr)
		{
			DrawStringRun(avPos,avSize,aCol,aAlign,apText,aEffect,afEffectWidth);
			return;
		}

		////////////////////////////////////
		// Text with key caps: the runs of text between them are drawn from
		// the left, each where the one before it ends.
		cVector3f vPos = avPos;
		if(aAlign == eFontAlign_Center)		vPos.x -= GetLength(avSize, apText)/2;
		else if(aAlign == eFontAlign_Right)	vPos.x -= GetLength(avSize, apText);

		const wchar_t* pText = apText;
		while(*pText != 0)
		{
			const wchar_t* pCap = wcschr(pText, kwcKeyCapStart);
			tWString sRun = pCap ? tWString(pText, pCap - pText) : tWString(pText);
			if(sRun.empty()==false)
			{
				DrawStringRun(vPos,avSize,aCol,eFontAlign_Left,sRun.c_str(),aEffect,afEffectWidth);
				vPos.x += GetLength(avSize, sRun.c_str());
			}
			if(pCap == nullptr) break;

			tWString sName;
			const wchar_t* pEnd = ReadKeyCapName(pCap+1, &sName);
			vPos.x += DrawKeyCap(vPos,avSize,aCol,sName,aEffect,afEffectWidth);
			if(*pEnd == 0) break;
			pText = pEnd+1;
		}
	}

	//-----------------------------------------------------------------------

	float iFontData::GetKeyCapLength(const cVector2f& avSize, const tWString& asName)
	{
		if(gpKeyCapGfx == nullptr) return GetLength(avSize, asName.c_str());

		float fName = GetLength(avSize * kfKeyCapName, asName.c_str());
		float fHeight = avSize.y * kfKeyCapHeight;
		// At least square, so that a single letter is on a square key
		float fKey = cMath::Max(fName + 2*kfKeyCapPadding*avSize.y, fHeight);
		return fKey + 2*kfKeyCapGap*avSize.y;
	}

	//-----------------------------------------------------------------------

	float iFontData::DrawKeyCap(const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
								const tWString& asName, eFontEffect aEffect, float afEffectWidth)
	{
		if(gpKeyCapGfx == nullptr)
		{
			DrawStringRun(avPos,avSize,aCol,eFontAlign_Left,asName.c_str(),aEffect,afEffectWidth);
			return GetLength(avSize, asName.c_str());
		}

		float fLength = GetKeyCapLength(avSize, asName);
		float fGap = kfKeyCapGap*avSize.y;
		float fW = fLength - 2*fGap;
		float fH = avSize.y * kfKeyCapHeight;
		float fCentreY = avPos.y + (mfBaseLine - kfKeyCapCentre)*avSize.y;
		// Under the name and the text, above what is drawn under the text
		cVector3f vKeyPos(avPos.x + fGap, fCentreY - fH/2, avPos.z - 0.5f);

		// An effect (outline or glow) is the key a little larger, under it
		if(aEffect != eFontEffect_None)
		{
			vKeyPos -= cVector3f(afEffectWidth, afEffectWidth, 0);
			fW += 2*afEffectWidth;
			fH += 2*afEffectWidth;
		}

		////////////////////////////////////
		// The ends at their own shape, the middle stretched between them
		float fEnd = fH * gfKeyCapAspect * gfKeyCapEdge;
		if(fW < 2*fEnd) fW = 2*fEnd;
		float fWhole = fEnd / gfKeyCapEdge;
		mpGraphicsDrawer->DrawGfxObjectPart(gpKeyCapGfx, vKeyPos, cVector2f(fWhole,fH), aCol,
											cRect2f(0,0,gfKeyCapEdge,1));
		mpGraphicsDrawer->DrawGfxObjectPart(gpKeyCapGfx, vKeyPos + cVector3f(fW - fWhole,0,0),
											cVector2f(fWhole,fH), aCol,
											cRect2f(1-gfKeyCapEdge,0,gfKeyCapEdge,1));
		if(fW > 2*fEnd)
		{
			float fMiddle = (fW - 2*fEnd) / (1 - 2*gfKeyCapEdge);
			mpGraphicsDrawer->DrawGfxObjectPart(gpKeyCapGfx,
											vKeyPos + cVector3f(fEnd - gfKeyCapEdge*fMiddle,0,0),
											cVector2f(fMiddle,fH), aCol,
											cRect2f(gfKeyCapEdge,0,1-2*gfKeyCapEdge,1));
		}

		////////////////////////////////////
		// The name, smaller, centred on the key
		cVector2f vNameSize = avSize * kfKeyCapName;
		float fName = GetLength(vNameSize, asName.c_str());
		float fFaceY = fCentreY + (kfKeyCapFace - 0.5f)*avSize.y*kfKeyCapHeight;
		float fNameBase = fFaceY + kfKeyCapCentre*vNameSize.y;
		cVector3f vNamePos(	avPos.x + fGap + (fLength - 2*fGap - fName)/2,
							fNameBase - mfBaseLine*vNameSize.y, avPos.z);
		DrawStringRun(vNamePos,vNameSize,aCol,eFontAlign_Left,asName.c_str(),aEffect,afEffectWidth);

		return fLength;
	}

	//-----------------------------------------------------------------------

	void iFontData::DrawStringRun(const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
								eFontAlign aAlign, const wchar_t* apText,
								eFontEffect aEffect, float afEffectWidth)
	{
		cVector3f vPos = avPos;

		if(aAlign == eFontAlign_Center){
			vPos.x -= GetLength(avSize, apText)/2;
		}
		else if(aAlign == eFontAlign_Right)
		{
			vPos.x -= GetLength(avSize, apText);
		}

		////////////////////////////////////
		// Bitmap font: the glyphs are scaled to the size.
		// Effects are made of copies of the text, moved to each side.
		if(mbVectorFont==false)
		{
			float fW = afEffectWidth;

			if(aEffect == eFontEffect_None)
			{
				DrawBitmapString(vPos,avSize,aCol,apText);
			}
			else if(aEffect == eFontEffect_Outline)
			{
				DrawBitmapString(vPos + cVector3f(fW,fW,0),avSize,aCol,apText);
				DrawBitmapString(vPos + cVector3f(-fW,-fW,0),avSize,aCol,apText);
				DrawBitmapString(vPos + cVector3f(fW,-fW,0),avSize,aCol,apText);
				DrawBitmapString(vPos + cVector3f(-fW,fW,0),avSize,aCol,apText);
			}
			else if(aEffect == eFontEffect_Glow)
			{
				float fInner = fW * (2.0f/3.0f);
				cColor outerCol(aCol.r,aCol.g,aCol.b,aCol.a*0.5f);

				DrawBitmapString(vPos + cVector3f(fInner,fInner,0),avSize,aCol,apText);
				DrawBitmapString(vPos + cVector3f(-fInner,-fInner,0),avSize,aCol,apText);
				DrawBitmapString(vPos + cVector3f(fW,fW,-1),avSize,outerCol,apText);
				DrawBitmapString(vPos + cVector3f(-fW,-fW,-1),avSize,outerCol,apText);
			}
			return;
		}

		////////////////////////////////////
		// Vector font: the glyphs are rendered for the size they get in
		// pixels and placed on whole pixels. The advances do not depend on
		// the resolution, so the layout is the same at any screen size.
		if(avSize.y <= 0) return;

		cVector2f vScreenSize = mpLowLevelGraphics->GetScreenSize();
		cVector2f vVirtualSize = mpLowLevelGraphics->GetVirtualSize();
		cVector2f vVirtualOffset = mpLowLevelGraphics->GetVirtualOffset();

		//Pixels per virtual unit, the same on both axes.
		float fScale = vScreenSize.y / (vVirtualSize.y + vVirtualOffset.y*2);
		if(fScale <= 0) return;

		int lLineHeight = (int)lroundf(avSize.y * fScale);
		if(lLineHeight < 1) lLineHeight = 1;

		float fStretch = avSize.x / avSize.y;

		int lEffectWidth = 0;
		if(aEffect != eFontEffect_None)
		{
			float fPixels = afEffectWidth * fScale;
			if(fPixels < 1) fPixels = 1;
			lEffectWidth = (int)lroundf(fPixels * 4);
		}

		float fBaseY = (vPos.y + vVirtualOffset.y + mfBaseLine*avSize.y) * fScale;
		if(mbVerticalPixelSnap) fBaseY = floorf(fBaseY + 0.5f);

		// A pixel font advances by whole pixels of the font, rounded down, so
		// the text is a little shorter than GetLength says, never longer.
		float fPixelStep = (float)GetVectorPixelScale(lLineHeight,fStretch);
		auto VectorAdvance = [&](wchar_t alChar)
		{
			float fAdvance = mvVectorAdvance[alChar]*avSize.x;
			if(fPixelStep <= 0) return fAdvance;
			return cMath::Max(1.0f, floorf(fAdvance*fScale/fPixelStep + 0.001f)) * fPixelStep / fScale;
		};

		if(fPixelStep > 0)
		{
			if(aAlign != eFontAlign_Left)
			{
				float fShorter = 0;
				for(int lCount=0; apText[lCount] != 0; lCount++)
				{
					wchar_t lChar = apText[lCount];
					if(HasVectorGlyph(lChar)) fShorter += mvVectorAdvance[lChar]*avSize.x - VectorAdvance(lChar);
				}
				vPos.x += aAlign == eFontAlign_Center ? fShorter/2 : fShorter;
			}

			//Start on a whole pixel, the advances then keep the glyphs on them.
			vPos.x = floorf((vPos.x + vVirtualOffset.x) * fScale + 0.5f) / fScale - vVirtualOffset.x;
		}

		for(int lCount=0; apText[lCount] != 0; lCount++)
		{
			wchar_t lChar = apText[lCount];

			if(HasVectorGlyph(lChar)==false)
			{
				//The vector font lacks the character, use the bitmap.
				cGlyph *pGlyph = GetBitmapGlyph(lChar);
				if(pGlyph==nullptr) continue;

				if(aEffect == eFontEffect_None)
				{
					mpGraphicsDrawer->DrawGfxObject(pGlyph->mpGfxObject,
													vPos + cVector3f(pGlyph->mvOffset.x*avSize.x,
																	pGlyph->mvOffset.y*avSize.y,0),
													pGlyph->mvSize * avSize,aCol);
				}
				vPos.x += pGlyph->mfAdvance*avSize.x;
				continue;
			}

			cFontSizedGlyph *pGlyph = GetSizedGlyph(lChar,lLineHeight,fStretch,aEffect,lEffectWidth);
			if(pGlyph && pGlyph->mpGfxObject)
			{
				float fX = floorf((vPos.x + vVirtualOffset.x) * fScale + 0.5f) + (float)pGlyph->mvOffset.x;
				float fY = fBaseY + (float)pGlyph->mvOffset.y;

				mpGraphicsDrawer->DrawGfxObject(pGlyph->mpGfxObject,
												cVector3f(	fX / fScale - vVirtualOffset.x,
															fY / fScale - vVirtualOffset.y, vPos.z),
												cVector2f(	(float)pGlyph->mvSize.x / fScale,
															(float)pGlyph->mvSize.y / fScale),
												aCol);
			}

			vPos.x += VectorAdvance(lChar);
		}
	}

	//-----------------------------------------------------------------------

	cGlyph* iFontData::CreateGlyph(iBitmap2D* apBmp, const cVector2l &avOffset,const cVector2l &avSize,
								const cVector2l& avFontSize, int alAdvance)
	{
		//Here the bitmap should be saved to diskk for faster loading.

		//////////////////////////
		//Gfx object
		cGfxObject* pObject = mpGraphicsDrawer->CreateGfxObject(apBmp,"fontnormal",false);

		//////////////////////////
		//Gui gfx
		cGuiGfxElement* pGuiGfx = mpGui->CreateGfxFilledRect(cColor(1,1),eGuiMaterial_FontNormal,false);
		cResourceImage *pImage = pObject->GetMaterial()->GetImage(eMaterialTexture_Diffuse);
		pImage->IncUserCount();
		pGuiGfx->AddImage(pImage);

		//////////////////////////
		//Sizes
		cVector2f vSize;
		vSize.x = ((float)avSize.x)/((float)avFontSize.x) * mvSizeRatio.x;
		vSize.y = ((float)avSize.y)/((float)avFontSize.y) * mvSizeRatio.y;

		cVector2f vOffset;
		vOffset.x = ((float)avOffset.x)/((float)avFontSize.x);
		vOffset.y = ((float)avOffset.y)/((float)avFontSize.y);

		float fAdvance = ((float)alAdvance)/((float)avFontSize.x) * mvSizeRatio.x;

		cGlyph* pGlyph = hplNew( cGlyph,(pObject,pGuiGfx,vOffset,vSize,fAdvance));

		return pGlyph;
	}

	//-----------------------------------------------------------------------

	void iFontData::AddGlyph(cGlyph *apGlyph)
	{
		mvGlyphs.push_back(apGlyph);
	}


	//-----------------------------------------------------------------------

}
