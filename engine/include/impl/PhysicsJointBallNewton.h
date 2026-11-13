// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_JOINT_BALL_NEWTON_H
#define HPL_PHYSICS_JOINT_BALL_NEWTON_H

#include "physics/PhysicsJointBall.h"
#include "impl/PhysicsJointNewton.h"

namespace hpl {

	class cPhysicsJointBallNewton : public iPhysicsJointNewton<iPhysicsJointBall>
	{
	public:
		cPhysicsJointBallNewton(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
			iPhysicsWorld *apWorld, const cVector3f &avPivotPoint);
		~cPhysicsJointBallNewton() override;

		void SetConeLimits(const cVector3f& avPin, float afMaxConeAngle, float afMaxTwistAngle) override;
		cVector3f GetAngles() override;

		cVector3f GetVelocity() override;
		cVector3f GetAngularVelocity() override;
		cVector3f GetForce() override;

		float GetDistance() override;
		float GetAngle() override;

	private:
	};
};
#endif // HPL_PHYSICS_JOINT_BALL_NEWTON_H
