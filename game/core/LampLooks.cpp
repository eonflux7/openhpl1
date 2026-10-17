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
#include "core/StdAfx.h"
#include "core/LampLooks.h"
#include "core/Init.h"
#include "core/MapHandler.h"
#include "entities/GameEntity.h"
#include "player/PlayerHands.h"

//////////////////////////////////////////////////////////////////////////
// KINDS OF LAMP
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

enum eLampKind
{
	eLampKind_None = -1,
	eLampKind_Flame,
	eLampKind_Flare,
	eLampKind_GlowStick,
	eLampKind_Bulb,
	eLampKind_Emergency,
	eLampKind_Tube,
	eLampKind_CeilingTube,
	eLampKind_TunnelLight,
	eLampKind_MapLight,
	eLampKind_LastEnum
};

class cLampKind
{
public:
	// What the light's own colour is multiplied by at full strength
	float mvTint[3];
	// How much the brightness wavers (a share of it), and how often in a second
	float mfFlicker;
	float mfFlickerRate;
	// How far the flame wanders, in metres
	float mfSway;
	// Half the tube's length in metres, along the entity's z axis, and how
	// far above the light its middle is
	float mfTubeHalfLength;
	float mfTubeUp;
};

static const cLampKind gvLampKinds[eLampKind_LastEnum] = {
	// Paraffin lamps: the data has them a pale yellow
	{{1.00f, 0.78f, 0.50f},	0.14f, 7.0f,	0.03f,	0, 0},
	// The flare, in the hand and thrown: red already, and restless
	{{1.00f, 1.00f, 1.00f},	0.30f, 16.0f,	0.04f,	0, 0},
	// The glow stick: steady, greener than the data's grey-green
	{{0.72f, 1.00f, 0.80f},	0, 0,			0,		0, 0},
	// Bulbs
	{{1.00f, 0.88f, 0.68f},	0, 0,			0,		0, 0},
	// Emergency lights: orange already
	{{1.00f, 0.82f, 0.62f},	0, 0,			0,		0, 0},
	// Fluorescent tubes, 1.9 m long: the data has them yellow-green
	{{0.90f, 1.00f, 1.10f},	0, 0,			0,		0.9f, 0},
	// The ceiling lights of the base: the light hangs 0.6 m below them
	{{0.94f, 1.00f, 1.08f},	0, 0,			0,		0.7f, 0.42f},
	// Tunnel lights: blue-white already
	{{0.92f, 1.00f, 1.06f},	0, 0,			0,		0, 0},
	// A light of the map: what it stands for is not known, its colour stays
	{{1.00f, 1.00f, 1.00f},	0, 0,			0,		0, 0},
};

// The kind is in the name of the entity's file. "flour" is the data's
// spelling.
static const struct { const char *msPart; eLampKind mKind; } gvLampNames[] = {
	{"paraffin",				eLampKind_Flame},
	{"flare",					eLampKind_Flare},
	{"flour",					eLampKind_Tube},
	{"base_entr_light_ceil",	eLampKind_CeilingTube},
	{"tunnellight",				eLampKind_TunnelLight},
	{"emergency_light",			eLampKind_Emergency},
	{"iron_mine_lamp",			eLampKind_Bulb},
	{"new_storage_lamp",		eLampKind_Bulb},
	{"desklamp",				eLampKind_Bulb},
};

static eLampKind GetLampKind(const tString& asFile)
{
	tString sFile = cString::ToLowerCase(asFile);
	for(const auto &name : gvLampNames)
	{
		if(sFile.find(name.msPart) != tString::npos) return name.mKind;
	}
	return eLampKind_None;
}

//-----------------------------------------------------------------------

// Smooth noise between -1 and 1 over time, the same in every run: a
// number for each whole step of afT, eased from one to the next.
static float StepValue(int alStep, unsigned int alSeed)
{
	unsigned int lX = (unsigned int)alStep * 747796405u + alSeed * 2891336453u + 12345u;
	lX = ((lX >> ((lX >> 28) + 4)) ^ lX) * 277803737u;
	lX ^= lX >> 22;
	return (float)(lX & 0xffffff) / (float)0xffffff * 2.0f - 1.0f;
}

static float Noise(double afT, unsigned int alSeed)
{
	double fWhole = floor(afT);
	float fPart = (float)(afT - fWhole);
	fPart = fPart * fPart * (3.0f - 2.0f * fPart);
	int lStep = (int)fWhole;
	return StepValue(lStep, alSeed) * (1.0f - fPart) + StepValue(lStep + 1, alSeed) * fPart;
}

// A flame: mostly slow, with quicker shivers on top.
static float FlameNoise(double afT, unsigned int alSeed)
{
	return	0.6f * Noise(afT, alSeed) + 0.28f * Noise(afT * 2.3, alSeed + 1) +
			0.12f * Noise(afT * 5.1, alSeed + 2);
}

