// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_WORLD_NEWTON_H
#define HPL_PHYSICS_WORLD_NEWTON_H

#include "physics/PhysicsWorld.h"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif
#include <Newton.h>

namespace hpl {
	class cPhysicsWorldNewton : public iPhysicsWorld
	{
	public:
		cPhysicsWorldNewton();
		~cPhysicsWorldNewton() override;

		void Simulate(float afTimeStep) override;

		void  SetMaxTimeStep(float afTimeStep) override;
		float GetMaxTimeStep() override;

		void SetWorldSize(const cVector3f &avMin,const cVector3f &avMax) override;
		cVector3f GetWorldSizeMin() override;
		cVector3f GetWorldSizeMax() override;

		void SetGravity(const cVector3f& avGravity) override;
		cVector3f GetGravity() override;

		void SetAccuracyLevel(ePhysicsAccuracy aAccuracy) override;
		ePhysicsAccuracy GetAccuracyLevel() override;

		iCollideShape* CreateNullShape() override;
		iCollideShape* CreateBoxShape(const cVector3f &avSize, cMatrixf* apOffsetMtx) override;
		iCollideShape* CreateSphereShape(const cVector3f &avRadii, cMatrixf* apOffsetMtx) override;
		iCollideShape* CreateCylinderShape(float afRadius, float afHeight, cMatrixf* apOffsetMtx) override;
		iCollideShape* CreateCapsuleShape(float afRadius, float afHeight, cMatrixf* apOffsetMtx) override;
		iCollideShape* CreateMeshShape(iVertexBuffer *apVtxBuffer) override;
		iCollideShape* CreateCompoundShape(tCollideShapeVec &avShapes) override;

		iPhysicsJointBall* CreateJointBall(const tString &asName,const cVector3f& avPivotPoint,
												iPhysicsBody* apParentBody, iPhysicsBody *apChildBody) override;
		iPhysicsJointHinge* CreateJointHinge(const tString &asName,const cVector3f& avPivotPoint,
												const cVector3f& avPinDir,
												iPhysicsBody* apParentBody, iPhysicsBody *apChildBody) override;
		iPhysicsJointSlider* CreateJointSlider(const tString &asName,const cVector3f& avPivotPoint,
												const cVector3f& avPinDir,
												iPhysicsBody* apParentBody, iPhysicsBody *apChildBody) override;
		iPhysicsJointScrew* CreateJointScrew(const tString &asName,const cVector3f& avPivotPoint,
												const cVector3f& avPinDir,
												iPhysicsBody* apParentBody, iPhysicsBody *apChildBody) override;

		iPhysicsBody* CreateBody(const tString &asName,iCollideShape *apShape) override;

		iCharacterBody *CreateCharacterBody(const tString &asName, const cVector3f &avSize) override;

		iPhysicsMaterial* CreateMaterial(const tString &asName) override;

		iPhysicsController *CreateController(const tString &asName) override;

		void CastRay(iPhysicsRayCallback *apCallback,
							const cVector3f &avOrigin, const cVector3f& avEnd,
							bool abCalcDist, bool abCalcNormal, bool abCalcPoint,
							bool abUsePrefilter = false) override;

		bool CheckShapeCollision(	iCollideShape* apShapeA, const cMatrixf& a_mtxA,
						iCollideShape* apShapeB, const cMatrixf& a_mtxB,
						cCollideData & aCollideData, int alMaxPoints=4) override;

		void RenderDebugGeometry(iLowLevelGraphics *apLowLevel, const cColor& aColor) override;

		NewtonWorld* GetNewtonWorld(){ return mpNewtonWorld;}
	private:
		NewtonWorld *mpNewtonWorld;

		float* mpTempPoints;
		float* mpTempNormals;
		float* mpTempDepths;

		cVector3f mvWorldSizeMin;
		cVector3f mvWorldSizeMax;
		cVector3f mvGravity;
		float mfMaxTimeStep;

		ePhysicsAccuracy mAccuracy;
	};
};
#endif // HPL_PHYSICS_WORLD_NEWTON_H
