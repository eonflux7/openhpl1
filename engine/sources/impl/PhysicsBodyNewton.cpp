// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "impl/PhysicsBodyNewton.h"

#include <algorithm>

#include "impl/CollideShapeNewton.h"
#include "impl/PhysicsWorldNewton.h"
#include "impl/PhysicsMaterialNewton.h"
#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "math/Math.h"
#include "scene/Node3D.h"

namespace hpl {

	bool cPhysicsBodyNewton::mbUseCallback = true;

	cPhysicsBodyNewton::cPhysicsBodyNewton(const tString &asName,iPhysicsWorld *apWorld,iCollideShape *apShape)
	: iPhysicsBody(asName,apWorld, apShape)
	{
		cPhysicsWorldNewton *pWorldNewton = static_cast<cPhysicsWorldNewton*>(apWorld);
		cCollideShapeNewton *pShapeNewton = static_cast<cCollideShapeNewton*>(apShape);

		mpNewtonWorld = pWorldNewton->GetNewtonWorld();
		mpNewtonBody = NewtonCreateBody(pWorldNewton->GetNewtonWorld(),
										pShapeNewton->GetNewtonCollision(),
										&cMatrixf::Identity.m[0][0]);

		mpCallback = hplNew( cPhysicsBodyNewtonCallback, () );

		AddCallback(mpCallback);

		// Setup the callbacks and set this body as user data
		// This is so that the transform gets updated and
		// to add gravity, forces and user sink.
		NewtonBodySetForceAndTorqueCallback(mpNewtonBody,OnUpdateCallback);
		NewtonBodySetTransformCallback(mpNewtonBody, OnTransformCallback);
		NewtonBodySetUserData(mpNewtonBody, this);

		//Set default property settings
		mbGravity = true;

		mfMaxLinearSpeed =0;
		mfMaxAngularSpeed =0;
		mfMass =0;

		mfAutoDisableLinearThreshold = 0.01f;
		mfAutoDisableAngularThreshold = 0.01f;
		mlAutoDisableNumSteps = 10;

	}

	cPhysicsBodyNewton::~cPhysicsBodyNewton()
	{
	}

	void cPhysicsBodyNewton::DeleteLowLevel()
	{
		NewtonDestroyBody(mpNewtonWorld,mpNewtonBody);
		hplDelete(mpCallback);
	}

	// CALLBACK METHODS

	void cPhysicsBodyNewtonCallback::OnTransformUpdate(iEntity3D * apEntity)
	{
		if(cPhysicsBodyNewton::mbUseCallback==false) return;

		cPhysicsBodyNewton *pRigidBody = static_cast<cPhysicsBodyNewton*>(apEntity);
		cMatrixf mtxTranspose = apEntity->GetLocalMatrix().GetTranspose();

		//Newton only takes a rigid matrix: a scaled one (Black Plague's maps place
		//the artefact statue with its mesh scale) makes the body NaN under Newton 2,
		//and one NaN body hides most of the world from ray casts. Remove the scale.
		bool bScaled = false;
		for(int i=0; i<3; ++i)
		{
			cVector3f vAxis(mtxTranspose.m[i][0], mtxTranspose.m[i][1], mtxTranspose.m[i][2]);
			float fLength = vAxis.Length();
			if(fLength > 0 && std::fabs(fLength - 1) > 1e-4f)
			{
				for(int j=0; j<3; ++j) mtxTranspose.m[i][j] /= fLength;
				bScaled = true;
			}
		}

		//The body keeps the rigid matrix too. Newton's first step would give it
		//anyway, but until then its bounding volume had the scale: level 19's
		//rocks (x200) and trash can lids (x33) were lifted metres "out of the
		//floor" into the air at load.
		if(bScaled)
		{
			pRigidBody->m_mtxLocalTransform.FromTranspose(&mtxTranspose.m[0][0]);
			cPhysicsBodyNewton::mbUseCallback = false;
			pRigidBody->SetTransformUpdated(true);
			cPhysicsBodyNewton::mbUseCallback = true;
		}

		NewtonBodySetMatrix(pRigidBody->mpNewtonBody, &mtxTranspose.m[0][0]);

		if(pRigidBody->mpNode) pRigidBody->mpNode->SetMatrix(apEntity->GetLocalMatrix());
	}

