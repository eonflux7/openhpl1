// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_WORLD_H
#define HPL_PHYSICS_WORLD_H

#include <map>
#include "system/SystemTypes.h"
#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"

#include "physics/CollideData.h"

#include "game/SaveGame.h"

namespace hpl {

	class iCollideShape;
	class iVertexBuffer;
	class iPhysicsBody;
	class iLowLevelGraphics;
	class iPhysicsMaterial;
	class iCharacterBody;
	class iPhysicsJoint;
	class iPhysicsJointBall;
	class iPhysicsJointHinge;
	class iPhysicsJointScrew;
	class iPhysicsJointSlider;
	class iPhysicsController;
	class cWorld3D;
	class cBoundingVolume;

	typedef std::list<iCollideShape*> tCollideShapeList;

	typedef std::vector<iCollideShape*> tCollideShapeVec;
	typedef tCollideShapeVec::iterator tCollideShapeVecIt;

	typedef std::list<iPhysicsBody*> tPhysicsBodyList;
	typedef tPhysicsBodyList::iterator tPhysicsBodyListIt;

	typedef std::list<iPhysicsJoint*> tPhysicsJointList;
	typedef tPhysicsJointList::iterator tPhysicsJointListIt;

	typedef std::list<iPhysicsController*> tPhysicsControllerList;
	typedef tPhysicsControllerList::iterator tPhysicsControllerListIt;

	typedef std::list<iCharacterBody*> tCharacterBodyList;
	typedef tCharacterBodyList::iterator tCharacterBodyListIt;


	typedef std::map<tString, iPhysicsMaterial*> tPhysicsMaterialMap;
	typedef tPhysicsMaterialMap::iterator tPhysicsMaterialMapIt;

	typedef cSTLMapIterator<iPhysicsMaterial*, tPhysicsMaterialMap, tPhysicsMaterialMapIt> cPhysicsMaterialIterator;

	typedef cSTLIterator<iPhysicsBody*, tPhysicsBodyList, tPhysicsBodyListIt> cPhysicsBodyIterator;
	typedef cSTLIterator<iPhysicsJoint*, tPhysicsJointList, tPhysicsJointListIt> cPhysicsJointIterator;

	enum ePhysicsAccuracy
	{
		ePhysicsAccuracy_Low,
		ePhysicsAccuracy_Medium,
		ePhysicsAccuracy_High,
		ePhysicsAccuracy_LastEnum
	};

	class cPhysicsRayParams
	{
	public:
		float mfT;
		float mfDist;
		cVector3f mvNormal;
		cVector3f mvPoint;
	};

