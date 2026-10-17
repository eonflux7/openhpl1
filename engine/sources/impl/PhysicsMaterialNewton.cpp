/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "impl/PhysicsMaterialNewton.h"

#include "impl/PhysicsWorldNewton.h"
#include "impl/PhysicsBodyNewton.h"
#include "physics/SurfaceData.h"

#include "system/LowLevelSystem.h"
#include <algorithm>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cPhysicsMaterialNewton::cPhysicsMaterialNewton(const tString &asName, iPhysicsWorld *apWorld,int alMatId)
		: iPhysicsMaterial(asName,apWorld)
	{
		cPhysicsWorldNewton *pNWorld = static_cast<cPhysicsWorldNewton*>(mpWorld);

		mpNewtonWorld = pNWorld->GetNewtonWorld();

		if(alMatId==-1)
		{
			mlMaterialId = NewtonMaterialCreateGroupID(mpNewtonWorld);
		}
		else
		{
			mlMaterialId = alMatId;
		}

		//Setup default properties
		mFrictionMode = ePhysicsMaterialCombMode_Average;
		mElasticityMode  = ePhysicsMaterialCombMode_Average;

		mfElasticity = 0.5f;
		mfStaticFriction = 0.3f;
		mfKineticFriction = 0.3f;

		//Log(" Created physics material '%s' with Newton id %d\n",asName.c_str(),mlMaterialId);
	}

	//-----------------------------------------------------------------------

	cPhysicsMaterialNewton::~cPhysicsMaterialNewton()
	{
		/*Might be just as well to let newton handle this*/
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cPhysicsMaterialNewton::SetElasticity(float afElasticity)
	{
		mfElasticity = afElasticity;

		UpdateMaterials();
	}

	float cPhysicsMaterialNewton::GetElasticity() const
	{
		return mfElasticity;
	}

	//-----------------------------------------------------------------------

	void cPhysicsMaterialNewton::SetStaticFriction(float afElasticity)
	{
		mfStaticFriction = afElasticity;

		UpdateMaterials();
	}

	float cPhysicsMaterialNewton::GetStaticFriction() const
	{
		return mfStaticFriction;
	}

	//-----------------------------------------------------------------------

	void cPhysicsMaterialNewton::SetKineticFriction(float afElasticity)
	{
		mfKineticFriction = afElasticity;

		UpdateMaterials();
	}

	float cPhysicsMaterialNewton::GetKineticFriction() const
	{
		return mfKineticFriction;
	}

	//-----------------------------------------------------------------------

	void cPhysicsMaterialNewton::SetFrictionCombMode(ePhysicsMaterialCombMode aMode)
	{
		mFrictionMode = aMode;

		UpdateMaterials();
	}

	ePhysicsMaterialCombMode cPhysicsMaterialNewton::GetFrictionCombMode() const
	{
		return mFrictionMode;
	}

	//-----------------------------------------------------------------------

	void cPhysicsMaterialNewton::SetElasticityCombMode(ePhysicsMaterialCombMode aMode)
	{
		mElasticityMode = aMode;

		UpdateMaterials();
	}

	//-----------------------------------------------------------------------

	ePhysicsMaterialCombMode cPhysicsMaterialNewton::GetElasticityCombMode() const
	{
		return mElasticityMode;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cPhysicsMaterialNewton::UpdateMaterials()
	{
		cPhysicsMaterialIterator MatIt = mpWorld->GetMaterialIterator();

		while(MatIt.HasNext())
		{
			cPhysicsMaterialNewton* pMat = static_cast<cPhysicsMaterialNewton*>(MatIt.Next());

			ePhysicsMaterialCombMode frictionMode =   (ePhysicsMaterialCombMode) std::max(mFrictionMode,
				pMat->mFrictionMode);
			ePhysicsMaterialCombMode elasticityMode = (ePhysicsMaterialCombMode) std::max(mElasticityMode,
				pMat->mElasticityMode);

			//If the material is the same do not blend.
			if(pMat == this){
				frictionMode = 	ePhysicsMaterialCombMode_Average;
				elasticityMode = ePhysicsMaterialCombMode_Average;
			}


			NewtonMaterialSetDefaultElasticity(mpNewtonWorld,mlMaterialId,pMat->mlMaterialId,
				Combine(elasticityMode,mfElasticity, pMat->mfElasticity));

			NewtonMaterialSetDefaultFriction(mpNewtonWorld,mlMaterialId,pMat->mlMaterialId,
				Combine(frictionMode,mfStaticFriction, pMat->mfStaticFriction),
				Combine(frictionMode,mfKineticFriction, pMat->mfKineticFriction));

			NewtonMaterialSetContinuousCollisionMode(mpNewtonWorld,mlMaterialId,pMat->mlMaterialId,
													1);
			NewtonMaterialSetSurfaceThickness(mpNewtonWorld,mlMaterialId,pMat->mlMaterialId, mfContactSkin);

			NewtonMaterialSetCollisionCallback(mpNewtonWorld,mlMaterialId,pMat->mlMaterialId,
				(void*)nullptr,BeginContactCallback,ProcessContactCallback);
		}
	}

	//-----------------------------------------------------------------------

	float cPhysicsMaterialNewton::Combine(ePhysicsMaterialCombMode aMode, float afX, float afY)
	{
		switch(aMode)
		{
		case ePhysicsMaterialCombMode_Average: return (afX + afY)/2;
		case ePhysicsMaterialCombMode_Min: return std::min(afX, afY);
		case ePhysicsMaterialCombMode_Max: return std::max(afX, afY);
		case ePhysicsMaterialCombMode_Multiply: return afX * afY;
		}

		return (afX + afY) /2;
	}

	//////////////////////////////////////////////////////////////////////////
	// STATIC NEWTON CALLBACKS
	//////////////////////////////////////////////////////////////////////////

	iPhysicsBody *cPhysicsMaterialNewton::mpContactBody1 = nullptr;
	iPhysicsBody *cPhysicsMaterialNewton::mpContactBody2 = nullptr;
	int cPhysicsMaterialNewton::mlContactNum =0;
	cPhysicsContactData cPhysicsMaterialNewton::mContactData;

	//-----------------------------------------------------------------------
	int cPhysicsMaterialNewton::BeginContactCallback(const NewtonMaterial* material,
									const NewtonBody* apBody1, const NewtonBody* apBody2, int alThreadIndex)
	{
		mpContactBody1 = (cPhysicsBodyNewton*) NewtonBodyGetUserData(apBody1);
		mpContactBody2 = (cPhysicsBodyNewton*) NewtonBodyGetUserData(apBody2);

		if(mpContactBody1->GetCollide()==false) return 0;
		if(mpContactBody2->GetCollide()==false) return 0;

		if(mpContactBody1->IsActive()==false) return 0;
		if(mpContactBody2->IsActive()==false) return 0;

		if(mpContactBody1->IsRagDoll() && mpContactBody2->GetCollideRagDoll()==false) return 0;
		if(mpContactBody2->IsRagDoll() && mpContactBody1->GetCollideRagDoll()==false) return 0;

		if(mpContactBody1->IsCharacter() && mpContactBody2->GetCollideCharacter()==false) return 0;
		if(mpContactBody2->IsCharacter() && mpContactBody1->GetCollideCharacter()==false) return 0;

		//Log("----- Begin contact between body '%s' and '%s'.\n",mpContactBody1->GetName().c_str(),
		//													mpContactBody2->GetName().c_str());

		//Reset contact num
		mlContactNum =0;

		if(mpContactBody1->OnBeginCollision(mpContactBody2)==false) return 0;
		if(mpContactBody2->OnBeginCollision(mpContactBody1)==false) return 0;

		return 1;
	}

	//-----------------------------------------------------------------------

	// Newton 2 reports every contact of a body pair in one callback, so this
	// does the work of Newton 1's per-contact ProcessContactCallback followed
	// by EndContactCallback.
	void cPhysicsMaterialNewton::ProcessContactCallback(const NewtonJoint* apContactJoint,
											float afTimeStep, int alThreadIndex)
	{
		NewtonBody *pNewtonBody1 = NewtonJointGetBody0(apContactJoint);
		NewtonBody *pNewtonBody2 = NewtonJointGetBody1(apContactJoint);
		mpContactBody1 = (cPhysicsBodyNewton*) NewtonBodyGetUserData(pNewtonBody1);
		mpContactBody2 = (cPhysicsBodyNewton*) NewtonBodyGetUserData(pNewtonBody2);

		mlContactNum =0;

		for(void *pContact = NewtonContactJointGetFirstContact(apContactJoint); pContact;
			pContact = NewtonContactJointGetNextContact(apContactJoint, pContact))
		{
			NewtonMaterial *pMaterial = NewtonContactGetMaterial(pContact);

			//Normal speed
			float fNormSpeed = NewtonMaterialGetContactNormalSpeed(pMaterial);
			if(mContactData.mfMaxContactNormalSpeed < fNormSpeed) mContactData.mfMaxContactNormalSpeed = fNormSpeed;

			//Tangent speed
			float fTanSpeed0 = NewtonMaterialGetContactTangentSpeed(pMaterial,0);
			float fTanSpeed1 = NewtonMaterialGetContactTangentSpeed(pMaterial,1);
			if(std::abs(mContactData.mfMaxContactTangentSpeed) < std::abs(fTanSpeed0)) mContactData.mfMaxContactTangentSpeed = fTanSpeed0;
			if(std::abs(mContactData.mfMaxContactTangentSpeed) < std::abs(fTanSpeed1)) mContactData.mfMaxContactTangentSpeed = fTanSpeed1;

			//Force
			cVector3f vForce;
			NewtonMaterialGetContactForce(pMaterial,pNewtonBody1,vForce.v);
			mContactData.mvForce += vForce;

			//Position and normal
			cVector3f vPos, vNormal;
			NewtonMaterialGetContactPositionAndNormal(pMaterial,pNewtonBody1,vPos.v, vNormal.v);
			mContactData.mvContactNormal += vNormal;
			mContactData.mvContactPosition += vPos;

			if(mpContactBody1->GetWorld()->GetSaveContactPoints())
			{
				cCollidePoint collidePoint;
				collidePoint.mfDepth = 1;
				collidePoint.mvPoint = vPos;
				collidePoint.mvNormal = vNormal;
				mpContactBody1->GetWorld()->GetContactPoints()->push_back(collidePoint);
			}

			mlContactNum++;
		}

		if(mlContactNum <= 0)
		{
			mpContactBody1 = nullptr;
			mpContactBody2 = nullptr;
			return;
		}

		iPhysicsMaterial *pMaterial1 = mpContactBody1->GetMaterial();
		iPhysicsMaterial *pMaterial2 = mpContactBody2->GetMaterial();

		mContactData.mvContactNormal = mContactData.mvContactNormal / (float)mlContactNum;
		mContactData.mvContactPosition = mContactData.mvContactPosition / (float)mlContactNum;

		pMaterial1->GetSurfaceData()->CreateImpactEffect(mContactData.mfMaxContactNormalSpeed,
													mContactData.mvContactPosition,
													mlContactNum,pMaterial2->GetSurfaceData());

		int lPrio1 = pMaterial1->GetSurfaceData()->GetPriority();
		int lPrio2 = pMaterial2->GetSurfaceData()->GetPriority();

		if(lPrio1 >= lPrio2)
		{
			if(std::abs(mContactData.mfMaxContactNormalSpeed) > 0)
				pMaterial1->GetSurfaceData()->OnImpact(mContactData.mfMaxContactNormalSpeed,
														mContactData.mvContactPosition,
														mlContactNum,mpContactBody1);
			if(std::abs(mContactData.mfMaxContactTangentSpeed) > 0)
				pMaterial1->GetSurfaceData()->OnSlide(mContactData.mfMaxContactTangentSpeed,
														mContactData.mvContactPosition,
														mlContactNum,mpContactBody1,mpContactBody2);
		}

		if(lPrio2 >= lPrio1 && pMaterial2 != pMaterial1)
		{
			if(std::abs(mContactData.mfMaxContactNormalSpeed) > 0)
				pMaterial2->GetSurfaceData()->OnImpact(mContactData.mfMaxContactNormalSpeed,
														mContactData.mvContactPosition,
														mlContactNum,mpContactBody2);
			if(std::abs(mContactData.mfMaxContactTangentSpeed) > 0)
				pMaterial2->GetSurfaceData()->OnSlide(mContactData.mfMaxContactTangentSpeed,
														mContactData.mvContactPosition,
														mlContactNum,mpContactBody2,mpContactBody1);
		}

		mpContactBody1->OnCollide(mpContactBody2,&mContactData);
		mpContactBody2->OnCollide(mpContactBody1,&mContactData);

		//Reset contact data
		mpContactBody1 = nullptr;
		mpContactBody2 = nullptr;

		mlContactNum =0;

		mContactData.mfMaxContactTangentSpeed =0;
		mContactData.mfMaxContactNormalSpeed =0;

		mContactData.mvContactPosition = cVector3f(0,0,0);
		mContactData.mvContactNormal = cVector3f(0,0,0);
		mContactData.mvForce = cVector3f(0,0,0);
	}

	//-----------------------------------------------------------------------

}
