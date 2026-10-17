/* This file is part of OpenHPL1, licensed under the GNU GPL v3 or later. */
#include "impl/ComparisonImage.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace hpl {
	// Five-column, seven-row lettering keeps screenshots independent of
	// installed fonts and game assets. Bits run from the top row down.
	static void DrawLabel(cComparisonImage& aImage, const char *asLabel, int alX, int alY, int alScale)
	{
		static const char sLetters[] = "VANILMPROED";
		static const unsigned char vGlyphs[][5] = {
			{0x1f,0x20,0x40,0x20,0x1f}, {0x7e,0x09,0x09,0x09,0x7e},
			{0x7f,0x02,0x04,0x08,0x7f}, {0x41,0x41,0x7f,0x41,0x41},
			{0x7f,0x40,0x40,0x40,0x40}, {0x7f,0x02,0x0c,0x02,0x7f},
			{0x7f,0x09,0x09,0x09,0x06}, {0x7f,0x09,0x19,0x29,0x46},
			{0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x49,0x49,0x49,0x41},
			{0x7f,0x41,0x41,0x41,0x3e}};
		for(const char *p = asLabel; *p; ++p, alX += 6*alScale)
		{
			const char *pLetter = std::strchr(sLetters, *p);
			if(!pLetter) continue;
			for(int x=0; x<5; ++x)
				for(int y=0; y<7; ++y)
					if(vGlyphs[pLetter-sLetters][x] & (1 << y))
					{
						for(int dy=0; dy<alScale; ++dy)
							for(int dx=0; dx<alScale; ++dx)
							{
								const size_t lPixel = ((size_t)(alY+(y+4)*alScale+dy)*aImage.mvSize.x +
									alX+x*alScale+dx)*3;
								aImage.mvPixels[lPixel] = 240;
								aImage.mvPixels[lPixel+1] = 242;
								aImage.mvPixels[lPixel+2] = 246;
							}
					}
		}
	}

	bool ComposeComparisonImage(cComparisonImage& aImage, const cVector2l& avSize,
		const std::vector<unsigned char>& avVanilla, const std::vector<unsigned char>& avImproved)
	{
		const int lMinWidth = 52;
		const int lScale = std::clamp(std::min(avSize.y/360, avSize.x/lMinWidth), 1, 4);
		const int lHeader = 15*lScale;
		if(avSize.x < lMinWidth || avSize.y <= 0 || avSize.x > std::numeric_limits<int>::max()/3 ||
			avSize.y > std::numeric_limits<int>::max()/2-lHeader)
			return false;
		const size_t lStride = (size_t)avSize.x * 3;
		if(avVanilla.size() != lStride*avSize.y || avImproved.size() != avVanilla.size())
			return false;

		aImage.mvSize = cVector2l(avSize.x, (avSize.y+lHeader)*2);
		aImage.mvPixels.resize((size_t)aImage.mvSize.x*aImage.mvSize.y*3);
		for(size_t i=0; i<aImage.mvPixels.size(); i+=3)
		{
			aImage.mvPixels[i] = 22;
			aImage.mvPixels[i+1] = 25;
			aImage.mvPixels[i+2] = 31;
		}
		for(int y=0; y<avSize.y; ++y)
		{
			auto *pRow = aImage.mvPixels.data() + (size_t)(y+lHeader)*lStride;
			std::memcpy(pRow, avVanilla.data() + y*lStride, lStride);
			std::memcpy(pRow + (size_t)(avSize.y+lHeader)*lStride,
				avImproved.data() + y*lStride, lStride);
		}
		DrawLabel(aImage, "VANILLA", 4*lScale, 0, lScale);
		DrawLabel(aImage, "IMPROVED", 4*lScale, avSize.y+lHeader, lScale);
		return true;
	}
}
