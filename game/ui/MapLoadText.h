// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_MAP_LOAD_TEXT_H
#define GAME_MAP_LOAD_TEXT_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"
#include "core/GraphicsHelper.h"

using namespace hpl;

class cInit;

class cMapLoadText : public iUpdateable
{
public:
	cMapLoadText(cInit *apInit);
	~cMapLoadText() override;

	void Reset() override;

	void OnPostSceneDraw() override;
	
	void OnDraw() override;
    
	void Update(float afTimeStep) override;
	
	void OnMouseDown(eMButton aButton);
	void OnMouseUp(eMButton aButton);

	void SetText(tString asCat, tString asEntry){
		msTextCat = asCat;
		msTextEntry =asEntry;
	}

	// Black Plague: the picture behind the text (Overture's is fixed)
	void SetBackground(const tString &asFile){ msBackground = asFile; }

	void SetActive(bool abActive);
	bool IsActive(){ return mbActive;}

	void OnExit() override;
private:
	void DrawText(bool abLoading);

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;
	iFontData *mpTextFont;

	tWStringVec mvRows;

	cScreenImage mBackground;

	tString msTextCat;
	tString msTextEntry;
	tString msBackground;

	float mfPulsePhase;
	float mfPulseRatePerSecond;

	bool mbActive;
};

#endif // GAME_MAP_LOAD_TEXT_H
