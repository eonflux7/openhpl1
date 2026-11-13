// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_PLAYER_STATE_WEAPON_H
#define GAME_PLAYER_STATE_WEAPON_H

#include "core/StdAfx.h"
#include "player/PlayerState.h"

using namespace hpl;

// THROW STATE

class cHudModel_Throw;

class cPlayerState_Throw : public iPlayerState
{
public:
	cPlayerState_Throw(cInit *apInit,cPlayer *apPlayer);

	void OnUpdate(float afTimeStep) override;

	void OnDraw() override;
	void OnPostSceneDraw() override;

	bool OnJump() override;

	void OnStartInteractMode() override;

	void OnStartInteract() override;
	void OnStopInteract() override;
	void OnStartExamine() override;
	void OnStartHolster() override;

	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;

	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;

	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;

	void OnStartRun() override;
	void OnStopRun() override;

	void OnStartCrouch() override;
	void OnStopCrouch() override;

	bool OnStartInventory() override;
	bool OnStartInventoryShortCut(int alNum) override;

	void SetHudObject(cHudModel_Throw* apHudObject){ mpHudObject = apHudObject;}

private:
	cHudModel_Throw *mpHudObject;
	float mfLastForward;
	float mfLastSideways;
};

// WEAPON MELEE STATE

class cHudModel_WeaponMelee;

class cPlayerState_WeaponMelee : public iPlayerState
{
public:
	
	cPlayerState_WeaponMelee(cInit *apInit,cPlayer *apPlayer);
	
	void OnUpdate(float afTimeStep) override;
	
	void OnDraw() override;
	void OnPostSceneDraw() override;
	
	bool OnJump() override;
	
	void OnStartInteractMode() override;
	
	void OnStartInteract() override;
	void OnStopInteract() override;
	void OnStartExamine() override;
	void OnStartHolster() override;
	
	bool OnAddYaw(float afVal) override;
	bool OnAddPitch(float afVal) override;
	
	bool OnMoveForwards(float afMul, float afTimeStep) override;
	bool OnMoveSideways(float afMul, float afTimeStep) override;
	
	void EnterState(iPlayerState* apPrevState) override;
	void LeaveState(iPlayerState* apNextState) override;
	
	void OnStartRun() override;
	void OnStopRun() override;
	
	void OnStartCrouch() override;
	void OnStopCrouch() override;
	
	bool OnStartInventory() override;
	bool OnStartInventoryShortCut(int alNum) override;

	void SetHudWeapon(cHudModel_WeaponMelee* apHudWeapon){ mpHudWeapon = apHudWeapon;}

private:
	cHudModel_WeaponMelee *mpHudWeapon;
	float mfLastForward;
	float mfLastSideways;
};

#endif // GAME_PLAYER_STATE_WEAPON_H
