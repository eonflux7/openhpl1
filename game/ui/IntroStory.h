// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_INTRO_STORY_H
#define GAME_INTRO_STORY_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

#define INTRO_IMAGE_NUM (6)

class cIntroImage
{
public:
	cIntroImage();
	~cIntroImage();

	void Update(float afTimeStep);

	void OnDraw();

	void FadeBrightnessTo(float afBrightness,float afDurationSeconds);
	void FadeAlphaTo(float afAlpha,float afDurationSeconds);

	void MoveTo(const cVector3f &avPos,float afDurationSeconds);

	void Reset();
	
	//Variables
    cVector3f mvCameraPosition;
	cVector3f mvFinalPos;
	cVector3f mvVelocity;
	float mfPosCount;
	cVector3f mvPosDistMul;

	tVector3fList mlstPrevPos;
	int mlMaxPrevPos;
	
	iTexture *mpTexture;

	bool mbActive;

	float mfBrightness;
	float mfBrightnessRatePerSecond;
	float mfFinalBrightness;

	float mfAlpha;
	float mfAlphaRatePerSecond;
	float mfFinalAlpha;


	tVertexVec mvVtxVec;

	cInit *mpInit;
	iLowLevelGraphics *mpLowGfx;
	cTextureManager *mpTexManager;
};

class cIntroStory : public iUpdateable
{
public:
	cIntroStory(cInit *apInit);
	~cIntroStory() override;

	void Update(float afTimeStep) override;
	void Reset() override;
	void OnDraw() override;
	void OnPostSceneDraw() override;

	void Exit();
	
	bool IsActive(){ return mbActive;}
	void SetActive(bool abActive);
    
private:
	void SetMessage(const tWString &asMess);
	
	void Image00();
	void Image01();
	void Image02();
	void Image03();
	void Image04();
	void Image05();
	
	bool mbSearchNext;

	cInit *mpInit;
	iLowLevelGraphics *mpLowGfx;
	cTextureManager *mpTexManager;
	cSoundHandler *mpSoundHandler;

	iTexture *mpBlackTexture;

	cIntroImage mvImages[INTRO_IMAGE_NUM];

	float mfCueRemainingSeconds;
	int mlNextStop;

	float mfVoiceVol;

	iFontData *mpFont;

	bool mbActive;

	tWString msMessage;
	tWString msCentreText;

	eButtonHandlerState mLastButtonState;
};

#endif // GAME_INTRO_STORY_H
