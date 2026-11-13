// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_NODE_H
#define HPL_NODE_H

#include <list>
#include "scene/Entity.h"
#include "math/MathTypes.h"
#include "system/Container.h"
#include "game/SaveGame.h"

namespace hpl {

	class iNode;

	typedef std::list<iNode*> tNodeList;
	typedef tNodeList::iterator tNodeListIt;

	typedef cSTLIterator<iNode*,tNodeList,tNodeListIt> cNodeIterator;

	kSaveData_BaseClass(iNode)
	{
		kSaveData_ClassInit(iNode)
	public:
		cContainerList<int> mlstEntities;
		cContainerList<int> mlstNodes;
	};

	class iNode : public iSaveObject
	{
	#ifdef __GNUC__
		typedef iSaveObject __super;
	#endif
	public:
		iNode() : mbActive(true){}
		~iNode() override;

		int SetVisible(bool abX, bool abCascade);
		bool AddEntity(iEntity* apEntity);
		bool RemoveEntity(iEntity* apEntity);
		void ClearEntities();

		void SetActive(bool abX){mbActive = abX;}
		bool IsActive(){return mbActive;}

		virtual iNode* CreateChild()=0;

		cNodeIterator GetChildIterator();
		cEntityIterator GetEntityIterator();

		//SaveObject implementation
		iSaveData* CreateSaveData() override{return nullptr;}
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	protected:
		tEntityList mlstEntity;
		tNodeList mlstNode;

		bool mbActive;
	};

};
#endif // HPL_NODE_H
