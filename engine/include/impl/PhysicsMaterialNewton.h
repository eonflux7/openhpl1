// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_MATERIAL_NEWTON_H
#define HPL_PHYSICS_MATERIAL_NEWTON_H

#include <Newton.h>

#include "physics/PhysicsMaterial.h"

namespace hpl {

	class iPhysicsBody;
	class cPhysicsContactData;

	class cPhysicsMaterialNewton : public iPhysicsMaterial
	{
	public:
		cPhysicsMaterialNewton(const tString &asName, iPhysicsWorld *apWorld, int alMatId=-1);
		~cPhysicsMaterialNewton() override;

		void SetElasticity(float afElasticity) override;
		float GetElasticity() const override;
		void SetStaticFriction(float afElasticity) override;
		float GetStaticFriction() const override;
		void SetKineticFriction(float afElasticity) override;
		float GetKineticFriction() const override;

		void SetFrictionCombMode(ePhysicsMaterialCombMode aMode) override;
		ePhysicsMaterialCombMode GetFrictionCombMode() const override;
		void SetElasticityCombMode(ePhysicsMaterialCombMode aMode) override;
		ePhysicsMaterialCombMode GetElasticityCombMode() const override;

		void UpdateMaterials();

		int GetId(){ return mlMaterialId;}
	private:
		float Combine(ePhysicsMaterialCombMode aMode, float afX, float afY);

		static int BeginContactCallback(const NewtonMaterial* material,
										const NewtonBody* apBody1, const NewtonBody* apBody2,
										int alThreadIndex);
		static void ProcessContactCallback(const NewtonJoint* apContactJoint,
											float afTimeStep, int alThreadIndex);

		static iPhysicsBody *mpContactBody1;
		static iPhysicsBody *mpContactBody2;
		static cPhysicsContactData mContactData;
		static int mlContactNum;

		NewtonWorld *mpNewtonWorld;

		int mlMaterialId;

		ePhysicsMaterialCombMode mFrictionMode;
		ePhysicsMaterialCombMode mElasticityMode;

		float mfElasticity;
		float mfStaticFriction;
		float mfKineticFriction;

	};
};
#endif // HPL_PHYSICS_MATERIAL_NEWTON_H
