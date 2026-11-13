// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ENTITY_H
#define HPL_ENTITY_H

#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "game/SaveGame.h"

namespace hpl {

	class iNode;

	kSaveData_BaseClass(iEntity)
	{
		kSaveData_ClassInit(iEntity)
	public:
		int mlParentNodeId;
		tString msName;
		bool mbIsVisible;
		bool mbIsActive;
	};

	class iEntity : public iSaveObject
	{
	#ifdef __GNUC__
		typedef iSaveObject __super;
	#endif
	public:
		iEntity(tString asName) : msName(asName), mbIsVisible(true),
			mbIsActive(true), mpParentNode(nullptr) {}

		~iEntity() override;

		virtual tString GetEntityType()=0;

		virtual void UpdateLogic(float afTimeStep){}

		tString& GetName(){return msName;}
		void SetName(const tString& asName){msName = asName;}

		iNode* GetParent(){ return mpParentNode;}
		void SetParent(iNode* apNode){ mpParentNode = apNode;}
		bool HasParent(){ return mpParentNode!=nullptr;}

		bool IsActive(){ return mbIsActive; }
		void SetActive(bool abActive){ mbIsActive = abActive; }
		virtual bool IsVisible(){ return mbIsVisible; }
		virtual void SetVisible(bool abVisible){ mbIsVisible = abVisible; }

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	protected:
		iNode* mpParentNode;

		tString msName;
		bool mbIsVisible;
		bool mbIsActive;
	};

	typedef std::list<iEntity*> tEntityList;
	typedef tEntityList::iterator tEntityListIt;

	typedef cSTLIterator<iEntity*,tEntityList,tEntityListIt> cEntityIterator;

};
#endif // HPL_ENTITY2D_H
