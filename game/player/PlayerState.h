// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_PLAYER_STATES_H
#define GAME_PLAYER_STATES_H

#include "core/ButtonHandler.h"
#include "systems/GameMessageHandler.h"
#include "entities/GameItemType.h"
#include "entities/GameObject.h"
#include "ui/Inventory.h"
#include "ui/Notebook.h"
#include "ui/NumericalPanel.h"
#include "ui/DeathMenu.h"
#include "player/PlayerHands.h"

// BASE STATE

class iPlayerState
{
public:
	ePlayerState mType;
	ePlayerState mPreviousState;

	iPlayerState(cInit *apInit,cPlayer *apPlayer,ePlayerState aType)
	{
		mpInit = apInit;

        mpPlayer = apPlayer;
		mType = aType;

		mPreviousState = ePlayerState_LastEnum;
	}

	virtual ~iPlayerState(){}

	void InitState(iPlayerState* apPrevState)
	{
		if(apPrevState){
			apPrevState->LeaveState(this);
		}

		EnterState(apPrevState);
		mPreviousState = apPrevState->mType;
	}

	virtual void OnUpdate(float afTimeStep){}
	virtual void OnPostSceneDraw(){}
	virtual void OnDraw(){}

	/**
	 * Where the crosshair is drawn in a frame between two logic steps
	 * (cRenderInterpolation), if the state places it on something in the
	 * world; false for where the logic put it.
	 */
	virtual bool GetFrameCrossHairPos(cVector2f &avPos){ return false; }

	virtual void OnStartInteract(){}
	virtual void OnStopInteract(){}
	virtual void OnStartExamine(){}
	virtual void OnStopExamine(){}
	virtual void OnStartHolster(){}

	virtual bool OnJump(){ return true;}

	virtual void OnStartRun(){}
	virtual void OnStopRun(){}

	// Black Plague: the Rotate key held
	virtual void OnStartRotate(){}
	virtual void OnStopRotate(){}

	virtual void OnStartCrouch(){}
	virtual void OnStopCrouch(){}

	virtual void OnStartInteractMode(){}

	virtual bool OnStartInventory(){return true;}

	virtual bool OnStartInventoryShortCut(int alNum){ return true;}

	virtual bool OnMoveForwards(float afMul, float afTimeStep){return true;}
	virtual bool OnMoveSideways(float afMul, float afTimeStep){return true;}

	virtual bool OnAddYaw(float afVal){ return true;}
	virtual bool OnAddPitch(float afVal){return true;}

	virtual void EnterState(iPlayerState* apPrevState){}
	virtual void LeaveState(iPlayerState* apNextState){}

protected:
	cInit *mpInit;
	cPlayer* mpPlayer;

};

#endif // GAME_PLAYER_STATES_H
