// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_HUD_MODEL_WEAPON_H
#define GAME_HUD_MODEL_WEAPON_H

#include "core/StdAfx.h"
#include "core/GameTypes.h"

#include "player/PlayerHands.h"

using namespace hpl;

class cMeleeWeaponAttack
{
public:
	cHudModelPose mStart;
	cHudModelPose mEnd;
	
	float mfAttackLength;
	float mfChargeLength;
	float mfDamageProgress;

	float mfMaxImpulse;
	float mfMinImpulse;

	float mfMinMass;
	float mfMaxMass;

	float mfMinDamage;
	float mfMaxDamage;

	cVector3f mvSpinMul;

	float mfDamageRange;
	cVector3f mvDamageSize;

	float mfAttackRange;

	float mfAttackSpeed;
	int mlAttackStrength;

	tString msHitPS;
	int mlHitPSPrio;

	tString msSwingSound;
	tString msChargeSound;
	tString msHitSound;

	iCollideShape *mpCollider;
	cBoundingVolume mBV;
};

class cMeleeRayCallback : public iPhysicsRayCallback
{
public:
	void Reset();
	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	iPhysicsBody *mpClosestBody;
	float mfShortestDist;
	cVector3f mvPosition;
	cVector3f mvNormal;
};

class cHudModel_WeaponMelee : public iHudModel
{
	friend class cPlayerHands;
public:
	cHudModel_WeaponMelee();

	void LoadData(TiXmlElement *apRootElem) override;
	
	void OnAttackDown();
	void OnAttackUp();

	bool OnMouseMove(const cVector2f &avMovement);

	bool UpdatePoseMatrix(cMatrixf& aPoseMtx, float afTimeStep) override;
	
	void PostSceneDraw() override;

	bool IsAttacking();


	cMeleeWeaponAttack* GetAttack(int alAttackIndex){return  &mvAttacks[alAttackIndex];}
private:
	void ResetExtraData() override;

	void Attack();
	void HitBody(iPhysicsBody *apBody);

	void PlaySound(const tString &asSound);
	
	void LoadExtraEntities() override;
	void DestroyExtraEntities() override;

	bool mbDrawDebug;

	int mlCurrentAttack;
	int mlAttackState;
	// Progress within the current attack pose transition, separate from equip progress.
	float mfAttackProgress;

	cMatrixf m_mtxPrevPose;
	cMatrixf m_mtxNextPose;
	
	float mfPoseProgressRate;

	bool mbButtonDown;
	bool mbAttacked;

	cMeleeRayCallback mRayCallback;
	
	std::vector<cMeleeWeaponAttack> mvAttacks;
};

#endif // GAME_HUD_MODEL_WEAPON_H
