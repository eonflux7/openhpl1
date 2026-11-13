// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_FRAMETEXTURE_H
#define HPL_FRAMETEXTURE_H

#include "resources/FrameBase.h"

namespace hpl {

	class iTexture;

	class cFrameTexture : public iFrameBase
	{
	public:
		cFrameTexture(iTexture *pTex, int alHandle);
		~cFrameTexture();

		iTexture* GetTexture();
		int GetHandle(){return mlHandle;}

	private:
		iTexture *mpTexture;
		int mlHandle;
	};

};
#endif // HPL_FRAMETEXTURE_H
