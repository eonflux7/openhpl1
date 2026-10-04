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
#ifndef GAME_GAME_WHEEL_H
#define GAME_GAME_WHEEL_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

// A wheel or valve of Black Plague and Requiem: a body on a hinge that the
// player turns by hand, through more than one full turn. It can drive a
// joint's limits or a controller elsewhere in the map. Written after the
// games' binaries; the source was not released.

//----------------------------------------------

enum eGameWheelState
{
	eGameWheelState_Middle,
	eGameWheelState_Min,
	eGameWheelState_Max,
	eGameWheelState_LastEnum
};

enum eGameWheelAction
{
	eGameWheelAction_Enter,
	eGameWheelAction_Leave,
	eGameWheelAction_During,
	eGameWheelAction_LastEnum
};

//----------------------------------------------

class cGameWheel_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameWheel_SaveData);
public:
	int mlState;
	bool mbStuck;

	float mfSpinLimitJointAngle;
	float mfAngle;
	float mfPrevAngle;
	float mfPrevJointAngle;
	float mfJointAngle;

	tString msControllerEntity;
	tString msController;
	float mfControllerScale;
	float mfControllerOffset;

	tString msConnectedJoint;
	float mfConnectedScale;
	float mfConnectedOffset;
	float mfConnectedRange;

	tString msEnterCallback;
	tString msLeaveCallback;
	tString msDuringCallback;

	iGameEntity* CreateEntity() override;
};

//----------------------------------------------

class cGameWheel : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
friend class cEntityLoader_GameWheel;
public:
	cGameWheel(cInit *apInit,const tString& asName);
	~cGameWheel(void) override;

	void OnPlayerInteract() override;
	void OnPlayerPick() override;

	void Update(float afTimeStep) override;

	void Init();

	eGameWheelState GetState(){ return mState;}

	// The angle turned in all, in radians, and how fast it turns
	float GetAngle(){ return mfAngle;}
	float GetSpeed();

	void SetCallback(eGameWheelAction aAction, const tString& asCallback){ mvCallbacks[aAction] = asCallback;}

	// Holds the wheel at an end of its travel
	void SetStuck(eGameWheelState aState);
	void SetIsStuck(bool abX);

	// The joint's limits follow the wheel: afMin at the wheel's least
	// angle, afMax at its greatest, kept afRange apart
	void ConnectToJointLimit(iPhysicsJoint *apJoint, float afMin, float afMax, float afRange);
	// The same for the value a controller of the entity's joints aims at
	void ConnectToController(iGameEntity *apEntity, const tString& asController, float afMin, float afMax);

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	void RunCallback(eGameWheelState aState, eGameWheelAction aAction);

	void SetMaxJointAngle(float afAngle);
	void SetMinJointAngle(float afAngle);

	void UpdateController(float afTimeStep);
	void UpdateConnectedJoint(float afTimeStep);

	// The hinge's own limits, as the model was made
	float mfJointMaxLimit;
	float mfJointMinLimit;
	iPhysicsJointHinge *mpJoint;
	iPhysicsBody *mpBody;

	// The hinge only tells an angle within a turn; these add them up
	float mfAngle;
	float mfPrevAngle;
	float mfPrevJointAngle;
	float mfJointAngle;

	// 1 or -1: the wheel only turns that way, and not back
	int mlSpinDir;
	float mfSpinLimitJointAngle;
	float mfSpinMaxAngle;
	float mfSpinMinAngle;

	eGameWheelState mState;

	// The angles at which the ends are reached
	float mfMinLimit;
	float mfMaxLimit;

	bool mbStuckAtMin;
	bool mbStuckAtMax;
	bool mbStuck;

	tString mvCallbacks[eGameWheelAction_LastEnum];

	tString msMinSound;
	tString msMaxSound;

	iPhysicsJoint *mpConnectedJoint;
	float mfConnectedScale;
	float mfConnectedOffset;
	float mfConnectedRange;

	iGameEntity *mpControllerEntity;
	iPhysicsController *mpController;
	float mfControllerScale;
	float mfControllerOffset;
};

//-----------------------------------------------------------------------

class cEntityLoader_GameWheel : public cEntityLoader_Object
{
public:
	cEntityLoader_GameWheel(const tString &asName, cInit *apInit);
	~cEntityLoader_GameWheel() override;

private:
	void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;
	void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

	cInit *mpInit;
};

#endif // GAME_GAME_WHEEL_H
