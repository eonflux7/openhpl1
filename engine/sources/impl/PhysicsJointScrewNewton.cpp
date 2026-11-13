// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/PhysicsJointScrewNewton.h"

#include "impl/PhysicsBodyNewton.h"
#include "impl/PhysicsWorldNewton.h"

namespace hpl {

	cPhysicsJointScrewNewton::cPhysicsJointScrewNewton(const tString &asName,
		iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
		iPhysicsWorld *apWorld,const cVector3f &avPivotPoint, const cVector3f avPinDir)
		: iPhysicsJointNewton<iPhysicsJointScrew>(asName,apParentBody,apChildBody,apWorld,avPivotPoint)
	{
		mvPin = avPinDir;
		mvPin.Normalise();

		mpNewtonJoint = NewtonConstraintCreateCorkscrew(mpNewtonWorld, avPivotPoint.v, avPinDir.v, mpNewtonChildBody,
			mpNewtonParentBody);

		NewtonJointSetUserData(mpNewtonJoint, (void*) this);
		NewtonCorkscrewSetUserCallback(mpNewtonJoint,LimitCallback);

		mfMaxDistance =0;
		mfMinDistance =0;

		mvPinDir = avPinDir;
		mvPivotPoint = avPivotPoint;
	}

	cPhysicsJointScrewNewton::~cPhysicsJointScrewNewton()
	{

	}

	void cPhysicsJointScrewNewton::SetMaxDistance(float afX)
	{
		if(mfMaxDistance != afX) WakeBodies();
		mfMaxDistance = afX;
	}
	void cPhysicsJointScrewNewton::SetMinDistance(float afX)
	{
		if(mfMinDistance != afX) WakeBodies();
		mfMinDistance = afX;
	}
	float cPhysicsJointScrewNewton::GetMaxDistance()
	{
		return mfMaxDistance;
	}
	float cPhysicsJointScrewNewton::GetMinDistance()
	{
		return mfMinDistance;
	}

	cVector3f cPhysicsJointScrewNewton::GetVelocity()
	{
		float fSpeed = NewtonCorkscrewGetJointVeloc(mpNewtonJoint);
		return mvPin * fSpeed;
	}
	cVector3f cPhysicsJointScrewNewton::GetAngularVelocity()
	{
		return cVector3f(0,0,0);
	}
	cVector3f cPhysicsJointScrewNewton::GetForce()
	{
		cVector3f vForce;
		NewtonCorkscrewGetJointForce(mpNewtonJoint,&vForce.v[0]);
		return vForce;
	}

	float cPhysicsJointScrewNewton::GetDistance()
	{
		return NewtonCorkscrewGetJointPosit(mpNewtonJoint);
	}
	float cPhysicsJointScrewNewton::GetAngle()
	{
		return NewtonCorkscrewGetJointAngle(mpNewtonJoint);
	}

	unsigned cPhysicsJointScrewNewton::LimitCallback(const NewtonJoint* pScrew, NewtonHingeSliderUpdateDesc* pDesc)
	{
		cPhysicsJointScrewNewton* pScrewJoint = (cPhysicsJointScrewNewton*)NewtonJointGetUserData(pScrew);

		//pScrewJoint->OnPhysicsUpdate();

		float fDistance = NewtonCorkscrewGetJointPosit (pScrew);
		//Log("Dist: %f\n",fDistance);

		if(pScrewJoint->mfMinDistance == 0 && pScrewJoint->mfMaxDistance == 0) return 0;

		//Avoid oscillation
		CheckLimitAutoSleep(pScrewJoint, pScrewJoint->mfMinDistance,pScrewJoint->mfMaxDistance,fDistance);

		if (fDistance < pScrewJoint->mfMinDistance)
		{
			pScrewJoint->OnMinLimit();

			pDesc->m_accel = NewtonCorkscrewCalculateStopAccel (pScrew, pDesc, pScrewJoint->mfMinDistance);
			pDesc->m_minFriction =0;
			return 1;
		}
		else if (fDistance > pScrewJoint->mfMaxDistance)
		{
			pScrewJoint->OnMaxLimit();

			pDesc->m_accel = NewtonCorkscrewCalculateStopAccel (pScrew, pDesc, pScrewJoint->mfMaxDistance);
			pDesc->m_maxFriction =0;
			return 1;
		}
		else
		{
			if(pScrewJoint->mpParentBody ==nullptr || pScrewJoint->mpParentBody->GetMass()==0)
			{
				if(	(pScrewJoint->mfStickyMaxDistance != 0 &&
					fabs(fDistance - pScrewJoint->mfMaxDistance) < pScrewJoint->mfStickyMaxDistance)
					||
					(pScrewJoint->mfStickyMinDistance != 0 &&
					fabs(fDistance - pScrewJoint->mfMinDistance) < pScrewJoint->mfStickyMinDistance)
				 )
				{
					pScrewJoint->mpChildBody->SetAngularVelocity(0);
					pScrewJoint->mpChildBody->SetLinearVelocity(0);
				}
			}

			pScrewJoint->OnNoLimit();
		}


		return 0;
	}
}
