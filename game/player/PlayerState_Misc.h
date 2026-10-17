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
#ifndef GAME_PLAYER_STATE_MISC_H
#define GAME_PLAYER_STATE_MISC_H

#include "core/StdAfx.h"
#include "player/PlayerState.h"

using namespace hpl;

//////////////////////////////////////////////////////////////////////////
// NORMAL STATE
//////////////////////////////////////////////////////////////////////////

class cPlayerState_Normal : public iPlayerState
{
public:
	cPlayerState_Normal(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	
	void OnStartInteract() override;
	void OnStartExamine() override;
	
	void OnStartRun() override;
	void OnStopRun() override;
	void OnStartCrouch() override;
	void OnStopCrouch() override;
	
	void OnStartInteractMode() override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
};
//////////////////////////////////////////////////////////////////////////
// INTERACT MODE STATE
//////////////////////////////////////////////////////////////////////////

class cPlayerState_InteractMode : public iPlayerState
{
public:
	cPlayerState_InteractMode(cInit *apInit,cPlayer *apPlayer);

	void OnUpdate(float afTimeStep) override;
	
	void OnStartInteract() override;
	void OnStartExamine() override;
	
	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;
	
	void OnStartInteractMode() override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
	void OnStartCrouch() override;
	void OnStopCrouch() override;

private:
	ePlayerMoveState mPrevMoveState;

	cVector2f mvLookSpeed;
	float mfRange;

};

//////////////////////////////////////////////////////////////////////////
// USE ITEM STATE
//////////////////////////////////////////////////////////////////////////

class cPlayerState_UseItem : public iPlayerState
{
private:
	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;

public:
	cPlayerState_UseItem(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	
	void OnStartInteract() override;
	void OnStartExamine() override;
	
	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
	void OnStartCrouch() override;
	void OnStopCrouch() override;
	
	bool OnStartInventory() override;
};


//////////////////////////////////////////////////////////////////////////
// MESSAGE STATE
//////////////////////////////////////////////////////////////////////////

class cPlayerState_Message : public iPlayerState
{
private:
	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;

public:
	
	cPlayerState_Message(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	
	bool OnJump() override;
	
	void OnStartInteract() override;
	void OnStopInteract() override;
	void OnStartExamine() override;
	
	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;

	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;

	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
	bool OnStartInventory() override;
	bool OnStartInventoryShortCut(int alNum) override;
};


//////////////////////////////////////////////////////////////////////////
// CLIMB
//////////////////////////////////////////////////////////////////////////
class cGameLadder;
class cPlayerState_Climb : public iPlayerState
{
private:
	int mlState;

	float mfAttachRemainingSeconds;

	cVector3f mvCharPosition;

	cVector3f mvGoalPos;
	cVector3f mvGoalRot;

	cVector3f mvAttachVelocity;
	cVector3f mvAttachAngularVelocity;

	float mfUpSpeed;
	float mfDownSpeed;

	float mfStepLength;
	float mfStepTravelDistance;
	bool mbPlayedSound;

	cVector2f mvPrevPitchLimits;

	float mfTopExitRemainingSeconds;

    void PlaySound(const tString &asSound);
public:
	cPlayerState_Climb(cInit *apInit,cPlayer *apPlayer);

	void OnUpdate(float afTimeStep) override;

	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;

	void OnStartInteract() override;
	void OnStartExamine() override;

	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;

	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;
	
	void OnStartCrouch() override;
	void OnStopCrouch() override;

	bool OnJump() override;

	bool OnStartInventory() override;

	cGameLadder* mpLadder;
	cVector3f mvStartPosition;
};




#endif // GAME_PLAYER_STATE_MISC_H
