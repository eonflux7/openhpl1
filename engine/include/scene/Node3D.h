// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_NODE3D_H
#define HPL_NODE3D_H

#include <list>
#include "math/MathTypes.h"
#include "scene/Node.h"

namespace hpl {

	kSaveData_ChildClass(iNode,cNode3D)
	{
		kSaveData_ClassInit(cNode3D)
	public:
		tString msName;
		tString msSource;
		bool mbAutoDeleteChildren;
		cMatrixf m_mtxLocalTransform;
		int mlParentId;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class cNode3D : public iNode
	{
	#ifdef __GNUC__
		typedef iNode __super;
	#endif
	public:
		cNode3D(const tString &asName="", bool abAutoDeleteChildren = true);
		~cNode3D() override;

		iNode* CreateChild() override;
		cNode3D* CreateChild3D(const tString &asName="", bool abAutoDeleteChildren = true);

		cVector3f GetLocalPosition();
		cMatrixf& GetLocalMatrix();

		cVector3f GetWorldPosition();
		cMatrixf& GetWorldMatrix();

		void SetPosition(const cVector3f& avPos);
		void SetMatrix(const cMatrixf& a_mtxTransform, bool abSetChildrenUpdated=true);

		void SetWorldPosition(const cVector3f& avWorldPos);
		void SetWorldMatrix(const cMatrixf& a_mtxWorldTransform);

		const char* GetName();

		cNode3D* GetParent();

		void AddRotation(const cVector3f& avRot, eEulerRotationOrder aOrder);
		void AddRotation(const cQuaternion& aqRotation);

		void AddScale(const cVector3f& avScale);

		void AddTranslation(const cVector3f& avTrans);

		void SetSource(const tString &asSource);
		const char* GetSource();

		/**
		 * Updates the matrix with the added scales, translations and rotation. It also resets these values.
		 */
		void UpdateMatrix(bool abSetChildrenUpdated);

		//Extra stuff that shouldn't be used externally
		void SetParent(cNode3D* apNode);
		void AddChild(cNode3D* apChild);

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

		void UpdateWorldTransform();
		void SetWorldTransformUpdated();
	private:
		tString msName;
		tString msSource;

		bool mbAutoDeleteChildren;

		cMatrixf m_mtxLocalTransform;
		cMatrixf m_mtxWorldTransform;

		cVector3f mvWorldPosition;

		cMatrixf m_mtxRotation;
		cVector3f mvScale;
		cVector3f mvTranslation;

		bool mbTransformUpdated;

		cNode3D* mpParent;
	};

};
#endif // HPL_NODE3D_H
