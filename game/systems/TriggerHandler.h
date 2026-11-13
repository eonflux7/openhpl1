// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_TRIGGER_HANDLER_H
#define GAME_TRIGGER_HANDLER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;
class cMapHandler;

class cGameTrigger : public iEntity3D
{
friend class cTriggerHandler;
public:
	cGameTrigger();
	~cGameTrigger() override{}
	
	int GetPrio(){ return mlPrio;}
	float GetRemainingSeconds(){ return mfRemainingSeconds;}
	eGameTriggerType GetType(){ return mType;}
	
	float GetRadius(){ return mfRadius;}
	void SetRadius(float afRadius);

	cBoundingVolume* GetBoundingVolume() override{ return &mBoundingVolume;}
	
	tString GetEntityType() override{ return "GameTrigger"; }

protected:
	int mlPrio;
	float mfRemainingSeconds;
	eGameTriggerType mType;
private:
	float mfRadius;
};

typedef std::multimap<int, cGameTrigger*, std::greater<int> > tGameTriggerMap;
typedef tGameTriggerMap::iterator tGameTriggerMapIt;

class cTriggerHandler : public iUpdateable
{
public:
	cTriggerHandler(cInit *apInit);
	~cTriggerHandler() override;

	cGameTrigger* Add(cGameTrigger *apTrigger,eGameTriggerType aType , const cVector3f &avLocalPos,
												int alPrio, float afLifetimeSeconds, float afRadius);

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;
private:
	cInit *mpInit;
	cMapHandler *mpMapHandler;

	tGameTriggerMap m_mapTriggers;
};



#endif // GAME_TRIGGER_HANDLER_H
