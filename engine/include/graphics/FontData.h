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
#ifndef HPL_FONTDATA_H
#define HPL_FONTDATA_H

#include <vector>
#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "system/SystemTypes.h"
#include "graphics/GraphicsTypes.h"
#include "resources/ResourceBase.h"
#include "resources/LowLevelResources.h"

namespace hpl {

	class ImageManager;
	class iLowLevelGraphics;
	class cGraphicsDrawer;
	class cGfxObject;
	class iBitmap2D;
	class cGuiGfxElement;
	class cGui;

	//------------------------------------------------

	class cGlyph
	{
	public:
		cGlyph(	cGfxObject *apObject,cGuiGfxElement *apGuiGfx,const cVector2f &avOffset,
				const cVector2f &avSize, float afAdvance);
		~cGlyph();

		cGfxObject *mpGfxObject;
		cGuiGfxElement *mpGuiGfx;
		cVector2f mvOffset;
		cVector2f mvSize;
		float mfAdvance;
	};

	typedef std::vector<cGlyph*> tGlyphVec;
	typedef tGlyphVec::iterator tGlyphVecIt;

	//------------------------------------------------

	enum eFontEffect
	{
		eFontEffect_None,
		/// The text widened on all sides, to be drawn under the text itself.
		eFontEffect_Outline,
		/// The text blurred, to be drawn under the text itself.
		eFontEffect_Glow,
		eFontEffect_LastEnum
	};

	/**
	 * A glyph of a vector font rendered for one size in screen pixels. It is
	 * drawn with one texel on each pixel.
	 */
	class cFontSizedGlyph
	{
	public:
		/// nullptr when there is nothing to draw (a space).
		cGfxObject *mpGfxObject = nullptr;
		/// Top left of the bitmap in pixels, from the pen position on the baseline.
		cVector2l mvOffset = cVector2l(0,0);
		cVector2l mvSize = cVector2l(0,0);
	};

	class iFontData : public iResourceBase
	{
	public:
		iFontData(const tString &asName, iLowLevelGraphics* apLowLevelGraphics);
		~iFontData() override;

		virtual bool CreateFromFontFile(const tString &asFileName, int alSize,unsigned short alFirstChar,
									unsigned short alLastChar)=0;

		virtual bool CreateFromBitmapFile(const tString &asFileName)=0;

		/**
		 * Draw the glyphs from a vector (TrueType) font file, rendered at the size
		 * they have on screen, instead of scaling the bitmaps. The font must
		 * already be created from a bitmap file: its line height and baseline
		 * are kept, and its glyphs are used for characters the file lacks.
		 * \param asFileName
		 * \param afSize The em size, as a part of the line height.
		 * \param afWidth Horizontal scale of the glyphs and their advances.
		 */
		virtual bool SetVectorFont(const tString &asFileName, float afSize, float afWidth){ return false; }


		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}

		/**
		 * Used internally
		 */
		void SetUp(cGraphicsDrawer *apGraphicsDrawer, iLowLevelResources* apLowLevelResources,
					cGui *apGui)
		{
			mpGraphicsDrawer = apGraphicsDrawer;
			mpLowLevelResources = apLowLevelResources;
			mpGui = apGui;
		}

		/**
		 * Used internally
		 * \param alNum
		 * \return
		 */
		inline cGlyph* GetGlyph(int alNum)const { return mvGlyphs[alNum];}

		inline unsigned short GetFirstChar(){ return mlFirstChar;}
		inline unsigned short GetLastChar(){ return mlLastChar;}

		inline const cVector2f& GetSizeRatio()const{ return mvSizeRatio;}

