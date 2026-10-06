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
#ifndef GAME_CREDITS_H
#define GAME_CREDITS_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

//-------------------------------------------------

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

	void SetActive(bool abX);
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
	void SetSecretText(int alNum);
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
	float mfPageTime;

	float mfBlink;
	float mfBlinkSpeed;

	int mlSecretText;
	tWStringVec mvSecretRows;
	int mlSecretChars;
	int mlSecretCharsShown;
	float mfCharTime;
	float mfTypeSoundTime;
		
	bool mbActive;
};

//---------------------------------------------


#endif // GAME_CREDITS_H
