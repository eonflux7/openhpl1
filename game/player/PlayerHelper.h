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
#ifndef GAME_PLAYER_HELPER_H
#define GAME_PLAYER_HELPER_H

#include "core/StdAfx.h"

using namespace hpl;

#include "core/GameTypes.h"

class cInit;
class cPlayer;

//---------------------------------------------

class iPlayerWeaponCallback
{
public:
	virtual tString GetModelFile()=0;
	virtual cMatrixf GetOffsetMatrix()=0;

	virtual void OnAttackDown()=0;
	virtual void OnAttackUp()=0;
};


//---------------------------------------------

class cPlayerGroundRayCallback : public iPhysicsRayCallback
{
public:
	cPlayerGroundRayCallback();

	void Clear();
	void Reset();

	void OnWorldExit();

	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	bool mbOnGround;
	iPhysicsMaterial *mpMaterial;
	float mfMinDist;
};

//---------------------------------------------

class cPlayerPickRayCallback : public iPhysicsRayCallback
{
public:
	cPlayerPickRayCallback();

	void OnWorldExit();

	void Clear();

	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	void CalculateResults();

	iPhysicsBody *mpPickedBody;
	iPhysicsBody *mpPickedAreaBody;
	
	float mfMaxDistance;
	
	float mfPickedDist;
	float mfPickedAreaDist;
	
	eGameEntityType mLastEntityType;
	
	cVector3f mvPickedPos;
	cVector3f mvPickedAreaPos;

	float mfMinStaticDist;
};

//---------------------------------------------

class cPlayerHeadMove
{
public:
	cPlayerHeadMove(cPlayer *apPlayer);

	void Update(float afTimeStep);

	void Start();
	void Stop();

	float GetPos();

	bool IsActive(){ return mbActive;}
	float GetPosition(){ return mfPos;}
	float GetClimbPosition(){ return mfClimbPos;}
	float GetClimbCount(){ return mfClimbCount;}

	float mfMaxHeadMove;
	float mfMinHeadMove;
	float mfHeadMoveSpeed;
	float mfHeadMoveBackSpeed;

	float mfFootStepMul;
private:
	float mfAdd;
	float mfPos;
	bool mbActive;
	bool mbFootStepped;
	bool mbWasActive;
	
	float mfClimbCount;
	float mfClimbPos;

	cPlayer *mpPlayer;
};

//---------------------------------------------

class cPlayerHealth
{
public:
	cPlayerHealth(cInit *apInit);

	void Reset();

	void Update(float afTimeStep);

	void Draw();
	float GetVisualAmount() const { return cMath::Clamp(mfGfxGlobalAlpha * mfGfxAlpha, 0.0f, 1.0f); }
	float GetVisionAmount() const { return mfVisionAmount; }

private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	float mfTimeCount;
	float mfGfxAlpha;
	float mfGfxAlphaAdd;
	float mfGfxGlobalAlpha;
	float mfVisionAmount = 0;

	cSoundEntry *mpSoundEntry;
	float mfTerrorCheckCount;
	int mlTerrorLevel;

	cGfxObject *mpDamageGfx;

	// Black Plague: air under water. Short of it, a picture pulses on the
	// screen and the player gasps at each peak.
	void UpdateOxygen(float afTimeStep);
	float mfOxygenAlpha = 0;
	float mfOxygenPulseSpeed = 0;
	float mfOxygenAlphaAdd = 1;
	cGfxObject *mpLowOxygenGfx = nullptr;
};

//---------------------------------------------

class cPlayerEarRing
{
public:
	cPlayerEarRing(cInit *apInit,cPlayer *apPlayer);

	void Update(float afTimeStep);

	void Start(float afTime);
	void Stop(bool abStopDirectly);

	void Reset();

private:
	cInit *mpInit;
	cPlayer *mpPlayer;

	cSoundEntry *mpSoundEntry;

