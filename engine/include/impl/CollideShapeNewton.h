// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_COLLIDE_SHAPE_NEWTON_H
#define HPL_COLLIDE_SHAPE_NEWTON_H

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif
#include <Newton.h>
#include "physics/CollideShape.h"

#include "math/MathTypes.h"

namespace hpl {

	class iVertexBuffer;
	class iCollideShape;

	typedef std::vector<iCollideShape*> tCollideShapeVec;
	typedef tCollideShapeVec::iterator tCollideShapeVecIt;

	class cCollideShapeNewton : public iCollideShape
	{
	public:
		cCollideShapeNewton(eCollideShapeType aType, const cVector3f &avSize,
							cMatrixf* apOffsetMtx,NewtonWorld* apNewtonWorld,
							iPhysicsWorld *apWorld);
		~cCollideShapeNewton() override;

		iCollideShape* GetSubShape(int alIdx) override;
		int GetSubShapeNum() override;

		cVector3f GetInertia(float afMass);

		void CreateFromShapeVec(tCollideShapeVec &avShapes);
		void CreateFromVertices(const unsigned int* apIndexArray, int alIndexNum,
								const float *apVertexArray, int alVtxStride, int alVtxNum);

		NewtonCollision* GetNewtonCollision(){ return mpNewtonCollision;}

	private:
		NewtonCollision* mpNewtonCollision;
		NewtonWorld *mpNewtonWorld;

		tCollideShapeVec mvSubShapes;
	};
};
#endif // HPL_COLLIDE_SHAPE_NEWTON_H
