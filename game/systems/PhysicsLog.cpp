/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "systems/PhysicsLog.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "player/Player.h"

#include <SDL3/SDL.h>

// Seconds a body may stay awake before it is reported, and between reports
static const float kfAwakeLogInterval = 10.0f;

static const char *gvPlayerStateNames[] = {
	"Normal", "Push", "Move", "InteractMode", "UseItem", "Message",
	"Grab", "WeaponMelee", "Throw", "Climb",
};

static tString VecStr(const cVector3f &avVec)
{
	char sBuff[64];
	snprintf(sBuff, sizeof(sBuff), "(%.2f %.2f %.2f)", avVec.x, avVec.y, avVec.z);
	return sBuff;
}

static bool IsTracked(iPhysicsBody *apBody)
{
	return apBody->GetMass() > 0 && !apBody->IsCharacter();
}

//-----------------------------------------------------------------------

cPhysicsLog::cPhysicsLog(cInit *apInit) : iUpdateable("PhysicsLog")
{
	mpInit = apInit;
	Reset();
}

//-----------------------------------------------------------------------

bool cPhysicsLog::IsRequested()
{
	const char *pVal = SDL_getenv("OVERTURE_PHYSLOG");
	return pVal && pVal[0] && tString(pVal) != "0";
}

//-----------------------------------------------------------------------

void cPhysicsLog::Reset()
{
	// Loading a save or a map rebuilds the world, so start over.
	mpWorld = nullptr;
	mfTime = 0;
	m_mapBodies.clear();
	m_mapJoints.clear();
	mPlayerState = ePlayerState_LastEnum;
	mpHeldBody = nullptr;
	mpReleasedBody = nullptr;
	mfPlayerHealth = -1;
	mbPlayerOnGround = true;
	mfPlayerAirTime = 0;
	mfPlayerFallSpeed = 0;
}

//-----------------------------------------------------------------------

void cPhysicsLog::Update(float afTimeStep)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld == nullptr || pWorld->GetPhysicsWorld() == nullptr) return;

	if(pWorld->GetPhysicsWorld() != mpWorld) StartWorld(pWorld);
	mfTime += afTimeStep;

	// Joints can break and bodies can be destroyed, so rebuild every frame.
	m_mapJoints.clear();
	cPhysicsJointIterator jointIt = mpWorld->GetJointIterator();
	while(jointIt.HasNext())
	{
		iPhysicsJoint *pJoint = jointIt.Next();
		if(pJoint->GetChildBody()) m_mapJoints[pJoint->GetChildBody()] = pJoint;
	}

	std::map<iPhysicsBody*, cBodyTrack> mapSeen;
	cPhysicsBodyIterator bodyIt = mpWorld->GetBodyIterator();
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = bodyIt.Next();
		if(!IsTracked(pBody)) continue;

		std::map<iPhysicsBody*, cBodyTrack>::iterator it = m_mapBodies.find(pBody);
		cBodyTrack Track;
		if(it != m_mapBodies.end())
		{
			Track = it->second;
		}
		else
		{
			// Bodies created after the map loaded (thrown items, broken pieces)
			Track.mbAwake = false;
			Track.mbFrozen = pBody->IsFrozen();
		}
		UpdateBody(pBody, Track);
		mapSeen[pBody] = Track;
	}
	m_mapBodies.swap(mapSeen);

	UpdatePlayer(afTimeStep);
}

//-----------------------------------------------------------------------

void cPhysicsLog::StartWorld(cWorld3D *apWorld)
{
	Reset();
	mpWorld = apWorld->GetPhysicsWorld();

	int lBodies = 0, lAwake = 0, lFrozen = 0;
	cPhysicsBodyIterator bodyIt = mpWorld->GetBodyIterator();
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = bodyIt.Next();
		if(!IsTracked(pBody)) continue;

		// Report the state the map starts in, then only changes from it.
		cBodyTrack &Track = m_mapBodies[pBody];
		Track.mbAwake = false;
		Track.mbFrozen = pBody->IsFrozen();
		lBodies++;
		if(!pBody->IsSleeping()) lAwake++;
		if(Track.mbFrozen) lFrozen++;
	}

	int lJoints = 0;
	cPhysicsJointIterator jointIt = mpWorld->GetJointIterator();
	while(jointIt.HasNext()){ jointIt.Next(); lJoints++; }

	Log("PHYS: map %s: %d dynamic bodies (%d awake, %d frozen), %d joints\n",
		apWorld->GetFileName().c_str(), lBodies, lAwake, lFrozen, lJoints);
}

