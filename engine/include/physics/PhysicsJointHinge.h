// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_JOINT_HINGE_H
#define HPL_PHYSICS_JOINT_HINGE_H

#include "physics/PhysicsJoint.h"

namespace hpl {

	kSaveData_ChildClass(iPhysicsJoint, iPhysicsJointHinge)
	{
		kSaveData_ClassInit(iPhysicsJointHinge)
	public:
		float mfMaxAngle;
		float mfMinAngle;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class iPhysicsJointHinge : public iPhysicsJoint
	{
	#ifdef __GNUC__
		typedef iPhysicsJoint __super;
	#endif
	public:
		iPhysicsJointHinge(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
			iPhysicsWorld *apWorld,const cVector3f &avPivotPoint)
			: iPhysicsJoint(asName,apParentBody,apChildBody, apWorld,avPivotPoint){}
		~iPhysicsJointHinge() override{}

		virtual void SetMaxAngle(float afAngle)=0;
		virtual void SetMinAngle(float afAngle)=0;
		virtual float GetMaxAngle()=0;
		virtual float GetMinAngle()=0;

		ePhysicsJointType GetType() override{ return ePhysicsJointType_Hinge;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	protected:
		float mfMaxAngle;
		float mfMinAngle;
	};
};
#endif // HPL_PHYSICS_JOINT_HINGE_H