	float mfTimeCount;
	float mfDeafTime;
	bool mbDeaf;
};

//---------------------------------------------

class cPlayerLean : public iPhysicsRayCallback
{
public:
	cPlayerLean(cInit *apInit,cPlayer *apPlayer);

	void OnWorldLoad();

	void Update(float afTimeStep);

	void Lean(float afMul, float afTimeStep);

	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	void Reset();

	float mfDir;
	float mfMovement;
	float mfRotation;

private:
	cPlayer *mpPlayer;
	cInit *mpInit;

	iCollideShape *mpHeadShape;

	float mfMaxMovement;
	float mfMaxRotation;
	float mfMaxTime;

	float mfMoveSpeed;
	
	bool mbIntersect;

	bool mbPressed;
};

//---------------------------------------------

class cPlayerDamage
{
public:
	cPlayerDamage(cInit *apInit);
	~cPlayerDamage();

	void Start(float afSize, ePlayerDamageType aType);

	void Update(float afTimeStep);

	void Draw();
	void Reset();
	float GetVisualAmount() const { return mbActive ? mfAlpha : 0.0f; }
	float GetBloodSeed() const { return (float)mlBloodSeed; }
	ePlayerDamageType GetType() const { return mType; }

private:
	bool mbActive;

	float mfAlpha;
	float mfAlphaAdd;
	// Visual randomness must not consume the gameplay random sequence.
	unsigned int mlBloodSeed = 17;

	cVector2f mvHeadSwingAcc;
	cVector2f mvHeadSwingSpeed;

	float mfSize;

	cGfxObject *mvHitGfx[ePlayerDamageType_LastEnum];

	ePlayerDamageType mType;

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;
	cRendererPostEffects *mpPostEffects;
};

//---------------------------------------------

class cPlayerDeath
{
public:
	cPlayerDeath(cInit *apInit);
	~cPlayerDeath();

	void Reset();

	void Start();

	void Update(float afTimeStep);

	void Draw();

	bool IsActive(){ return mbActive;}
	float GetVisualAmount() const { return mbActive ? mfFadeAlpha : 0.0f; }
	float GetBlackAmount() const { return mbActive ? mfBlackAlpha : 0.0f; }

	float GetHeighAdd(){ return mfHeightAdd;}

private:
	bool mbActive;

	float mfHeightAdd;
	float mfRoll;

	float mfMinHeightAdd;

	cGfxObject *mpFadeGfx;
	cGfxObject *mpBlackGfx;

	bool mbStartFade;
	float mfFadeAlpha;
	float mfBlackAlpha;

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;
	cRendererPostEffects *mpPostEffects;
};

//---------------------------------------------

/**
 * How the flashlight looks (Milestone 5). With mbImproved off it is the
 * original: the light of the hand model with its picture and falloff ramp.
 */
class cFlashlightSettings
{
public:
	bool mbImproved = false;
	/// Scales the amount of light.
	float mfBrightness = 1.0f;
	/// 0 is the white of the original, 1 the yellow of a weak bulb.
	float mfWarmth = 0.5f;
	/// How far the beam goes, in times the original's 12 m.
	float mfReach = 1.0f;
	/// The distance, as a share of the original's reach, at which the beam is as bright as the original's.
	float mfEvenAt = 0.4f;
	/// How many times brighter than that it gets at most, nearer.
	float mfNearBoost = 2.5f;
	/// The hotspot's size, the brightness of the field around it and the beam's size (see cLightLook).
	float mfHotspot = 0.3f;
	float mfField = 0.45f;
	float mfWidth = 0.9f;
	/// The light that comes back from what the beam lands on: 0 none.
	float mfBounce = 0.2f;
};

class cPlayerFlashLight : iPhysicsRayCallback
{
public:
	cPlayerFlashLight(cInit *apInit);
	~cPlayerFlashLight() override;

	void Reset();
	
	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	void OnWorldLoad();
	