//-----------------------------------------------------------------------

void cPhysicsLog::UpdateBody(iPhysicsBody *apBody, cBodyTrack &aTrack)
{
	const tString &sName = apBody->GetName();
	cVector3f vPos = apBody->GetWorldPosition();

	bool bFrozen = apBody->IsFrozen();
	if(bFrozen != aTrack.mbFrozen)
	{
		Log("PHYS: %7.2f %s %s at %s\n", mfTime, bFrozen ? "frozen" : "unfrozen",
			sName.c_str(), VecStr(vPos).c_str());
		aTrack.mbFrozen = bFrozen;
	}

	bool bAwake = !apBody->IsSleeping() && !bFrozen;
	if(bAwake && !aTrack.mbAwake)
	{
		aTrack.mbAwake = true;
		aTrack.mfWakeTime = mfTime;
		aTrack.mfNextAwakeLog = mfTime + kfAwakeLogInterval;
		aTrack.mvWakePos = vPos;
		aTrack.mvLastPos = vPos;
		aTrack.mfPath = 0;
		aTrack.mfMaxSpeed = 0;
		aTrack.mfMaxAngSpeed = 0;
		Log("PHYS: %7.2f wake %s at %s%s\n", mfTime, sName.c_str(), VecStr(vPos).c_str(),
			JointInfo(apBody).c_str());
	}
	else if(!bAwake && aTrack.mbAwake)
	{
		aTrack.mbAwake = false;
		Log("PHYS: %7.2f rest %s after %.2fs, moved %.2fm (path %.2fm), peak %.2fm/s %.2frad/s, at %s%s\n",
			mfTime, sName.c_str(), mfTime - aTrack.mfWakeTime,
			cMath::Vector3Dist(aTrack.mvWakePos, vPos), aTrack.mfPath,
			aTrack.mfMaxSpeed, aTrack.mfMaxAngSpeed, VecStr(vPos).c_str(),
			JointInfo(apBody).c_str());
	}

	if(!aTrack.mbAwake) return;

	aTrack.mfPath += cMath::Vector3Dist(aTrack.mvLastPos, vPos);
	aTrack.mvLastPos = vPos;
	float fSpeed = apBody->GetLinearVelocity().Length();
	float fAngSpeed = apBody->GetAngularVelocity().Length();
	if(fSpeed > aTrack.mfMaxSpeed) aTrack.mfMaxSpeed = fSpeed;
	if(fAngSpeed > aTrack.mfMaxAngSpeed) aTrack.mfMaxAngSpeed = fAngSpeed;

	// Bodies that jitter instead of settling never send a rest line.
	if(mfTime >= aTrack.mfNextAwakeLog)
	{
		aTrack.mfNextAwakeLog = mfTime + kfAwakeLogInterval;
		Log("PHYS: %7.2f awake %s for %.0fs, now %.3fm/s %.3frad/s at %s (sleep thresholds %.3f %.3f, %d steps)%s\n",
			mfTime, sName.c_str(), mfTime - aTrack.mfWakeTime, fSpeed, fAngSpeed,
			VecStr(vPos).c_str(), apBody->GetAutoDisableLinearThreshold(),
			apBody->GetAutoDisableAngularThreshold(), apBody->GetAutoDisableNumSteps(),
			JointInfo(apBody).c_str());
	}
}

//-----------------------------------------------------------------------

