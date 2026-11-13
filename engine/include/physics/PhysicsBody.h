// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_BODY_H
#define HPL_PHYSICS_BODY_H

#include "scene/Entity3D.h"
#include "graphics/GraphicsTypes.h"

#include <vector>

namespace hpl {

	class iPhysicsWorld;
	class iCollideShape;
	class iPhysicsMaterial;
	class iLowLevelGraphics;
	class cNode3D;
	class cSoundEntity;
	class iPhysicsJoint;
	class cPhysicsContactData;
	class iCharacterBody;

	class iPhysicsBody;
	class iPhysicsBodyCallback
	{
	public:
		virtual ~iPhysicsBodyCallback(){}
		virtual bool OnBeginCollision(iPhysicsBody *apBody, iPhysicsBody *apCollideBody)=0;
		virtual void OnCollide(iPhysicsBody *apBody, iPhysicsBody *apCollideBody,
								cPhysicsContactData* apContactData)=0;
	};

	typedef std::list<iPhysicsBodyCallback*> tPhysicsBodyCallbackList;
	typedef tPhysicsBodyCallbackList::iterator tPhysicsBodyCallbackListIt;

	class cSaveData_iCollideShape : public iSerializable
	{
		kSerializableClassInit(cSaveData_iCollideShape)
	public:
		int mType;
		cMatrixf m_mtxOffset;
		cVector3f mvSize;
	};

