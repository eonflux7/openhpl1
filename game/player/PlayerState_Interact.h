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
#ifndef GAME_PLAYER_STATE_INTERACT_H
#define GAME_PLAYER_STATE_INTERACT_H

#include "core/StdAfx.h"
#include "player/PlayerState.h"

using namespace hpl;

//-----------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// GRAB STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------

class cPlayerState_Grab : public iPlayerState
{
public:
	cPlayerState_Grab(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	
	void OnDraw() override;
	
	void OnPostSceneDraw() override;
	
	bool OnJump() override;
		
	void OnStartInteractMode() override;
	
	void OnStartInteract() override;
	void OnStopInteract() override;
	void OnStartExamine() override;
	
	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;
	
	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
	void OnStartCrouch() override;
	void OnStopCrouch() override;
	bool OnStartInventory() override;
	bool OnStartInventoryShortCut(int alNum) override;

	// Black Plague: turning the object with the Rotate key held, running
	// while holding it, and the flare put out by Holster
	void OnStartRotate() override;
	void OnStopRotate() override;
	void OnStartRun() override;
	void OnStopRun() override;
	void OnStartHolster() override;

	static float mfMassDiv;
private:
	void UpdateRotation(float afTimeStep, const cVector3f &avOmega);

	cVector3f mvRelPickPoint;
	iPhysicsBody *mpPushBody;

	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;

	cSpringVec3 mGrabSpring;
	cPidControllerVec3 mGrabPid;
	cSpringVec3 mRotateSpring;
	cPidControllerVec3 mRotatePid;

	bool mbHasGravity;

	bool mbMoveHand;

	float mfGrabDist;

	float mfDefaultMass;

	bool mbHasPlayerGravityPush;

	bool mbPickAtPoint;
	bool mbRotateWithPlayer;

	cVector3f mvObjectUp;
	cVector3f mvObjectRight;

	cVector3f mvCurrentUp;
	cVector3f mvCurrentUpAxis;

	float mfStartYaw;

	cVector3f mvCurrentDisered;

	float mfSpeedMul;

	// How much the object is still to turn, about the world's axes. Overture
	// only uses y, which follows the player turning; Black Plague adds the
	// mouse moved with Rotate held.
	cVector3f mvRotation;
	bool mbRotating;

	float mfMaxPidForce;

	float mfMinThrowMass;
	float mfMaxThrowMass;

	float mfMinThrowImpulse;
	float mfMaxThrowImpulse;

	iFontData *mpFont;
};

//-----------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// MOVE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------

class cPlayerState_Move_BodyCallback : public iPhysicsBodyCallback
{
public:
	cPlayerState_Move_BodyCallback(cPlayer *apPlayer, float afTimeStep);
	
	bool OnBeginCollision(iPhysicsBody *apBody, iPhysicsBody *apCollideBody) override;
	void OnCollide(iPhysicsBody *apBody, iPhysicsBody *apCollideBody,cPhysicsContactData* apContactData) override;
	
	float mfTimeStep;
	int mlBackoffUpdatesRemaining;
	cPlayer *mpPlayer;
private:
};

//-----------------------------------------------------------------

class cPlayerState_Move : public iPlayerState
{
public:
	cPlayerState_Move(cInit *apInit,cPlayer *apPlayer);
	~cPlayerState_Move() override;
	
	void OnUpdate(float afTimeStep) override;
	
	void OnStartInteract() override;
	void OnStopInteract() override;
	
	bool OnJump() override;
	
	void OnStartExamine() override;
	
	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;
	
	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
	void OnPostSceneDraw() override;
	
	bool OnStartInventory() override;
	bool OnStartInventoryShortCut(int alNum) override;

private:
	cVector3f mvForward;
	cVector3f mvRight;
	cVector3f mvUp;

	cVector3f mvRelPickPoint;
	cVector3f mvPickPoint;

	bool mbPausedGravity;

	iPhysicsBody *mpPushBody;

	int mlMoveInputGraceCount;

	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;

	cPlayerState_Move_BodyCallback *mpCallback;
};

//////////////////////////////////////////////////////////////////////////
// PUSH STATE
//////////////////////////////////////////////////////////////////////////

class cPlayerState_Push : public iPlayerState
{
public:
	cPlayerState_Push(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	bool OnJump() override;
	
	void OnStartInteract() override;
	void OnStopInteract() override;
	
	void OnStartExamine() override;
	
	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;

	void OnPostSceneDraw() override;
	
	bool OnStartInventory() override;
	bool OnStartInventoryShortCut(int alNum) override;

private:
	cVector3f mvForward;
	cVector3f mvRight;

	cVector3f mvRelPickPoint;
	
	cVector3f mvLocalPickPoint;

	iPhysicsBody *mpPushBody;
	cVector3f mvLastBodyPos;
	cVector3f mvRelPlayerPos;	//Black Plague: the player keeps to the body

	cVector2f mvPrevPitchLimits;

	bool mbHasPlayerGravityPush;

	float mfMaxSpeed;

	int mlForward;
	int mlSideways;

	ePlayerMoveState mPrevMoveState;
	ePlayerState mPrevState;
};



#endif // GAME_PLAYER_STATE_INTERACT_H