void cPhysicsLog::UpdatePlayer(float afTimeStep)
{
	cPlayer *pPlayer = mpInit->mpPlayer;
	iCharacterBody *pCharBody = pPlayer->GetCharacterBody();

	// Falls: fall damage is decided by the speed at landing.
	cVector3f vVel = pCharBody->GetForceVelocity();
	bool bOnGround = pCharBody->IsOnGround();
	if(!bOnGround)
	{
		mfPlayerAirTime += afTimeStep;
		if(-vVel.y > mfPlayerFallSpeed) mfPlayerFallSpeed = -vVel.y;
	}
	else if(!mbPlayerOnGround)
	{
		if(mfPlayerFallSpeed > 3.0f || mfPlayerAirTime > 0.25f)
		{
			Log("PHYS: %7.2f player landed at %s after %.2fs in the air, fall speed %.2fm/s\n",
				mfTime, VecStr(pCharBody->GetPosition()).c_str(), mfPlayerAirTime, mfPlayerFallSpeed);
		}
		mfPlayerAirTime = 0;
		mfPlayerFallSpeed = 0;
	}
	mbPlayerOnGround = bOnGround;

	float fHealth = pPlayer->GetHealth();
	if(mfPlayerHealth >= 0 && fHealth < mfPlayerHealth)
	{
		Log("PHYS: %7.2f player health %.0f -> %.0f at %s, velocity %s, %s\n", mfTime,
			mfPlayerHealth, fHealth, VecStr(pCharBody->GetPosition()).c_str(),
			VecStr(vVel).c_str(), bOnGround ? "on ground" : "in the air");
	}
	mfPlayerHealth = fHealth;

	// Speed one frame after release, so a throw impulse is included.
	if(mpReleasedBody)
	{
		if(m_mapBodies.find(mpReleasedBody) != m_mapBodies.end())
		{
			Log("PHYS: %7.2f released %s at %.2fm/s %.2frad/s\n", mfTime,
				mpReleasedBody->GetName().c_str(),
				mpReleasedBody->GetLinearVelocity().Length(),
				mpReleasedBody->GetAngularVelocity().Length());
		}
		mpReleasedBody = nullptr;
	}

	ePlayerState State = pPlayer->GetState();
	if(State == mPlayerState) return;

	if(mPlayerState != ePlayerState_LastEnum)
	{
		Log("PHYS: %7.2f player %s -> %s at %s\n", mfTime,
			gvPlayerStateNames[mPlayerState], gvPlayerStateNames[State],
			VecStr(pCharBody->GetPosition()).c_str());
	}
	mPlayerState = State;

	if(mpHeldBody)
	{
		mpReleasedBody = mpHeldBody;
		mpHeldBody = nullptr;
	}

	if(State == ePlayerState_Grab || State == ePlayerState_Move || State == ePlayerState_Push)
	{
		iPhysicsBody *pBody = pPlayer->GetPushBody();
		if(pBody && m_mapBodies.find(pBody) != m_mapBodies.end())
		{
			mpHeldBody = pBody;
			Log("PHYS: %7.2f holding %s (%.1fkg) at %s%s\n", mfTime, pBody->GetName().c_str(),
				pBody->GetMass(), VecStr(pBody->GetWorldPosition()).c_str(),
				JointInfo(pBody).c_str());
		}
	}
}

//-----------------------------------------------------------------------

tString cPhysicsLog::JointInfo(iPhysicsBody *apBody)
{
	std::map<iPhysicsBody*, iPhysicsJoint*>::iterator it = m_mapJoints.find(apBody);
	if(it == m_mapJoints.end()) return "";

	iPhysicsJoint *pJoint = it->second;
	char sBuff[128];
	switch(pJoint->GetType())
	{
	case ePhysicsJointType_Hinge:
	case ePhysicsJointType_Ball:
		snprintf(sBuff, sizeof(sBuff), ", joint %s %.1fdeg", pJoint->GetName().c_str(),
			cMath::ToDeg(pJoint->GetAngle()));
		break;
	default:
		snprintf(sBuff, sizeof(sBuff), ", joint %s %.3fm", pJoint->GetName().c_str(),
			pJoint->GetDistance());
		break;
	}
	return sBuff;
}
