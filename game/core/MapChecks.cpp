/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/MapChecks.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "entities/GameEntity.h"
#include "entities/GameObject.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"

#include <SDL3/SDL.h>
#include <cstdarg>
#include <set>

// Frames the map runs untouched before bodies are compared with the start
static const int klSettleFrames = 300;
// Frames each way joints are pushed
static const int klJointFrames = 120;
// Frames the player stands at each start position
static const int klLinkFrames = 60;

//-----------------------------------------------------------------------

// Every body a ray hits, in the order Newton reports them
class cCheckRayCollector : public iPhysicsRayCallback
{
public:
	struct cHit { iPhysicsBody *mpBody; float mfDist; };
	std::vector<cHit> mvHits;

	bool OnIntersect(iPhysicsBody *apBody, cPhysicsRayParams *apParams) override
	{
		mvHits.push_back({apBody, apParams->mfDist});
		return true;
	}

	//Closest hit of world geometry (no entity, not a character)
	float WorldDist()
	{
		float fMin = 1e10f;
		for(const cHit &hit : mvHits)
			if(hit.mpBody->GetUserData() == nullptr && !hit.mpBody->IsCharacter() && hit.mfDist < fMin)
				fMin = hit.mfDist;
		return fMin;
	}
	float BodyDist(iGameEntity *apEntity)
	{
		float fMin = 1e10f;
		for(const cHit &hit : mvHits)
			if(hit.mpBody->GetUserData() == apEntity && hit.mfDist < fMin) fMin = hit.mfDist;
		return fMin;
	}
};

static tString VecStr(const cVector3f &avVec)
{
	char sBuff[64];
	snprintf(sBuff, sizeof(sBuff), "(%.2f %.2f %.2f)", avVec.x, avVec.y, avVec.z);
	return sBuff;
}

static float AxisAngle(cVector3f avA, cVector3f avB)
{
	float fDot = cMath::Vector3Dot(avA, avB) / std::max(avA.Length() * avB.Length(), 1e-6f);
	return cMath::ToDeg(acosf(cMath::Clamp(fDot, -1.0f, 1.0f)));
}

//-----------------------------------------------------------------------

cMapChecks::cMapChecks(cInit *apInit)
{
	mpInit = apInit;
	mStep = eStep_Done;
	mlFrame = 0;
	mlFindings = 0;
	mlStartPos = 0;

	tString sSpec = SDL_getenv("OPENHPL_CHECKS") ? SDL_getenv("OPENHPL_CHECKS") : "";
	bool bAll = sSpec == "1" || sSpec == "all";
	mbSettle = bAll || sSpec.find("settle") != tString::npos;
	mbPick = bAll || sSpec.find("pick") != tString::npos;
	mbAreas = bAll || sSpec.find("areas") != tString::npos;
	mbJoints = bAll || sSpec.find("joints") != tString::npos;
	mbLinks = bAll || sSpec.find("links") != tString::npos;
	mbCalls = sSpec.find("calls") != tString::npos; //not in "all": it wrecks the map
	mlCall = 0;
}

bool cMapChecks::IsRequested()
{
	const char *pSpec = SDL_getenv("OPENHPL_CHECKS");
	return pSpec && pSpec[0] && tString(pSpec) != "0";
}

//-----------------------------------------------------------------------

void cMapChecks::Report(const char *asKind, const char *asFormat, ...)
{
	char sMess[1024];
	va_list ap;
	va_start(ap, asFormat);
	vsnprintf(sMess, sizeof(sMess), asFormat, ap);
	va_end(ap);

	Log("CHECK %s %s: %s\n", asKind, msMap.c_str(), sMess);
	mlFindings++;
}

//-----------------------------------------------------------------------

void cMapChecks::Start(const tString &asMap, const tString &asFile)
{
	msMap = asMap;
	msFile = asFile;
	mbReturning = false;
	mlFrame = 0;
	mStep = eStep_Settle;
	StartSettle();
}

