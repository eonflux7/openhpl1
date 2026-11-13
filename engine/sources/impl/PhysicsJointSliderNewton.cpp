// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/PhysicsJointSliderNewton.h"

#include <algorithm>

#include "impl/PhysicsBodyNewton.h"
#include "impl/PhysicsWorldNewton.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	cPhysicsJointSliderNewton::cPhysicsJointSliderNewton(const tString &asName,
		iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
		iPhysicsWorld *apWorld,const cVector3f &avPivotPoint, const cVector3f avPinDir)
		: iPhysicsJointNewton<iPhysicsJointSlider>(asName,apParentBody,apChildBody,apWorld,avPivotPoint)
	{
		mvPin = avPinDir;
		mvPin.Normalise();

		mpNewtonJoint = NewtonConstraintCreateSlider(mpNewtonWorld, avPivotPoint.v, avPinDir.v, mpNewtonChildBody,
													mpNewtonParentBody);
		NewtonJointSetUserData(mpNewtonJoint, (void*) this);
		NewtonSliderSetUserCallback(mpNewtonJoint,LimitCallback);

		mfMaxDistance =0;
		mfMinDistance =0;

		mvPinDir = avPinDir;
		mvPivotPoint = avPivotPoint;
	}

	cPhysicsJointSliderNewton::~cPhysicsJointSliderNewton()
	{

	}

	void cPhysicsJointSliderNewton::SetMaxDistance(float afX)
	{
		if(mfMaxDistance != afX) WakeBodies();
		mfMaxDistance = afX;
	}
	void cPhysicsJointSliderNewton::SetMinDistance(float afX)
	{
		if(mfMinDistance != afX) WakeBodies();
		mfMinDistance = afX;
	}
	float cPhysicsJointSliderNewton::GetMaxDistance()
	{
		return mfMaxDistance;
	}
	float cPhysicsJointSliderNewton::GetMinDistance()
	{
		return mfMinDistance;
	}

	cVector3f cPhysicsJointSliderNewton::GetVelocity()
	{
		float fSpeed = NewtonSliderGetJointVeloc(mpNewtonJoint);
		return mvPin * fSpeed;
	}
	cVector3f cPhysicsJointSliderNewton::GetAngularVelocity()
	{
		return cVector3f(0,0,0);
	}
	cVector3f cPhysicsJointSliderNewton::GetForce()
	{
		cVector3f vForce;
		NewtonSliderGetJointForce(mpNewtonJoint,&vForce.v[0]);
		return vForce;
	}

	float cPhysicsJointSliderNewton::GetDistance()
	{
		return NewtonSliderGetJointPosit(mpNewtonJoint);
	}
	float cPhysicsJointSliderNewton::GetAngle()
	{
		return 0;
	}

	// How near a limit its stop holds, and how much of the way back to it
	// that stop pulls in one step
	static const float kfLimitBand = 0.003f;
	static const float kfLimitPull = 0.3f;

	unsigned cPhysicsJointSliderNewton::LimitCallback(const NewtonJoint* pSlider, NewtonHingeSliderUpdateDesc* pDesc)
	{
		cPhysicsJointSliderNewton* pSliderJoint = (cPhysicsJointSliderNewton*)NewtonJointGetUserData(pSlider);

		float fDistance = NewtonSliderGetJointPosit (pSlider);

		if(pSliderJoint->mfMinDistance == 0 && pSliderJoint->mfMaxDistance == 0) return 0;

		//Avoid oscillation
		CheckLimitAutoSleep(pSliderJoint, pSliderJoint->mfMinDistance,pSliderJoint->mfMaxDistance,fDistance);

		const bool bUseBand = pSliderJoint->mpParentBody == nullptr || pSliderJoint->mpParentBody->GetMass() == 0;

		if (fDistance < pSliderJoint->mfMinDistance)
		{
			pSliderJoint->OnMinLimit();

			pDesc->m_accel = NewtonSliderCalculateStopAccel (pSlider, pDesc, pSliderJoint->mfMinDistance);
			pDesc->m_minFriction =0;

			return 1;
		}
		else if (fDistance > pSliderJoint->mfMaxDistance)
		{
			pSliderJoint->OnMaxLimit();

			pDesc->m_accel = NewtonSliderCalculateStopAccel (pSlider, pDesc, pSliderJoint->mfMaxDistance);
			pDesc->m_maxFriction =0;

			return 1;
		}
		// Within a few millimetres of a limit the stop already holds, pushing
		// away from it only. Engaged only once past it, a slider pushed onto its
		// limit (a door kept shut by its controller) went free one step and was
		// stopped the next, and Newton 2 never let it come to rest there. It aims
		// a little past the limit, so what rests there reads as being at it.
		// Only on the static world: the pull is on the child alone (see the hinge).
		else if (bUseBand && fDistance < pSliderJoint->mfMinDistance + kfLimitBand)
		{
			pSliderJoint->OnMinLimit();

			float fSpeed = std::min(NewtonSliderGetJointVeloc(pSlider), 0.0f);
			pDesc->m_accel = (kfLimitPull * (pSliderJoint->mfMinDistance - kfLimitBand*0.25f - fDistance) / pDesc->m_timestep - fSpeed) / pDesc->m_timestep;
			pDesc->m_minFriction =0;
			return 1;
		}
		else if (bUseBand && fDistance > pSliderJoint->mfMaxDistance - kfLimitBand)
		{
			pSliderJoint->OnMaxLimit();

			float fSpeed = std::max(NewtonSliderGetJointVeloc(pSlider), 0.0f);
			pDesc->m_accel = (kfLimitPull * (pSliderJoint->mfMaxDistance + kfLimitBand*0.25f - fDistance) / pDesc->m_timestep - fSpeed) / pDesc->m_timestep;
			pDesc->m_maxFriction =0;
			return 1;
		}
		else
		{
			if(pSliderJoint->mpParentBody ==nullptr || pSliderJoint->mpParentBody->GetMass()==0)
			{
				if(	(pSliderJoint->mfStickyMaxDistance != 0 &&
					 fabs(fDistance - pSliderJoint->mfMaxDistance) < pSliderJoint->mfStickyMaxDistance)
					 ||
					(pSliderJoint->mfStickyMinDistance != 0 &&
					 fabs(fDistance - pSliderJoint->mfMinDistance) < pSliderJoint->mfStickyMinDistance)
				 )
				{
					pSliderJoint->mpChildBody->SetAngularVelocity(0);
					pSliderJoint->mpChildBody->SetLinearVelocity(0);
				}
			}

			pSliderJoint->OnNoLimit();
		}

		return 0;
	}
}
