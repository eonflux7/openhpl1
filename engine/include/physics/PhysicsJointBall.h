// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_JOINT_BALL_H
#define HPL_PHYSICS_JOINT_BALL_H

#include "physics/PhysicsJoint.h"

namespace hpl {

	kSaveData_ChildClass(iPhysicsJoint, iPhysicsJointBall)
	{
		kSaveData_ClassInit(iPhysicsJointBall)
	public:
		float mfMaxConeAngle;
		float mfMaxTwistAngle;
		cVector3f mvConePin;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class iPhysicsJointBall : public iPhysicsJoint
	{
	#ifdef __GNUC__
		typedef iPhysicsJoint __super;
	#endif
	public:
		iPhysicsJointBall(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
			iPhysicsWorld *apWorld, const cVector3f &avPivotPoint)
			: iPhysicsJoint(asName,apParentBody,apChildBody, apWorld,avPivotPoint){}
		~iPhysicsJointBall() override{}

		virtual void SetConeLimits(const cVector3f& avPin, float afMaxConeAngle, float afMaxTwistAngle)=0;
		virtual cVector3f GetAngles()=0;

		float GetMaxConeAngle(){ return mfMaxConeAngle;}
		float GetMaxTwistAngle(){ return mfMaxTwistAngle;}
		cVector3f GetConePin(){ return mvConePin;}

		ePhysicsJointType GetType() override{ return ePhysicsJointType_Ball;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	protected:
		float mfMaxConeAngle;
		float mfMaxTwistAngle;
		cVector3f mvConePin;
	};
};
#endif // HPL_PHYSICS_JOINT_BALL_H
