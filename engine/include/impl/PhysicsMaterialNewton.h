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
#ifndef HPL_PHYSICS_MATERIAL_NEWTON_H
#define HPL_PHYSICS_MATERIAL_NEWTON_H

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif
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
