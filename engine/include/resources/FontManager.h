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
#ifndef HPL_FONT_MANAGER_H
#define HPL_FONT_MANAGER_H

#include <map>
#include "resources/ResourceManager.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class cGui;
	class iFontData;

	class cFontManager : public iResourceManager
	{
	public:
		cFontManager(cGraphics* apGraphics,cGui *apGui,cResources *apResources);
		~cFontManager() override;

		iResourceBase* Create(const tString& asName) override;
		/**
		 * Create a new font
		 * \param asName name of the font
		 * \param alSize size the characters are rendered in
		 * \param alFirstChar first ASCII character to be rendered
		 * \param alLastChar last ASCII character to be rendered
		 * \return
		 */
		iFontData* CreateFontData(const tString& asName, int alSize=16,unsigned short alFirstChar=32,
								unsigned short alLastChar=255);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

		/**
		 * Load a list of vector fonts that replace the glyphs of bitmap fonts
		 * created after this (see iFontData::SetVectorFont). The list is the
		 * file fonts.cfg in the directory, with the font files next to it.
		 * \param asDir directory, ending with a slash
		 * \return false if there is no list
		 */
		bool LoadVectorFonts(const tString& asDir);

		/**
		 * True if the list has a vector font for asName. A name in it with a
		 * Base attribute is a font of its own: the Base .fnt's metrics drawn
		 * with another font file (e.g. a bold one).
		 */
		bool HasVectorFont(const tString& asName) const;

	private:
		class cVectorFont
		{
		public:
			tString msFile;
			tString msBase; // For a name the game has no .fnt for: the .fnt to use
			float mfSize;
			float mfWidth;
			float mfPixels;
		};
		std::map<tString, cVectorFont> m_mapVectorFonts;

		cGraphics* mpGraphics;
		cResources *mpResources;
		cGui *mpGui;
	};

};
#endif // HPL_FONT_MANAGER_H