//-----------------------------------------------------------------------

bool cMapChecks::Update(float afTimeStep)
{
	if(mStep == eStep_Done) return true;

	//A script changed the map (an area at a start position): the rest
	//would check another map
	//A called function changed the map: go back and call the next
	if(mStep == eStep_Calls)
	{
		bool bHere = mpInit->mpMapHandler->GetCurrentMapName() == msMap;
		if(!bHere && !mbReturning)
		{
			Log("CHECK: %s: the map changed to '%s', going back\n", msMap.c_str(), mpInit->mpMapHandler->GetCurrentMapName().c_str());
			mpInit->mpMapHandler->ChangeMap(msFile, "", "", "", 0, 0, "", "");
			mbReturning = true;
		}
		if(mbReturning)
		{
			if(bHere) mbReturning = false;
			return false;
		}
	}

	if(mpInit->mpMapHandler->GetCurrentMapName() != msMap)
	{
		Report("map", "the map changed to '%s' during the checks", mpInit->mpMapHandler->GetCurrentMapName().c_str());
		mStep = eStep_Done;
		return true;
	}

	switch(mStep)
	{
	case eStep_Settle:
		if(++mlFrame < (mbSettle || mbPick || mbAreas ? klSettleFrames : 1)) break;
		if(mbSettle) FinishSettle();
		if(mbPick) CheckPicks();
		if(mbAreas) CheckAreas();

		mStep = eStep_Joints;
		mlFrame = 0;
		if(mbJoints) StartJoints();
		break;

	case eStep_Joints:
		if(mbJoints && !UpdateJoints()) break;

		mStep = eStep_Links;
		mlFrame = 0;
		mvStartPos.clear();
		mlStartPos = 0;
		if(mbLinks)
		{
			tStartPosEntityList *pList = mpInit->mpGame->GetScene()->GetWorld3D()->GetStartPosEntityList();
			for(cStartPosEntity *pStart : *pList) mvStartPos.push_back(pStart->GetName());
		}
		break;

	case eStep_Links:
		if(!UpdateLinks()) break;
		mStep = eStep_Calls;
		mvCalls.clear();
		mlCall = 0;
		if(mbCalls && mpInit->mpGame->GetScene()->GetWorld3D()->GetScript())
			mpInit->mpGame->GetScene()->GetWorld3D()->GetScript()->GetTestCalls(mvCalls);
		break;

	case eStep_Calls:
		//One a frame, so that what they start can run
		if(mlCall >= mvCalls.size()){ mStep = eStep_Done; break; }
		if(mvCalls[mlCall] != "OnExit()")
		{
			Log("CHECK: %s: calling %s\n", msMap.c_str(), mvCalls[mlCall].c_str());
			mpInit->RunScriptCommand(mvCalls[mlCall]);
		}
		mlCall++;
		break;

	case eStep_Done:
		break;
	}

	return mStep == eStep_Done;
}

//-----------------------------------------------------------------------

static bool IsSettleTracked(iPhysicsBody *apBody)
{
	if(apBody->GetMass() <= 0 || apBody->IsCharacter() || apBody->IsRagDoll()) return false;
	if(!apBody->IsActive()) return false;
	iGameEntity *pEntity = (iGameEntity*)apBody->GetUserData();
	if(pEntity && pEntity->GetType() == eGameEntityType_Enemy) return false;
	return true;
}

void cMapChecks::StartSettle()
{
	m_mapBodies.clear();
	m_mapJoints.clear();

	iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	cPhysicsBodyIterator bodyIt = pWorld->GetBodyIterator();
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = bodyIt.Next();
		if(!IsSettleTracked(pBody)) continue;
		cBodyStart &start = m_mapBodies[pBody];
		start.msName = pBody->GetName();
		start.mvPos = pBody->GetWorldPosition();
		start.m_mtxRot = pBody->GetWorldMatrix().GetRotation();
	}

	cPhysicsJointIterator jointIt = pWorld->GetJointIterator();
	while(jointIt.HasNext())
	{
		iPhysicsJoint *pJoint = jointIt.Next();
		cJointStart &start = m_mapJoints[pJoint];
		start.msName = pJoint->GetName();
		start.mfAngle = pJoint->GetAngle();
		start.mfDist = pJoint->GetDistance();
	}
}

