// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "player/PlayerState_Weapon.h"

#include "core/Init.h"
#include "player/Player.h"

#include "player/HudModel_Weapon.h"
#include "player/HudModel_Throw.h"


// THROW STATE

cPlayerState_Throw::cPlayerState_Throw(cInit *apInit,cPlayer *apPlayer) : iPlayerState(apInit,apPlayer,ePlayerState_Throw)
{
	mfLastForward = 1.0f;
	mfLastSideways = 1.0f;

	mpHudObject = nullptr;
}

void cPlayerState_Throw::OnUpdate(float afTimeStep)
{	
	if(	mpInit->mpNotebook->IsActive()==false && 
		mpInit->mpInventory->IsActive()==false &&
		mpInit->mpNumericalPanel->IsActive()==false &&
		mpInit->mpDeathMenu->IsActive()==false)
	{
		mpPlayer->ResetCrossHairPos();
	}

	// If run is down, run!!
	cInput *pInput = mpInit->mpGame->GetInput();
	if(	pInput->IsTriggered("Run") &&
		mpPlayer->GetMoveState() == ePlayerMoveState_Walk)
	{
		mpPlayer->ChangeMoveState(ePlayerMoveState_Run);
	}

	// Cast ray to see if anything is picked.
	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	cVector3f vStart,vEnd;

	vStart = mpPlayer->GetCamera()->GetPosition();
	vEnd = vStart + mpPlayer->GetCamera()->GetForward() * mpPlayer->GetPickRay()->mfMaxDistance;

	mpPlayer->GetPickRay()->Clear();
	pPhysicsWorld->CastRay(mpPlayer->GetPickRay(),vStart,vEnd,true,false,true);
	mpPlayer->GetPickRay()->CalculateResults();

	if(mpPlayer->GetPickedBody())
	{
		iGameEntity *pEntity = (iGameEntity*)mpPlayer->GetPickedBody()->GetUserData();

		//Set cross hair state
		eCrossHairState CrossState = pEntity->GetPickCrossHairState(mpPlayer->GetPickedBody());
		if(CrossState != eCrossHairState_None && pEntity->GetDescription()!=_W("") &&
			!pEntity->GetHasBeenExamined())
		{
			mpPlayer->SetCrossHairState(eCrossHairState_Examine);
		}
		else
		{
			mpPlayer->SetCrossHairState(eCrossHairState_None);
		}

	}
	else
	{
		mpPlayer->SetCrossHairState(eCrossHairState_None);
	}	

	
}

void cPlayerState_Throw::OnDraw()
{

}

void cPlayerState_Throw::OnPostSceneDraw()
{
}

bool cPlayerState_Throw::OnJump()
{ 
	return true;
}

void cPlayerState_Throw::OnStartInteractMode()
{
}

void cPlayerState_Throw::OnStartInteract()
{
	mpHudObject->OnAttackDown();
}

void cPlayerState_Throw::OnStopInteract()
{
	mpHudObject->OnAttackUp();
}

void cPlayerState_Throw::OnStartExamine()
{
	if(mpPlayer->GetPickedBody())
	{
		iGameEntity *pEntity = (iGameEntity*)mpPlayer->GetPickedBody()->GetUserData();

		pEntity->PlayerExamine();
	}
}

void cPlayerState_Throw::OnStartHolster()
{
	mpInit->mpPlayerHands->SetCurrentModel(1,"");
	mpInit->mpPlayer->ChangeState(ePlayerState_Normal);
}

bool cPlayerState_Throw::OnAddYaw(float afVal)
{ 
	return true;//mpHudObject->OnMouseMove(cVector2f(afVal,0));
}

bool cPlayerState_Throw::OnAddPitch(float afVal)
{
	return true;//return mpHudObject->OnMouseMove(cVector2f(0,afVal));
}

bool cPlayerState_Throw::OnMoveForwards(float afMul, float afTimeStep)
{	
	mfLastForward = afMul;
	return true;
	
}

bool cPlayerState_Throw::OnMoveSideways(float afMul, float afTimeStep)
{
	mfLastSideways = afMul;
	return true;
}

void cPlayerState_Throw::EnterState(iPlayerState* apPrevState)
{
	mpHudObject->OnStart();
}

