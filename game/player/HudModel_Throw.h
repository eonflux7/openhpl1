// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_HUD_MODEL_THROW_H
#define GAME_HUD_MODEL_THROW_H

#include "core/StdAfx.h"
#include "core/GameTypes.h"

#include "player/PlayerHands.h"

using namespace hpl;

class cInventoryItem;

class cHudModel_Throw : public iHudModel
{
	friend class cPlayerHands;
public:
	cHudModel_Throw();

	void LoadData(TiXmlElement *apRootElem) override;
	
	void OnStart();

	void OnAttackDown();
	void OnAttackUp();

	bool OnMouseMove(const cVector2f &avMovement);

	bool UpdatePoseMatrix(cMatrixf& aPoseMtx, float afTimeStep) override;

	void SetItem(cInventoryItem *apItem){ mpItem = apItem;}
private:
	void ResetExtraData() override;

	bool mbButtonDown;
	float mfChargeProgress;

	cHudModelPose mChargePose;

	float mfChargeDurationSeconds;
	float mfMinImpulse;
	float mfMaxImpulse;

	float mfReloadTime;

	cVector3f mvTorque;

	tString msChargeSound;
	tString msThrowSound;

	tString msThrowEntity;

	cInventoryItem *mpItem;
};

#endif // GAME_HUD_MODEL_THROW_H