//-----------------------------------------------------------------------

void cMapChecks::FinishSettle()
{
	iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	cPhysicsBodyIterator bodyIt = pWorld->GetBodyIterator();
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = bodyIt.Next();
		cVector3f vPos = pBody->GetWorldPosition();
		if(std::isnan(vPos.x) || std::isnan(pBody->GetBV()->GetMin().x))
		{
			Report("settle", "body '%s' is NaN", pBody->GetName().c_str());
			continue;
		}
		if(!IsSettleTracked(pBody)) continue;

		auto it = m_mapBodies.find(pBody);
		if(it == m_mapBodies.end() || it->second.msName != pBody->GetName()) continue;
		const cBodyStart &start = it->second;

		//Moved by a script on purpose
		iGameEntity *pEntity = (iGameEntity*)pBody->GetUserData();
		if(pEntity && pEntity->IsMoving()) continue;
		if(mpInit->mpPlayer->GetPushBody() == pBody) continue;

		float fMoved = cMath::Vector3Dist(start.mvPos, vPos);
		cMatrixf mtxRot = pBody->GetWorldMatrix().GetRotation();
		float fTurned = std::max(AxisAngle(start.m_mtxRot.GetUp(), mtxRot.GetUp()),
								AxisAngle(start.m_mtxRot.GetForward(), mtxRot.GetForward()));
		float fSpeed = pBody->GetLinearVelocity().Length();
		float fAngSpeed = pBody->GetAngularVelocity().Length();
		bool bAwake = !pBody->IsSleeping() && !pBody->IsFrozen();
		bool bJointed = pBody->GetJointNum() > 0;
		tString sEntity = pEntity ? pEntity->GetName() : tString("");

		if(vPos.y < start.mvPos.y - 2.0f)
		{
			Report("settle", "body '%s' (entity '%s', %.1f kg) fell %.1f m: %s -> %s", pBody->GetName().c_str(),
				sEntity.c_str(), pBody->GetMass(), start.mvPos.y - vPos.y, VecStr(start.mvPos).c_str(), VecStr(vPos).c_str());
		}
		else if(fMoved > 0.15f || fTurned > 15.0f)
		{
			Report("settle", "body '%s' (entity '%s', %.1f kg%s) moved %.2f m, turned %.0f deg on its own: %s -> %s%s",
				pBody->GetName().c_str(), sEntity.c_str(), pBody->GetMass(), bJointed ? ", jointed" : "",
				fMoved, fTurned, VecStr(start.mvPos).c_str(), VecStr(vPos).c_str(), bAwake ? ", still awake" : "");
		}
		else if(bAwake && (fSpeed > 0.05f || fAngSpeed > 0.2f))
		{
			Report("settle", "body '%s' (entity '%s', %.1f kg%s) still moving after %.0f s: %.3f m/s %.3f rad/s",
				pBody->GetName().c_str(), sEntity.c_str(), pBody->GetMass(), bJointed ? ", jointed" : "",
				klSettleFrames / 60.0f, fSpeed, fAngSpeed);
		}
	}

	cPhysicsJointIterator jointIt = pWorld->GetJointIterator();
	while(jointIt.HasNext())
	{
		iPhysicsJoint *pJoint = jointIt.Next();
		auto it = m_mapJoints.find(pJoint);
		if(it == m_mapJoints.end() || it->second.msName != pJoint->GetName()) continue;
		iPhysicsBody *pChild = pJoint->GetChildBody();
		if(pChild == nullptr || !IsSettleTracked(pChild)) continue;
		iGameEntity *pEntity = (iGameEntity*)pChild->GetUserData();
		if(pEntity && pEntity->IsMoving()) continue;

		float fAngle = cMath::ToDeg(std::abs(pJoint->GetAngle() - it->second.mfAngle));
		float fDist = std::abs(pJoint->GetDistance() - it->second.mfDist);
		bool bSlider = pJoint->GetType() == ePhysicsJointType_Slider;
		if((bSlider && fDist > 0.05f) || (!bSlider && pJoint->GetType() == ePhysicsJointType_Hinge && fAngle > 10.0f))
		{
			Report("settle", "joint '%s' (body '%s', entity '%s') left its start: %s %.3f -> %.3f",
				pJoint->GetName().c_str(), pChild->GetName().c_str(), pEntity ? pEntity->GetName().c_str() : "",
				bSlider ? "distance" : "angle", bSlider ? it->second.mfDist : cMath::ToDeg(it->second.mfAngle),
				bSlider ? pJoint->GetDistance() : cMath::ToDeg(pJoint->GetAngle()));
		}
	}
}

