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
#ifndef GAME_EFFECT_HANDLER_H
#define GAME_EFFECT_HANDLER_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;


//-----------------------------------------

class cEffect_Underwater
{
public:
	cEffect_Underwater(cInit *apInit, cGraphicsDrawer *apDrawer);
	~cEffect_Underwater();

	void Update(float afTimeStep);
	void OnDraw();
	void Reset();

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

	void SetColor(const cColor &aColor){mColor = aColor;}
	const cColor &GetColor() const { return mColor; }

private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	bool mbActive;

	cColor mColor;

	// Black Plague: the sounds of diving and of being under water
	cSoundHandler *mpSoundHandler;
	iSoundChannel *mpLoopChannel;
	float mfSoundCount;
};


//-----------------------------------------

class cEffect_ShakeScreen_Shake
{
public:
	float mfMaxSize;
	float mfSize;
	float mfTime;
	float mfFadeInTime;
	float mfMaxFadeInTime;
	float mfFadeOutTime;
	float mfMaxFadeOutTime;
};

class cEffect_ShakeScreen
{
public:
	cEffect_ShakeScreen(cInit *apInit);
	~cEffect_ShakeScreen();

	void Start(float afAmount, float afTime, float afFadeInTime,float afFadeOutTime);

	void Update(float afTimeStep);
	void Reset();

	cVector3f GetScreenAdd(){return mvAdd;}
private:
	cInit *mpInit;

	cVector3f mvAdd;

	iHapticForce *mpForce;
	
	std::list<cEffect_ShakeScreen_Shake> mlstShakes;
};

//-----------------------------------------
class cGameSaveArea;
class cEffect_SaveEffect
{
public:
	cEffect_SaveEffect(cInit *apInit, cGraphicsDrawer *apDrawer);
	~cEffect_SaveEffect();

	/**
	 * apSaveArea may be nullptr (Requiem: the effect at an area, abSave false: no game is saved)
	 */
	void NormalSave(const cVector3f &avPos, cGameSaveArea *apSaveArea, bool abSave=true);
	// Requiem names the save (its quicksave); Overture and Black Plague
	// always save as "auto"
	void AutoSave(const tWString &asName = _W("auto"));

	void Update(float afTimeStep);
	void OnDraw();
	void Reset();
	float GetVisualAmount() const { return mbActive ? mfFlashAlpha : 0.0f; }

private:
	void NormalSaveUpdate(float afTimeStep);
	void AutoSaveUpdate(float afTimeStep);

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpFlashGfx;

	cGameSaveArea *mpSaveArea;

	tWString msMessage;
	tWString msAutoSaveName;

	bool mbActive;
	int mlState;
	bool mbAutoSave;
	bool mbSave = true;
	
	cVector3f mvPosition;
	
	float mfFlashAlpha;
	
	float mfTime;
	
	float mfStartFov;
};

//-----------------------------------------

class cEffect_DepthOfField
{
public:
	cEffect_DepthOfField(cInit *apInit);
	~cEffect_DepthOfField();

	void SetDisabled(bool abX);
	bool IsDisabled(){ return mbDisabled;}
	
	void SetActive(bool abX, float afFadeTime);
	bool IsActive(){ return mbActive;}

	void SetUp(float afNearPlane, float afFocalPlane, float afFarPlane);

	void SetFocusBody(iPhysicsBody *apBody){mpFocusBody = apBody;}
	void FocusOnBody(iPhysicsBody *apBody);
	
	void Update(float afTimeStep);
	
	void Reset();
private:
	cInit *mpInit;
	cRendererPostEffects *mpPostEffects;

	iPhysicsBody *mpFocusBody;
	
	bool mbActive;
	bool mbDisabled;

	float mfFadeSpeed;
	
	float mfMaxBlur;
};
//-----------------------------------------

class cEffect_Flash
{
public:
	cEffect_Flash(cInit *apInit, cGraphicsDrawer *apDrawer);
	~cEffect_Flash();

	void Start(float afFadeIn, float afWhite, float afFadeOut);

	void Update(float afTimeStep);
	void OnDraw();
	void Reset();
	float GetVisualAmount() const { return mbActive ? mfAlpha : 0.0f; }

private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpWhiteGfx;

	bool mbActive;
	float mfAlpha;
	
	int mlStep;
	
	float mfCount;

	float mfFadeInSpeed;
	float mfWhiteSpeed;
	float mfFadeOutSpeed;
};

//-----------------------------------------

// Black Plague: a picture fades in over the view, stays and fades out
class cEffect_ImageFlash
{
public:
	cEffect_ImageFlash(cInit *apInit, cGraphicsDrawer *apDrawer);
	~cEffect_ImageFlash();

	/**
	 * abStartFlash, abEndFlash: with a white flash as the picture comes and before it goes
	 */
	void Start(float afFadeIn, float afStay, float afFadeOut, const tString &asFile,
				bool abStartFlash, bool abEndFlash);

