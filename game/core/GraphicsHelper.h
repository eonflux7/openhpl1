/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GAME_GRAPHICS_HELPER_H
#define GAME_GRAPHICS_HELPER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;


//---------------------------
class cGraphicsHelper
{
public:
	cGraphicsHelper(cInit *apInit);
	~cGraphicsHelper();

	void ClearScreen(const cColor &aColor);
	void DrawTexture(iTexture *apTex, const cVector3f& avPos, const cVector3f& avSize, const cColor &aColor);
	// Black Plague: blended with what is on the screen
	void DrawTexture(iTexture *apTex, const cVector3f& avPos, const cVector3f& avSize,
					const cColor &aColor, eBlendFunc aSrcBlend, eBlendFunc aDestBlend);
	
	void DrawLoadingScreen(const tString &asFile, bool abDrawLoadingText = true);

	// The whole screen in the 800x600 virtual coordinates. The 800x600 layout
	// is centred, so in a window that isn't 4:3 the screen starts left of (or
	// above) 0 and is larger than 800x600.
	cVector3f GetScreenPos(float afZ=0);
	cVector2f GetScreenSize();
	cVector2f GetScreenOffset();
	void ClampToScreen(cVector2f &avPos);

	void SwapBuffers();

private:
	tVertexVec mvVtx;
	
	cInit *mpInit;
	iLowLevelGraphics *mpLowLevelGfx;
	cTextureManager *mpTexManager;
	
    cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;
};

//---------------------------------------------


#endif // GAME_GRAPHICS_HELPER_H
