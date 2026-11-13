// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_BONE_H
#define HPL_BONE_H

#include <map>

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class cSkeleton;
	class cBone;

	typedef std::list<cBone*> tBoneList;
	typedef tBoneList::iterator tBoneListIt;

	typedef cSTLIterator<cBone*,tBoneList,tBoneListIt> cBoneIterator;

	class cBone
	{
	friend class cSkeleton;
	public:
		cBone(const tString &asName, cSkeleton* apSkeleton);
		~cBone();

		cBone* CreateChildBone(const tString &asName);

		void SetTransform(const cMatrixf &a_mtxTransform);
		const cMatrixf& GetLocalTransform();
		const cMatrixf& GetWorldTransform();
		const cMatrixf& GetInvWorldTransform();

		const tString& GetName(){ return msName;}

		cBoneIterator GetChildIterator();

		void Detach();

		cBone* GetParent(){ return mpParent;}

		//Needed for some loading stuff..
		int GetValue(){ return mlValue;}
		void SetValue(int alVal){ mlValue = alVal;}

	private:
		void NeedsUpdate();

		void UpdateMatrix();

		tString msName;

		cMatrixf m_mtxTransform;

		cMatrixf m_mtxWorldTransform;
		cMatrixf m_mtxInvWorldTransform;

		cBone* mpParent;
		tBoneList mlstChildren;

		cSkeleton* mpSkeleton;

		bool mbNeedsUpdate;

		int mlValue;
	};

};
#endif // HPL_BONE_H
