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
#ifndef GAME_PRE_MENU_H
#define GAME_PRE_MENU_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cRaindrop
{
public:
	cVector2f	vPos;
	cVector2f	vDir;
	float		fLength;
	float		fColor;
	
	cGfxObject *mpGfx;
};

class cInit;

//-------------------------------------------------

class cPreMenu : public iUpdateable
{
public:
	cPreMenu(cInit *apInit);
	~cPreMenu() override;

	void Reset() override;

	void OnPostSceneDraw() override;
	
	void OnDraw() override;
    
	void Update(float afTimeStep) override;
	
	void OnMouseDown(eMButton aButton);
	void OnButtonDown();

	void SetActive(bool abActive);
	bool IsActive(){ return mbActive;}

	void OnExit() override;
private:
	void LoadConfig();

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	std::vector<iTexture*> mvTextures;
	tStringVec mvTexNames;

	iTexture* mpLogoTexture;
	iTexture* mpEpTexture;

	iFontData *mpFont;
	iFontData *mpTextFont;

	bool mbShowText;
	tWStringVec mvTextRows;
	int mlCurrentTextChar;
	int mlMaxChars;
	float mfNewCharCount;
	float mfTypeSoundDelaySeconds;

	float mfStartupLogoBrightness;
	float mfLogoBrightnessRatePerSecond;

	int mlState;
	int mlCurrentLogo;

	bool mbActive;

	float mfStateElapsedSeconds;

	cVector3f mvecLastTextPos;
	
	float mfLastTextSize;
	float mfLastTextColor;
	float mfLastTextSpeed1;
	float mfLastTextSpeed2;

	bool mbFadeWindSound;
	bool mbFadeRainSound;
	bool mbPlayingWindSound;
	bool mbPlayingRainSound;
	bool mbPlayingThunderSound;
	bool mbPlayingMusic;

	float mfFontColor;
	
	bool mbFlash;

	float mfLogoSizeFactor;
	
	float mfRaindropFade;

	cVector3f mvecLogoSize;
	float	mfLogoFade;
	float	mfEpFade;
	float	mfLogoScaleStep;

	iSoundChannel* mpRainSound;
	iSoundChannel* mpWindSound;

	std::vector<cRaindrop> mvRaindropVector;
	cGfxObject*		mpRaindropGfx;
	cGfxObject*		mpFlashGfx;

};

//---------------------------------------------


#endif // GAME_PRE_MENU_H
