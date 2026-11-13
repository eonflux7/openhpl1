// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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
