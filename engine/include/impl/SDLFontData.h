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
#ifndef HPL_SDL_FONTDATA_H
#define HPL_SDL_FONTDATA_H

#include "graphics/FontData.h"

#include <SDL3/SDL.h>

#include <map>

struct FT_FaceRec_;

namespace hpl {

	class cSDLFontSizedGlyphKey
	{
	public:
		wchar_t mlChar;
		int mlLineHeight;
		int mlStretch;
		int mlEffect;
		int mlEffectWidth;

		bool operator<(const cSDLFontSizedGlyphKey &aKey) const;
	};

	typedef std::map<cSDLFontSizedGlyphKey, cFontSizedGlyph*> tSDLFontSizedGlyphMap;

	class cSDLFontData : public iFontData
	{
	public:
		cSDLFontData(const tString &asName, iLowLevelGraphics* apLowLevelGraphics);
		~cSDLFontData() override;

		bool CreateFromFontFile(const tString &asFileName, int alSize,unsigned short alFirstChar,
								unsigned short alLastChar) override;
		bool CreateFromBitmapFile(const tString &asFileName) override;

		bool SetVectorFont(const tString &asFileName, float afSize, float afWidth, float afPixels) override;

	protected:
		cFontSizedGlyph* GetSizedGlyph(wchar_t alChar, int alLineHeight, float afStretch,
										eFontEffect aEffect, int alEffectWidth) override;

	private:
		cFontSizedGlyph* CreateSizedGlyph(const cSDLFontSizedGlyphKey &aKey);

		FT_FaceRec_ *mpFace;
		float mfVectorSize;
		float mfVectorWidth;
		float mfVectorPixels;

		tSDLFontSizedGlyphMap m_mapSizedGlyphs;

	};

};
#endif // HPL_FONTDATA_H
