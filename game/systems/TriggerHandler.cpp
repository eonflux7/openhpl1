// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "systems/TriggerHandler.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "enemies/GameEnemy.h"

// TRIGGER

cGameTrigger::cGameTrigger() : iEntity3D("")
{
	SetRadius(1);
	SetRenderInterpolated(false);
}

void cGameTrigger::SetRadius(float afRadius)
{
	mfRadius = afRadius;
	mBoundingVolume.SetSize(mfRadius*2);
}

cTriggerHandler::cTriggerHandler(cInit *apInit)  : iUpdateable("TriggerHandler")
{
	mpInit = apInit;
}

cTriggerHandler::~cTriggerHandler(void)
{
	STLMapDeleteAll(m_mapTriggers);
}

cGameTrigger* cTriggerHandler::Add(cGameTrigger *apTrigger,eGameTriggerType aType ,
								   const cVector3f &avLocalPos,
								   int alPrio, float afLifetimeSeconds, float afRadius)
{
	apTrigger->SetRadius(afRadius);
	apTrigger->mfRemainingSeconds = afLifetimeSeconds;
	apTrigger->mlPrio = alPrio;
	apTrigger->mType = aType;
	apTrigger->SetPosition(avLocalPos);
	
	m_mapTriggers.insert(tGameTriggerMap::value_type(alPrio, apTrigger));

	return apTrigger;
}

void cTriggerHandler::OnStart()
{
	mpMapHandler = mpInit->mpMapHandler;
}

void cTriggerHandler::Update(float afTimeStep)
{
	//Go through the enemies and hand them triggers
	tGameEnemyIterator EnemyIt = mpMapHandler->GetGameEnemyIterator();
	while(EnemyIt.HasNext())
	{
		iGameEnemy *pEnemy = EnemyIt.Next();
		if(pEnemy->IsActive()==false || pEnemy->GetUsesTriggers()==false) continue;
		
		//Check if it is time to update triggers.
		/*if(pEnemy->GetTriggerUpdateCount() >= pEnemy->GetTriggerUpdateRate())
		{
			pEnemy->SetTriggerUpdateCount(0);
		}
		else
		{
			pEnemy->SetTriggerUpdateCount(pEnemy->GetTriggerUpdateCount() + afTimeStep);
			continue;
		}*/
		
		//Go through triggers by priority
		tGameTriggerMapIt TriggerIt = m_mapTriggers.begin();
		for(; TriggerIt != m_mapTriggers.end(); ++TriggerIt)
		{
			cGameTrigger *pTrigger = TriggerIt->second;

            //Check if trigger is of right type
			if(!(pTrigger->GetType() & pEnemy->GetTriggerTypes()))
			{
				continue;
			}

			//Check if trigger is in reach
			if(cMath::PointBVCollision(pEnemy->GetPosition(),*pTrigger->GetBoundingVolume()) == false)
			{
				continue;
			}
			
			//Let enemy handle trigger, if false get next trigger else next enemy.
			if(pEnemy->HandleTrigger(pTrigger))
			{
				break;
			}
		}
	}
	
	//Go through triggers and remove when timer is out.
	tGameTriggerMapIt TriggerIt = m_mapTriggers.begin();
	for(; TriggerIt != m_mapTriggers.end(); )
	{
		cGameTrigger *pTrigger = TriggerIt->second;
		
		pTrigger->mfRemainingSeconds -= afTimeStep;

		if(pTrigger->mfRemainingSeconds <= 0)
		{
			hplDelete( pTrigger );
			m_mapTriggers.erase(TriggerIt++);
		} else {
			++TriggerIt;
		}
	}
}

void cTriggerHandler::Reset()
{

}