		/**
		 * Draw a string.
		 * \param avPos Screen pos
		 * \param avSize size of the characters
		 * \param aCol color
		 * \param mAlign alignment
		 * \param fmt
		 * \param ...
		 */
		void Draw(const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,eFontAlign mAlign,
					const wchar_t* fmt,...);
		/**
		 * Draw a string  with word wrap.
		 * \param avPos Screen pos
		 * \param afLength Max length of a line
		 * \param afFontHeight The distance from base of character above to base of character below
		 * \param avSize size of the characters
		 * \param aCol color
		 * \param aAlign alignment
		 * \param asString
		 * \return Extra number of rows generated.
		 */
		int DrawWordWrap(cVector3f avPos,float afLength,float afFontHeight,cVector2f avSize,const cColor& aCol,
							eFontAlign aAlign,	const tWString &asString,
							eFontEffect aEffect=eFontEffect_None, float afEffectWidth=0);

		/**
		 * Draw the outline or glow of a string, to go under the same string
		 * drawn normally (give it a lower z).
		 * \param aEffect
		 * \param afEffectWidth How far the effect reaches from the text, in virtual screen size "pixels"
		 */
		void DrawEffect(eFontEffect aEffect, float afEffectWidth,
						const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
						eFontAlign aAlign, const tWString &asString);


		void GetWordWrapRows(float afLength,float afFontHeight,cVector2f avSize,const tWString& asString,
								tWStringVec *apRowVec);

		/**
		 * Vector font glyphs are placed on whole pixels. For text that moves
		 * slowly up or down, turn that off for the vertical position while
		 * drawing it: the text gets a little softer but moves evenly.
		 */
		void SetVerticalPixelSnap(bool abX){ mbVerticalPixelSnap = abX; }

		/**
		 * Get height of the font.
		 * \return
		 */
		inline float GetHeight()const{ return mfHeight; }

		/**
		 * Get the length in virtual screen size "pixels" of a formated string
		 * \param avSize size of the characters
		 * \param fmt
		 * \param ...
		 * \return
		 */
		float GetLengthFmt(const cVector2f& avSize,const wchar_t* fmt,...);
		/**
		 * Get the length in virtual screen size "pixels" of a string
		 * \param avSize size of the characters
		 * \param sText
		 * \return
		 */
		float GetLength(const cVector2f& avSize,const wchar_t* sText);

	protected:
		iLowLevelGraphics* mpLowLevelGraphics;
		iLowLevelResources* mpLowLevelResources;
		cGraphicsDrawer *mpGraphicsDrawer;
		cGui *mpGui;

		tGlyphVec mvGlyphs;

		float mfHeight;
		unsigned short mlFirstChar;
		unsigned short mlLastChar;

		cVector2f mvSizeRatio;

		/// Distance from the top of a line to the baseline, as a part of the line height.
		float mfBaseLine;

		bool mbVectorFont;
		bool mbVerticalPixelSnap;
		/// Advance of each character in the vector font, in line heights. Below 0 if the font lacks it.
		tFloatVec mvVectorAdvance;

		/**
		 * Get a vector font glyph rendered for a size on screen.
		 * \param alLineHeight Line height in pixels
		 * \param afStretch Extra horizontal scale
		 * \param aEffect
		 * \param alEffectWidth Width of the effect in quarter pixels
		 * 
eturn nullptr if the glyph could not be made.
		 */
		virtual cFontSizedGlyph* GetSizedGlyph(wchar_t alChar, int alLineHeight, float afStretch,
												eFontEffect aEffect, int alEffectWidth){ return nullptr; }

		cGlyph* GetBitmapGlyph(wchar_t alChar);
		bool HasVectorGlyph(wchar_t alChar);

		void DrawString(const cVector3f& avPos,const cVector2f& avSize, const cColor& aCol,
						eFontAlign aAlign, const wchar_t* apText,
						eFontEffect aEffect, float afEffectWidth);
		void DrawBitmapString(cVector3f avPos,const cVector2f& avSize, const cColor& aCol,
								const wchar_t* apText);

		cGlyph* CreateGlyph(iBitmap2D* apBmp, const cVector2l &avOffset,const cVector2l &avSize,
							const cVector2l& avFontSize, int alAdvance);
		void AddGlyph(cGlyph *apGlyph);
	};

};
#endif // HPL_FONTDATA_H