	kSaveData_ChildClass(iEntity3D,iPhysicsBody)
	{
		kSaveData_ClassInit(iPhysicsBody)
	public:
		cContainerList<cSaveData_iCollideShape> mlstShapes;

		tString msMaterial;

		bool mbBlocksSound;
		bool mbIsCharacter;
		bool mbCollideCharacter;

		cVector3f mvLinearVelocity;
		cVector3f mvAngularVelocity;
		float mfLinearDamping;
		float mfAngularDamping;
		float mfMaxLinearSpeed;
		float mfMaxAngularSpeed;

		float mfMass;

		bool mbEnabled;
		bool mbAutoDisable;
		bool mbContinuousCollision;

		bool mbGravity;

		bool mbCollide;


		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	struct cPhysicsBody_Buoyancy
	{
		cPhysicsBody_Buoyancy() : mbActive(false), mfDensity(1),
								mfLinearViscosity(1), mfAngularViscosity(1){}

		bool mbActive;

		float mfDensity;
		float mfLinearViscosity;
		float mfAngularViscosity;

		cPlanef mSurface;
	};

	class iPhysicsBody : public iEntity3D
	{
	#ifdef __GNUC__
		typedef iEntity3D __super;
	#endif
	public:
		iPhysicsBody(const tString &asName, iPhysicsWorld *apWorld,iCollideShape *apShape);
		~iPhysicsBody() override;

		void Destroy();

		virtual void SetMaterial(iPhysicsMaterial* apMaterial)=0;
		iPhysicsMaterial* GetMaterial();

		cNode3D* GetNode();
		cNode3D* CreateNode();

		iCollideShape* GetShape();

		void AddJoint(iPhysicsJoint *apJoint);
		iPhysicsJoint* GetJoint(int alIndex);
		int GetJointNum();
		void RemoveJoint(iPhysicsJoint *apJoint);

		virtual void SetLinearVelocity(const cVector3f &avVel)=0;
		virtual cVector3f GetLinearVelocity() const=0;
		virtual void SetAngularVelocity(const cVector3f &avVel)=0;
		virtual cVector3f GetAngularVelocity() const=0;
		virtual void SetLinearDamping(float afDamping)=0;
		virtual float GetLinearDamping() const=0;
		virtual void SetAngularDamping(float afDamping)=0;
		virtual float GetAngularDamping() const=0;
		virtual void SetMaxLinearSpeed(float afSpeed)=0;
		virtual float GetMaxLinearSpeed() const=0;
		virtual void SetMaxAngularSpeed(float afDamping)=0;
		virtual float GetMaxAngularSpeed() const=0;
		virtual cMatrixf GetInertiaMatrix() =0;

		cVector3f GetVelocityAtPosition(cVector3f avPos);

		virtual void  SetMass(float afMass)=0;
		virtual float GetMass() const=0;
		virtual void  SetMassCentre(const cVector3f& avCentre)=0;
		virtual cVector3f GetMassCentre() const=0;

		virtual void AddForce(const cVector3f &avForce)=0;
		virtual void AddForceAtPosition(const cVector3f &avForce, const cVector3f &avPos)=0;
		virtual void AddTorque(const cVector3f &avTorque)=0;
		virtual void AddImpulse(const cVector3f &avImpulse)=0;
		virtual void AddImpulseAtPosition(const cVector3f &avImpulse, const cVector3f &avPos)=0;

		virtual void SetEnabled(bool abEnabled)=0;
		virtual bool GetEnabled() const=0;
		// The physics engine's own rest states, for debugging. Newton 2 keeps
		// sleeping (wakes on contact) and frozen (wakes only when forced) apart.
		virtual bool IsSleeping() const=0;
		virtual bool IsFrozen() const=0;
		virtual void SetAutoDisable(bool abEnabled)=0;
		virtual bool GetAutoDisable() const=0;
		virtual void SetAutoDisableLinearThreshold(float afThresold)=0;
		virtual float GetAutoDisableLinearThreshold() const=0;
		virtual void SetAutoDisableAngularThreshold(float afThresold)=0;
		virtual float GetAutoDisableAngularThreshold() const=0;
		virtual void SetAutoDisableNumSteps(int alNum)=0;
		virtual int GetAutoDisableNumSteps() const=0;
		virtual void SetContinuousCollision(bool abOn)=0;
		virtual bool GetContinuousCollision()=0;

		virtual void SetGravity(bool abEnabled)=0;
		virtual bool GetGravity() const=0;

		virtual void RenderDebugGeometry(iLowLevelGraphics *apLowLevel,const cColor &aColor)=0;

		void UpdateBeforeSimulate(float afTimeStep);
		void UpdateAfterSimulate(float afTimeStep);

		cBoundingVolume* GetBV(){ return &mBoundingVolume;}

		void SetBlocksSound(bool abX){ mbBlocksSound = abX;}
		bool GetBlocksSound(){ return mbBlocksSound;}

		void SetBlocksLight(bool abX){ mbBlocksLight = abX;}
		bool GetBlocksLight(){ return mbBlocksLight;}

		void SetScrapeSoundEntity(cSoundEntity *apEntity){ mpScrapeSoundEntity = apEntity;}
		cSoundEntity* GetScrapeSoundEntity(){ return mpScrapeSoundEntity;}
		void SetScrapeBody(iPhysicsBody *apBody){ mpScrapeBody =apBody;}
		iPhysicsBody * GetScrapeBody(){ return mpScrapeBody;}
		const cMatrixf& GetPreviousScrapeMatrix(){ return m_mtxPrevScrapeMatrix;}
		void SetPreviousScrapeMatrix(const cMatrixf& a_mtxMtx){ m_mtxPrevScrapeMatrix = a_mtxMtx;}

		void SetRollSoundEntity(cSoundEntity *apEntity){ mpRollSoundEntity = apEntity;}
		cSoundEntity* GetRollSoundEntity(){ return mpRollSoundEntity;}

		void SetHasImpact(bool abX){ mbHasImpact = abX;}
		bool HasImpact(){ return mbHasImpact;}
		void SetHasSlide(bool abX){ mbHasSlide = abX;}
		bool HasSlide(){ return mbHasSlide;}

		bool HasCollision(){ return mbHasCollision;}

		void SetUserData(void* apUserData){ mpUserData = apUserData;}
		void* GetUserData(){ return mpUserData;}

		void AddBodyCallback(iPhysicsBodyCallback *apCallback);
		void RemoveBodyCallback(iPhysicsBodyCallback *apCallback);

		bool OnBeginCollision(iPhysicsBody *apBody);
		void OnCollide(iPhysicsBody *apBody,cPhysicsContactData* apContactData);

		void SetCollide(bool abX){mbCollide = abX;}
		bool GetCollide(){ return mbCollide;}
		// Off: no friction against static bodies. Not saved; for what holds a
		// body for a moment, and is set back when it lets go.
		void SetStaticFriction(bool abX){mbStaticFriction = abX;}
		bool GetStaticFriction() const { return mbStaticFriction;}
		// One body this one passes through. Not saved, as above.
		void SetPassThroughBody(iPhysicsBody *apBody){mpPassThroughBody = apBody;}
		iPhysicsBody* GetPassThroughBody() const { return mpPassThroughBody;}

		void SetPassThroughStatic(bool abX){mbPassThroughStatic = abX;}
		bool GetPassThroughStatic() const { return mbPassThroughStatic;}
		// A body being pushed slides over the edges of static geometry at its
		// underside: contacts within 1 cm of its lowest point that do not hold
		// it up are dropped (see PhysicsMaterialNewton). Resting bodies sink a
		// few mm into the floor and catch on the lip of a hole beside them.
		void SetSlideOverLips(bool abX){mbSlideOverLips = abX;}
		bool GetSlideOverLips() const { return mbSlideOverLips;}
		// Static bodies this one passes through until it no longer overlaps
		// them: those it was inside when a script made it dynamic (see
		// iPhysicsWorld::LetOutOfStaticBodies). Not saved.
		void AddEmbeddingBody(iPhysicsBody *apBody){ mvEmbeddingBodies.push_back(apBody);}
		bool IsEmbeddingBody(iPhysicsBody *apBody) const
		{
			for(iPhysicsBody *pBody : mvEmbeddingBodies) if(pBody == apBody) return true;
			return false;
		}
		std::vector<iPhysicsBody*>& GetEmbeddingBodies(){ return mvEmbeddingBodies;}

		void SetIsCharacter(bool abX){ mbIsCharacter = abX;}
		bool IsCharacter(){ return mbIsCharacter;}

		void SetCollideCharacter(bool abX){ mbCollideCharacter = abX;}
		bool GetCollideCharacter(){ return mbCollideCharacter;}

		void SetCharacterBody(iCharacterBody *apCharBody){ mpCharacterBody = apCharBody;}
		iCharacterBody* GetCharacterBody(){ return mpCharacterBody;}

		void SetIsRagDoll(bool abX){ mbIsRagDoll = abX;}
		bool IsRagDoll(){ return mbIsRagDoll;}

		void SetCollideRagDoll(bool abX){ mbCollideRagDoll = abX;}
		bool GetCollideRagDoll(){ return mbCollideRagDoll;}

		void SetVolatile(bool abX){ mbVolatile = abX;}
		bool IsVolatile(){ return mbVolatile;}

		void SetPushedByCharacterGravity(bool abX){ mbPushedByCharacterGravity = abX;}
		bool GetPushedByCharacterGravity(){ return mbPushedByCharacterGravity;}

		void SetBuoyancyId(int alX){ mlBuoyancyId = alX;}
		void SetBuoyancyActive(bool abX){ mBuoyancy.mbActive = abX;}
		void SetBuoyancyDensity(float afX){ mBuoyancy.mfDensity = afX;}
		void SetBuoyancyLinearViscosity(float afX){ mBuoyancy.mfLinearViscosity = afX;}
		void SetBuoyancyAngularViscosity(float afX){ mBuoyancy.mfAngularViscosity = afX;}
		void SetBuoyancySurface(const cPlanef &aP){ mBuoyancy.mSurface = aP;}

		int GetBuoyancyId(){ return mlBuoyancyId;}
		bool GetBuoyancyActive(){ return mBuoyancy.mbActive;}
		float GetBuoyancyDensity(){ return mBuoyancy.mfDensity;}
		float GetBuoyancyLinearViscosity(){ return mBuoyancy.mfLinearViscosity;}
		float GetBuoyancyAngularViscosity(){ return mBuoyancy.mfAngularViscosity;}
		cPlanef SetBuoyancySurface(){ return mBuoyancy.mSurface;}

		void SetCanAttachCharacter(bool abX){ mbCanAttachCharacter = abX;}
		bool GetCanAttachCharacter(){ return mbCanAttachCharacter;}
		void AddAttachedCharacter(iCharacterBody *apChar);
		void RemoveAttachedCharacter(iCharacterBody *apChar);

		iPhysicsWorld *GetWorld(){ return mpWorld;}

		void DisableAfterSimulation(){ mbDisableAfterSimulation = true;}

		//Entity implementation
		tString GetEntityType() override{ return "Body";}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

		virtual void DeleteLowLevel()=0;

		/**
		 * Render interpolation, see cGame::Run(). Only what is attached to
		 * the body's node is moved, the body itself stays where it is.
		 */
		void StoreInterpolation();
		void ApplyInterpolation(float afT);
		void RestoreInterpolation();
	protected:
		void CreateSaveCollideShapes(cContainerList<cSaveData_iCollideShape> *apShapeList);

		iPhysicsWorld *mpWorld;
		iCollideShape *mpShape;
		iPhysicsMaterial *mpMaterial;
		cNode3D *mpNode;

		cMatrixf m_mtxInterpolatePrev;
		bool mbInterpolatePrevValid;
		bool mbInterpolated;

		iCharacterBody *mpCharacterBody;

		std::vector<iPhysicsJoint*> mvJoints;

		std::list<iCharacterBody*> mlstAttachedCharacters;

		iPhysicsBody *mpScrapeBody;
		cSoundEntity *mpScrapeSoundEntity;
		cSoundEntity *mpRollSoundEntity;
		cMatrixf m_mtxPrevScrapeMatrix;
		bool mbHasImpact;
		bool mbHasSlide;
		int mlSlideCount;
		int mlImpactCount;

		bool mbPushedByCharacterGravity;

		bool mbBlocksSound;
		bool mbBlocksLight;
		bool mbIsCharacter;
		bool mbCollideCharacter;
		bool mbIsRagDoll;
		bool mbCollideRagDoll;
		bool mbVolatile;

		bool mbCanAttachCharacter;

		cPhysicsBody_Buoyancy mBuoyancy;
		int mlBuoyancyId;

		bool mbDisableAfterSimulation;

		bool mbHasCollision;

		tPhysicsBodyCallbackList mlstBodyCallbacks;

		void *mpUserData;

		bool mbCollide;
		bool mbStaticFriction;
		iPhysicsBody *mpPassThroughBody;
		bool mbPassThroughStatic;
		bool mbSlideOverLips;
		std::vector<iPhysicsBody*> mvEmbeddingBodies;
	};
};
#endif // HPL_PHYSICS_BODY_H
