// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_FADE_HANDLER_H
#define GAME_FADE_HANDLER_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

class cFadeHandler : public iUpdateable
{
public:
	cFadeHandler(cInit *apInit);
	~cFadeHandler() override;

	void FadeOut(float afDurationSeconds);
	void FadeIn(float afDurationSeconds);
	bool IsActive();

	void SetWideScreenActive(bool abActive);

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;
	void OnDraw() override;
	
private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpBlackGfx;

	bool mbActive;
	float mfAlpha;
	float mfAlphaRatePerSecond;

	bool mbWideScreenActive;
	float mfWideScreenAlpha;
};


#endif // GAME_FADE_HANDLER_H
