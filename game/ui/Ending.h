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
#ifndef GAME_ENDING_H
#define GAME_ENDING_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

//-------------------------------------------------

// Black Plague's endings: e-mails typed out on a black screen (the "Ending"
// texts of the language file). Ending 1 shows two and goes back to the
// game; ending 2 three, then "message sent" blinks and the credits come.
// Written after the binary.
class cEnding : public iUpdateable
{
public:
	cEnding(cInit *apInit);
	~cEnding() override;

	void Reset() override;

	void OnPostSceneDraw() override;
	void OnDraw() override;
	void Update(float afTimeStep) override;
	void OnExit() override;

	void OnMouseDown(eMButton aButton);
	void OnButtonDown();

	/**
	 * alType: 0 ending 1 (Mail01, Mail02), 3 ending 2 (Mail03 - Mail05).
	 * Switched off with 1 it goes back to the game, with anything else to the credits.
	 */
	void SetActive(bool abActive, int alType);
	bool IsActive(){ return mbActive;}

private:
	void SetCurrentText(const tString &asEntry);
	void UpdateText(float afTimeStep);
	void DrawText();

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;
	iFontData *mpFont;

	int mlState;
	bool mbTextActive;
	tWStringVec mvRows;

	int mlTextLength;
	int mlShownChars;
	float mfCharacterDelaySeconds;
	float mfTypeSoundDelaySeconds;

	float mfTextColor;
	float mfWaitElapsedSeconds;
	float mfAlpha;

	// "Message sent" blinking
	float mfBlinkMax;
	float mfBlinkMin;
	float mfBlinkAlpha;
	float mfBlinkUpSpeed;
	float mfBlinkDownSpeed;
	float mfBlinkSpeed;

	bool mbActive;
};

//---------------------------------------------

#endif // GAME_ENDING_H