	class iPhysicsRayCallback
	{
	public:
		virtual ~iPhysicsRayCallback(){}
		virtual bool BeforeIntersect(iPhysicsBody *pBody){return true;}
		virtual bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams)=0;
	};

	class iPhysicsWorldCollisionCallback
	{
	public:
		virtual ~iPhysicsWorldCollisionCallback(){}
		virtual void OnCollision(iPhysicsBody *apBody, cCollideData *apCollideData)=0;
	};

	class iPhysicsWorld
	{
	public:
		iPhysicsWorld();
		virtual ~iPhysicsWorld();

		//! \name General
		//! @{

		void Update(float afTimeStep);
		virtual void Simulate(float afTimeStep)=0;

		virtual void  SetMaxTimeStep(float afTimeStep)=0;
		virtual float GetMaxTimeStep()=0;

		virtual void SetWorldSize(const cVector3f &avMin,const cVector3f &avMax)=0;
		virtual cVector3f GetWorldSizeMin()=0;
		virtual cVector3f GetWorldSizeMax()=0;

		virtual void SetGravity(const cVector3f& avGravity)=0;
		virtual cVector3f GetGravity()=0;

		virtual void SetAccuracyLevel(ePhysicsAccuracy aAccuracy)=0;
		virtual ePhysicsAccuracy GetAccuracyLevel()=0;
		//! @}

		//! \name Shapes
		//! @{

		virtual iCollideShape* CreateNullShape()=0;
		virtual iCollideShape* CreateBoxShape(const cVector3f &avSize, cMatrixf* apOffsetMtx)=0;
		virtual iCollideShape* CreateSphereShape(const cVector3f &avRadii, cMatrixf* apOffsetMtx)=0;
		virtual iCollideShape* CreateCylinderShape(float afRadius, float afHeight, cMatrixf* apOffsetMtx)=0;
		virtual iCollideShape* CreateCapsuleShape(float afRadius, float afHeight, cMatrixf* apOffsetMtx)=0;
		virtual iCollideShape* CreateMeshShape(iVertexBuffer *apVtxBuffer)=0;
		virtual iCollideShape* CreateCompoundShape(tCollideShapeVec &avShapes)=0;
		void DestroyShape(iCollideShape *apShape);

		//! @}

		//! \name Joints
		//! @{

		virtual iPhysicsJointBall* CreateJointBall(const tString &asName,
											const cVector3f& avPivotPoint,
											iPhysicsBody* apParentBody, iPhysicsBody *apChildBody)=0;
		virtual iPhysicsJointHinge* CreateJointHinge(const tString &asName,
											const cVector3f& avPivotPoint,const cVector3f& avPinDir,
											iPhysicsBody* apParentBody, iPhysicsBody *apChildBody)=0;
		virtual iPhysicsJointSlider* CreateJointSlider(const tString &asName,
											const cVector3f& avPivotPoint,const cVector3f& avPinDir,
											iPhysicsBody* apParentBody, iPhysicsBody *apChildBody)=0;
		virtual iPhysicsJointScrew* CreateJointScrew(const tString &asName,
											const cVector3f& avPivotPoint,const cVector3f& avPinDir,
											iPhysicsBody* apParentBody, iPhysicsBody *apChildBody)=0;
		void DestroyJoint(iPhysicsJoint* apJoint);
		iPhysicsJoint *GetJoint(const tString &asName);
		cPhysicsJointIterator GetJointIterator();

		//! @}

		//! \name Materials
		//! @{

		virtual iPhysicsMaterial* CreateMaterial(const tString &asName)=0;
		iPhysicsMaterial* GetMaterialFromName(const tString &asName);
		cPhysicsMaterialIterator GetMaterialIterator();

		//! @}

		//! \name Bodies
		//! @{

		virtual iPhysicsBody* CreateBody(const tString &asName,iCollideShape *apShape)=0;
		void DestroyBody(iPhysicsBody* apBody);
		iPhysicsBody *GetBody(const tString &asName);
		cPhysicsBodyIterator GetBodyIterator();

		/**
		 * Render interpolation of everything that bodies and character
		 * bodies move, see cGame::Run().
		 */
		void StoreInterpolation();
		void ApplyInterpolation(float afT);
		void RestoreInterpolation();

		virtual iCharacterBody *CreateCharacterBody(const tString &asName, const cVector3f &avSize)=0;
		void DestroyCharacterBody(iCharacterBody* apBody);
		iPhysicsBody *GetCharacterBody(const tString &asName);

		void GetBodiesInBV(cBoundingVolume *apBV, tPhysicsBodyList *apBodyList);
		void EnableBodiesInBV(cBoundingVolume *apBV, bool abEnabled);


		//! @}


		//! \name Tools
		//! @{

		void SetLogDebug(bool abX){ mbLogDebug = abX;}
		bool GetLogDebug(){ return mbLogDebug;}

		void AddSaveData(cSaveDataHandler* apHandler);

		virtual iPhysicsController *CreateController(const tString &asName)=0;
		void DestroyController(iPhysicsController *apController);

		tCollidePointVec* GetContactPoints(){ return &mvContactPoints;}
		void SetSaveContactPoints(bool abX){ mbSaveContactPoints = abX;}
		bool GetSaveContactPoints(){ return mbSaveContactPoints;}
		void RenderContactPoints(iLowLevelGraphics *apLowLevel, const cColor& aPointColor,
								const cColor& aLineColor);

		virtual void CastRay(iPhysicsRayCallback *apCallback,
							const cVector3f &avOrigin, const cVector3f& avEnd,
							bool abCalcDist, bool abCalcNormal, bool abCalcPoint,
							bool abUsePrefilter=false)=0;

		virtual void RenderDebugGeometry(iLowLevelGraphics *apLowLevel, const cColor& aColor)=0;

		virtual bool CheckShapeCollision(	iCollideShape* apShapeA, const cMatrixf& a_mtxA,
										iCollideShape* apShapeB, const cMatrixf& a_mtxB,
										cCollideData & aCollideData, int alMaxPoints=4)=0;

		// How far two bodies overlap, or -1 if they do not.
		float GetBodyOverlap(iPhysicsBody *apBodyA, iPhysicsBody *apBodyB);

		/**
		 * For a static body just made dynamic: it passes through the static
		 * bodies it overlaps by afMinDepth or more until it no longer overlaps
		 * them. Maps place such bodies a little inside the level, and Newton 2
		 * pushes them out with forces that jam them where they are.
		 * \return how many bodies it passes through
		 */
		int LetOutOfStaticBodies(iPhysicsBody *apBody, float afMinDepth);

		/**
		 * The overlap from which a script's SetBodyProperty "Mass" lets a static
		 * body it makes dynamic out of the static bodies it is inside. 0, the
		 * default, is off.
		 */
		static void SetScriptReleaseDepth(float afX){ mfScriptReleaseDepth = afX;}
		static float GetScriptReleaseDepth(){ return mfScriptReleaseDepth;}

		bool CheckShapeWorldCollision(cVector3f *apNewPos,
										iCollideShape* apShape, const cMatrixf& a_mtxTransform,
										iPhysicsBody *apSkipBody=nullptr, bool abSkipStatic=false,
										bool abIsCharacter=false,
										iPhysicsWorldCollisionCallback *apCallback=nullptr,
										bool abCollideCharacter=true,
										bool abDebug=false);

		void DestroyAll();

		cWorld3D* GetWorld3D(){ return mpWorld3D;}
		void SetWorld3D(cWorld3D *apWorld3D){ mpWorld3D = apWorld3D;}
		//! @}

	private:
		static float mfScriptReleaseDepth;

	protected:
		tCollideShapeList mlstShapes;
		tPhysicsBodyList mlstBodies;
		tCharacterBodyList mlstCharBodies;
		tPhysicsMaterialMap m_mapMaterials;
		tPhysicsJointList mlstJoints;
		tPhysicsControllerList mlstControllers;
		cWorld3D *mpWorld3D;

		bool mbLogDebug;

		tCollidePointVec mvContactPoints;
		bool mbSaveContactPoints;
	};
};
#endif // HPL_PHYSICS_WORLD_H
