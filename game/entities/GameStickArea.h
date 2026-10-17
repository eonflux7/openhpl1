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
#ifndef GAME_GAME_STICK_AREA_H
#define GAME_GAME_STICK_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

//------------------------------------------

class cGameStickArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameStickArea_SaveData);
public:
	cVector3f mvSize;

	tString msAttachFunction;
	tString msDetachFunction;

	tString msAttachSound;
	tString msDetachSound;

	tString msAttachPS;
	tString msDetachPS;

	bool mbCanDeatch;

	tString msAttachedBody;

	bool mbCheckCenterInArea;

	float mfPoseTime;
	
	float mfSetMtxTime;
	
	iGameEntity* CreateEntity() override;
};

//------------------------------------------

class cGameStickArea : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameStickArea;
public:
	cGameStickArea(cInit *apInit,const tString& asName);
	~cGameStickArea(void) override;

	void OnPlayerPick() override;

	void Update(float afTimeStep) override;

	void OnPostSceneDraw() override;

	iPhysicsBody* GetAttachedBody(){ return mpAttachedBody;}
	void AttachBody(iPhysicsBody *apBody);
	void DetachBody();

	void SetAttachFunction(const tString& asFunction){ msAttachFunction = asFunction;}
	void SetDetachFunction(const tString& asFunction){ msDetachFunction = asFunction;}

	void SetAttachSound(const tString& asSound){ msAttachSound = asSound;}
	void SetDetachSound(const tString& asSound){ msDetachSound = asSound;}

	void SetAttachPS(const tString& asParticleSystem){ msAttachPS = asParticleSystem;}
	void SetDetachPS(const tString& asParticleSystem){ msDetachPS = asParticleSystem;}

	void SetCanDetach(bool abCanDetach){ mbCanDeatch = abCanDetach;}
	// Detachment is blocked until pose interpolation completes.
	bool CanDetachBody();

	void SetCheckCenterInArea(bool abCheckCenter){ mbCheckCenterInArea = abCheckCenter;}

	void SetPoseTime(float afDurationSeconds){ mfPoseTime = afDurationSeconds;}

	void SetMoveBody(bool abMoveBody) {mbMoveBody = abMoveBody;}
	bool GetMoveBody(){ return mbMoveBody;}
	void SetRotateBody(bool abRotateBody) {mbRotateBody = abRotateBody;}
	bool GetRotateBody(){ return mbRotateBody;}

	tString BuildBodyCallbackCommand(const tString &asFunc,iPhysicsBody *apBody);
	
	static bool mbAllowAttachment;

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
	void SetupSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	tString msAttachFunction;
	tString msDetachFunction;

	tString msAttachSound;
	tString msDetachSound;

	tString msAttachPS;
	tString msDetachPS;

	bool mbMoveBody;
	bool mbRotateBody;

	bool mbCheckCenterInArea;

	float mfPoseTime;

	bool mbCanDeatch;

	bool mbBodyGravity;
	float mfBodyMass;
	iPhysicsBody *mpAttachedBody;
	iPhysicsBody *mpLastAttachedBody;

	float mfSetMtxTime; // Pose interpolation progress; complete at >=1.
	cMatrixf m_mtxAttachedStart;
};

//------------------------------------------

class cAreaLoader_GameStickArea : public iArea3DLoader
{

public:
	cAreaLoader_GameStickArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameStickArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_STICK_AREA_H