	void Update(float afTimeStep);

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

	void SetDisabled(bool abX);
	bool IsDisabled(){ return mbDisabled;}

	void OnPostSceneDraw();

	void SetSettings(const cFlashlightSettings& aSettings)
	{
		// The bounce follows a switch at once, not eased
		if(aSettings.mbImproved != mSettings.mbImproved) mbSnapBounce = true;
		mSettings = aSettings;
	}
	const cFlashlightSettings& GetSettings(){ return mSettings; }

private:
	void UpdateLook(float afTimeStep);

	cFlashlightSettings mSettings;
	//Where the bounced light is, and how much of it there is, eased
	cVector3f mvBouncePos;
	float mfBounceAmount;
	bool mbSnapBounce = false;
	//The spotlight's reach as the hand model has it, and as last set here
	float mfBaseReach;
	float mfSetReach;

	bool mbActive;

	bool mbDisabled;

	float mfAlpha;
	
	float mfFlickerTime;

	float mfRayCastTime;
	float mfClosestDist;
	iPhysicsBody *mpClosestBody;

	iTexture *mpTextureCache;

	cInit *mpInit;

	cVector3f mvStart;
	cVector3f mvEnd;
};

//---------------------------------------------

class cPlayerGlowStick
{
public:
	cPlayerGlowStick(cInit *apInit);
	~cPlayerGlowStick();

	void Reset();

	void OnWorldLoad();
	void Destroy();

	void Update(float afTimeStep);

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

private:
	bool mbActive;

	cInit *mpInit;
};

//---------------------------------------------

class iHudModel;
class cPlayerFlare
{
public:
	cPlayerFlare(cInit *apInit);
	~cPlayerFlare();

	void Reset();

	void OnWorldLoad();

	void Update(float afTimeStep);

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

	float GetTime(){ return mfTime;}
	void SetTime(float afX){ mfTime = afX;}

private:
	bool mbActive;

	float mfTime;

	iLight3D *mpLight;
	iHudModel *mpModel;

	float mfMaxLightRadius;
	float mfLightRadius;
	float mfLightPulse;
	float mfLightPulseAdd;
	
	cInit *mpInit;
};

//---------------------------------------------

// What the noise filter is made of.
enum eNoiseFilterType
{
	/// Tiles of noise images drawn over the screen: the original.
	eNoiseFilterType_Original,
	/// Grain made by the shader that finishes the 3D scene, under the HUD:
	/// noise, or film (eFilmGrain_Noise, eFilmGrain_Film).
	eNoiseFilterType_Grain,
	eNoiseFilterType_Film,
	eNoiseFilterType_LastEnum
};

class cPlayerNoiseFilter
{
public:
	cPlayerNoiseFilter(cInit *apInit);
	~cPlayerNoiseFilter();

	void Draw();
	
	void Update(float afTimeStep);

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

	void SetType(eNoiseFilterType aType);
	eNoiseFilterType GetType(){ return mType;}

private:
	void UpdateGrain();

	bool mbActive;
	eNoiseFilterType mType;

	float mfAlpha;
	float mfTimeCount;

	int mlAmount;

	cLight3DPoint *mpLight;
	cMeshEntity *mpMeshEntity;

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	std::vector<cGfxObject*> mvGfxNoise;
	tIntVec mvCurrentGfx;
};

//---------------------------------------------

class cPlayerFearFilter
{
public:
	cPlayerFearFilter(cInit *apInit);
	~cPlayerFearFilter();

	void Draw();

	void Update(float afTimeStep);

	void Reset();

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}
	float GetVisualAmount() const { return mfAlpha / mfMaxAlpha; }

private:
	bool mbActive;

	float mfAlpha;
	float mfMaxAlpha;
	
	cInit *mpInit;
};

//---------------------------------------------

class cPlayerLookAt
{
public:
	cPlayerLookAt(cPlayer *apPlayer);
	~cPlayerLookAt();

