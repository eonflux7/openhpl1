// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_JOINT_SLIDER_NEWTON_H
#define HPL_PHYSICS_JOINT_SLIDER_NEWTON_H

#include "physics/PhysicsJointSlider.h"
#include "impl/PhysicsJointNewton.h"

namespace hpl {

	class cPhysicsJointSliderNewton : public iPhysicsJointNewton<iPhysicsJointSlider>
	{
	public:
		cPhysicsJointSliderNewton(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
							iPhysicsWorld *apWorld,const cVector3f &avPivotPoint, const cVector3f avPinDir) ;
		~cPhysicsJointSliderNewton() override;

		void SetMaxDistance(float afX) override;
		void SetMinDistance(float afX) override;
		float GetMaxDistance() override;
		float GetMinDistance() override;

		cVector3f GetVelocity() override;
		cVector3f GetAngularVelocity() override;
		cVector3f GetForce() override;

		float GetDistance() override;
		float GetAngle() override;

	private:
		static unsigned LimitCallback(const NewtonJoint* pSlider, NewtonHingeSliderUpdateDesc* pDesc);
	};
};
#endif // HPL_PHYSICS_JOINT_SLIDER_NEWTON_H