//-----------------------------------------------------------------------

static bool IsUsable(iGameEntity *apEntity, iPhysicsBody *apBody)
{
	eGameEntityType type = apEntity->GetType();
	if(type == eGameEntityType_Enemy || type == eGameEntityType_StickArea ||
		type == eGameEntityType_SaveArea || type == eGameEntityType_LiquidArea ||
		type == eGameEntityType_VentArea) return false;

	bool bInteract = apEntity->GetCallbackScript(eGameEntityScriptType_PlayerInteract) || apEntity->GetHasInteraction();
	bool bExamine = apEntity->GetCallbackScript(eGameEntityScriptType_PlayerExamine) || apEntity->GetDescription() != _W("");

	if(type == eGameEntityType_Object)
	{
		cGameObject *pObject = static_cast<cGameObject*>(apEntity);
		if(pObject->GetInteractMode() == eObjectInteractMode_Static && !bExamine &&
			apEntity->GetCallbackScript(eGameEntityScriptType_PlayerInteract) == nullptr) return false;
	}

	if(bInteract && (apBody->GetMass() != 0 || type == eGameEntityType_Area ||
		apEntity->GetCallbackScript(eGameEntityScriptType_PlayerInteract))) return true;
	return bExamine;
}

void cMapChecks::CheckPicks()
{
	iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	tGameEntityIterator it = mpInit->mpMapHandler->GetGameEntityIterator();
	while(it.HasNext())
	{
		iGameEntity *pEntity = it.Next();
		if(!pEntity->IsActive()) continue;

		for(int i = 0; i < pEntity->GetBodyNum(); ++i)
		{
			iPhysicsBody *pBody = pEntity->GetBody(i);
			if(pBody == nullptr || !pBody->IsActive() || !IsUsable(pEntity, pBody)) continue;

			cVector3f vCenter = pBody->GetBV()->GetWorldCenter();
			float fRadius = cMath::Clamp(pBody->GetBV()->GetRadius() + 0.6f, 0.9f, 2.0f);

			int lOk = 0, lTried = 0;
			float fMinGap = 1e10f;
			std::map<tString, int> mapStolen;
			for(int lPitch = 0; lPitch < 2; ++lPitch)
			for(int lYaw = 0; lYaw < 8; ++lYaw)
			{
				float fYaw = lYaw * kPif / 4.0f;
				float fPitch = lPitch == 0 ? 0.0f : 0.6f;
				cVector3f vDir(cos(fPitch) * sin(fYaw), sin(fPitch), cos(fPitch) * cos(fYaw));
				cVector3f vEye = vCenter + vDir * fRadius;

				//From the body out to the eye: world geometry in between means
				//the player cannot stand there
				cCheckRayCollector out;
				pWorld->CastRay(&out, vCenter, vEye, true, false, true);
				if(out.WorldDist() < fRadius) continue;

				//And back: does the ray hit the body at all
				cCheckRayCollector in;
				cVector3f vEnd = vEye - vDir * (fRadius * 2.0f);
				pWorld->CastRay(&in, vEye, vEnd, true, false, true);
				float fBodyDist = in.BodyDist(pEntity);
				if(fBodyDist > 1e9f || in.WorldDist() < fBodyDist) continue;

				lTried++;
				cPlayerPickRayCallback pick;
				pick.mfMaxDistance = fRadius * 2.0f;
				pick.Clear();
				pWorld->CastRay(&pick, vEye, vEnd, true, false, true);
				pick.CalculateResults();

				if(pick.mpPickedBody && pick.mpPickedBody->GetUserData() == pEntity)
				{
					lOk++;
				}
				else
				{
					tString sBy = "nothing";
					if(pick.mpPickedBody)
						sBy = "'" + ((iGameEntity*)pick.mpPickedBody->GetUserData())->GetName() + "' body '" + pick.mpPickedBody->GetName() + "'";
					mapStolen[sBy]++;
					//How far in front of the body the one picked is: about 0 when
					//two bodies overlap, more when one is in a closed cabinet
					float fGap = pick.mpPickedBody ? fBodyDist - pick.mfPickedDist : 1e10f;
					if(fGap < fMinGap) fMinGap = fGap;
				}
			}

			if(lOk == 0 && lTried > 0)
			{
				tString sStolen;
				for(auto &stolen : mapStolen)
					sStolen += (sStolen.empty() ? "" : ", ") + stolen.first + " x" + cString::ToString(stolen.second);
				//Behind something else on every side (in a closed cabinet) is
				//usually what the map means; logged apart as "pick-behind"
				Report(fMinGap < 0.02f ? "pick" : "pick-behind",
					"'%s' (type %d, body '%s' at %s) cannot be picked from any of %d sides: picked %s, %.3f m in front",
					pEntity->GetName().c_str(), (int)pEntity->GetType(), pBody->GetName().c_str(),
					VecStr(vCenter).c_str(), lTried, sStolen.c_str(), fMinGap);
			}
		}
	}
}

