// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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

#endif // GAME_PRE_MENU_H
