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
#ifndef GAME_CHARACTER_MOVE_H
#define GAME_CHARACTER_MOVE_H

#include "core/StdAfx.h"

using namespace hpl;

class iGameEnemy;

#include "core/Init.h"

//------------------------------------------

class cCharacterMove;

class cCharacterAStarCallback : public iAStarCallback, public iAIFreePathCallback
{
public:
	cCharacterAStarCallback(cCharacterMove *apMove);

	bool CanAddNode(cAINode *apParentNode,cAINode *apChildNode) override;

	bool Intersects(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	bool mbCheckDynamic;

	cCharacterMove *mpMove;
};

//------------------------------------------

class cMoverRayCallback : public iPhysicsRayCallback
{
public:
	void Reset();
	bool BeforeIntersect(iPhysicsBody *pBody) override;
	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	bool mbIntersection;
};

//------------------------------------------

class cCharacterMove
{
	friend class cCharacterAStarCallback;
public:
	cCharacterMove(cInit *apInit);
	~cCharacterMove();

	void Update(float afTimeStep);

	void OnDraw(cInit *apInit);
	void OnPostSceneDraw(iLowLevelGraphics *apLowLevelGfx);
	
	///////////////////////////////////
	// Actions 
	/**
	 * returns false if no path could be found.
	 */
	bool MoveToPos(const cVector3f &avPos, bool abCheckDynamic = false);
	void MoveDirectToPos(const cVector3f &avFeetPos, float afTimeStep);

	void TurnToAngle(float afAngle);
	void TurnToPos(const cVector3f &avPos);

	void Stop();

	///////////////////////////////////
	// Helpers
	
	bool FreeDirectPathToChar(iCharacterBody *apBody);
	bool FreeDirectPathToPos(const cVector3f &avFeetPos);
	void SetSmoothNavigation(bool abEnabled){ mbSmoothNavigation = abEnabled; }

	float DistanceToChar(iCharacterBody *apBody);
	float DistanceToChar2D(iCharacterBody *apBody);

	cAINode* GetAINodeInRange(float afMinDistance,float afMaxDistance);
	cAINode* GetAINodeAtPosInRange(const cVector3f &avPos,float afMinDistance,float afMaxDistance,
									bool abRayCheck, float afEndOffset);

	
	//////////////////////////////////////////
	// Properties
	bool IsMoving(){ return mbMoving;}

	bool IsTurning(){ return mbTurning;}

	void SetCharBody(iCharacterBody *apCharBody);
	iCharacterBody* GetCharBody(){ return mpCharBody;}

	void SetNodeContainer(cAINodeContainer *apContainer);
	cAINodeContainer* GetNodeContainer(){ return mpContainer;}

	void SetAStar(cAStarHandler *apAStar);
	cAStarHandler* GetAStar(){ return mpAStar;}

	void SetMaxTurnSpeed(float afRadiansPerSecond){ mfMaxTurnSpeed = afRadiansPerSecond; }
	void SetAngleDistTurnMul(float afTurnGain){ mfAngleDistTurnMul = afTurnGain;};

	void SetMinBreakAngle(float afRadians){ mfMinBreakAngle = afRadians; }
	void SetBreakAngleMul(float afBrakeGain){ mfBreakAngleMul = afBrakeGain; }

	void SetMaxPushMass(float afMass){mfMaxPushMass = afMass;}

	float GetTurnSpeed(){ return mfTurnSpeed;}

	const cVector3f& GetGoalPos(){ return mvGoalPos;}

	float GetStuckCounter(){ return mfStuckCounter;}
	void ResetStuckCounter(){ mfStuckCounter = 0;}

	void SetMaxDoorToughness(int alToughness){mlMaxDoorToughness = alToughness;}
	int GetMaxDoorToughness(){ return mlMaxDoorToughness;}

private:
	cInit *mpInit;

	iCharacterBody *mpCharBody;

	cAINodeContainer *mpContainer;
	cAStarHandler *mpAStar;
	cBoundingVolume mBoundingVolume;
	
	tAINodeList mlstNodes;

	bool mbMoving;
	bool mbTurning;
	bool mbSmoothNavigation;

	float mfTurnSpeed;

	cVector3f mvGoalPos;

	float mfGoalAngle;

	float mfStuckCounter;
	float mfStuckLimit;

	std::list<float> mlstNodeSquaredDistances;
	int mlMaxNodeDistances;
	// Sum of squared-distance differences from the oldest sample.
	float mfNodeDistanceChangeSum;

	//Properties
	float mfMaxTurnSpeed;
	float mfAngleDistTurnMul;

	float mfMinBreakAngle;
	float mfBreakAngleMul;

	float mfMaxPushMass;

	cMoverRayCallback mRayCallback;

	int mlMaxDoorToughness;
	
	//Debug:
	float mfCurrentBreak;	
	cVector3f mvTempStart;
	cVector3f mvTempEnd;
	
	bool mbMoveToNewNode;

	cCharacterAStarCallback *mpAStarCallback;
};

//--------------------------------------

#endif // GAME_CHARACTER_MOVE_H
