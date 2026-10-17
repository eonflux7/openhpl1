/* This file is part of OpenHPL1, licensed under the GNU GPL v3 or later. */
#ifndef HPL_COMPARISON_IMAGE_H
#define HPL_COMPARISON_IMAGE_H

#include "math/MathTypes.h"
#include "system/SystemTypes.h"

namespace hpl {
	struct cComparisonImage
	{
		cVector2l mvSize;
		std::vector<unsigned char> mvPixels;
	};
	bool ComposeComparisonImage(cComparisonImage& aImage, const cVector2l& avSize,
		const std::vector<unsigned char>& avVanilla, const std::vector<unsigned char>& avImproved);

	// Full-size RGB24 frames stacked vertically, with Vanilla above Improved
	// and labels above each image. Rejects mismatched frames.
	bool SaveComparisonPNG(const tString& asFile, const cVector2l& avSize,
		const std::vector<unsigned char>& avVanilla, const std::vector<unsigned char>& avImproved);
}
#endif