//-----------------------------------------------------------------------

void cMapChecks::CheckAreas()
{
	iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	iPhysicsBody *pPlayerBody = mpInit->mpPlayer->GetCharacterBody()->GetBody();

	int lCount = 0;
	for(auto &callback : mpInit->mpPlayer->GetCollideScripts())
	{
		iGameEntity *pEntity = callback.second->mpEntity;
		if(pEntity == nullptr || !pEntity->IsActive() || pEntity->GetBodyNum() == 0) continue;
		//Callbacks with things (crates hanging on chains) have no middle to stand in
		if(pEntity->GetType() == eGameEntityType_Object || pEntity->GetType() == eGameEntityType_Item ||
			pEntity->GetType() == eGameEntityType_Enemy) continue;

		//The player stood in the middle of each of the entity's bodies
		bool bFound = false;
		for(int i = 0; i < pEntity->GetBodyNum() && !bFound; ++i)
		{
			iPhysicsBody *pBody = pEntity->GetBody(i);
			cMatrixf mtxPlayer = cMatrixf::Identity;
			mtxPlayer.SetTranslation(pBody->GetBV()->GetWorldCenter());
			cCollideData collideData;
			collideData.SetMaxSize(1);
			bFound = pWorld->CheckShapeCollision(pPlayerBody->GetShape(), mtxPlayer,
								pBody->GetShape(), pBody->GetLocalMatrix(), collideData, 1);
		}
		lCount++;
		if(!bFound)
		{
			iPhysicsBody *pBody = pEntity->GetBody(0);
			Report("areas", "the player standing in the middle of '%s' (type %d) does not collide with it (callback '%s'; "
				"body at %s, box %s..%s, shape type %d size %s)",
				pEntity->GetName().c_str(), (int)pEntity->GetType(),
				callback.second->msFuncName[eGameCollideScriptType_Enter].c_str(),
				VecStr(pBody->GetLocalMatrix().GetTranslation()).c_str(), VecStr(pBody->GetBV()->GetMin()).c_str(),
				VecStr(pBody->GetBV()->GetMax()).c_str(), (int)pBody->GetShape()->GetType(),
				VecStr(pBody->GetShape()->GetSize()).c_str());
		}
	}
	Log("CHECK: %s: tried %d player collide callbacks\n", msMap.c_str(), lCount);
}

