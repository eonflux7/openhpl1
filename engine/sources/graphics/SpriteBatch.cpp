// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
#include "graphics/SpriteBatch.h"
#include "graphics/LowLevelGraphics.h"
#include <cassert>

namespace hpl {

	void cSpriteBatch::Add(iTexture* apTexture, const cSpriteBlend& aBlend,
						  const std::array<cSpriteVertex,4>& avCorners)
	{
		if(mvBatches.empty() || mvBatches.back().mpTexture != apTexture || !(mvBatches.back().mBlend == aBlend))
			mvBatches.push_back({apTexture, aBlend, mvVertices.size(), 0});
		for(int i : {0,1,2,0,2,3}) mvVertices.push_back(avCorners[i]);
		mvBatches.back().mlCount += 6;
	}

	void cSpriteBatch::Add(iTexture* apTexture, const cSpriteBlend& aBlend, const tVertexVec& avCorners)
	{
		assert(avCorners.size() == 4);
		std::array<cSpriteVertex,4> corners;
		for(int i=0; i<4; ++i) corners[i] = cSpriteVertex(avCorners[i]);
		Add(apTexture, aBlend, corners);
	}

	void cSpriteBatch::Flush(iLowLevelGraphics* apGraphics)
	{
		for(const cBatch& batch : mvBatches)
			apGraphics->DrawSpriteBatch(mvVertices.data() + batch.mlFirst, batch.mlCount, batch.mpTexture, batch.mBlend);
		mvVertices.clear();
		mvBatches.clear();
	}
}