static unsigned int NameSeed(const tString& asName)
{
	unsigned int lSeed = 2166136261u;
	for(char c : asName) lSeed = (lSeed ^ (unsigned char)c) * 16777619u;
	return lSeed;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cLampLooks::cLampLooks(cInit *apInit)  : iUpdateable("LampLooks")
{
	mpInit = apInit;

	mbStill = false;
	mfTime = 0;
	mlLampCount = 0;
}

//-----------------------------------------------------------------------

cLampLooks::~cLampLooks()
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cLampLooks::Reset()
{
	// The lights go with the map
	mlLampCount = 0;
}

//-----------------------------------------------------------------------

void cLampLooks::Update(float afTimeStep)
{
	mfTime += afTimeStep;
	mlLampCount = 0;

	if(mpInit->mpGame->GetScene()->GetWorld3D()==nullptr) return;

	///////////////////////////////////////
	// The lamps of the world, and the flare once thrown
	tGameEntityIterator entityIt = mpInit->mpMapHandler->GetGameEntityIterator();
	while(entityIt.HasNext())
	{
		iGameEntity *pEntity = entityIt.Next();
		if(pEntity->GetLightNum() == 0) continue;
		eLampKind kind = GetLampKind(pEntity->GetFileName());
		if(kind == eLampKind_None) continue;

		const cMatrixf *pEntityMtx = nullptr;
		if(pEntity->GetMeshEntity()) pEntityMtx = &pEntity->GetMeshEntity()->GetWorldMatrix();
		for(int i=0; i<pEntity->GetLightNum(); ++i)
			SetLook(pEntity->GetLight(i), kind, pEntityMtx, true);
	}

	///////////////////////////////////////
	// The lights that are part of the map
	cLight3DListIterator lightIt = mpInit->mpGame->GetScene()->GetWorld3D()->GetLightIterator();
	while(lightIt.HasNext())
	{
		iLight3D *pLight = lightIt.Next();
		if(pLight->IsPartOfMap()) SetMapLightLook(pLight);
	}

	///////////////////////////////////////
	// In the hand
	const struct { const char *msModel; eLampKind mKind; } vHandLights[] = {
		{"Glowstick", eLampKind_GlowStick}, {"Flare", eLampKind_Flare}};
	for(const auto &hand : vHandLights)
	{
		iHudModel *pModel = mpInit->mpPlayerHands->GetModel(hand.msModel);
		if(pModel==nullptr) continue;
		for(iLight3D *pLight : pModel->mvLights) SetLook(pLight, hand.mKind, nullptr, true);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

// Nothing in a map says what a light of its own stands for: a lamp of the
// static geometry, light from a window, a fill for a dark corner. They
// get the falloff by distance, which is what makes the lamps next to them
// look different, unless they reach so far that they are the sky or a
// whole cave, or have a falloff picture of their own. The furnace's
// lights are flames. Sparks and lights that scripts flash are left alone.
void cLampLooks::SetMapLightLook(iLight3D *apLight)
{
	const float kfLongestReach = 20.0f;

	tString sName = cString::ToLowerCase(apLight->GetName());
	bool bSkip = mSettings.mbMapLights==false ||
				sName.find("spark") != tString::npos || sName.find("broke") != tString::npos;
	if(bSkip)
	{
		apLight->SetLook(cLightLook());
		return;
	}

	iTexture *pFalloff = apLight->GetFalloffMap();
	bool bFalloff =	apLight->GetFarAttenuation() <= kfLongestReach &&
					(pFalloff==nullptr || pFalloff->GetName().find("core_falloff_linear") != tString::npos);

	SetLook(apLight, sName.find("furn") != tString::npos ? eLampKind_Flame : eLampKind_MapLight, nullptr, bFalloff);
}

//-----------------------------------------------------------------------

void cLampLooks::SetLook(iLight3D *apLight, int alKind, const cMatrixf *apEntityMtx, bool abFalloff)
{
	cLightLook look;
	mlLampCount++;

	const cLampKind &kind = gvLampKinds[alKind];

	///////////////////////////////////////
	// Colour
	float fColour = cMath::Clamp(mSettings.mfColour, 0.0f, 1.0f);
	look.mTint = cColor(1.0f + (kind.mvTint[0] - 1.0f) * fColour,
						1.0f + (kind.mvTint[1] - 1.0f) * fColour,
						1.0f + (kind.mvTint[2] - 1.0f) * fColour, 1.0f);

	///////////////////////////////////////
	// Falloff
	if(mSettings.mbFalloff && abFalloff)
	{
		look.mbDistanceFalloff = true;
		look.mfEvenAt = mSettings.mfEvenAt;
		look.mfNearBoost = mSettings.mfNearBoost;
	}

	///////////////////////////////////////
	// A flame: every light wavers and wanders on its own
	if(mbStill==false && (kind.mfFlicker > 0 || kind.mfSway > 0))
	{
		unsigned int lSeed = NameSeed(apLight->GetName());
		double fT = mfTime * kind.mfFlickerRate;

		float fFlicker = kind.mfFlicker * mSettings.mfFlicker;
		look.mfBrightness = cMath::Max(1.0f + fFlicker * FlameNoise(fT, lSeed), 0.05f);

		// Slower than the flicker, and less up and down than sideways
		float fSway = kind.mfSway * mSettings.mfSway;
		look.mvOffset = cVector3f(	FlameNoise(fT * 0.37, lSeed + 10),
									FlameNoise(fT * 0.37, lSeed + 20) * 0.5f,
									FlameNoise(fT * 0.37, lSeed + 30)) * fSway;
	}

	///////////////////////////////////////
	// A tube lies along its entity
	if(kind.mfTubeHalfLength > 0 && mSettings.mfTube > 0 && apEntityMtx)
	{
		cMatrixf mtxRot = apEntityMtx->GetRotation();
		cVector3f vAlong = cMath::MatrixMul(mtxRot, cVector3f(0,0,1));
		cVector3f vUp = cMath::MatrixMul(mtxRot, cVector3f(0,1,0));
		vAlong.Normalise(); vUp.Normalise();
		look.mvTubeAxis = vAlong * (kind.mfTubeHalfLength * mSettings.mfTube);
		look.mvTubeOffset = vUp * kind.mfTubeUp;
	}

	apLight->SetLook(look);
}

//-----------------------------------------------------------------------
