// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_SAVE_TYPES_H
#define GAME_SAVE_TYPES_H

#include "core/StdAfx.h"

using namespace hpl;

class cEngineBody_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineBody_SaveData)
public:
	void FromBody(iPhysicsBody *pBody);
	void ToBody(iPhysicsBody *pBody);
	
	float mfMass;
	bool mbActive;
	bool mbCollideCharacter;
	cVector3f mvLinearVelocity;
	cVector3f mvAngularVelocity;
	cMatrixf m_mtxTransform;
};

class cEngineJointController_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineJointController_SaveData)
public:
	tString msName;
	
	float mfDestValue;
	bool mbActive;
};

class cEngineJoint_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineJoint_SaveData)
public:
	void FromJoint(iPhysicsJoint *apJoint);
	void ToJoint(iPhysicsJoint *apJoint);
	
	tString msName;
	float mfMinLimit;
	float mfMaxLimit;

	tString msOnMinCallback;
	tString msOnMaxCallback;

	cContainerVec<cEngineJointController_SaveData> mvControllers;
};

class cEnginePSEmitter_SaveData : public iSerializable
{
	kSerializableClassInit(cEnginePSEmitter_SaveData)
public:
	bool mbActive;
};

class cEnginePS_SaveData : public iSerializable
{
	kSerializableClassInit(cEnginePS_SaveData)
public:
	void FromPS(cParticleSystem3D *apPS);
	void ToPS(cParticleSystem3D *apPS);
	
	tString msName;
	tString msType;
	cVector3f mvSize;
	cMatrixf m_mtxTransform;
	//Switched off with SetParticleSystemActive. Not in the original games'
	//saves, so their map systems came back lit; true for older saves.
	bool mbActive = true;
	
	cContainerVec<cEnginePSEmitter_SaveData> mvEmitterActive;
};

class cEngineBeam_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineBeam_SaveData)
public:
	void FromBeam(cBeam *apBeam);
	void ToBeam(cBeam *apBeam);

	tString msName;
	tString msFile;
	
	cVector3f mvStartPos;
	cVector3f mvEndPos;
};

class cEngineSound_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineSound_SaveData)
public:
	void FromSound(cSoundEntity* apSound);
	void ToSound(cSoundEntity* apSound);

	tString msName;
	bool mbActive;
	bool mbStopped;
};

class cEngineLightAttachBB_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineLightAttachBB_SaveData)
public:
	tString msName;
};

class cEngineLight_SaveData : public iSerializable
{
	kSerializableClassInit(cEngineLight_SaveData)
public:
	void FromLight(iLight3D *apLight);
	void ToLight(iLight3D *apLight);

	tString msName;
	bool mbActive;
	bool mbVisible;

	cColor mDiffuseColor;
	float mfFarAttenuation;

	cContainerVec<cEngineLightAttachBB_SaveData> mvBillboards;
	
	bool mbFlickering;
	tString msFlickerOffSound;
	tString msFlickerOnSound;
	tString msFlickerOffPS;
	tString msFlickerOnPS;
	float mfFlickerOnMinLength;
	float mfFlickerOffMinLength;
	float mfFlickerOnMaxLength;
	float mfFlickerOffMaxLength;
	cColor mFlickerOffColor;
	float mfFlickerOffRadius;
	bool mbFlickerFade;
	float mfFlickerOnFadeLength;
	float mfFlickerOffFadeLength;
};

#endif // GAME_SAVE_TYPES_H
