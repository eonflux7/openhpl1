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
#ifndef GAME_BUTTON_HANDLER_H
#define GAME_BUTTON_HANDLER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;
class cPlayer;

class cButtonHandler : public iUpdateable
{
public:
	cButtonHandler(cInit *apInit);
	~cButtonHandler() override;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void OnInterpolate(float afT) override;
	void Reset() override;
	void OnExit() override;
	void OnPostSceneDraw() override;

	void SetDefaultKeys();

	tString GetActionName(const tString &asInputName,const tString &asSkipAction);

	void ChangeState(eButtonHandlerState aState);
	eButtonHandlerState GetState(){ return mState;}

	bool GetInvertMouseY(){ return mbInvertMouseY;}
	bool GetToggleCrouch(){ return mbToggleCrouch;}
	
	//Settings
	float mfMouseSensitivity;
	bool mbInvertMouseY;
	bool mbToggleCrouch;
private:
	// What the mouse did to the camera in a logic step, for frames drawn
	// between steps (see OnInterpolate).
	unsigned int mlLookStep;
	cVector2f mvLookDelta;
	cVector2f mvLookPerRel;
	int mlLookPlayerState;

	iAction * ActionFromTypeAndVal(const tString& asName,const tString& asType, const tString& asVal);
	void TypeAndValFromAction(iAction *apAction, tString *apType, tString *apVal);
	
	int mlNumOfActions;

	cInit *mpInit;

	cInput* mpInput;
	iLowLevelGraphics *mpLowLevelGraphics;
	iLowLevelHaptic *mpLowLevelHaptic;

	cPlayer *mpPlayer;

	eButtonHandlerState mState;
};



#endif // GAME_BUTTON_HANDLER_H
