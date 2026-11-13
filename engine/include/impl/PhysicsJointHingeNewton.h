// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_JOINT_HINGE_NEWTON_H
#define HPL_PHYSICS_JOINT_HINGE_NEWTON_H

#include "physics/PhysicsJointHinge.h"
#include "impl/PhysicsJointNewton.h"

namespace hpl {

	class cPhysicsJointHingeNewton : public iPhysicsJointNewton<iPhysicsJointHinge>
	{
	public:
		cPhysicsJointHingeNewton(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
			iPhysicsWorld *apWorld,const cVector3f &avPivotPoint, const cVector3f avPinDir);
		~cPhysicsJointHingeNewton() override;

		void SetMaxAngle(float afAngle) override;
		void SetMinAngle(float afAngle) override;
		float GetMaxAngle() override;
		float GetMinAngle() override;

		cVector3f GetVelocity() override;
		cVector3f GetAngularVelocity() override;
		cVector3f GetForce() override;

		float GetDistance() override;
		float GetAngle() override;

	private:
		float mfPreviousAngle;
		static unsigned LimitCallback(const NewtonJoint* pHinge, NewtonHingeSliderUpdateDesc* pDesc);
	};
};
#endif // HPL_PHYSICS_JOINT_HINGE_NEWTON_H
