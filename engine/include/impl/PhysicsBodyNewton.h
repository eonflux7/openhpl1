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
#ifndef HPL_PHYSICS_BODY_NEWTON_H
#define HPL_PHYSICS_BODY_NEWTON_H

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif
#include <Newton.h>

#include "physics/PhysicsBody.h"

namespace hpl {

	class cPhysicsBodyNewtonCallback : public iEntityCallback
	{
		void OnTransformUpdate(iEntity3D * apEntity) override;
	};

	class cPhysicsBodyNewton : public iPhysicsBody
	{
	friend class cPhysicsBodyNewtonCallback;
	public:
		cPhysicsBodyNewton(const tString &asName,iPhysicsWorld *apWorld,iCollideShape *apShape);
		~cPhysicsBodyNewton() override;

		void SetMaterial(iPhysicsMaterial* apMaterial) override;

		void SetLinearVelocity(const cVector3f &avVel) override;
		cVector3f GetLinearVelocity() const override;
		void SetAngularVelocity(const cVector3f &avVel) override;
		cVector3f GetAngularVelocity() const override;
		void SetLinearDamping(float afDamping) override;
		float GetLinearDamping() const override;
		void SetAngularDamping(float afDamping) override;
		float GetAngularDamping() const override;
		void SetMaxLinearSpeed(float afSpeed) override;
		float GetMaxLinearSpeed() const override;
		void SetMaxAngularSpeed(float afDamping) override;
		float GetMaxAngularSpeed() const override;
		cMatrixf GetInertiaMatrix() override;

		void  SetMass(float afMass) override;
		float GetMass() const override;
		void  SetMassCentre(const cVector3f& avCentre) override;
		cVector3f GetMassCentre() const override;

		void AddForce(const cVector3f &avForce) override;
		void AddForceAtPosition(const cVector3f &avForce, const cVector3f &avPos) override;
		void AddTorque(const cVector3f &avTorque) override;
		void AddImpulse(const cVector3f &avImpulse) override;
		void AddImpulseAtPosition(const cVector3f &avImpulse, const cVector3f &avPos) override;

		void SetEnabled(bool abEnabled) override;
		bool GetEnabled() const override;
		bool IsSleeping() const override;
		bool IsFrozen() const override;
		void SetAutoDisable(bool abEnabled) override;
		bool GetAutoDisable() const override;
		void SetAutoDisableLinearThreshold(float afThresold) override;
		float GetAutoDisableLinearThreshold() const override;
		void SetAutoDisableAngularThreshold(float afThresold) override;
		float GetAutoDisableAngularThreshold() const override;
		void SetAutoDisableNumSteps(int alNum) override;
		int GetAutoDisableNumSteps() const override;
		void SetContinuousCollision(bool abOn) override;
		bool GetContinuousCollision() override;


		void SetGravity(bool abEnabled) override;
		bool GetGravity() const override;

		void RenderDebugGeometry(iLowLevelGraphics *apLowLevel,const cColor &aColor) override;

		NewtonBody *GetNewtonBody(){ return mpNewtonBody;}

		void ClearForces();

		void DeleteLowLevel() override;

		static void SetUseCallback(bool abX){ mbUseCallback = abX;}
	private:

		static void OnTransformCallback(const NewtonBody* apBody, const dFloat* apMatrix, int alThreadIndex);
		static void OnUpdateCallback(NewtonBody* apBody, float afTimeStep, int alThreadIndex);

		NewtonBody *mpNewtonBody;
		NewtonWorld *mpNewtonWorld;

		cPhysicsBodyNewtonCallback *mpCallback;

		static bool mbUseCallback;

		//Properties
		bool mbGravity;

		float mfMaxLinearSpeed;
		float mfMaxAngularSpeed;
		float mfMass;

		float mfAutoDisableLinearThreshold;
		float mfAutoDisableAngularThreshold;
		int mlAutoDisableNumSteps;

		// Forces that will be set and clear on update callback
		cVector3f mvTotalForce;
		cVector3f mvTotalTorque;
	};
};
#endif // HPL_PHYSICS_BODY_NEWTON_H
