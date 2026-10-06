/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_MAP_CHECKS_H
#define GAME_MAP_CHECKS_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//----------------------------------------

// Checks for bugs of the kind the move to Newton 2 brought, run by the smoke
// test on every map when OVERTURE_CHECKS lists them:
//   settle  bodies that move, fall, keep shaking or go NaN with nobody
//           touching them, joints that leave where the map put them
//   pick    things the player can use that the pick ray cannot reach from
//           any side because something else is picked instead
//   areas   areas the player has a collide callback with that the game's
//           collision test does not find him in, standing in their middle
//   joints  doors, drawers, levers and wheels that do not move when pushed
//           both ways with a force about as strong as the player's
//   links   start positions the player falls from or ends up off the ground
//   calls   every function of the level script called once, with empty
//           strings and zeros, last (it leaves the map in a mess): crashes
//           and script exceptions
//   all     all of them
// Findings are logged as "CHECK <kind> <map>: ..." lines in hpl.log; they
// are leads to look at, not failures of the smoke test.
class cMapChecks
{
public:
	cMapChecks(cInit *apInit);

	static bool IsRequested();

	//Called when a map has loaded
	void Start(const tString &asMap, const tString &asFile);
	//True while script functions are called (they may open load screens)
	bool IsCalling(){ return mStep == eStep_Calls;}
	//Called every frame after; true when all checks of the map are done
	bool Update(float afTimeStep);

	int GetFindings(){ return mlFindings;}

private:
	enum eStep
	{
		eStep_Settle,
		eStep_Joints,
		eStep_Links,
		eStep_Calls,
		eStep_Done,
	};

	struct cBodyStart
	{
		tString msName;
		cVector3f mvPos;
		cMatrixf m_mtxRot;
	};
	struct cJointStart
	{
		tString msName;
		float mfAngle;
		float mfDist;
	};

	void Report(const char *asKind, const char *asFormat, ...);

	void StartSettle();
	void FinishSettle();
	void CheckPicks();
	void CheckAreas();
	void StartJoints();
	bool UpdateJoints();
	bool UpdateLinks();

	cInit *mpInit;
	bool mbSettle, mbPick, mbAreas, mbJoints, mbLinks, mbCalls;
	tStringVec mvCalls;
	size_t mlCall;

	tString msMap;
	tString msFile;
	bool mbReturning = false;
	eStep mStep;
	int mlFrame;
	int mlFindings;

	std::map<iPhysicsBody*, cBodyStart> m_mapBodies;
	std::map<iPhysicsJoint*, cJointStart> m_mapJoints;

	struct cJointPush
	{
		iPhysicsJoint *mpJoint;
		tString msName;
		float mfStart, mfMin, mfMax;
	};
	std::vector<cJointPush> mvPushes;

	tStringVec mvStartPos;
	size_t mlStartPos;
	cVector3f mvStartFeet;
};

//----------------------------------------

#endif // GAME_MAP_CHECKS_H