	void cPhysicsBodyNewton::SetMaterial(iPhysicsMaterial* apMaterial)
	{
		mpMaterial = apMaterial;

		if(apMaterial == nullptr) return;

		cPhysicsMaterialNewton* pNewtonMat = static_cast<cPhysicsMaterialNewton*>(mpMaterial);

		NewtonBodySetMaterialGroupID(mpNewtonBody, pNewtonMat->GetId());
	}

	void cPhysicsBodyNewton::SetLinearVelocity(const cVector3f &avVel)
	{
		NewtonBodySetVelocity(mpNewtonBody, avVel.v);
	}
	cVector3f cPhysicsBodyNewton::GetLinearVelocity() const
	{
		cVector3f vVel;
		NewtonBodyGetVelocity(mpNewtonBody, vVel.v);
		return vVel;
	}

	void cPhysicsBodyNewton::SetAngularVelocity(const cVector3f &avVel)
	{
		NewtonBodySetOmega(mpNewtonBody, avVel.v);
	}
	cVector3f cPhysicsBodyNewton::GetAngularVelocity() const
	{
		cVector3f vVel;
		NewtonBodyGetOmega(mpNewtonBody, vVel.v);
		return vVel;
	}

	void cPhysicsBodyNewton::SetLinearDamping(float afDamping)
	{
		NewtonBodySetLinearDamping(mpNewtonBody,afDamping);
	}
	float cPhysicsBodyNewton::GetLinearDamping() const
	{
		return NewtonBodyGetLinearDamping(mpNewtonBody);
	}

	void cPhysicsBodyNewton::SetAngularDamping(float afDamping)
	{
		float fDamp[3] = {afDamping,afDamping,afDamping};
		NewtonBodySetAngularDamping(mpNewtonBody,fDamp);
	}
	float cPhysicsBodyNewton::GetAngularDamping() const
	{
		float fDamp[3];
		NewtonBodyGetAngularDamping(mpNewtonBody,fDamp);
		return fDamp[0];
	}

	void cPhysicsBodyNewton::SetMaxLinearSpeed(float afSpeed)
	{
		mfMaxLinearSpeed = afSpeed;
	}
	float cPhysicsBodyNewton::GetMaxLinearSpeed() const
	{
		return mfMaxLinearSpeed;
	}

	void cPhysicsBodyNewton::SetMaxAngularSpeed(float afDamping)
	{
		mfMaxAngularSpeed = afDamping;
	}
	float cPhysicsBodyNewton::GetMaxAngularSpeed() const
	{
		return mfMaxAngularSpeed;
	}

	cMatrixf cPhysicsBodyNewton::GetInertiaMatrix()
	{
		float fIxx, fIyy, fIzz, fMass;

		NewtonBodyGetMassMatrix(mpNewtonBody,&fMass, &fIxx, &fIyy, &fIzz);

		cMatrixf mtxRot = GetLocalMatrix().GetRotation();
		cMatrixf mtxTransRot = mtxRot.GetTranspose();
		cMatrixf mtxI(	fIxx,0,	  0,	0,
						0,	 fIyy,0,	0,
						0,	 0,	  fIzz,	0,
						0,	 0,	  0,	1);

		return cMath::MatrixMul(cMath::MatrixMul(mtxRot,mtxI), mtxTransRot);
	}

	namespace
	{
		// Volume integrals over polygons (Newton's polyhedra mass properties).
		struct cMassIntegrals
		{
			double mfVolume = 0;			// times 6
			double mvFirst[3] = {0,0,0};	// integral of x, y, z times 24
			double mvSecond[3] = {0,0,0};	// integral of x², y², z² times 60
		};