//-----------------------------------------------------------------------

static float JointValue(iPhysicsJoint *apJoint)
{
	return apJoint->GetType() == ePhysicsJointType_Hinge ? cMath::ToDeg(apJoint->GetAngle()) : apJoint->GetDistance();
}

void cMapChecks::StartJoints()
{
	mvPushes.clear();
	cPhysicsJointIterator it = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->GetJointIterator();
	while(it.HasNext())
	{
		iPhysicsJoint *pJoint = it.Next();
		iPhysicsBody *pChild = pJoint->GetChildBody();
		if(pChild == nullptr || pChild->GetMass() <= 0 || pChild->IsCharacter() || pChild->IsRagDoll()) continue;
		if(!pChild->IsActive() || pJoint->IsBroken()) continue;

		iGameEntity *pEntity = (iGameEntity*)pChild->GetUserData();
		if(pEntity == nullptr || !pEntity->IsActive() || pEntity->IsMoving()) continue;
		eGameEntityType type = pEntity->GetType();
		if(type != eGameEntityType_SwingDoor && type != eGameEntityType_Object &&
			type != eGameEntityType_Lever && type != eGameEntityType_Wheel) continue;
		//Things the player cannot take hold of
		if(type == eGameEntityType_Object &&
			static_cast<cGameObject*>(pEntity)->GetInteractMode() == eObjectInteractMode_Static) continue;

		float fRange = 0;
		if(pJoint->GetType() == ePhysicsJointType_Hinge)
		{
			iPhysicsJointHinge *pHinge = static_cast<iPhysicsJointHinge*>(pJoint);
			fRange = cMath::ToDeg(pHinge->GetMaxAngle() - pHinge->GetMinAngle());
			if(fRange < 20) continue;
		}
		else if(pJoint->GetType() == ePhysicsJointType_Slider)
		{
			iPhysicsJointSlider *pSlider = static_cast<iPhysicsJointSlider*>(pJoint);
			fRange = pSlider->GetMaxDistance() - pSlider->GetMinDistance();
			if(fRange < 0.1f) continue;
		}
		else continue;

		cJointPush push;
		push.mpJoint = pJoint;
		push.msName = pJoint->GetName();
		push.mfStart = push.mfMin = push.mfMax = JointValue(pJoint);
		mvPushes.push_back(push);
	}
}

