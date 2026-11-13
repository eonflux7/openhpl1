// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/PhysicsJointBallNewton.h"

#include "impl/PhysicsBodyNewton.h"
#include "impl/PhysicsWorldNewton.h"

namespace hpl {

	cPhysicsJointBallNewton::cPhysicsJointBallNewton(const tString &asName,
						iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
						iPhysicsWorld *apWorld, const cVector3f &avPivotPoint)
	: iPhysicsJointNewton<iPhysicsJointBall>(asName,apParentBody,apChildBody,apWorld,avPivotPoint)
	{
		mpNewtonJoint = NewtonConstraintCreateBall(mpNewtonWorld,avPivotPoint.v,
												mpNewtonChildBody, mpNewtonParentBody);

		mvPinDir = cVector3f(0,0,0);
		mvPivotPoint = avPivotPoint;

		mfMaxConeAngle =0;
		mfMaxTwistAngle = 0;
		mvConePin = mvPinDir;
	}

	cPhysicsJointBallNewton::~cPhysicsJointBallNewton()
	{
	}

	void cPhysicsJointBallNewton::SetConeLimits(const cVector3f& avPin, float afMaxConeAngle, float afMaxTwistAngle)
	{
		NewtonBallSetConeLimits(mpNewtonJoint, avPin.v,afMaxConeAngle,afMaxTwistAngle);
		mvConePin = avPin;
		mvPinDir = mvConePin;
		mfMaxConeAngle = afMaxConeAngle;
		mfMaxTwistAngle = afMaxTwistAngle;
	}

	cVector3f cPhysicsJointBallNewton::GetAngles()
	{
		cVector3f vAngles;
		NewtonBallGetJointAngle(mpNewtonJoint,&vAngles.v[0]);
		return vAngles;
	}

	cVector3f cPhysicsJointBallNewton::GetVelocity()
	{
		return cVector3f(0,0,0);
	}
	cVector3f cPhysicsJointBallNewton::GetAngularVelocity()
	{
		cVector3f vVel;
		NewtonBallGetJointOmega(mpNewtonJoint,&vVel.v[0]);
		return vVel;
	}
	cVector3f cPhysicsJointBallNewton::GetForce()
	{
		cVector3f vForce;
		NewtonBallGetJointForce(mpNewtonJoint,&vForce.v[0]);
		return vForce;
	}

	float cPhysicsJointBallNewton::GetDistance()
	{
		return 0;
	}
	float cPhysicsJointBallNewton::GetAngle()
	{
		return 0;
	}
}