		void AddFaceIntegrals(void *apUserData, int alVertexCount, const dFloat *apVertex, int)
		{
			cMassIntegrals *pData = static_cast<cMassIntegrals*>(apUserData);
			const dFloat *p0 = &apVertex[0];

			// Newton 1.53 left out faces whose first three corners span a
			// normal of squared length 1e-8 or less.
			double vA[3] = {apVertex[3]-p0[0], apVertex[4]-p0[1], apVertex[5]-p0[2]};
			double vB[3] = {apVertex[6]-p0[0], apVertex[7]-p0[1], apVertex[8]-p0[2]};
			double vN[3] = {	vA[1]*vB[2] - vA[2]*vB[1],
								vA[2]*vB[0] - vA[0]*vB[2],
								vA[0]*vB[1] - vA[1]*vB[0]};
			if(vN[0]*vN[0] + vN[1]*vN[1] + vN[2]*vN[2] <= 1.0e-8) return;

			for(int i = 2; i < alVertexCount; ++i)
			{
				const dFloat *p1 = &apVertex[(i-1)*3];
				const dFloat *p2 = &apVertex[i*3];
				double vE1[3] = {p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2]};
				double vE2[3] = {p2[0]-p0[0], p2[1]-p0[1], p2[2]-p0[2]};
				double vD[3] = {	vE1[1]*vE2[2] - vE1[2]*vE2[1],
									vE1[2]*vE2[0] - vE1[0]*vE2[2],
									vE1[0]*vE2[1] - vE1[1]*vE2[0]};
				for(int k = 0; k < 3; ++k)
				{
					double w0 = p0[k], w1 = p1[k], w2 = p2[k];
					double fTemp0 = w0 + w1;
					double fTemp1 = w0 * w0;
					double fTemp2 = fTemp1 + w1 * fTemp0;
					double f1 = fTemp0 + w2;
					double f2 = fTemp2 + w2 * f1;
					double f3 = w0 * fTemp1 + w1 * fTemp2 + w2 * f2;
					if(k == 0) pData->mfVolume += vD[0] * f1;
					pData->mvFirst[k] += vD[k] * f2;
					pData->mvSecond[k] += vD[k] * f3;
				}
			}
		}