void cPlayerState_Throw::LeaveState(iPlayerState* apNextState)
{
	if(	apNextState->mType != ePlayerState_Message &&
		apNextState->mType != ePlayerState_WeaponMelee &&
		mpInit->mpPlayerHands->GetCurrentModel(1) == mpHudObject)
	{
		mpInit->mpPlayerHands->SetCurrentModel(1,"");
	}
}

void cPlayerState_Throw::OnStartRun()
{
	if(mpPlayer->GetMoveState() == ePlayerMoveState_Walk)
		mpPlayer->ChangeMoveState(ePlayerMoveState_Run);
	if( mpPlayer->GetMoveState() == ePlayerMoveState_Jump)
	{
		mpPlayer->SetPrevMoveState(ePlayerMoveState_Run);
	}
}
void cPlayerState_Throw::OnStopRun()
{
	if(mpPlayer->GetMoveState() == ePlayerMoveState_Run)
		mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);
	else if(mpPlayer->GetMoveState() == ePlayerMoveState_Jump)
		mpPlayer->SetPrevMoveState(ePlayerMoveState_Walk);
}

void cPlayerState_Throw::OnStartCrouch()
{
	if(mpPlayer->GetMoveState() == ePlayerMoveState_Jump)return;

	if(mpInit->mpButtonHandler->GetToggleCrouch())
	{
		if(mpPlayer->GetMoveState() == ePlayerMoveState_Crouch)
			mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);
		else
			mpPlayer->ChangeMoveState(ePlayerMoveState_Crouch);
	}
	else
	{
		mpPlayer->ChangeMoveState(ePlayerMoveState_Crouch);
	}
}

void cPlayerState_Throw::OnStopCrouch()
{
	if( mpPlayer->GetMoveState() == ePlayerMoveState_Crouch &&
		mpInit->mpButtonHandler->GetToggleCrouch()==false)
	{
		mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);
	}
}

bool cPlayerState_Throw::OnStartInventory()
{
	return true;
}

bool cPlayerState_Throw::OnStartInventoryShortCut(int alNum)
{ 
	return true;
}

// WEAPON MELEE STATE

cPlayerState_WeaponMelee::cPlayerState_WeaponMelee(cInit *apInit,cPlayer *apPlayer) : iPlayerState(apInit,apPlayer,ePlayerState_WeaponMelee)
{
	mfLastForward = 1.0f;
	mfLastSideways = 1.0f;
}

void cPlayerState_WeaponMelee::OnUpdate(float afTimeStep)
{	
	if(	mpInit->mpNotebook->IsActive()==false && 
		mpInit->mpInventory->IsActive()==false &&
		mpInit->mpNumericalPanel->IsActive()==false &&
		mpInit->mpDeathMenu->IsActive()==false)
	{
		mpPlayer->ResetCrossHairPos();
	}

	// If run is down, run!!
	cInput *pInput = mpInit->mpGame->GetInput();
	if(	pInput->IsTriggered("Run") &&
		mpPlayer->GetMoveState() == ePlayerMoveState_Walk)
	{
		mpPlayer->ChangeMoveState(ePlayerMoveState_Run);
	}

	// Cast ray to see if anything is picked.
	iPhysicsWorld *pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	cVector3f vStart,vEnd;

	vStart = mpPlayer->GetCamera()->GetPosition();
	vEnd = vStart + mpPlayer->GetCamera()->GetForward() * mpPlayer->GetPickRay()->mfMaxDistance;

	mpPlayer->GetPickRay()->Clear();
	pPhysicsWorld->CastRay(mpPlayer->GetPickRay(),vStart,vEnd,true,false,true);
	mpPlayer->GetPickRay()->CalculateResults();

	if(mpPlayer->GetPickedBody() && mpHudWeapon->IsAttacking()==false)
	{
		iGameEntity *pEntity = (iGameEntity*)mpPlayer->GetPickedBody()->GetUserData();

		//Set cross hair state
		eCrossHairState CrossState = pEntity->GetPickCrossHairState(mpPlayer->GetPickedBody());
		if(CrossState != eCrossHairState_None && pEntity->GetDescription()!=_W("") &&
			!pEntity->GetHasBeenExamined())
		{
			mpPlayer->SetCrossHairState(eCrossHairState_Examine);
		}
		else
		{
			mpPlayer->SetCrossHairState(eCrossHairState_None);
		}

	}
	else
	{
		mpPlayer->SetCrossHairState(eCrossHairState_None);
	}	
	
	
	
	
}

