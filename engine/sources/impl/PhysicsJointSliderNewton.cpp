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
#include "impl/PhysicsJointSliderNewton.h"

#include <algorithm>

#include "impl/PhysicsBodyNewton.h"
#include "impl/PhysicsWorldNewton.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	cPhysicsJointSliderNewton::~cPhysicsJointSliderNewton()
	{

	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cPhysicsJointSliderNewton::SetMaxDistance(float afX)
	{
		mfMaxDistance = afX;
	}
	void cPhysicsJointSliderNewton::SetMinDistance(float afX)
	{
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

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	float cPhysicsJointSliderNewton::GetDistance()
	{
		return NewtonSliderGetJointPosit(mpNewtonJoint);
	}
	float cPhysicsJointSliderNewton::GetAngle()
	{
		return 0;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// STATIC CALLBACKS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	// How near a limit its stop holds, and how much of the way back to it
	// that stop pulls in one step
	static const float kfLimitBand = 0.003f;
	static const float kfLimitPull = 0.3f;

	unsigned cPhysicsJointSliderNewton::LimitCallback(const NewtonJoint* pSlider, NewtonHingeSliderUpdateDesc* pDesc)
	{
		cPhysicsJointSliderNewton* pSliderJoint = (cPhysicsJointSliderNewton*)NewtonJointGetUserData(pSlider);

		//pSliderJoint->OnPhysicsUpdate();

		float fDistance = NewtonSliderGetJointPosit (pSlider);
		//Log("Dist: %f\n",fDistance);

		if(pSliderJoint->mfMinDistance == 0 && pSliderJoint->mfMaxDistance == 0) return 0;

		//Avoid oscillation
		CheckLimitAutoSleep(pSliderJoint, pSliderJoint->mfMinDistance,pSliderJoint->mfMaxDistance,fDistance);

		if (fDistance < pSliderJoint->mfMinDistance)
		{
			pSliderJoint->OnMinLimit();

			pDesc->m_accel = NewtonSliderCalculateStopAccel (pSlider, pDesc, pSliderJoint->mfMinDistance);
			pDesc->m_minFriction =0;

			//Log("Under Min. Acc: %f Dist %f\n",pDesc->m_accel,fDistance);

			return 1;
		}
		else if (fDistance > pSliderJoint->mfMaxDistance)
		{
			pSliderJoint->OnMaxLimit();

			pDesc->m_accel = NewtonSliderCalculateStopAccel (pSlider, pDesc, pSliderJoint->mfMaxDistance);
			pDesc->m_maxFriction =0;

			//Log("Over Max. Acc: %f Dist %f\n",pDesc->m_accel,fDistance);
			return 1;
		}
		// Within a few millimetres of a limit the stop already holds, pushing
		// away from it only. Engaged only once past it, a slider pushed onto its
		// limit (a door kept shut by its controller) went free one step and was
		// stopped the next, and Newton 2 never let it come to rest there. It aims
		// a little past the limit, so what rests there reads as being at it.
		else if (fDistance < pSliderJoint->mfMinDistance + kfLimitBand)
		{
			pSliderJoint->OnMinLimit();

			float fSpeed = std::min(NewtonSliderGetJointVeloc(pSlider), 0.0f);
			pDesc->m_accel = (kfLimitPull * (pSliderJoint->mfMinDistance - kfLimitBand*0.25f - fDistance) / pDesc->m_timestep - fSpeed) / pDesc->m_timestep;
			pDesc->m_minFriction =0;
			return 1;
		}
		else if (fDistance > pSliderJoint->mfMaxDistance - kfLimitBand)
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

		//Log("Nothing, Dist %f\n",fDistance);

		return 0;
	}
	//-----------------------------------------------------------------------

}
