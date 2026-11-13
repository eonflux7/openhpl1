// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PHYSICS_JOINT_SLIDER_H
#define HPL_PHYSICS_JOINT_SLIDER_H

#include "physics/PhysicsJoint.h"

namespace hpl {

	kSaveData_ChildClass(iPhysicsJoint, iPhysicsJointSlider)
	{
		kSaveData_ClassInit(iPhysicsJointSlider)
	public:
		float mfMaxDistance;
		float mfMinDistance;
		cVector3f mvPin;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class iPhysicsJointSlider : public iPhysicsJoint
	{
	#ifdef __GNUC__
		typedef iPhysicsJoint __super;
	#endif
	public:
		iPhysicsJointSlider(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
			iPhysicsWorld *apWorld,const cVector3f &avPivotPoint)
			: iPhysicsJoint(asName,apParentBody,apChildBody, apWorld,avPivotPoint){}
			~iPhysicsJointSlider() override{}
		/**
		 * Set the maximum distance the bodies can be from each other, relative to the start dist between them
		 * This is true if pin points towards the child.
		 * In other words, distance increases as the distance between start pivot and current pivot
		 * increases in the opposite direction of the pin.
		*/
		virtual void SetMaxDistance(float afX)=0;
		/**
		* Set the minimum distance the bodies can be from each other, relative to the start dist between them
		* This is true if pin points towards the child.
		*/
		virtual void SetMinDistance(float afX)=0;
		virtual float GetMaxDistance()=0;
		virtual float GetMinDistance()=0;

		ePhysicsJointType GetType() override{ return ePhysicsJointType_Slider;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;
	protected:
		float mfMaxDistance;
		float mfMinDistance;
		cVector3f mvPin;
	};
};
#endif // HPL_PHYSICS_JOINT_SLIDER_H
