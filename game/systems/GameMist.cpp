// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
#include "systems/GameMist.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "entities/GameEntity.h"

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

cGameMist::cGameMist(cInit *apInit) : mpInit(apInit)
{
}

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

cGameMist::eKind cGameMist::KindOf(const tString &asDataName)
{
	tString sName = cString::ToLowerCase(asDataName);
	auto has = [&sName](const char *apText){ return sName.find(apText) != tString::npos; };
	// The ice cave's mist is called smoke; black steam is smoke from fires
	if(has("ice_cave_smoke")) return eKind_Mist;
	if(has("black") || has("smoke") || has("dust")) return eKind_None;
	if(has("steam") || has("gas")) return eKind_Steam;
	if(has("mist") || has("fog")) return eKind_Mist;
	return eKind_None;
}

//-----------------------------------------------------------------------

void cGameMist::GetMistVolumes(cCamera3D *apCamera, std::vector<cMistVolume> &avVolumes)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld != mpWorld)
	{
		m_mapPockets.clear();
		mpWorld = pWorld;
	}
	if(pWorld == nullptr) return;

	///////////////////////////////////////
	// A low layer over water
	tGameEntityIterator entIt = mpInit->mpMapHandler->GetGameEntityIterator();
	while(entIt.HasNext())
	{
		iGameEntity *pEntity = entIt.Next();
		if(pEntity->GetType() != eGameEntityType_LiquidArea || pEntity->IsActive()==false) continue;
		if(pEntity->GetBodyNum() == 0) continue;
		cBoundingVolume *pBV = pEntity->GetBody(0)->GetBV();
		cVector3f vMin = pBV->GetMin(), vMax = pBV->GetMax();
		float fWidth = cMath::Min(vMax.x - vMin.x, vMax.z - vMin.z);

		cMistVolume vol;
		vol.mvMin = cVector3f(vMin.x, vMax.y - 0.3f, vMin.z);
		vol.mvMax = cVector3f(vMax.x, vMax.y + 1.6f, vMax.z);
		vol.mfDensity = 0.12f;
		vol.mfFalloff = 0.45f;
		vol.mfSoftness = cMath::Clamp(fWidth * 0.25f, 0.3f, 2.0f);
		vol.mfNoise = 0.6f;
		vol.mColor = cColor(0.55f, 0.6f, 0.63f, 1);
		avVolumes.push_back(vol);
	}

	///////////////////////////////////////
	// Pockets where mist, fog and steam are let out
	std::set<cParticleSystem3D*> setLive;
	std::vector<cMistVolume> vPockets;
	cParticleSystem3DIterator psIt = pWorld->GetParticleSystemIterator();
	while(psIt.HasNext())
	{
		cParticleSystem3D *pPS = psIt.Next();
		eKind kind = KindOf(pPS->GetDataName());
		if(kind == eKind_None || pPS->IsDead()) continue;
		setLive.insert(pPS);

		// Where the particles are, with all the room they have taken so far
		bool bActive = false;
		cVector3f vMin = pPS->GetWorldPosition(), vMax = vMin;
		for(int i=0; i<pPS->GetEmitterNum(); ++i)
		{
			iParticleEmitter3D *pPE = pPS->GetEmitter(i);
			if(pPE->IsActive()) bActive = true;
			cBoundingVolume *pBV = pPE->GetBoundingVolume();
			cVector3f vA = pBV->GetMin(), vB = pBV->GetMax();
			if(vA.x > vB.x) continue;
			vMin = cVector3f(cMath::Min(vMin.x, vA.x), cMath::Min(vMin.y, vA.y), cMath::Min(vMin.z, vA.z));
			vMax = cVector3f(cMath::Max(vMax.x, vB.x), cMath::Max(vMax.y, vB.y), cMath::Max(vMax.z, vB.z));
		}

		auto it = m_mapPockets.find(pPS);
		if(it == m_mapPockets.end() || it->second.msName != pPS->GetName())
		{
			cPocket pocket;
			pocket.msName = pPS->GetName();
			pocket.mvMin = vMin;
			pocket.mvMax = vMax;
			it = m_mapPockets.insert_or_assign(pPS, pocket).first;
		}
		cPocket &pocket = it->second;
		pocket.mvMin = cVector3f(cMath::Min(pocket.mvMin.x, vMin.x), cMath::Min(pocket.mvMin.y, vMin.y), cMath::Min(pocket.mvMin.z, vMin.z));
		pocket.mvMax = cVector3f(cMath::Max(pocket.mvMax.x, vMax.x), cMath::Max(pocket.mvMax.y, vMax.y), cMath::Max(pocket.mvMax.z, vMax.z));
		if(bActive == false) continue;

		// As wide as the particles reach, round where they are let out:
		// mist lies on the floor or the water there and is thickest at
		// that height, however far the particles fall or rise
		bool bSteam = kind == eKind_Steam;
		float fMinHalf = bSteam ? 1.5f : 2.5f;
		cVector3f vSource = pPS->GetWorldPosition();
		cVector3f vCentre = (pocket.mvMin + pocket.mvMax) * 0.5f;
		vCentre = (vCentre + vSource) * 0.5f;
		cVector3f vHalf = (pocket.mvMax - pocket.mvMin) * 0.5f;
		float fHalfX = cMath::Clamp(vHalf.x + 1.0f, fMinHalf, 6.0f);
		float fHalfZ = cMath::Clamp(vHalf.z + 1.0f, fMinHalf, 6.0f);
		float fBottom = vSource.y - 0.3f;
		float fTop = fBottom + (bSteam ? 2.5f : 3.0f);

		cMistVolume vol;
		vol.mvMin = cVector3f(vCentre.x - fHalfX, fBottom, vCentre.z - fHalfZ);
		vol.mvMax = cVector3f(vCentre.x + fHalfX, fTop, vCentre.z + fHalfZ);
		vol.mfDensity = bSteam ? 0.04f : 0.08f;
		vol.mfFalloff = bSteam ? 1.5f : 1.2f;
		vol.mfSoftness = cMath::Min(fHalfX, fHalfZ) * 0.7f;
		vol.mfNoise = 0.5f;
		vol.mColor = bSteam ? cColor(0.7f, 0.7f, 0.7f, 1) : cColor(0.6f, 0.63f, 0.66f, 1);
		vPockets.push_back(vol);
	}

	// Pockets of the same kind that lie on the same floor and overlap are
	// one: a room full of mist is one box, not many that come and go as
	// the nearest few are drawn
	for(bool bMerged = true; bMerged;)
	{
		bMerged = false;
		for(size_t i=0; i<vPockets.size() && bMerged==false; ++i)
		for(size_t j=i+1; j<vPockets.size(); ++j)
		{
			cMistVolume &a = vPockets[i], &b = vPockets[j];
			if(a.mfDensity != b.mfDensity || std::fabs(a.mvMin.y - b.mvMin.y) > 1.0f) continue;
			float fOverX = cMath::Min(a.mvMax.x, b.mvMax.x) - cMath::Max(a.mvMin.x, b.mvMin.x);
			float fOverZ = cMath::Min(a.mvMax.z, b.mvMax.z) - cMath::Max(a.mvMin.z, b.mvMin.z);
			if(fOverX <= 0 || fOverZ <= 0) continue;
			float fSmaller = cMath::Min((a.mvMax.x - a.mvMin.x) * (a.mvMax.z - a.mvMin.z),
										(b.mvMax.x - b.mvMin.x) * (b.mvMax.z - b.mvMin.z));
			if(fOverX * fOverZ < fSmaller * 0.25f) continue;

			a.mvMin = cVector3f(cMath::Min(a.mvMin.x, b.mvMin.x), cMath::Min(a.mvMin.y, b.mvMin.y), cMath::Min(a.mvMin.z, b.mvMin.z));
			a.mvMax = cVector3f(cMath::Max(a.mvMax.x, b.mvMax.x), cMath::Max(a.mvMax.y, b.mvMax.y), cMath::Max(a.mvMax.z, b.mvMax.z));
			a.mfSoftness = cMath::Max(a.mfSoftness, b.mfSoftness);
			vPockets.erase(vPockets.begin() + j);
			bMerged = true;
			break;
		}
	}
	avVolumes.insert(avVolumes.end(), vPockets.begin(), vPockets.end());

	// Forget the systems that are gone
	for(auto it = m_mapPockets.begin(); it != m_mapPockets.end();)
	{
		if(setLive.count(it->first)) ++it;
		else it = m_mapPockets.erase(it);
	}
}

//-----------------------------------------------------------------------