		/**
		 * A shape's centre of mass and inertia per unit mass as Newton 1.53
		 * gave them. It summed volume integrals over the faces of the shape
		 * (of each part of a compound), but:
		 *  - skipped faces whose first three corners were nearly in line. A
		 *    cylinder's ends are 24-gons, so those of cylinders under about
		 *    7.5 cm radius were left out: the volume, centre and inertia of
		 *    such a cylinder came from its side only;
		 *  - divided by at least 0.001 m³ (Newton 2.36: 1e-6 for compounds),
		 *    which pulls the centre of small shapes towards the body's origin.
		 * Maps were made for the centres this gave: Requiem level 5's pipe only
		 * fits its socket with it.
		 */
		void MassLikeNewton153(cCollideShapeNewton *apShape, cVector3f &avInertia, cVector3f &avOffset)
		{
			cMassIntegrals total;
			cMatrixf mtxIdentity = cMatrixf::Identity;
			for(int i = 0; i < apShape->GetSubShapeNum(); ++i)
			{
				cCollideShapeNewton *pPart = static_cast<cCollideShapeNewton*>(apShape->GetSubShape(i));
				NewtonCollisionForEachPolygonDo(pPart->GetNewtonCollision(), &mtxIdentity.m[0][0],
												AddFaceIntegrals, &total);
			}

			double fVolume = std::max(total.mfVolume / 6.0, 0.001);
			double vCentre[3], vSecond[3];
			for(int k = 0; k < 3; ++k)
			{
				vCentre[k] = total.mvFirst[k] / 24.0 / fVolume;
				vSecond[k] = total.mvSecond[k] / 60.0 / fVolume;
			}
			avOffset = cVector3f((float)vCentre[0], (float)vCentre[1], (float)vCentre[2]);
			avInertia = cVector3f(
				(float)(vSecond[1] + vSecond[2] - (vCentre[1]*vCentre[1] + vCentre[2]*vCentre[2])),
				(float)(vSecond[0] + vSecond[2] - (vCentre[2]*vCentre[2] + vCentre[0]*vCentre[0])),
				(float)(vSecond[0] + vSecond[1] - (vCentre[0]*vCentre[0] + vCentre[1]*vCentre[1])));
		}
	}

	void  cPhysicsBodyNewton::SetMass(float afMass)
	{
		cCollideShapeNewton *pShapeNewton = static_cast<cCollideShapeNewton*>(mpShape);

		cVector3f vInertia;// = pShapeNewton->GetInertia(afMass);
		cVector3f vOffset;

		switch(pShapeNewton->GetType())
		{
		case eCollideShapeType_Box:
		case eCollideShapeType_Sphere:
		case eCollideShapeType_Cylinder:
		case eCollideShapeType_Capsule:
		case eCollideShapeType_ConvexHull:
		case eCollideShapeType_Compound:
			MassLikeNewton153(pShapeNewton, vInertia, vOffset);
			break;
		default:
			NewtonConvexCollisionCalculateInertialMatrix(pShapeNewton->GetNewtonCollision(),
														vInertia.v, vOffset.v);
		}
		vInertia = vInertia * afMass;

		// Newton 1.53 kept each axis of inertia above 0.01 * mass; Newton 2 only
		// above 0.001 * mass. The game's torques (e.g. the damping on a grabbed
		// object) are tuned for the old limit and make thin objects spin up
		// without it.
		float fMinInertia = 0.01f * afMass;
		if(vInertia.x < fMinInertia) vInertia.x = fMinInertia;
		if(vInertia.y < fMinInertia) vInertia.y = fMinInertia;
		if(vInertia.z < fMinInertia) vInertia.z = fMinInertia;

		NewtonBodySetCentreOfMass(mpNewtonBody,vOffset.v);

		NewtonBodySetMassMatrix(mpNewtonBody, afMass, vInertia.x, vInertia.y, vInertia.z);
		mfMass = afMass;
	}
	float cPhysicsBodyNewton::GetMass() const
	{
		return mfMass;
	}

	void  cPhysicsBodyNewton::SetMassCentre(const cVector3f& avCentre)
	{
		NewtonBodySetCentreOfMass(mpNewtonBody,avCentre.v);
	}

	cVector3f cPhysicsBodyNewton::GetMassCentre() const
	{
		cVector3f vCentre;
		NewtonBodyGetCentreOfMass(mpNewtonBody,vCentre.v);
		return vCentre;
	}

	void cPhysicsBodyNewton::AddForce(const cVector3f &avForce)
	{
		mvTotalForce += avForce;
		SetEnabled(true);

	}

	void cPhysicsBodyNewton::AddForceAtPosition(const cVector3f &avForce, const cVector3f &avPos)
	{
		mvTotalForce += avForce;

		cVector3f vLocalPos = avPos - GetLocalPosition();
		cVector3f vMassCentre = GetMassCentre();
		if(vMassCentre != cVector3f(0,0,0))
		{
			vMassCentre = cMath::MatrixMul(GetLocalMatrix().GetRotation(),vMassCentre);
			vLocalPos -= vMassCentre;
		}

		cVector3f vTorque = cMath::Vector3Cross(vLocalPos, avForce);

		mvTotalTorque += vTorque;
		SetEnabled(true);

	}

	void cPhysicsBodyNewton::AddTorque(const cVector3f &avTorque)
	{
		mvTotalTorque += avTorque;
		SetEnabled(true);
	}

	void cPhysicsBodyNewton::AddImpulse(const cVector3f &avImpulse)
	{
		cVector3f vMassCentre = GetMassCentre();
		if(vMassCentre != cVector3f(0,0,0))
		{
			cVector3f vCentreOffset = cMath::MatrixMul( GetWorldMatrix().GetRotation(),
														vMassCentre);

			cVector3f vWorldPosition = GetWorldPosition() + vCentreOffset;
			NewtonBodyAddImpulse(mpNewtonBody, avImpulse.v, vWorldPosition.v);
		}
		else
		{
			NewtonBodyAddImpulse(mpNewtonBody, avImpulse.v, GetWorldPosition().v);
		}
	}
	void cPhysicsBodyNewton::AddImpulseAtPosition(const cVector3f &avImpulse, const cVector3f &avPos)
	{
		NewtonBodyAddImpulse(mpNewtonBody, avImpulse.v, avPos.v);
	}

	void cPhysicsBodyNewton::SetEnabled(bool abEnabled)
	{
		NewtonBodySetFreezeState(mpNewtonBody, abEnabled ? 0 : 1);

		//Newton 1's unfreeze woke a body at rest. Newton 2 keeps sleeping apart
		//from freezing, and only an impulse clears it, so without this nothing
		//woke what had gone to sleep: crates stayed in the air after what they
		//lay on was moved or switched off (Requiem level 3's container).
		if(abEnabled && NewtonBodyGetSleepState(mpNewtonBody))
		{
			const cVector3f vZero(0,0,0);
			NewtonBodyAddImpulse(mpNewtonBody, vZero.v, GetWorldPosition().v);
		}
	}
	bool cPhysicsBodyNewton::GetEnabled() const
	{
		return NewtonBodyGetSleepState(mpNewtonBody) ==0?false: true;
	}
	bool cPhysicsBodyNewton::IsSleeping() const
	{
		return NewtonBodyGetSleepState(mpNewtonBody) != 0;
	}
	bool cPhysicsBodyNewton::IsFrozen() const
	{
		return NewtonBodyGetFreezeState(mpNewtonBody) != 0;
	}

	void cPhysicsBodyNewton::SetAutoDisable(bool abEnabled)
	{
		NewtonBodySetAutoSleep(mpNewtonBody, abEnabled ? 1 : 0);
	}
	bool cPhysicsBodyNewton::GetAutoDisable() const
	{
		return NewtonBodyGetAutoSleep(mpNewtonBody) == 0 ? false : true;
	}

	void cPhysicsBodyNewton::SetAutoDisableLinearThreshold(float afThresold)
	{
		mfAutoDisableLinearThreshold = afThresold;
		// Newton 2 has no per-body sleep thresholds; the value is only stored.
	}
	float cPhysicsBodyNewton::GetAutoDisableLinearThreshold() const
	{
		return mfAutoDisableLinearThreshold;
	}

	void cPhysicsBodyNewton::SetAutoDisableAngularThreshold(float afThresold)
	{
		mfAutoDisableAngularThreshold = afThresold;
		// Newton 2 has no per-body sleep thresholds; the value is only stored.
	}
	float cPhysicsBodyNewton::GetAutoDisableAngularThreshold() const
	{
		return mfAutoDisableAngularThreshold;
	}

	void cPhysicsBodyNewton::SetAutoDisableNumSteps(int anNum)
	{
		mlAutoDisableNumSteps = anNum;
		// Newton 2 has no per-body sleep thresholds; the value is only stored.
	}

	int cPhysicsBodyNewton::GetAutoDisableNumSteps() const
	{
		return mlAutoDisableNumSteps;
	}

	void cPhysicsBodyNewton::SetContinuousCollision(bool abOn)
	{
		NewtonBodySetContinuousCollisionMode(mpNewtonBody,abOn ? 1 : 0);
	}

	bool cPhysicsBodyNewton::GetContinuousCollision()
	{
		return NewtonBodyGetContinuousCollisionMode(mpNewtonBody)==1 ? true : false;
	}

	void cPhysicsBodyNewton::SetGravity(bool abEnabled)
	{
		mbGravity = abEnabled;
	}
	bool cPhysicsBodyNewton::GetGravity() const
	{
		return mbGravity;
	}

	static iLowLevelGraphics *gpLowLevelGraphics;
	static cColor gDebugColor;

	static void RenderDebugPolygon(void* apUserData,
		int alVertexCount, const dFloat* apFaceVertex, int alId)
	{
		int i;

		i = alVertexCount - 1;
		cVector3f vP0(apFaceVertex[i * 3 + 0], apFaceVertex[i * 3 + 1], apFaceVertex[i * 3 + 2]);
		for (i = 0; i < alVertexCount; ++i)
		{
			cVector3f vP1 (apFaceVertex[i * 3 + 0], apFaceVertex[i * 3 + 1], apFaceVertex[i * 3 + 2]);

			gpLowLevelGraphics->DrawLine(vP0, vP1,gDebugColor);

			vP0 = vP1;
		}
	}

	void cPhysicsBodyNewton::RenderDebugGeometry(iLowLevelGraphics *apLowLevel,const cColor &aColor)
	{
		gpLowLevelGraphics = apLowLevel;
		gDebugColor = aColor;
		cMatrixf mtxTranspose = GetLocalMatrix().GetTranspose();
		NewtonCollisionForEachPolygonDo(NewtonBodyGetCollision(mpNewtonBody), &mtxTranspose.m[0][0],
										RenderDebugPolygon, nullptr);
	}

	void cPhysicsBodyNewton::ClearForces()
	{
		mvTotalForce = cVector3f(0,0,0);
		mvTotalTorque = cVector3f(0,0,0);
	}

	void cPhysicsBodyNewton::OnTransformCallback(const NewtonBody* apBody, const dFloat* apMatrix, int alThreadIndex)
	{
		cPhysicsBodyNewton* pRigidBody = (cPhysicsBodyNewton*) NewtonBodyGetUserData(apBody);

		pRigidBody->m_mtxLocalTransform.FromTranspose(apMatrix);

		mbUseCallback = false;
		pRigidBody->SetTransformUpdated(true);
		mbUseCallback = true;

		if(pRigidBody->mpNode)pRigidBody->mpNode->SetMatrix(pRigidBody->m_mtxLocalTransform);
	}

	//callback for buoyancy
	static cPlanef gSurfacePlane;
	static int BuoyancyPlaneCallback (const int32 alCollisionID, void *const apContext,
									const float* const afGlobalSpaceMatrix, float* const afGlobalSpacePlane)
	{
		afGlobalSpacePlane[0] = gSurfacePlane.a;
		afGlobalSpacePlane[1] = gSurfacePlane.b;
		afGlobalSpacePlane[2] = gSurfacePlane.c;
		afGlobalSpacePlane[3] = gSurfacePlane.d;
		return 1;
	}

	void cPhysicsBodyNewton::OnUpdateCallback(NewtonBody* apBody, float afTimeStep, int alThreadIndex)
	{
		float fMass;
		float fX,fY,fZ;

		cPhysicsBodyNewton* pRigidBody = (cPhysicsBodyNewton*) NewtonBodyGetUserData(apBody);

		if(pRigidBody->IsActive()==false) return;

		cVector3f vGravity = pRigidBody->mpWorld->GetGravity();

		//Create some gravity
		if (pRigidBody->mbGravity)
		{
			NewtonBodyGetMassMatrix(apBody, &fMass, &fX, &fY, &fZ);

			float fForce[3] = { fMass * vGravity.x, fMass * vGravity.y, fMass * vGravity.z};

			NewtonBodyAddForce(apBody, &fForce[0]);
		}

		// Create Buoyancy
		if (pRigidBody->mBuoyancy.mbActive)
		{
			gSurfacePlane = pRigidBody->mBuoyancy.mSurface;
			NewtonBodyAddBuoyancyForce( apBody,
										pRigidBody->mBuoyancy.mfDensity,
										pRigidBody->mBuoyancy.mfLinearViscosity,
										pRigidBody->mBuoyancy.mfAngularViscosity,
										vGravity.v, BuoyancyPlaneCallback,
										pRigidBody);
		}

		// Add forces from calls to Addforce(..), etc
		NewtonBodyAddForce(apBody, pRigidBody->mvTotalForce.v);
		NewtonBodyAddTorque(apBody, pRigidBody->mvTotalTorque.v);

		// Check so that all speeds are within thresholds
		// Linear
		if (pRigidBody->mfMaxLinearSpeed > 0)
		{
			cVector3f vVel = pRigidBody->GetLinearVelocity();
			float fSpeed = vVel.Length();
			if (fSpeed > pRigidBody->mfMaxLinearSpeed)
			{
				vVel = cMath::Vector3Normalize(vVel) * pRigidBody->mfMaxLinearSpeed;
				pRigidBody->SetLinearVelocity(vVel);
			}
		}
		// Angular
		if (pRigidBody->mfMaxAngularSpeed > 0)
		{
			cVector3f vVel = pRigidBody->GetAngularVelocity();
			float fSpeed = vVel.Length();
			if (fSpeed > pRigidBody->mfMaxAngularSpeed)
			{
				vVel = cMath::Vector3Normalize(vVel) * pRigidBody->mfMaxAngularSpeed;
				pRigidBody->SetAngularVelocity(vVel);
			}
		}

	}
}