void cPlayerState_WeaponMelee::OnDraw()
{

}

void cPlayerState_WeaponMelee::OnPostSceneDraw()
{
	mpHudWeapon->PostSceneDraw();
}

bool cPlayerState_WeaponMelee::OnJump()
{ 
	return true;
}

void cPlayerState_WeaponMelee::OnStartInteractMode()
{
}

void cPlayerState_WeaponMelee::OnStartInteract()
{
	mpHudWeapon->OnAttackDown();
}

void cPlayerState_WeaponMelee::OnStopInteract()
{
	mpHudWeapon->OnAttackUp();
}

void cPlayerState_WeaponMelee::OnStartExamine()
{
	if(mpPlayer->GetPickedBody() && mpHudWeapon->IsAttacking()==false)
	{
		iGameEntity *pEntity = (iGameEntity*)mpPlayer->GetPickedBody()->GetUserData();

		pEntity->PlayerExamine();
	}
}

void cPlayerState_WeaponMelee::OnStartHolster()
{
	mpInit->mpPlayer->ChangeState(ePlayerState_Normal);
}

bool cPlayerState_WeaponMelee::OnAddYaw(float afVal)
{ 
	return mpHudWeapon->OnMouseMove(cVector2f(afVal,0));
}

bool cPlayerState_WeaponMelee::OnAddPitch(float afVal)
{
	return mpHudWeapon->OnMouseMove(cVector2f(0,afVal));
}

bool cPlayerState_WeaponMelee::OnMoveForwards(float afMul, float afTimeStep)
{	
	{
		mfLastForward = afMul;
		return true;
	}
}

bool cPlayerState_WeaponMelee::OnMoveSideways(float afMul, float afTimeStep)
{
	{
		mfLastSideways = afMul;
		return true;
	}


}

void cPlayerState_WeaponMelee::EnterState(iPlayerState* apPrevState)
{
}

void cPlayerState_WeaponMelee::LeaveState(iPlayerState* apNextState)
{
	if(	apNextState->mType != ePlayerState_Message &&
		apNextState->mType != ePlayerState_Throw &&
		mpInit->mpPlayerHands->GetCurrentModel(1) == mpHudWeapon)
	{
		mpInit->mpPlayerHands->SetCurrentModel(1,"");
	}
}

void cPlayerState_WeaponMelee::OnStartRun()
{
	if(mpPlayer->GetMoveState() == ePlayerMoveState_Walk)
		mpPlayer->ChangeMoveState(ePlayerMoveState_Run);
	if( mpPlayer->GetMoveState() == ePlayerMoveState_Jump)
	{
		mpPlayer->SetPrevMoveState(ePlayerMoveState_Run);
	}
}
void cPlayerState_WeaponMelee::OnStopRun()
{
	if(mpPlayer->GetMoveState() == ePlayerMoveState_Run)
		mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);
	else if(mpPlayer->GetMoveState() == ePlayerMoveState_Jump)
		mpPlayer->SetPrevMoveState(ePlayerMoveState_Walk);
}

void cPlayerState_WeaponMelee::OnStartCrouch()
{
	if(mpPlayer->GetMoveState() == ePlayerMoveState_Jump)return;

	if(mpInit->mpButtonHandler->GetToggleCrouch())
	{
		if(mpPlayer->GetMoveState() == ePlayerMoveState_Crouch)
			mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);
		else
			mpPlayer->ChangeMoveState(ePlayerMoveState_Crouch);
	}
	else
	{
		mpPlayer->ChangeMoveState(ePlayerMoveState_Crouch);
	}
}

void cPlayerState_WeaponMelee::OnStopCrouch()
{
	if( mpPlayer->GetMoveState() == ePlayerMoveState_Crouch &&
		mpInit->mpButtonHandler->GetToggleCrouch()==false)
	{
		mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);
	}
}

bool cPlayerState_WeaponMelee::OnStartInventory()
{
	return true;
}

bool cPlayerState_WeaponMelee::OnStartInventoryShortCut(int alNum)
{ 
	return true;
}

// WEAPON STATE