	void Update(float afTimeStep);

	void Reset();
	
	void SetTarget(const cVector3f &avTargetPos, float afSpeedMul, float afMaxSpeed);
	void SetZoomTarget(float afFovMul, float afSpeedMul, float afMaxSpeed);
	
	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

private:
	bool mbActive;
	cVector3f mvTargetPos;

	float mfSpeedMul;
	float mfMaxSpeed;

	float mfDestFovMul;
	float mfFovMul;
	float mfFovSpeedMul;
	float mfFovMaxSpeed;

	cPlayer *mpPlayer;
};

//---------------------------------------------

class cPlayerHidden : public iPhysicsRayCallback
{
public:
	cPlayerHidden(cInit *apInit);
	~cPlayerHidden() override;

	void Draw();

	void Update(float afTimeStep);

	void Reset();

	void OnWorldLoad();
	void OnWorldExit();

	bool InShadows();
	bool IsHidden(){ return mbHidden;}
	float GetAdaptationAmount() const { return mfAdaptation; }
	float GetPanicPulse() const;
	float GetFeedbackFOV(float afAdaptation, float afPanic, float afPulse) const;
	float GetPanicAmount() const { return cMath::Clamp((mfEnemyTooCloseCount - 1.2f) / (mfEnemyTooCloseMax - 1.2f), 0.0f, 1.0f); }
	void UnHide();

	bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

	bool HasLineOfSight(iLight3D *pLight,iPhysicsWorld *pPhysicsWorld);

private:
	void UpdateEnemyTooClose(float afTimeStep);


	cInit *mpInit;
	cRenderer3D *mpRenderer;
	cGraphicsDrawer *mpDrawer;
	cSoundHandler *mpSoundHandler;

	cGfxObject *mpInShadowGfx;

	iLight3D *mpLight;

	float mfInShadowAlpha;
	float mfAdaptation;
	float mfFeedbackTime;
	float mfInShadowPulse;
	float mfInShadowPulseAdd;

	float mfStartAspect;
	float mfHiddenAspect;
	float mfAspect;
	float mfAspectAdd;

	float mfStartFov;
	float mfHiddenFov;
	float mfFov;
	float mfFovAdd;

	float mfStartEffectOffset;
	float mfHiddenEffectOffset;
	float mfEffectOffset;
	float mfEffectOffsetAdd;

	float mfHiddenOnTime;
	float mfHiddenOffTime;

	float mfCheckEnemyCloseCount;
	float mfEnemyTooCloseCount;
	float mfEnemyTooCloseMax;
	bool mbEnemyTooClose;

	float mfCloseEffectFov;
	float mfCloseEffectFovAdd;
	float mfCloseEffectFovMax;
	float mfCloseEffectFovMin;

	float mbInShadows;
	
	float mfUpdateCount;

	float mfLight;
	float mfMaxLight;

	float mfHiddenCount;
	float mfHiddenTime;
	bool mbHidden;
	float mfHiddenOnCount;

	bool mbIntersected;

	iFontData *mpFont;
};


//---------------------------------------------

class cPlayerBodyCallback : public iCharacterBodyCallback
{
public:
	cPlayerBodyCallback(cPlayer *apPlayer);

	void OnHitGround(iCharacterBody *apCharBody,const cVector3f &avVel) override;
	void OnGravityCollide(iCharacterBody *apCharBody,iPhysicsBody *apCollideBody,
									cCollideData *apCollideData) override{}

private:
	cPlayer *mpPlayer;

	float mfFallSpeed_Min;
	float mfFallDamage_Min;
	tString msFallSound_Min;
	
	float mfFallSpeed_Med;
	float mfFallDamage_Med;
	tString msFallSound_Med;
	
	float mfFallSpeed_Max;
	float mfFallDamage_Max;
	tString msFallSound_Max;
};

//---------------------------------------------

#endif // GAME_PLAYER_HELPER_H