	void Update(float afTimeStep);
	void OnDraw();
	void Reset();
private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpImageGfx;

	bool mbActive;
	float mfAlpha;

	int mlStep;

	float mfCount;
	bool mbEndFlash;

	float mfFadeInSpeed;
	float mfStaySpeed;
	float mfFadeOutSpeed;
};

//-----------------------------------------

// Black Plague: the view laid over itself, a little larger and swelling
class cEffect_DreamFilter
{
public:
	cEffect_DreamFilter(cInit *apInit);
	~cEffect_DreamFilter();

	/**
	 * afAlpha: how strong; afSpeed: how fast it gets there, per second (0 = at once)
	 */
	void Start(float afAlpha, float afSpeed);
	void Stop(float afSpeed);

	void Update(float afTimeStep);
	void OnPostSceneDraw();
	void Reset();
private:
	cInit *mpInit;

	float mfAlpha;
	float mfMaxAlpha;
	float mfAlphaAdd;

	float mfTime;

	iGpuProgram *mpVtxProgram;
	iGpuProgram *mpFragProgram;
};

//-----------------------------------------

// Black Plague: the gas mask on the player's face
class cEffect_GasMask
{
public:
	cEffect_GasMask(cInit *apInit);
	~cEffect_GasMask();

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

	void Update(float afTimeStep);
	void OnDraw();
	void Reset();
private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;
	cSoundHandler *mpSoundHandler;

	iSoundChannel *mpBreathChannel;
	cGfxObject *mpGfx;

	float mfAlpha;
	bool mbActive;
	float mfBreathCount;
};

//-----------------------------------------

class cSubTitle
{
public:
	tWString msMessage;
	float mfCount;
	float mfAlpha;
	bool mbActive;
};

typedef std::list<cSubTitle> tSubTitleList;
typedef tSubTitleList::iterator tSubTitleListIt;

class cEffect_SubTitle
{
public:
	cEffect_SubTitle(cInit *apInit, cGraphicsDrawer *apDrawer);
	~cEffect_SubTitle();

	void Add(const tWString& asMessage, float afTime, bool abRemovePrevious);

	void Update(float afTimeStep);
	void OnDraw();
	void Reset();

	/**
	 * Black Plague: the text is put on a black strip
	 */
	void SetBlackText(bool abX){ mbBlackText = abX;}
	bool GetBlackText(){ return mbBlackText;}
private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;

	// Black Plague: for text over something bright
	cGfxObject *mpBlackGfx;
	bool mbBlackText;
	
    tSubTitleList mlstSubTitles;	
};

//-----------------------------------------

class cEffect_WaveGravity
{
public:
	cEffect_WaveGravity(cInit *apInit);
	~cEffect_WaveGravity();

	void SetActive(bool abX);
	bool IsActive() const { return mbActive; }
	//alDir: 0=Xaxis 1=Zaxis
	void Setup(float afMaxAngle, float afSwingLength, float afGravitySize, int alDir);

	void Update(float afTimeStep);
	void Reset();

private:
	cInit *mpInit;

	bool mbActive;

	float mfMaxAngle;
	float mfSwingLength;
	float mfSize;
	int mlDir;

	float mfTime;
};

//-----------------------------------------

class cEffectHandler : public iUpdateable
{
public:
	cEffectHandler(cInit *apInit);
	~cEffectHandler() override;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;
	void OnDraw() override;
	void OnPostSceneDraw() override;

	cEffect_GasMask* GetGasMask(){ return mpGasMask;}
	cEffect_ImageFlash* GetImageFlash(){ return mpImageFlash;}
	cEffect_DreamFilter* GetDreamFilter(){ return mpDreamFilter;}

	cEffect_Flash* GetFlash(){ return mpFlash;}
	cEffect_WaveGravity* GetWaveGravity(){ return mpWaveGravity;}
	cEffect_SubTitle* GetSubTitle(){ return mpSubTitle;}
	cEffect_DepthOfField* GetDepthOfField(){ return mpDepthOfField;}
	cEffect_SaveEffect* GetSaveEffect(){ return mpSaveEffect;}
	cEffect_ShakeScreen* GetShakeScreen(){ return mpShakeScreen;}
	cEffect_Underwater* GetUnderwater(){ return mpUnderwater;}

private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cEffect_Flash *mpFlash;
	cEffect_WaveGravity *mpWaveGravity;
	cEffect_SubTitle *mpSubTitle;
	cEffect_DepthOfField *mpDepthOfField;
	cEffect_SaveEffect *mpSaveEffect;
	cEffect_ShakeScreen *mpShakeScreen;
	cEffect_Underwater *mpUnderwater;
	cEffect_ImageFlash *mpImageFlash;
	cEffect_DreamFilter *mpDreamFilter;
	cEffect_GasMask *mpGasMask;
};


#endif // GAME_EFFECT_HANDLER_H
