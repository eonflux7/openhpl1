// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_DEMO_END_TEXT_H
#define GAME_DEMO_END_TEXT_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

class cDemoEndText : public iUpdateable
{
public:
	cDemoEndText(cInit *apInit);
	~cDemoEndText() override;

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
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;

	bool mbActive;

	float mfImageBrightness;
	float mfBrightnessRatePerSecond;

	int mlCurrentImage;

	std::vector<iTexture*> mvTextures;
};

#endif // GAME_DEMO_END_TEXT_H
