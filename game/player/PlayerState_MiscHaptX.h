// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_PLAYER_STATE_MISC_HAPTX_H
#define GAME_PLAYER_STATE_MISC_HAPTX_H

#include "core/StdAfx.h"
#include "player/PlayerState.h"

using namespace hpl;

// NORMAL STATE

class cPlayerState_NormalHaptX : public iPlayerState
{
private:
	
public:
	cPlayerState_NormalHaptX(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	
	void OnStartInteract() override;
	void OnStartExamine() override;
	
	void OnStartRun() override;
	void OnStopRun() override;
	void OnStartCrouch() override;
	void OnStopCrouch() override;

	bool OnAddYaw(float afVal) override{ return true;}
	bool OnAddPitch(float afVal) override{return true;}
	
	void OnStartInteractMode() override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;

	void OnPostSceneDraw() override;
	
};
// INTERACT MODE STATE

class cPlayerState_InteractModeHaptX : public iPlayerState
{
public:
	cPlayerState_InteractModeHaptX(cInit *apInit,cPlayer *apPlayer);

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

// USE ITEM STATE

class cPlayerState_UseItemHaptX : public iPlayerState
{
private:
	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;

public:
	cPlayerState_UseItemHaptX(cInit *apInit,cPlayer *apPlayer);
	
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


// MESSAGE STATE

class cPlayerState_MessageHaptX : public iPlayerState
{
private:
	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;

public:
	
	cPlayerState_MessageHaptX(cInit *apInit,cPlayer *apPlayer);
	
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


// CLIMB
class cGameLadder;
class cPlayerState_ClimbHaptX : public iPlayerState
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
	cPlayerState_ClimbHaptX(cInit *apInit,cPlayer *apPlayer);

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
