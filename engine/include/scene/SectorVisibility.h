// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SECTOR_VISIBILTY_H
#define HPL_SECTOR_VISIBILTY_H

#include "math/BoundingVolume.h"
#include "math/Frustum.h"


namespace hpl {

	class cPortal;
	class cSector;
	class cPortalContainer;
	class cSectorVisibilityContainer;

	enum eSectorVisibilityType
	{
		eSectorVisibilityType_BV,
		eSectorVisibilityType_Frustum,
	};

	/**
	 * Represent the visibility for one portal
	 */
	class cPortalVisibility
	{
	public:
		cPortalVisibility();
		~cPortalVisibility();

		cShadowVolumeBV mShadow;
		cPortal *mpPortal;
		bool mbNullShadow;
	};

	/**
	* Represent the visibility from one sector to another
	* this may include several portals.
	*/
	class cPortalVisibilitySet
	{
	friend class cSectorVisibilityContainer;
	public:
		cPortalVisibilitySet(cSectorVisibilityContainer *apContainer, cPortalVisibilitySet *apParent);
		~cPortalVisibilitySet();

		cPortalVisibilitySet* GetParent(){ return mpParent;}

		inline cPortalVisibility* GetVisibility(size_t alIdx){ return mvVisibility[alIdx];}
		inline size_t GetVisibilityNum(){ return mvVisibility.size();}

		bool PortalExists(cPortal *apPortal);

		int AddPortalVisibility(cPortal *apPortal);

	private:
		std::vector<cPortalVisibility*> mvVisibility;
		cPortalVisibilitySet *mpParent;

		cSectorVisibilityContainer *mpContainer;
	};

	/**
	* Represent the visibility of one sector.
	*/
	class cSectorVisibility
	{
	friend class cSectorVisibilityContainer;
	public:
		cSectorVisibility(cSectorVisibilityContainer *apContainer);
		~cSectorVisibility();

		bool PortalExists(cPortal *apPortal);

		cPortalVisibilitySet* GetSetConnectingFromSector(cSector *apSector);

		void AddVisibilitySet(cPortalVisibilitySet* apSet);

		cSector* GetSector() const {return mpSector;}

		bool IntersectionBV(cBoundingVolume *apBV);

	private:
		cSector *mpSector;
		std::vector<cPortalVisibilitySet*> mvVisibilitySets;
		bool bStart;

		cSectorVisibilityContainer *mpContainer;
	};

	typedef std::map<cSector*,cSectorVisibility*> tSectorVisibilityMap;
	typedef tSectorVisibilityMap::iterator tSectorVisibilityMapIt;

	typedef std::list<cPortalVisibilitySet*> tPortalVisibilitySetList;
	typedef tPortalVisibilitySetList::iterator tPortalVisibilitySetListIt;

	typedef std::set<cSector*> tSectorSet;
	typedef tSectorSet::iterator tSectorSetIt;

	typedef cSTLMapIterator<cSectorVisibility*,tSectorVisibilityMap,tSectorVisibilityMapIt> tSectorVisibilityIterator;

	/**
	* Container of visibility of all the sectors.
	*/
	class cSectorVisibilityContainer
	{
	public:
		cSectorVisibilityContainer(eSectorVisibilityType aType);
		~cSectorVisibilityContainer();

		void SetFrustum(const cFrustum &aFrustum){ mFrustum = aFrustum;}
		void SetBV(const cBoundingVolume &aBV){ mBoundingVolume = aBV;}

		const cVector3f& GetOrigin(){ return mvOrigin;}

		void Compute(cPortalContainer *apContainer);

		cPortalVisibilitySet* CreatePortalVisibilitySet(cPortalVisibilitySet* apParent);

		cSectorVisibility* GetSectorVisibility(cSector *apSector);

		tSectorVisibilityIterator GetSectorIterator();

		bool IntersectionBV(cBoundingVolume *apBV, cPortalVisibilitySet *apSet);

		bool mbLog;
	private:

		void SearchSector(cSector *apSector, cPortalVisibilitySet *apParentSet,int alPortalIndex);

		tString GetTabs();

		cPortalContainer *mpContainer;

		tSectorSet m_setStartSectors;

		tSectorVisibilityMap m_mapSectors;
		eSectorVisibilityType mType;

		tPortalVisibilitySetList mlstPortalVisibility;

		cVector3f mvOrigin;
		cBoundingVolume mBoundingVolume;
		cFrustum mFrustum;

		int mlTabs;
	};
};
#endif // HPL_SECTOR_VISIBILTY_H
