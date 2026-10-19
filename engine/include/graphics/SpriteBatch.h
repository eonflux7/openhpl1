// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
#ifndef HPL_SPRITE_BATCH_H
#define HPL_SPRITE_BATCH_H

#include <array>
#include "graphics/GraphicsTypes.h"

namespace hpl {

	class iLowLevelGraphics;
	class iTexture;

	// Explicit blend state belongs to a batch, including whether blending is enabled.
	struct cSpriteBlend
	{
		bool mbActive;
		eBlendFunc mSrc, mDest;
		cSpriteBlend(eBlendFunc aSrc=eBlendFunc_One, eBlendFunc aDest=eBlendFunc_Zero, bool abActive=true)
			: mbActive(abActive), mSrc(aSrc), mDest(aDest) {}
		bool operator==(const cSpriteBlend& aOther) const
		{ return mbActive == aOther.mbActive && mSrc == aOther.mSrc && mDest == aOther.mDest; }
	};

	struct cSpriteVertex
	{
		cVector3f pos;
		cColor col;
		cVector3f tex;
		cSpriteVertex() = default;
		cSpriteVertex(const cVector3f& avPos, const cColor& aCol, const cVector3f& avTex)
			: pos(avPos), col(aCol), tex(avTex) {}
		explicit cSpriteVertex(const cVertex& aVertex) : pos(aVertex.pos), col(aVertex.col), tex(aVertex.tex) {}
	};

	// Adds triangles in submission order, merging only adjacent compatible sprites.
	// Flush before changing matrices, clipping, depth/culling, target or program.
	// Storage grows with demand; there is no fixed vertex/index array to overflow.
	class cSpriteBatch
	{
	public:
		void Add(iTexture* apTexture, const cSpriteBlend& aBlend, const std::array<cSpriteVertex,4>& avCorners);
		void Add(iTexture* apTexture, const cSpriteBlend& aBlend, const tVertexVec& avCorners);
		void Flush(iLowLevelGraphics* apGraphics);
	private:
		struct cBatch
		{
			iTexture* mpTexture;
			cSpriteBlend mBlend;
			size_t mlFirst, mlCount;
		};
		std::vector<cSpriteVertex> mvVertices;
		std::vector<cBatch> mvBatches;
	};
}
#endif
