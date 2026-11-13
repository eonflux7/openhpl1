// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GRAPHICS_HELPER_H
#define GAME_GRAPHICS_HELPER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

// A full-screen image: a menu's logo or background, or a loading screen
struct cScreenImage
{
	iTexture *mpTexture = nullptr;
	// A remake from <exe dir>/hud/screens/ (see hud/README.md): as high as the
	// rectangle the game draws the original in and wider, at the same pixel
	// aspect, with that rectangle in its middle, so it fills a wide screen
	bool mbWide = false;
};

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

	// The game's image, or its wide remake while the high-resolution HUD is on
	cScreenImage LoadScreenImage(const tString &asFile);
	void DestroyScreenImage(cScreenImage &aImage);
	// Draws the image where the game draws the original; a wide remake
	// reaches past the rectangle's sides by its own width
	void DrawScreenImage(const cScreenImage &aImage, const cVector3f& avPos, const cVector2f& avSize);
	// A loading screen: a remake fills the 800x600 layout and its sides; the
	// original is stretched over the whole screen
	void DrawFullScreenImage(const cScreenImage &aImage, float afZ=0);

	// The whole screen in the 800x600 virtual coordinates. The 800x600 layout
	// is centred, so in a window that isn't 4:3 the screen starts left of (or
	// above) 0 and is larger than 800x600.
	cVector3f GetScreenPos(float afZ=0);
	cVector2f GetScreenSize();
	cVector2f GetScreenOffset();
	void ClampToScreen(cVector2f &avPos);

	void SwapBuffers();

private:
	void DrawTextureRect(iTexture *apTex, const cVector3f& avPos, const cVector3f& avSize,
						const cVector2f& avUVMin, const cVector2f& avUVMax, const cColor &aColor,
						eBlendFunc aSrcBlend, eBlendFunc aDestBlend);

	tVertexVec mvVtx;
	
	cInit *mpInit;
	iLowLevelGraphics *mpLowLevelGfx;
	cTextureManager *mpTexManager;
	
    cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;
	/// The key drawn behind key names in text (iFontData::SetKeyCapGfx)
	cGfxObject *mpKeyCap;
};

#endif // GAME_GRAPHICS_HELPER_H
