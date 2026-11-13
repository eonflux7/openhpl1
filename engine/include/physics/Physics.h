// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_Physics_H
#define HPL_Physics_H

#include <list>
#include <map>
#include "game/Updateable.h"
#include "system/SystemTypes.h"
#include "physics/PhysicsMaterial.h"

namespace hpl {

	class iLowLevelPhysics;
	class iPhysicsWorld;
	class cSurfaceData;
	class cWorld3D;
	class cResources;

	typedef std::list<iPhysicsWorld*> tPhysicsWorldList;

	typedef std::map<tString, cSurfaceData*> tSurfaceDataMap;
	typedef tSurfaceDataMap::iterator tSurfaceDataMapIt;

	class cPhysicsImpactCount
	{
	public:
		cPhysicsImpactCount(){ mfCount =0;}

		float mfCount;
	};

	typedef std::list<cPhysicsImpactCount> tPhysicsImpactCountList;
	typedef tPhysicsImpactCountList::iterator tPhysicsImpactCountListIt;

	class cPhysics : public iUpdateable
	{
	public:
		cPhysics(iLowLevelPhysics *apLowLevelPhysics);
		~cPhysics() override;

		void Init(cResources *apResources);

		void Update(float afTimeStep) override;

		iPhysicsWorld* CreateWorld(bool abAddSurfaceData);
		void DestroyWorld(iPhysicsWorld* apWorld);

		cSurfaceData *CreateSurfaceData(const tString& asName);
		cSurfaceData *GetSurfaceData(const tString& asName);
		bool LoadSurfaceData(const tString& asFile);

		iLowLevelPhysics* GetLowLevel(){ return mpLowLevelPhysics;}

		void SetGameWorld(cWorld3D *apWorld){ mpGameWorld = apWorld;}
		cWorld3D* GetGameWorld(){ return mpGameWorld;}

		void SetImpactDuration(float afX){ mfImpactDuration = afX;}
		float GetImpactDuration(){ return mfImpactDuration;}

		void SetMaxImpacts(int alX){ mlMaxImpacts = alX;}
		int GetMaxImpacts(){ return mlMaxImpacts;}
		int GetNumOfImpacts(){ return (int)mlstImpactCounts.size();}

		bool CanPlayImpact();
		void AddImpact();

		void SetDebugLog(bool abX){ mbLog = abX;}
		bool GetDebugLog(){ return mbLog;}

	private:
		ePhysicsMaterialCombMode GetCombMode(const char *apName);

		void UpdateImpactCounts(float afTimeStep);

		iLowLevelPhysics *mpLowLevelPhysics;
		cResources *mpResources;

		cWorld3D *mpGameWorld;

		tPhysicsWorldList mlstWorlds;
		tSurfaceDataMap m_mapSurfaceData;

		tPhysicsImpactCountList mlstImpactCounts;
		float mfImpactDuration;
		int mlMaxImpacts;
		bool mbLog;
	};

};
#endif // HPL_Physics_H
