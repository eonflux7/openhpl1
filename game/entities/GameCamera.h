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
#ifndef GAME_GAME_CAMERA_H
#define GAME_GAME_CAMERA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

// The security camera of Black Plague: it turns from side to side with a
// pause at each end, and when the player stands in its light, in sight, it
// goes into alert (the light changes colour, a sound, a script callback)
// for a time. Written after the game's binary; the source was not released.

//----------------------------------------------

class cGameCamera_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameCamera_SaveData);
public:
	float mfMinAngle;
	float mfMaxAngle;
	float mfMinPause;
	float mfMaxPause;
	float mfPauseCount;
	float mfAngle;
	float mfDir;
	float mfSpeed;
	bool mbAlerted;
	tString msSpottedCallback;
	tString msAlertOverCallback;

	iGameEntity* CreateEntity() override;
};

//----------------------------------------------

class cGameCamera : public iGameEntity, public iPhysicsRayCallback
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
friend class cEntityLoader_GameCamera;
public:
	cGameCamera(cInit *apInit,const tString& asName);
	~cGameCamera(void) override;

	void Update(float afTimeStep) override;

	void OnSetActive(bool abX) override;

	void Init();

	// The angles are in radians, to each side of how the camera was placed
	void Setup(	float afMinAngle, float afMaxAngle, float afMinPause, float afMaxPause, float afSpeed,
				const tString &asSpottedCallback, const tString &asAlertOverCallback);

	void SetAlerted(bool abX);
	bool IsAlerted(){ return mbAlerted;}

	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	bool HasLineOfSight(iLight3D *apLight, iPhysicsWorld *apPhysicsWorld);
	void PlayerIsSpotted();

	void PlayTurnSound();
	void StopTurnSound();

	// How long an alert lasts
	float mfSpottedPlayerTime;
	cColor mAlertColor;
	cColor mNormalColor;
	float mfLightRadius;

	tString msAlertSound;
	tString msTurnSound;
	// Read from the entity file, but the game never plays it
	tString msStillSound;

	cMatrixf m_mtxStart;

	float mfMinAngle;
	float mfMaxAngle;
	float mfMinPause;
	float mfMaxPause;
	float mfSpeed;

	tString msSpottedCallback;
	tString msAlertOverCallback;

	float mfAngle;
	float mfDir;
	float mfPauseCount;

	float mfCheckCount;
	bool mbAlerted;
	int mlSpottedCount;
	bool mbRayHit;
	float mfAlertCount;

	cSoundEntity *mpTurnSound;
};

//----------------------------------------------

class cEntityLoader_GameCamera : public cEntityLoader_Object
{
public:
	cEntityLoader_GameCamera(const tString &asName, cInit *apInit);
	~cEntityLoader_GameCamera() override;

private:
	void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;
	void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

	cInit *mpInit;
};

#endif // GAME_GAME_CAMERA_H
