// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_FRUSTUM_H
#define HPL_FRUSTUM_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"
#include "math/BoundingVolume.h"

namespace hpl {

	class iLowLevelGraphics;

	enum eFrustumPlane
	{
		eFrustumPlane_Left=		0,
		eFrustumPlane_Right=	1,
		eFrustumPlane_Bottom=	2,
		eFrustumPlane_Top=		3,
		eFrustumPlane_Near=		4,
		eFrustumPlane_Far=		5,
		eFrustumPlane_LastEnum=	7,
	};

	enum eFrustumCollision
	{
		eFrustumCollision_Inside,
		eFrustumCollision_Outside,
		eFrustumCollision_Intersect,
		eFrustumCollision_LastEnum
	};


	class cFrustum
	{
	public:
		cFrustum();

		void SetViewProjMatrix(const cMatrixf& a_mtxProj, const cMatrixf& a_mtxView,
								float afFarPlane,float afNearPlane,float afFOV, float afAspect,
								const cVector3f &avOrigin, bool abInfFarPlane=false);

		cPlanef GetPlane(eFrustumPlane aType);

		eFrustumCollision CollideBoundingVolume(cBoundingVolume* aBV);

		eFrustumCollision CollideFrustumSphere(cBoundingVolume* aBV);

		eFrustumCollision CollideBVSphere(cBoundingVolume* aBV);
		eFrustumCollision CollideBVAABB(cBoundingVolume* aBV);

		/**
		* Checks intersection with a line
		*/
		bool CheckLineIntersection(const cVector3f& avPoint1,const cVector3f& avPoint2);

		/**
		 * Checks intersection with a quad mesh.
		 * \param apPoints the points of the quad mesh, every 4 points is a face.
		 */
		bool CheckQuadMeshIntersection(tVector3fVec *apPoints);


		bool CheckVolumeIntersection(cShadowVolumeBV *apVolume);

		cSpheref GetBoundingSphere(){ return mBoundingSphere;}
		const cBoundingVolume& GetBoundingVolume(){ return mBoundingVolume;}

		const cVector3f& GetOrigin();
		cBoundingVolume* GetOriginBV();

		cVector3f GetForward();

		void Draw(iLowLevelGraphics *apLowLevelGraphics);

	private:

		void UpdatePlanes();
		void UpdateSphere();
		void UpdateEndPoints();
		void UpdateBV();

		float mfFarPlane;
		float mfNearPlane;
		float mfAspect;
		float mfFOV;

		bool mbInfFarPlane;

		cVector3f mvOrigin;
		cBoundingVolume mOriginBV;

		cMatrixf m_mtxViewProj;
		cMatrixf m_mtxModelView;
		cPlanef mPlane[6];
		cSpheref mBoundingSphere;
		cBoundingVolume mBoundingVolume;

		cVector3f mvEndPoints[4];
		tVector3fVec mvPoints;
	};
};
#endif // HPL_FRUSTUM_H
