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
#ifndef GAME_GAME_FORCE_AREA_H
#define GAME_GAME_FORCE_AREA_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

//------------------------------------------

class cGameForceArea_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameForceArea_SaveData);
public:
	cVector3f mvSize;
	
	float mfMaxForce;
	float mfConstant;
	float mfDestSpeed;
	float mfMaxMass;

	bool mbMulWithMass;
	bool mbAffectCharacters;
	bool mbAffectBodies;
	bool mbForceAtPoint;
		
	iGameEntity* CreateEntity() override;
};

//------------------------------------------

class cGameForceArea : public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
	friend class cAreaLoader_GameForceArea;
public:
	cGameForceArea(cInit *apInit,const tString& asName);
	~cGameForceArea(void) override;

	void Setup();

	void Update(float afTimeStep) override;
	void OnPostSceneDraw() override;

	void SetMaxForce(float afMaxForce) {mfMaxForce = afMaxForce;}
	// Proportional gain applied to the speed error along the force direction.
	void SetConstant(float afSpeedErrorGain) {mfConstant = afSpeedErrorGain;}
	void SetDestSpeed(float afTargetSpeed) {mfDestSpeed = afTargetSpeed;}
	void SetMaxMass(float afMaxMass) {mfMaxMass = afMaxMass;}

	void SetMulWithMass(bool abMultiplyByMass){mbMulWithMass = abMultiplyByMass;}
	void SetAffectCharacters(bool abAffectCharacters){mbAffectCharacters = abAffectCharacters;}
	void SetAffectBodies(bool abAffectBodies){mbAffectBodies = abAffectBodies;}
	void SetForceAtPoint(bool abForceAtContact){ mbForceAtPoint = abForceAtContact;}
	
	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;
	void SetupSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	float mfMaxForce;
	float mfConstant;
	float mfDestSpeed;
	float mfMaxMass;

	bool mbMulWithMass;
	bool mbAffectCharacters;
	bool mbAffectBodies;
	bool mbForceAtPoint;

	cVector3f mvForceDirection;
};

//------------------------------------------

class cAreaLoader_GameForceArea : public iArea3DLoader
{

public:
	cAreaLoader_GameForceArea(const tString &asName, cInit *apInit);
	~cAreaLoader_GameForceArea() override;

	iEntity3D* Load(const tString &asName, const cVector3f &avSize, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

private:

	cInit *mpInit;
};


#endif // GAME_GAME_FORCE_AREA_H
