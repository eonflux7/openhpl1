/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_PHYSICS_LOG_H
#define GAME_PHYSICS_LOG_H

#include "core/StdAfx.h"
#include "core/GameTypes.h"

#include <map>

using namespace hpl;

class cInit;

//----------------------------------------

// Physics event log for debugging, enabled with OVERTURE_PHYSLOG=1.
// Writes "PHYS:" lines to hpl.log, stamped with seconds since the map loaded:
//   wake / rest   a dynamic body starts moving / the engine puts it to sleep,
//                 with how far and how fast it moved while awake and, for
//                 joint children, the joint's angle or distance at rest
//   awake         a body that has not gone to sleep for a while, repeated
//   frozen        a body the engine froze (never wakes from contact)
//   player        player state changes, and the body being grabbed, moved
//                 or pushed, with its speed just after release
class cPhysicsLog : public iUpdateable
{
public:
	cPhysicsLog(cInit *apInit);

	static bool IsRequested();

	void Update(float afTimeStep) override;
	void Reset() override;

private:
	struct cBodyTrack
	{
		bool mbAwake;
		bool mbFrozen;
		float mfWakeTime;
		float mfNextAwakeLog;
		cVector3f mvWakePos;
		float mfPath;
		cVector3f mvLastPos;
		float mfMaxSpeed;
		float mfMaxAngSpeed;
	};

	void StartWorld(cWorld3D *apWorld);
	void UpdateBody(iPhysicsBody *apBody, cBodyTrack &aTrack);
	void UpdatePlayer();
	tString JointInfo(iPhysicsBody *apBody);

	cInit *mpInit;

	iPhysicsWorld *mpWorld;
	float mfTime;
	std::map<iPhysicsBody*, cBodyTrack> m_mapBodies;
	std::map<iPhysicsBody*, iPhysicsJoint*> m_mapJoints; // child body -> joint

	ePlayerState mPlayerState;
	iPhysicsBody *mpHeldBody;
	iPhysicsBody *mpReleasedBody;
};

//----------------------------------------

#endif // GAME_PHYSICS_LOG_H
