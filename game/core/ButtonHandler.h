// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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

	// For tests (OPENHPL_SCRIPT "move:f,s"): movement held as if by the keys,
	// -1..1 forwards and sideways. A key that is down wins.
	static float mfTestForward;
	static float mfTestSideways;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void OnInterpolate(float afT) override;

	/**
	 * How far to draw a cursor from its position, for frames drawn between
	 * two logic steps: the mouse movement that has arrived since the last
	 * step, the way that step moved the cursor. Zero with the frame rate
	 * limited.
	 * \param abMenu the main menu's cursor, else the player's crosshair
	 * (inventory, notebook, panels, interact mode).
	 */
	cVector2f GetCursorDrawOffset(bool abMenu);
	void Reset() override;
	void OnExit() override;
	void OnPostSceneDraw() override;

	void SetDefaultKeys();

	tString GetActionName(const tString &asInputName,const tString &asSkipAction);

	/**
	 * The short name of a key or mouse button ("L Ctrl", "Esc", "L Mouse"),
	 * from an action's input name, for tips and the inventory.
	 */
	tWString GetKeyLabel(const tString &asInputName);
	/**
	 * The key or button of an action, as a key cap in text (see
	 * kwcKeyCapStart). The language's "None" if the action has none.
	 */
	tWString GetActionKeyCap(const tString &asAction);

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

	// The same for the cursors: index 0 is the main menu's, 1 the crosshair.
	cVector2f ReadMouseRel();
	void BeginCursorStep();
	void EndCursorStep();
	unsigned int mlCursorStep;
	cVector2f mvCursorRel;
	cVector2f mvCursorStart[2];
	cVector2f mvCursorPerRel[2];

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
