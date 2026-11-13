// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_CREDITS_H
#define GAME_CREDITS_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

class cCredits : public iUpdateable
{
public:
	cCredits(cInit *apInit);
	~cCredits() override;

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

	// Black Plague and Requiem: the pages after the scrolling text
	void DrawStatistics();
	void DrawStatRow(float afY, const tString &asLabel, const tWString &asValue);
	void NextPage();

	// Requiem's secret: "Incoming transmission..." blinking, then a message
	// typed out text by text, then the code
	void UpdateRequiemSecret(float afTimeStep);
	void SetSecretText(int alTextIndex);
	void DrawSecretText();

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;

	tWStringVec mvTextRows;
	float mfYPos;

	// 0: the text scrolling, 1: statistics, then for all artefacts found
	// Black Plague's secret (2: the message, 3: the code) or Requiem's
	// (2: the transmission, 3: the typed texts, 4: the code)
	int mlPage;
	float mfPageElapsedSeconds;

	float mfBlink;
	float mfBlinkSpeed;

	int mlSecretTextIndex;
	tWStringVec mvSecretRows;
	int mlSecretChars;
	int mlSecretCharsShown;
	float mfCharacterDelaySeconds;
	float mfTypeSoundDelaySeconds;
		
	bool mbActive;
};

#endif // GAME_CREDITS_H
