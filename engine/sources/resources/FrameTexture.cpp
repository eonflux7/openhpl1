// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/FrameTexture.h"
#include "graphics/Texture.h"

namespace hpl {

	cFrameTexture::cFrameTexture(iTexture *apTex, int alHandle) : iFrameBase()
	{
		mpTexture = apTex;
		mlHandle =alHandle;
	}

	cFrameTexture::~cFrameTexture()
	{
		if(mpTexture) hplDelete(mpTexture);
		mpTexture = nullptr;
	}

	iTexture* cFrameTexture::GetTexture()
	{
		return mpTexture;
	}
}
