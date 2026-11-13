// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_LIQUID_AREA_H
#define GAME_GAME_LIQUID_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameLiquidArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameLiquidArea_SaveData);
public:
	cVector3f mvSize;
	
	float mfDensity;
	float mfLinearViscosity;
	float mfAngularViscosity;
	cColor mColor;

	tString msPhysicsMaterial;
	
	cPlanef mSurfacePlane;

	bool mbHasWaves;
		
	iGameEntity* CreateEntity() override;
};

class cGameLiquidArea : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameLiquidArea;
public:
	cGameLiquidArea(cInit *apInit,const tString& asName);
	~cGameLiquidArea(void) override;

	void OnPlayerPick() override;

	void Update(float afTimeStep) override;
	
	void SetDensity(float afDensity){ mfDensity = afDensity;}
	void SetLinearViscosity(float afLinearViscosity){ mfLinearViscosity = afLinearViscosity;}
	void SetAngularViscosity(float afAngularViscosity){ mfAngularViscosity = afAngularViscosity;}
	void SetPhysicsMaterial(const tString asName);
	void SetColor(const cColor &aColor){ mColor = aColor;}
	void SetHasWaves(bool abHasWaves){ mbHasWaves = abHasWaves;}
    
	void Setup();

	iPhysicsMaterial* GetPhysicsMaterial(){ return mpPhysicsMaterial;}
	
	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
	void SetupSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	void SplashEffect(iPhysicsBody *apBody);

	float mfDensity;
	float mfLinearViscosity;
	float mfAngularViscosity;
	cColor mColor;

	iPhysicsMaterial *mpPhysicsMaterial;

	bool mbHasWaves;
	float mfWaveAmplitude;
	float mfWaveAngularFrequency; // Radians per second.

	float mfWaveElapsedSeconds;

	cPlanef mSurfacePlane;
};

class cAreaLoader_GameLiquidArea : public iArea3DLoader
{

public:
	cAreaLoader_GameLiquidArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameLiquidArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_LIQUID_AREA_H
