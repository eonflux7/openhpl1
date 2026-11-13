// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/Updater.h"

#include "game/Updateable.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	cUpdater::cUpdater(iLowLevelSystem *apLowLevelSystem)
	{
		mpCurrentUpdates = nullptr;

		mpLowLevelSystem = apLowLevelSystem;
	}

	cUpdater::~cUpdater()
	{
	}

	void cUpdater::OnInterpolate(float afT)
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnInterpolate(afT);
		}

		if(mpCurrentUpdates)
		{
			for(tUpdateableListIt it = mpCurrentUpdates->begin();it!=mpCurrentUpdates->end();++it)
			{
				(*it)->OnInterpolate(afT);
			}
		}
	}

	void cUpdater::OnDraw()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnDraw();
		}

		if(mpCurrentUpdates)
		{
			for(tUpdateableListIt it = mpCurrentUpdates->begin();it!=mpCurrentUpdates->end();++it)
			{
				(*it)->OnDraw();
			}
		}
	}

	void cUpdater::OnPostSceneDraw()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnPostSceneDraw();
		}

		if(mpCurrentUpdates)
		{
			for(tUpdateableListIt it = mpCurrentUpdates->begin();it!=mpCurrentUpdates->end();++it)
			{
				(*it)->OnPostSceneDraw();
			}
		}
	}

	void cUpdater::OnPostGUIDraw()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnPostGUIDraw();
		}

		if(mpCurrentUpdates)
		{
			for(tUpdateableListIt it = mpCurrentUpdates->begin();it!=mpCurrentUpdates->end();++it)
			{
				(*it)->OnPostGUIDraw();
			}
		}
	}

	void cUpdater::OnPostBufferSwap()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnPostBufferSwap();
		}

		if(mpCurrentUpdates)
		{
			for(tUpdateableListIt it = mpCurrentUpdates->begin();it!=mpCurrentUpdates->end();++it)
			{
				(*it)->OnPostBufferSwap();
			}
		}
	}

	void cUpdater::OnStart()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnStart();
		}

		tUpdateContainerMapIt ContIt = m_mapUpdateContainer.begin();
		while(ContIt != m_mapUpdateContainer.end())
		{
			tUpdateableListIt UpIt = ContIt->second.begin();
			while(UpIt != ContIt->second.end())
			{
				(*UpIt)->OnStart();
				UpIt++;
			}

			ContIt++;
		}
	}

	void cUpdater::Reset()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->Reset();
		}

		tUpdateContainerMapIt ContIt = m_mapUpdateContainer.begin();
		while(ContIt != m_mapUpdateContainer.end())
		{

			tUpdateableList *pUpdates = &ContIt->second;
			tUpdateableListIt UpIt = pUpdates->begin();
			while(UpIt != pUpdates->end())
			{
				iUpdateable *pUpdate = *UpIt;

				pUpdate->Reset();

				++UpIt;
			}

			++ContIt;
		}
	}

	void cUpdater::OnExit()
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			(*it)->OnExit();
		}

		tUpdateContainerMapIt ContIt = m_mapUpdateContainer.begin();
		while(ContIt != m_mapUpdateContainer.end())
		{
			tUpdateableListIt UpIt = ContIt->second.begin();
			while(UpIt != ContIt->second.end())
			{
				(*UpIt)->OnExit();
				UpIt++;
			}

			ContIt++;
		}
	}

	void cUpdater::Update(float afTimeStep)
	{
		for(tUpdateableListIt it = mlstGlobalUpdateableList.begin();it!=mlstGlobalUpdateableList.end();++it)
		{
			START_TIMING_EX((*it)->GetName().c_str(),game)
			(*it)->Update(afTimeStep);
			STOP_TIMING(game)
		}

		if(mpCurrentUpdates)
		{
			tUpdateableList *pList = mpCurrentUpdates;
			for(tUpdateableListIt it = pList->begin();it!=pList->end();++it)
			{
				START_TIMING_EX((*it)->GetName().c_str(),game)
				(*it)->Update(afTimeStep);
				STOP_TIMING(game)
			}
		}
	}

	bool cUpdater::SetContainer(tString asContainer)
	{
		tUpdateContainerMapIt it = m_mapUpdateContainer.find(asContainer);
		if(it == m_mapUpdateContainer.end()) return false;

		msCurrentUpdates = asContainer;
		if(msCurrentUpdates == "Default"){
			SetUpdateLogActive(true);
		}
		else {
			SetUpdateLogActive(false);
		}

		mpCurrentUpdates = &it->second;

		return true;
	}

	tString cUpdater::GetCurrentContainerName()
	{
		if(mpCurrentUpdates==nullptr) return "";

		return msCurrentUpdates;

	}

	bool cUpdater::AddContainer(tString asName)
	{
		//Create the value for the map with key and Updateable
		tUpdateContainerMap::value_type val = tUpdateContainerMap::value_type(
														asName, tUpdateableList());
		//Add it to the map
		m_mapUpdateContainer.insert(val);

		return true;
	}

	bool cUpdater::AddUpdate(tString asContainer, iUpdateable* apUpdate)
	{
		if(apUpdate==nullptr){
			Error("Couldn't add NULL updatable!");
			return false;
		}

		//Search the map for the container name
		tUpdateContainerMapIt it = m_mapUpdateContainer.find(asContainer);
		if(it == m_mapUpdateContainer.end()) return false;

		//Add the updatable
		it->second.push_back(apUpdate);

		return true;
	}

	bool cUpdater::AddGlobalUpdate(iUpdateable* apUpdate)
	{
		mlstGlobalUpdateableList.push_back(apUpdate);
		return true;
	}
}