bool cMapChecks::UpdateJoints()
{
	//The joints may have been destroyed by a script meanwhile
	std::set<iPhysicsJoint*> setAlive;
	cPhysicsJointIterator it = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->GetJointIterator();
	while(it.HasNext()) setAlive.insert(it.Next());

	//One way, then the other way for twice as long
	float fSign = mlFrame < klJointFrames ? 1.0f : -1.0f;
	for(cJointPush &push : mvPushes)
	{
		iPhysicsJoint *pJoint = push.mpJoint;
		if(setAlive.count(pJoint) == 0 || pJoint->GetName() != push.msName) { push.mpJoint = nullptr; continue; }
		iPhysicsBody *pChild = pJoint->GetChildBody();

		cVector3f vPin = cMath::Vector3Normalize(pJoint->GetPinDir());
		cVector3f vCenter = pChild->GetWorldPosition();
		pChild->SetEnabled(true);
		if(pJoint->GetType() == ePhysicsJointType_Hinge)
		{
			cVector3f vArm = vCenter - pJoint->GetPivotPoint();
			vArm -= vPin * cMath::Vector3Dot(vArm, vPin);
			if(vArm.Length() < 0.05f)
			{
				//A wheel or valve turning about its middle: turn it at its rim
				float fRim = std::max(pChild->GetBV()->GetRadius(), 0.1f);
				pChild->AddTorque(vPin * (fSign * pChild->GetMass() * 15.0f * fRim));
			}
			else
			{
				cVector3f vDir = cMath::Vector3Normalize(cMath::Vector3Cross(vPin, vArm));
				pChild->AddForce(vDir * (fSign * pChild->GetMass() * 15.0f));
			}
		}
		else
		{
			pChild->AddForce(vPin * (fSign * pChild->GetMass() * 15.0f));
		}

		float fValue = JointValue(pJoint);
		push.mfMin = std::min(push.mfMin, fValue);
		push.mfMax = std::max(push.mfMax, fValue);
	}

	if(++mlFrame < klJointFrames * 3) return false;

	Log("CHECK: %s: pushed %d joints\n", msMap.c_str(), (int)mvPushes.size());

	for(cJointPush &push : mvPushes)
	{
		if(push.mpJoint == nullptr) continue;
		bool bHinge = push.mpJoint->GetType() == ePhysicsJointType_Hinge;
		float fMoved = push.mfMax - push.mfMin;
		if(fMoved < (bHinge ? 10.0f : 0.05f))
		{
			iGameEntity *pEntity = (iGameEntity*)push.mpJoint->GetChildBody()->GetUserData();
			Report("joints", "joint '%s' of '%s' (type %d, body '%s', %.1f kg) moved only %.3f %s when pushed both ways (from %.3f)",
				push.msName.c_str(), pEntity->GetName().c_str(), (int)pEntity->GetType(),
				push.mpJoint->GetChildBody()->GetName().c_str(), push.mpJoint->GetChildBody()->GetMass(),
				fMoved, bHinge ? "deg" : "m", push.mfStart);
		}
	}
	mvPushes.clear();
	return true;
}

//-----------------------------------------------------------------------

bool cMapChecks::UpdateLinks()
{
	if(mlStartPos >= mvStartPos.size()) return true;

	cPlayer *pPlayer = mpInit->mpPlayer;
	iCharacterBody *pCharBody = pPlayer->GetCharacterBody();

	if(mlFrame == 0)
	{
		pPlayer->SetStartPos(mvStartPos[mlStartPos]);
		mvStartFeet = pCharBody->GetFeetPosition();
	}

	if(++mlFrame < klLinkFrames) return false;

	cVector3f vFeet = pCharBody->GetFeetPosition();
	float fDrop = mvStartFeet.y - vFeet.y;
	float fSide = sqrtf((vFeet.x - mvStartFeet.x)*(vFeet.x - mvStartFeet.x) + (vFeet.z - mvStartFeet.z)*(vFeet.z - mvStartFeet.z));
	const char *sName = mvStartPos[mlStartPos].c_str();
	if(std::isnan(vFeet.x))
		Report("links", "start '%s' at %s: the player is NaN", sName, VecStr(mvStartFeet).c_str());
	else if(fDrop > 1.0f)
		Report("links", "start '%s' at %s: the player fell %.1f m to %s", sName, VecStr(mvStartFeet).c_str(), fDrop, VecStr(vFeet).c_str());
	else if(fDrop < -0.5f || fSide > 0.5f)
		Report("links", "start '%s' at %s: the player was pushed to %s", sName, VecStr(mvStartFeet).c_str(), VecStr(vFeet).c_str());
	else if(!pCharBody->IsOnGround() && pPlayer->GetState() != ePlayerState_Climb &&
		pPlayer->GetMoveState() != ePlayerMoveState_Swim)
		Report("links", "start '%s' at %s: the player is not on the ground", sName, VecStr(mvStartFeet).c_str());

	mlFrame = 0;
	mlStartPos++;
	return mlStartPos >= mvStartPos.size();
}
