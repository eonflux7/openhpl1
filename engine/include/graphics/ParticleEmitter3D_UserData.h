// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PARTICLE_EMITTER_3D_USER_DATA_H
#define HPL_PARTICLE_EMITTER_3D_USER_DATA_H

#include "graphics/Renderable.h"
#include "scene/Entity3D.h"
#include "graphics/ParticleEmitter3D.h"
#include "physics/PhysicsWorld.h"

class TiXmlElement;

namespace hpl {



	enum ePEStartPosType
	{
		ePEStartPosType_Box,
		ePEStartPosType_Sphere,
		ePEStartPosType_LastEnum,
	};

	enum ePEDeathType
	{
		ePEDeathType_Age,
		ePEDeathType_Color,
		ePEDeathType_LastEnum,
	};

	enum ePEGravityType
	{
		ePEGravityType_None,		//No gravity added
		ePEGravityType_Vector,		//Vector is used.
		ePEGravityType_Center,		//Y axis used. heading equals center.
		ePEGravityType_LastEnum,
	};

	enum ePESubDivType
	{
		ePESubDivType_Random,
		ePESubDivType_Animation,
		ePESubDivType_LastEnum,
	};

	enum ePEPartSpinType
	{
		ePEPartSpinType_Constant,
		ePEPartSpinType_Movement,
		ePEPartSpinType_LastEnum,
	};

	class cParticleEmitterData3D_UserData : public iParticleEmitterData, public iPhysicsRayCallback
	{
	friend class cParticleEmitter3D_UserData;
	public:
		cParticleEmitterData3D_UserData(const tString &asName,cResources* apResources,cGraphics *apGraphics);

		iParticleEmitter* Create(tString asName, cVector3f avSize) override;

		void LoadFromElement(TiXmlElement *apElement);

		bool CheckCollision(const cVector3f& avStart, const cVector3f &avEnd,
									iPhysicsWorld *apPhysicsWorld,
									cVector3f *apNormalVec,
									cVector3f *apPosVec);

	private:
		bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;
		float mfShortestDist;
		bool mbIntersected;
		cVector3f *mpIntersectNormal;
		cVector3f *mpIntersectPos;

		// GENERAL

		// NEW
		ePEType	mPEType;
		// ---

		int mlMaxParticleNum;

		bool mbRespawn;

		float mfParticlesPerSecond;
		float mfStartTimeOffset;

		float mfMinPauseLength;
		float mfMaxPauseLength;

		float mfMinPauseInterval;
		float mfMaxPauseInterval;

		cVector3f mvPosOffset;
		cVector3f mvAngleOffset;

		float mfFrameStep;
		float mfMaxFrameTime;

		// MATERIAL

		cVector2l mvSubDiv;
		ePESubDivType mSubDivType;

		// START POS
		ePEStartPosType mStartPosType;

		cVector3f mvMinStartPos;
		cVector3f mvMaxStartPos;

		cVector2f mvMinStartAngles;
		cVector2f mvMaxStartAngles;

		float mfMinStartRadius;
		float mfMaxStartRadius;

		// MOVEMENT
		ePEStartPosType mStartVelType;

		cVector3f mvMinStartVel;
		cVector3f mvMaxStartVel;

		cVector2f mvMinStartVelAngles;
		cVector2f mvMaxStartVelAngles;

		float mfMinStartVelSpeed;
		float mfMaxStartVelSpeed;

		float mfMinSpeedMultiply;
		float mfMaxSpeedMultiply;

		cVector3f mvMinStartAcc;
		cVector3f mvMaxStartAcc;

		float mfMinVelMaximum;
		float mfMaxVelMaximum;

		bool mbUsesDirection;
		ePEGravityType mGravityType;

		cVector3f mvGravityAcc;

		eParticleEmitter3DCoordSystem mCoordSystem;

		// NEW

		bool	mbUsePartSpin;
		ePEPartSpinType mPartSpinType;
		float	mfMinSpinRange;
		float	mfMaxSpinRange;

		bool	mbUseRevolution;
		cVector3f	mvMinRevVel;
		cVector3f	mvMaxRevVel;

		// ---

		// LIFESPAN
		float mfMinLifeSpan;
		float mfMaxLifeSpan;

		ePEDeathType mDeathType;

		tString msDeathPS;

		// RENDERING
		eParticleEmitter3DType mDrawType;

		cVector2f mvMinStartSize;
		cVector2f mvMaxStartSize;

		float mfStartRelSize;
		float mfMiddleRelSize;
		float mfMiddleRelSizeTime;
		float mfMiddleRelSizeLength;
		float mfEndRelSize;

		bool mbMultiplyRGBWithAlpha;

		// COLOR

		cColor mMinStartColor;
		cColor mMaxStartColor;

		cColor mStartRelColor;
		cColor mMiddleRelColor;
		float mfMiddleRelColorTime;
		float mfMiddleRelColorLength;
		cColor mEndRelColor;

		// COLLISION
		bool mbCollides;

		float mfMinBounceAmount;
		float mfMaxBounceAmount;

		int  mlMinCollisionMax;
		int  mlMaxCollisionMax;

		int  mlCollisionUpdateRate;

		// NEW
		// BEAM SPECIFIC

		bool mbUseBeamNoise;

		int mlLowFreqPoints;
		std::vector<unsigned int> mvLFIndices;
		cVector3f mvMinLowFreqNoise;
		cVector3f mvMaxLowFreqNoise;

		int mlHighFreqPoints;
		cVector3f mvMinHighFreqNoise;
		cVector3f mvMaxHighFreqNoise;

		std::vector<tBeamNoisePoint> mvBeamNoisePoints;


		// ---




};

	class cParticleEmitter3D_UserData : public iParticleEmitter3D
	{
	public:
		cParticleEmitter3D_UserData(	tString asName,tMaterialVec* avMaterials,
										cVector3f avSize, cGraphics* apGraphics,cResources *apResources,
										cParticleEmitterData3D_UserData *apData
										);
		~cParticleEmitter3D_UserData(void) override;

		void Kill() override;

		bool IsDying() override;

	private:
		void UpdateMotion(float afTimeStep) override;
		void SetParticleDefaults(cParticle *apParticle) override;


		cParticleEmitterData3D_UserData *mpData;

		float mfTime;
		float mfCreateCount;

		float mfCollideCount;

		float mfPauseCount;
		float mfPauseWaitCount;
		bool mbPaused;

		bool mbRespawn;
	};


};

#endif // HPL_PARTICLE_EMITTER_3D_USER_DATA_H
