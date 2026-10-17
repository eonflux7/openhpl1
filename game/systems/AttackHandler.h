/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GAME_ATTACK_HANDLER_H
#define GAME_ATTACK_HANDLER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;
class cMapHandler;
class cGameSwingDoor;

//---------------------------

#define eAttackTargetFlag_None 0
#define eAttackTargetFlag_Player (1 << 0)
#define eAttackTargetFlag_Enemy (1 << 1)
#define eAttackTargetFlag_Bodies (1 << 2)

typedef tFlag eAttackTargetFlag;


//---------------------------

class cAttackRayCallback : public iPhysicsRayCallback
{
public:
	cAttackRayCallback();

	void Reset();
	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;
	
	eAttackTargetFlag mTarget;
	iPhysicsBody *mpSkipBody;
	iPhysicsBody *mpClosestBody;
	float mfShortestDist;
	cVector3f mvPosition;
	bool mbSkipCharacter;
};

//---------------------------

class cSplashDamageBlockCheck : public iPhysicsRayCallback
{
public:
	cSplashDamageBlockCheck(cInit *apInit);

	bool CheckBlock(const cVector3f& avStart, const cVector3f&avEnd);
	
	bool BeforeIntersect(iPhysicsBody *pBody) override;
	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

private:
	cInit *mpInit;
	bool mbIntersected;
};

//---------------------------

class cAttackHandler : public iUpdateable
{
public:
	cAttackHandler(cInit *apInit);
	~cAttackHandler() override;

	bool CreateLineAttack(const cVector3f& avStart, const cVector3f& avEnd, float afDamage, 
						eAttackTargetFlag aTarget, iPhysicsBody *apSkipBody,
						iPhysicsBody **apPickedBody=nullptr);

	bool CreateShapeAttack(	iCollideShape *apShape, const cMatrixf& a_mtxShapeOffset,
							const cVector3f &avOrigin,float afDamage, 
							float afMinMass, float afMaxMass, float afMinImpulse, float afMaxImpulse,
							int alStrength,
							eAttackTargetFlag aTarget, iPhysicsBody *apSkipBody);
	bool HasFreePathToPlayer(const cVector3f &avOrigin, iPhysicsBody *apSkipBody = nullptr);
							

	bool CreateLineDestroyBody(const cVector3f& avStart, const cVector3f& avEnd, float afStrength, 
								float afForce,iPhysicsBody *apSkipBody, iPhysicsBody **apPickedBody=nullptr);

	void CreateSplashDamage(const cVector3f& avCenter, float afRadius, float afMinDamage, 
											float afMaxDamage,float afMinForce, float afMaxForce,
											float afMaxImpulse, 
											unsigned int aTarget, float afMinMass,
											int alStrength);

	cGameSwingDoor* GetLastSwingDoor(){ return mpLastSwingDoor;}

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;
private:
	cInit *mpInit;
	cMapHandler *mpMapHandler;

	
    cGameSwingDoor *mpLastSwingDoor;	

	cAttackRayCallback mRayCallback;
	cSplashDamageBlockCheck *mpSplashBlockCheck;
};



#endif // GAME_ATTACK_HANDLER_H
