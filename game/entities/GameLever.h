// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_LEVER_H
#define GAME_GAME_LEVER_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

// A lever of Black Plague and Requiem: a body on a hinge that the player
// moves by hand, with a state at each end of its travel. Written after the
// games' binaries; the source was not released.

enum eGameLeverState
{
	eGameLeverState_Middle,
	eGameLeverState_Min,
	eGameLeverState_Max,
	eGameLeverState_LastEnum
};

enum eGameLeverAction
{
	eGameLeverAction_Enter,
	eGameLeverAction_Leave,
	eGameLeverAction_During,
	eGameLeverAction_LastEnum
};

// Where the lever goes back to when it is let go
enum eGameLeverMovement
{
	eGameLeverMovement_None,
	eGameLeverMovement_Value,
	eGameLeverMovement_Min,
	eGameLeverMovement_Max,
	eGameLeverMovement_LastEnum
};

class cGameLever_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameLever_SaveData);
public:
	int mlState;
	bool mbStuck;

	tString msEnterCallback;
	tString msLeaveCallback;
	tString msDuringCallback;

	iGameEntity* CreateEntity() override;
};

class cGameLever : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
friend class cEntityLoader_GameLever;
public:
	cGameLever(cInit *apInit,const tString& asName);
	~cGameLever(void) override;

	void OnPlayerInteract() override;
	void OnPlayerPick() override;

	void Update(float afTimeStep) override;

	void Init();

	eGameLeverState GetState(){ return mState;}

	void SetCallback(eGameLeverAction aAction, const tString& asCallback){ mvCallbacks[aAction] = asCallback;}

	// Holds the lever at the end it is in (or at the given end)
	void SetStuck(eGameLeverState aState);
	void SetIsStuck(bool abStuck);

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	void RunCallback(eGameLeverState aState, eGameLeverAction aAction);

	// The hinge's own limits, as the model was made
	float mfJointMaxLimit;
	float mfJointMinLimit;
	iPhysicsJointHinge *mpJoint;
	iPhysicsBody *mpBody;

	cPidControllerf mMovePid;

	eGameLeverState mState;

	// The angles at which the ends are reached
	float mfMinLimit;
	float mfMaxLimit;

	bool mbStuckAtMin;
	bool mbStuckAtMax;
	bool mbStuck;

	tString mvCallbacks[eGameLeverAction_LastEnum];

	tString msMinSound;
	tString msMaxSound;

	eGameLeverMovement mMovementType;
	float mfMovementValue;
};

class cEntityLoader_GameLever : public cEntityLoader_Object
{
public:
	cEntityLoader_GameLever(const tString &asName, cInit *apInit);
	~cEntityLoader_GameLever() override;

private:
	void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;
	void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

	cInit *mpInit;
};

#endif // GAME_GAME_LEVER_H
