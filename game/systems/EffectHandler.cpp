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
#include "systems/EffectHandler.h"

#include "core/Init.h"
#include "player/Player.h"
#include "ui/Inventory.h"
#include "player/PlayerHelper.h"
#include "systems/GameMessageHandler.h"
#include "core/SaveHandler.h"
#include "systems/RadioHandler.h"
#include "ui/GameGui.h"
#include "entities/GameSaveArea.h"
#include "core/GraphicsHelper.h"

//////////////////////////////////////////////////////////////////////////
// UNDERWATER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_Underwater::cEffect_Underwater(cInit *apInit, cGraphicsDrawer *apDrawer)
{
	mpInit = apInit;
	mpDrawer = apDrawer;

	mpWhiteGfx = mpDrawer->CreateGfxObject("effect_white.jpg","smoke2d");

	mpSoundHandler = mpInit->mpGame->GetSound()->GetSoundHandler();
	mpLoopChannel = nullptr;
	mfSoundCount = 0;

	Reset();
}
//-----------------------------------------------------------------------

cEffect_Underwater::~cEffect_Underwater()
{

}

//-----------------------------------------------------------------------

void cEffect_Underwater::SetActive(bool abX)
{
	if(mbActive == abX) return;

	mbActive = abX;

	if(cInit::IsOverture()) return;

	// Black Plague: a sound as the head goes under and comes up, at most once a second
	if(mfSoundCount <= 0)
	{
		if(mbActive)
		{
			mpSoundHandler->PlayGui("water_dive_down",false,1,cVector3f(0,0,1),eSoundDest_Gui);
		}
		else
		{
			mpSoundHandler->PlayGui("water_leave",false,1,cVector3f(0,0,1),eSoundDest_Gui);

			//Out of breath
			if(mpInit->mpPlayer->GetOxygen() < 50)
			{
				mpSoundHandler->PlayGui("water_gasp",false,1,cVector3f(0,0,1),eSoundDest_Gui);
				mpInit->mpPlayer->AddOxygen(20);
			}
		}
	}
	mfSoundCount = 1;
}

//-----------------------------------------------------------------------

void cEffect_Underwater::Update(float afTimeStep)
{
	if(cInit::IsOverture()) return;

	mfSoundCount -= afTimeStep;
	if(mfSoundCount < 0) mfSoundCount = 0;

	// Black Plague: the sound of being under water, faded in and out
	if(mbActive)
	{
		if(mpLoopChannel==nullptr || mpSoundHandler->IsValid(mpLoopChannel)==false)
		{
			mpLoopChannel = mpSoundHandler->PlayGui("water_under",true,1,cVector3f(0,0,1),eSoundDest_Gui);

			cSoundEntry *pEntry = mpLoopChannel ? mpSoundHandler->GetEntryFromSound(mpLoopChannel) : nullptr;
			if(pEntry)
			{
				pEntry->mfNormalVolume = 0;
				pEntry->mfNormalVolumeFadeDest = 1;
				pEntry->mfNormalVolumeFadeSpeed = 3;
			}
		}
	}
	else if(mpLoopChannel)
	{
		if(mpSoundHandler->IsValid(mpLoopChannel))
		{
			cSoundEntry *pEntry = mpSoundHandler->GetEntryFromSound(mpLoopChannel);
			if(pEntry)
			{
				pEntry->mfNormalVolumeFadeDest = 0;
				pEntry->mfNormalVolumeFadeSpeed = -3;
			}
			else
			{
				mpLoopChannel->Stop();
			}
		}
		mpLoopChannel = nullptr;
	}
}
//-----------------------------------------------------------------------

void cEffect_Underwater::OnDraw()
{
	if(mbActive==false) return;
	
	cColor invColor(1-mColor.r,1-mColor.g,1-mColor.b,0);
	mpDrawer->DrawGfxObject(mpWhiteGfx,mpInit->mpGraphicsHelper->GetScreenPos(),mpInit->mpGraphicsHelper->GetScreenSize(),invColor);
}
//-----------------------------------------------------------------------

void cEffect_Underwater::Reset()
{
	mbActive = false;
	mpLoopChannel = nullptr;
	mfSoundCount = 0;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// SHAKE SCREEN
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_ShakeScreen::cEffect_ShakeScreen(cInit *apInit)
{
	mpInit = apInit;

	if(mpInit->mbHasHaptics)
	{
		mpForce = mpInit->mpGame->GetHaptic()->GetLowLevel()->CreateSinusWaveForce(
																cVector3f(0,1,0),0.1f,5);
	}
	else
	{
		mpForce = nullptr;
	}
	
}

cEffect_ShakeScreen::~cEffect_ShakeScreen()
{

}

void cEffect_ShakeScreen::Start(float afAmount, float afTime,float afFadeInTime,float afFadeOutTime)
{
	cEffect_ShakeScreen_Shake shake;
	shake.mfSize = afAmount;
	shake.mfMaxSize = afAmount;
	shake.mfTime = afTime;
	shake.mfFadeInTime = afFadeInTime;
	shake.mfMaxFadeInTime = afFadeInTime;
	shake.mfFadeOutTime = afFadeOutTime;
	shake.mfMaxFadeOutTime = afFadeOutTime;

	mlstShakes.push_back(shake);
}

void cEffect_ShakeScreen::Update(float afTimeStep)
{
	float fLargest = 0;

	std::list<cEffect_ShakeScreen_Shake>::iterator it = mlstShakes.begin();
	for(; it != mlstShakes.end(); )
	{
		cEffect_ShakeScreen_Shake &shake = *it;

		if(shake.mfFadeInTime >0)
		{
			shake.mfFadeInTime -= afTimeStep; if(shake.mfFadeInTime<0) shake.mfFadeInTime=0;
			float fT = shake.mfFadeInTime / shake.mfMaxFadeInTime;
			shake.mfSize = (1-fT) * shake.mfMaxSize;
		}
		else if(shake.mfTime >0)
		{
			shake.mfTime -= afTimeStep; if(shake.mfTime<0)shake.mfTime=0;
			shake.mfSize = shake.mfMaxSize;
		}
		else
		{
			shake.mfFadeOutTime -= afTimeStep; if(shake.mfFadeOutTime<0) shake.mfFadeOutTime=0;
			float fT = shake.mfFadeOutTime / shake.mfMaxFadeOutTime;
			shake.mfSize =  fT * shake.mfMaxSize;
		}

		//Log("%f, %f, %f size: %f\n",shake.mfFadeInTime,shake.mfTime,shake.mfFadeOutTime,shake.mfSize);
				
		if(fLargest < shake.mfSize) fLargest = shake.mfSize;
		
		if(shake.mfTime <= 0 && shake.mfFadeOutTime <= 0 && shake.mfFadeInTime <= 0)
		{
			it = mlstShakes.erase(it);
		}
		else
		{
			++it;
		}
	}
	
	mvAdd.x = cMath::RandRectf(-fLargest,fLargest);
	mvAdd.y = cMath::RandRectf(-fLargest,fLargest);
	mvAdd.z = cMath::RandRectf(-fLargest,fLargest);

	if(mpForce)
	{
		if(mlstShakes.empty()==false)
		{
			mpForce->SetActive(true);
			mpForce->SetAmp(fLargest*12);
		}
		else
		{
			mpForce->SetActive(false);
		}
	}
}

void cEffect_ShakeScreen::Reset()
{
	mlstShakes.clear();
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// SAVE EFFECT
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_SaveEffect::cEffect_SaveEffect(cInit *apInit, cGraphicsDrawer *apDrawer)
{
	mpInit = apInit;
	mpDrawer = apDrawer;

	mpFlashGfx = mpDrawer->CreateGfxObject("effect_white.jpg","diffalpha2d");

	Reset();
}
cEffect_SaveEffect::~cEffect_SaveEffect()
{
	
}

void cEffect_SaveEffect::NormalSave(const cVector3f &avPos, cGameSaveArea *apSaveArea, bool abSave)
{
	mpSaveArea = apSaveArea;
	mbSave = abSave;

	msMessage = _W("");
	if(apSaveArea && apSaveArea->GetMessageCat() != "")
	{
		msMessage = kTranslate(apSaveArea->GetMessageCat(), apSaveArea->GetMessageEntry());
		if(apSaveArea->GetHasBeenUsed())
			mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("horror_save",false,1);
		else
			mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("horror_flashback1",false,1);
	}
	else
	{
		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("horror_save",false,1);
	}

	mvPosition = avPos;
	mbActive = true;
	mfTime =0;
	
	mfStartFov = mpInit->mpPlayer->GetCamera()->GetFOV();
	mfFov = mpInit->mpPlayer->GetCamera()->GetFOV();

	mpInit->mpPlayer->SetActive(false);
	mpInit->mpPlayer->GetLookAt()->SetActive(true);
	mpInit->mpPlayer->GetLookAt()->SetTarget(avPos,2.1f,4);

	mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetImageTrailActive(true);
	mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetImageTrailAmount(0.8f);

	mfFlashAlpha =0;
	mFlashColor = cColor(	216.0f / 255.0f, 
							85.0f  / 255.0f,
							5.0f   / 255.0f, 0);

	mlState = 0;

	mbAutoSave = false;
}

void cEffect_SaveEffect::AutoSave(const tWString &asName)
{
	msAutoSaveName = asName;
	mbActive = true;
	mfTime =0;

	//mpInit->mpPlayer->SetActive(false);

	mfFlashAlpha =0;
	mFlashColor = cColor(	216.0f / 255.0f, 
							85.0f  / 255.0f,
							5.0f   / 255.0f, 0);

	mlState = 0;

	mbAutoSave = true;
}

void cEffect_SaveEffect::Update(float afTimeStep)
{
	if(mbActive==false) return;

    mfTime += afTimeStep;

	if(mbAutoSave)
	{
		AutoSaveUpdate(afTimeStep);
	}
	else
	{
		NormalSaveUpdate(afTimeStep);
	}
}

void cEffect_SaveEffect::OnDraw()
{
	if(mbActive==false) return;

	mpDrawer->DrawGfxObject(mpFlashGfx,mpInit->mpGraphicsHelper->GetScreenPos(),mpInit->mpGraphicsHelper->GetScreenSize(),
							cColor(1,1) * mfFlashAlpha + 
							mFlashColor * (1- mfFlashAlpha) 
							);
}

void cEffect_SaveEffect::Reset()
{
	mbActive = false;
}

//-----------------------------------------------------------------------

void cEffect_SaveEffect::NormalSaveUpdate(float afTimeStep)
{
	switch(mlState)
	{
		///////////////////////
		//State 0
	case 0:
		{
			//Flash
			mfFlashAlpha += 0.5f *afTimeStep;
			if(mfFlashAlpha > 1.0f)mfFlashAlpha = 1.0f;

			//Fov
			mfFov -= (mfFov - 0.2f) * afTimeStep * 1.3f;
			mpInit->mpPlayer->GetCamera()->SetFOV(mfFov);

			if(mfTime > 3.0f)
			{
				mlState++;
				mpInit->mpPlayer->GetCamera()->SetFOV(mfStartFov);

				if(mbSave) mpInit->mpSaveHandler->AutoSave(_W("spot"),10);
			}
			break;
		}
		///////////////////////
		//State 1
	case 1:
		{
			if(msMessage != _W("") && mpSaveArea && mpSaveArea->GetHasBeenUsed()==false)
			{
				mpInit->mpGameMessageHandler->SetBlackText(true);
				mpInit->mpGameMessageHandler->Add(msMessage);
			}
			mlState++;
			break;
		}
		///////////////////////
		//State 2
	case 2:
		{
			if(mpInit->mpGameMessageHandler->HasMessage()==false)
			{
				mlState++;
			}
			break;
		}
		///////////////////////
		//State3
	case 3:
		{
			//Flash
			mfFlashAlpha -= 0.6f *afTimeStep;
			if(mfFlashAlpha < 0.0f)
			{
				mfFlashAlpha = 0.0f;
				mlState++;
			}

			break;
		}
		///////////////////////
		//State4
	case 4:
		{
			/////////////
			//Reset all
			mbActive = false;
			mpInit->mpPlayer->SetActive(true);
			mpInit->mpPlayer->GetLookAt()->SetActive(false);
			mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetImageTrailActive(false);

			/////////////
			//Display message
			int lLevel = 0;
			int lMaxRand = 3;
			if(mpInit->mpPlayer->mlStat_NumOfSaves>0) lLevel = 1;
			if(mpInit->mpPlayer->mlStat_NumOfSaves>5) lLevel = 2;
			else if(mpInit->mpPlayer->mlStat_NumOfSaves>20) lLevel = 3;

			if(lLevel==0) lMaxRand =1;

			tString sEntry = "AfterSave_Default";
			if(mpSaveArea && mpSaveArea->GetHasBeenUsed()==false)
			{
				mpInit->mpPlayer->mlStat_NumOfSaves++;
				int lNum = mpInit->mpPlayer->mlStat_NumOfSaves;
				if(lNum >10) lNum =10;
				if(lNum<10)
					sEntry = "AfterSave_0"+cString::ToString(lNum);
				else
					sEntry = "AfterSave_"+cString::ToString(lNum);

				mpSaveArea->SetHasBeenUsed(true);
			}
			
			mpInit->mpGameMessageHandler->SetBlackText(false);
			mpInit->mpGameMessageHandler->Add(kTranslate("Save",sEntry));
			
			break;
		}
	}

}

//-----------------------------------------------------------------------

void cEffect_SaveEffect::AutoSaveUpdate(float afTimeStep)
{
	switch(mlState)
	{
		///////////////////////
		//State 0
	case 0:
		{
			//Flash
			mfFlashAlpha += 1.2f *afTimeStep;
			if(mfFlashAlpha > 0.75f) 
			{
				mfFlashAlpha =  0.75f;
				mlState++;
				// Black Plague and Requiem keep ten auto saves
				mpInit->mpSaveHandler->AutoSave(msAutoSaveName,cInit::IsOverture() ? 5 : 10);
				mpInit->mpPlayer->SetActive(true);
			}
			break;
		}
		///////////////////////
		//State 1
	case 1:
		{
			//Flash
			mfFlashAlpha -= 0.8f *afTimeStep;
			if(mfFlashAlpha < 0.0f)
			{
				mfFlashAlpha = 0.0f;
				mlState++;
			}

			break;
		}
		///////////////////////
		//State2
	case 2:
		{
			/////////////
			//Reset all
			mbActive = false;
			break;
		}
	}
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// DEPTH OF FIELD
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_DepthOfField::cEffect_DepthOfField(cInit *apInit)
{
	mbDisabled = false;
	mpInit = apInit;
	mpPostEffects = apInit->mpGame->GetGraphics()->GetRendererPostEffects();

	Reset();
}

cEffect_DepthOfField::~cEffect_DepthOfField()
{

}

void cEffect_DepthOfField::Reset()
{
	mbActive = false;
	mfMaxBlur =0;
	mpFocusBody = nullptr;
}

void cEffect_DepthOfField::SetDisabled(bool abX)
{
	mbDisabled = abX;

	if(mbDisabled)
	{
		mpPostEffects->SetDepthOfFieldActive(false);
	}
	else if(mbActive)
	{
		mpPostEffects->SetDepthOfFieldActive(true);
	}

}

void cEffect_DepthOfField::SetActive(bool abX, float afFadeTime)
{
	mbActive = abX;
	if(mbDisabled) return;
	
	if(mbActive)
		mpPostEffects->SetDepthOfFieldActive(true);
	
	if(afFadeTime >0)
		mfFadeSpeed = 1 / afFadeTime;
	else
		mfFadeSpeed = 100000.0f;

	Update(1.0f / 60.0f);
}

void cEffect_DepthOfField::SetUp(float afNearPlane, float afFocalPlane, float afFarPlane)
{
	mpPostEffects->SetDepthOfFieldNearPlane(afNearPlane);
	mpPostEffects->SetDepthOfFieldFocalPlane(afFocalPlane);
	mpPostEffects->SetDepthOfFieldFarPlane(afFarPlane);
}

void cEffect_DepthOfField::Update(float afTimeStep)
{
	/////////////////////////////////
	// Update focus to body
	if(mpFocusBody && mfMaxBlur >0)
	{
		FocusOnBody(mpFocusBody);
	}
	
	///////////////////////////
	//Update max blur
	if(mbActive)
	{
		mfMaxBlur += afTimeStep * mfFadeSpeed;
		if(mfMaxBlur >1) mfMaxBlur =1;
	}
	else if(mfMaxBlur >0)
	{
		mfMaxBlur -= afTimeStep * mfFadeSpeed;
		if(mfMaxBlur <0) {
			mfMaxBlur =0;
			mpPostEffects->SetDepthOfFieldActive(false);
		}
	}

	mpPostEffects->SetDepthOfFieldMaxBlur(mfMaxBlur);
}


//-----------------------------------------------------------------------

void cEffect_DepthOfField::FocusOnBody(iPhysicsBody *apBody)
{
	cBoundingVolume *pBV = apBody->GetBV();
	cVector3f vCamPos = mpInit->mpPlayer->GetCamera()->GetPosition();

	//Focal plane
	float fFocalPlane =  cMath::Vector3Dist(pBV->GetWorldCenter(),vCamPos);

	//Near plane
	float fNearPlane =	fFocalPlane - (pBV->GetRadius() + 0.3f);
	if(fNearPlane<0) fNearPlane =0;

	//Far plane
	float fFarPlane = fFocalPlane + (pBV->GetRadius() + 0.3f);
	
	//float fDist = cMath::Vector3Dist(vCamPos,apBody->GetWorldPosition());
	//Log("Body: %s Dist: %f PickedDist: %f\n",apBody->GetName().c_str(), fDist,mpInit->mpPlayer->GetPickedDist());
	//Log("Setup near %f focal %f far %f Radii %f\n",fNearPlane,fFocalPlane,fFarPlane,pBV->GetRadius());
	
	SetUp(fNearPlane,fFocalPlane,fFarPlane);
}


//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// FLASH
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_Flash::cEffect_Flash(cInit *apInit, cGraphicsDrawer *apDrawer)
{
	mpInit = apInit;
	mpDrawer = apDrawer;

	mpWhiteGfx = mpDrawer->CreateGfxObject("effect_white.jpg","diffadditive2d");

	Reset();
}
cEffect_Flash::~cEffect_Flash()
{
	mpDrawer->DestroyGfxObject(mpWhiteGfx);	
}

//-----------------------------------------------------------------------

void cEffect_Flash::Start(float afFadeIn, float afWhite, float afFadeOut)
{
	mbActive = true;
	
	mlStep = 0;

	// Black Plague: starts from nothing, and a time of 0 is taken as "at once"
	if(cInit::IsOverture()==false)
	{
		mfAlpha = 0;
		if(afFadeIn == 0) afFadeIn = 0.000001f;
		if(afWhite == 0) afWhite = 0.000001f;
		if(afFadeOut == 0) afFadeOut = 0.000001f;
	}

	mfFadeInSpeed = 1 / afFadeIn;
	mfWhiteSpeed = 1 / afWhite;
	mfFadeOutSpeed = 1 / afFadeOut;
}

//-----------------------------------------------------------------------

void cEffect_Flash::Update(float afTimeStep)
{
	if(mbActive==false) return;

	if(mlStep ==0)
	{
		mfAlpha += mfFadeInSpeed * afTimeStep;
		if(mfAlpha >= 1.0f)
		{
			mfAlpha = 1.0f;
			mlStep=1;
			mfCount = 1;
		}
	}
	else if(mlStep ==1)
	{
		mfCount -= mfWhiteSpeed * afTimeStep;
		if(mfCount <= 0)
		{
			mlStep = 2;
		}
	}
	else if(mlStep ==2)
	{
		mfAlpha -= mfFadeOutSpeed * afTimeStep;
		if(mfAlpha <= 0.0f)
		{
			mbActive = false;
		}
	}
	
}

//-----------------------------------------------------------------------

void cEffect_Flash::OnDraw()
{
	if(mbActive==false) return;

	mpDrawer->DrawGfxObject(mpWhiteGfx,mpInit->mpGraphicsHelper->GetScreenPos(),mpInit->mpGraphicsHelper->GetScreenSize(),cColor(1,mfAlpha));
}

//-----------------------------------------------------------------------

void cEffect_Flash::Reset()
{
	mbActive = false;
	if(cInit::IsOverture()==false) mfAlpha = 0;
}

//////////////////////////////////////////////////////////////////////////
// IMAGE FLASH
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_ImageFlash::cEffect_ImageFlash(cInit *apInit, cGraphicsDrawer *apDrawer)
{
	mpInit = apInit;
	mpDrawer = apDrawer;

	mpImageGfx = nullptr;
	mfAlpha = 0;
	mbActive = false;
}

cEffect_ImageFlash::~cEffect_ImageFlash()
{
	if(mpImageGfx) mpDrawer->DestroyGfxObject(mpImageGfx);
}

//-----------------------------------------------------------------------

void cEffect_ImageFlash::Start(float afFadeIn, float afStay, float afFadeOut, const tString &asFile,
								bool abStartFlash, bool abEndFlash)
{
	if(mpImageGfx)
	{
		mpDrawer->DestroyGfxObject(mpImageGfx);
		mpImageGfx = nullptr;
	}

	mbActive = true;
	mlStep = 0;

	if(afFadeIn == 0) afFadeIn = 0.000001f;
	if(afStay == 0) afStay = 0.000001f;
	if(afFadeOut == 0) afFadeOut = 0.000001f;

	mfFadeInSpeed = 1 / afFadeIn;
	mfStaySpeed = 1 / afStay;
	mfFadeOutSpeed = 1 / afFadeOut;

	mpImageGfx = mpDrawer->CreateGfxObjectFromTexture(asFile, "diffalpha2d");
	if(mpImageGfx == nullptr)
	{
		Error("Couldn't load the picture '%s' for an image flash\n", asFile.c_str());
		mbActive = false;
		return;
	}

	mbEndFlash = abEndFlash;

	if(abStartFlash)
	{
		mpInit->mpEffectHandler->GetFlash()->Start(afFadeIn*0.5f, 0, afFadeIn*0.5f);
	}
}

//-----------------------------------------------------------------------

void cEffect_ImageFlash::Update(float afTimeStep)
{
	if(mbActive==false) return;

	if(mlStep ==0)
	{
		mfAlpha += mfFadeInSpeed * afTimeStep;
		if(mfAlpha >= 1.0f)
		{
			mfAlpha = 1.0f;
			mlStep=1;
			mfCount = 1;
		}
	}
	else if(mlStep ==1)
	{
		mfCount -= mfStaySpeed * afTimeStep;
		if(mfCount <= 0)
		{
			if(mbEndFlash)
			{
				float fTime = 1 / mfFadeInSpeed;
				mpInit->mpEffectHandler->GetFlash()->Start(fTime, 0, fTime);
			}
			mlStep = 2;
		}
	}
	else if(mlStep ==2)
	{
		mfAlpha -= mfFadeOutSpeed * afTimeStep;
		if(mfAlpha <= 0.0f)
		{
			mpDrawer->DestroyGfxObject(mpImageGfx);
			mpImageGfx = nullptr;
			mbActive = false;
		}
	}
}

//-----------------------------------------------------------------------

void cEffect_ImageFlash::OnDraw()
{
	if(mbActive==false) return;

	// The picture is 4:3 and fills the 800 x 600 of the original. In a wider
	// window it is made as wide as the window and loses its top and bottom.
	cVector2f vScreen = mpInit->mpGraphicsHelper->GetScreenSize();
	cVector2f vSize(800,600);
	if(vScreen.x > vSize.x) vSize = cVector2f(vScreen.x, vScreen.x * 0.75f);
	if(vScreen.y > vSize.y) vSize = cVector2f(vScreen.y / 0.75f, vScreen.y);

	mpDrawer->DrawGfxObject(mpImageGfx,cVector3f(400 - vSize.x*0.5f, 300 - vSize.y*0.5f,3.1f),vSize,cColor(1,mfAlpha));
}

//-----------------------------------------------------------------------

void cEffect_ImageFlash::Reset()
{
	if(mpImageGfx)
	{
		mpDrawer->DestroyGfxObject(mpImageGfx);
		mpImageGfx = nullptr;
	}

	mfAlpha = 0;
	mbActive = false;
}

//////////////////////////////////////////////////////////////////////////
// GAS MASK
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_GasMask::cEffect_GasMask(cInit *apInit)
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
	mpSoundHandler = mpInit->mpGame->GetSound()->GetSoundHandler();

	mpGfx = nullptr;
	mpBreathChannel = nullptr;
	mbActive = false;
	mfAlpha = 0;
	mfBreathCount = 0;
}

cEffect_GasMask::~cEffect_GasMask()
{
	if(mpGfx) mpDrawer->DestroyGfxObject(mpGfx);
}

//-----------------------------------------------------------------------

void cEffect_GasMask::SetActive(bool abX)
{
	if(mbActive == abX) return;

	mbActive = abX;

	if(mbActive)
	{
		mpSoundHandler->PlayGui("player_gasmask_on", false, 1, cVector3f(0,0,1), eSoundDest_Gui);

		// The world is heard through the mask
		mpSoundHandler->SetVolume(0.3f, 3.0f, eSoundDest_World);

		if(mpGfx == nullptr)
			mpGfx = mpDrawer->CreateGfxObjectFromTexture("player_gasmask.tga", "diffalpha2d");
	}
	else
	{
		mpSoundHandler->PlayGui("player_gasmask_off", false, 1, cVector3f(0,0,1), eSoundDest_Gui);

		mpSoundHandler->SetVolume(1.0f, 3.0f, eSoundDest_World);

		if(mpBreathChannel && mpSoundHandler->IsValid(mpBreathChannel))
		{
			cSoundEntry *pEntry = mpSoundHandler->GetEntryFromSound(mpBreathChannel);
			if(pEntry)
			{
				pEntry->mfNormalVolumeFadeDest = 0;
				pEntry->mfNormalVolumeFadeSpeed = -3;
			}
			else
			{
				mpBreathChannel->Stop();
			}
		}
		mpBreathChannel = nullptr;
	}
}

//-----------------------------------------------------------------------

void cEffect_GasMask::Update(float afTimeStep)
{
	if(mbActive)
	{
		// A breath now and then, faster when hurt
		mfBreathCount += afTimeStep;

		float fInterval = mpInit->mpPlayer->GetHealth() < 50 ? 1.6f : 2.2f;
		if(mfBreathCount > fInterval)
		{
			mfBreathCount = 0;
			mpBreathChannel = mpSoundHandler->PlayGui("player_gasmask_breath", false, 1, cVector3f(0,0,1), eSoundDest_Gui);
		}

		if(mfAlpha < 1)
		{
			mfAlpha += afTimeStep*2;
			if(mfAlpha > 1) mfAlpha = 1;
		}
	}
	else if(mfAlpha > 0)
	{
		mfAlpha -= afTimeStep*2;
		if(mfAlpha <= 0)
		{
			mfAlpha = 0;
			if(mpGfx)
			{
				mpDrawer->DestroyGfxObject(mpGfx);
				mpGfx = nullptr;
			}
		}
	}
}

//-----------------------------------------------------------------------

void cEffect_GasMask::OnDraw()
{
	if(mfAlpha <= 0 || mpGfx == nullptr) return;

	// The mask's glass is drawn for 4:3; a wider window gets it over its whole width
	mpDrawer->DrawGfxObject(mpGfx, mpInit->mpGraphicsHelper->GetScreenPos(-1),
							mpInit->mpGraphicsHelper->GetScreenSize(), cColor(1, mfAlpha));
}

//-----------------------------------------------------------------------

void cEffect_GasMask::Reset()
{
	mfBreathCount = 0;
	mpBreathChannel = nullptr;
	mfAlpha = 0;

	if(mbActive)
	{
		mbActive = false;
		mpSoundHandler->SetVolume(1.0f, 3.0f, eSoundDest_World);
	}

	if(mpGfx)
	{
		mpDrawer->DestroyGfxObject(mpGfx);
		mpGfx = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
// DREAM FILTER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_DreamFilter::cEffect_DreamFilter(cInit *apInit)
{
	mpInit = apInit;

	Reset();
}

cEffect_DreamFilter::~cEffect_DreamFilter()
{
}

//-----------------------------------------------------------------------

void cEffect_DreamFilter::Start(float afAlpha, float afSpeed)
{
	mfMaxAlpha = afAlpha;
	mfAlphaAdd = afSpeed;
	if(mfAlpha > afAlpha) mfAlphaAdd = -afSpeed;

	if(afSpeed == 0) mfAlpha = afAlpha;
}

void cEffect_DreamFilter::Stop(float afSpeed)
{
	mfMaxAlpha = 0;
	mfAlphaAdd = -afSpeed;

	if(afSpeed == 0) mfAlpha = 0;
}

//-----------------------------------------------------------------------

void cEffect_DreamFilter::Update(float afTimeStep)
{
	mfTime += afTimeStep;

	if(mfAlpha == mfMaxAlpha) return;

	mfAlpha += mfAlphaAdd * afTimeStep;

	if(mfAlphaAdd < 0)
	{
		if(mfAlpha < mfMaxAlpha) mfAlpha = mfMaxAlpha;
	}
	else
	{
		if(mfAlpha > mfMaxAlpha) mfAlpha = mfMaxAlpha;
	}
}

//-----------------------------------------------------------------------

void cEffect_DreamFilter::OnPostSceneDraw()
{
	cGraphics *pGraphics = mpInit->mpGame->GetGraphics();
	cRendererPostEffects *pPostEffects = pGraphics->GetRendererPostEffects();
	iLowLevelGraphics *pLowLevel = pGraphics->GetLowLevel();

	if(pPostEffects->GetActive()==false) return;
	if(mfAlpha <= 0) return;

	iTexture *pScreenTexture = pPostEffects->GetFreeScreenTexture();
	if(pScreenTexture == nullptr) return;

	///////////////////////////////
	// The view as it is now
	cVector2f vSize = pLowLevel->GetRenderSize();
	pLowLevel->CopyContextToTexure(pScreenTexture, 0, cVector2l((int)vSize.x, (int)vSize.y));

	///////////////////////////////
	// Added to itself, larger by an amount that goes up and down
	float fT = (sinf(mfTime*2) + 1) * 0.5f * 1.3f + 0.9f;

	cVector3f vPos(-5*fT, -3*fT, 0);
	cVector2f vQuad(800 + 10*fT, 600 + 6*fT);
	cColor col(mfAlpha, 1);

	tVertexVec vVtx;
	vVtx.push_back( cVertex(vPos,									cVector2f(0,vSize.y), col) );
	vVtx.push_back( cVertex(vPos + cVector3f(vQuad.x,0,0),			cVector2f(vSize.x,vSize.y), col) );
	vVtx.push_back( cVertex(vPos + cVector3f(vQuad.x,vQuad.y,0),	cVector2f(vSize.x,0), col) );
	vVtx.push_back( cVertex(vPos + cVector3f(0,vQuad.y,0),			cVector2f(0,0), col) );

	pLowLevel->SetDepthTestActive(false);
	pLowLevel->PushMatrix(eMatrix_ModelView);
	pLowLevel->SetIdentityMatrix(eMatrix_ModelView);
	pLowLevel->SetOrthoProjection(cVector2f(800,600),-1000,1000);

	for(int i=0;i<6;++i) pLowLevel->SetTexture(i,nullptr);

	//A copy of the scene is drawn as it is
	pLowLevel->SetLinearSource(true);

	pLowLevel->SetBlendActive(true);
	pLowLevel->SetBlendFunc(eBlendFunc_One, eBlendFunc_One);

	pLowLevel->SetTexture(0,pScreenTexture);
	pLowLevel->DrawQuad(vVtx);
	pLowLevel->SetTexture(0,nullptr);

	pLowLevel->SetBlendActive(false);
	pLowLevel->SetLinearSource(false);

	pLowLevel->PopMatrix(eMatrix_ModelView);
}

//-----------------------------------------------------------------------

void cEffect_DreamFilter::Reset()
{
	mfAlpha = 0;
	mfMaxAlpha = 0;
	mfAlphaAdd = 0;
	mfTime = 0;
}

//////////////////////////////////////////////////////////////////////////
// SUB TITLE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------
cEffect_SubTitle::cEffect_SubTitle(cInit *apInit, cGraphicsDrawer *apDrawer)
{
	mpInit = apInit;
	mpDrawer = apDrawer;

	mpFont = mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");

	mpBlackGfx = mpDrawer->CreateGfxObject("effect_black.bmp","diffalpha2d");
	mbBlackText = false;
}
cEffect_SubTitle::~cEffect_SubTitle()
{
	mpDrawer->DestroyGfxObject(mpBlackGfx);
}

void cEffect_SubTitle::Add(const tWString& asMessage, float afTime, bool abRemovePrevious)
{
	if(abRemovePrevious)
	{
		tSubTitleListIt it = mlstSubTitles.begin();
		for(; it != mlstSubTitles.end(); )
		{
			cSubTitle &subTitle = *it;

			if(subTitle.mbActive)
			{
				subTitle.mfCount =0;
				++it;
			}
			else
			{
				it = mlstSubTitles.erase(it);
			}
		}
	}
	
	
	cSubTitle subTitle;
	subTitle.mfAlpha = 0.0f;
	subTitle.mfCount = afTime;
	subTitle.msMessage = asMessage;
	subTitle.mbActive = false;

	mlstSubTitles.push_back(subTitle);
}

void cEffect_SubTitle::Update(float afTimeStep)
{
	bool bFoundFirst = false;

	tSubTitleListIt it = mlstSubTitles.begin();
	for(; it != mlstSubTitles.end(); )
	{
		cSubTitle &subTitle = *it;

        if(subTitle.mbActive)
		{
			if(subTitle.mfCount >0)
			{
				bFoundFirst = true;

				subTitle.mfCount -= afTimeStep;
				
				subTitle.mfAlpha += afTimeStep *0.9f;
				if(subTitle.mfAlpha > 1)subTitle.mfAlpha = 1;
			}
			else
			{
				subTitle.mfAlpha -= afTimeStep *0.9f;
				if(subTitle.mfAlpha <= 0)
				{
					it = mlstSubTitles.erase(it);
					continue;
				}
			}
		}
		else if(bFoundFirst == false)
		{
			subTitle.mbActive = true;
			bFoundFirst = true;
		}

		++it;
	}

}
void cEffect_SubTitle::OnDraw()
{
	if(mpInit->mbSubtitles==false) return;

	// Black Plague: a radio message out of hearing shows no text and leaves room for these
	if(cInit::IsOverture())
	{
		if(mpInit->mpRadioHandler->IsActive()) return;
	}
	else if(mpInit->mpRadioHandler->IsShowingText()) return;

	tSubTitleListIt it = mlstSubTitles.begin();
	for(; it != mlstSubTitles.end(); ++it)
	{
		cSubTitle &subTitle = *it;

		float fAlpha = subTitle.mfAlpha * (1 - mpInit->mpInventory->GetAlpha());

		if(subTitle.mbActive==false) continue;

		// Over an open computer terminal the text is drawn by the gui, which is on top
		if(mpInit->mpGameGuiHandler->HasOpenScreenGui())
		{
			mpInit->mpGameGuiHandler->DrawText(mpFont, subTitle.msMessage, fAlpha);
		}
		// Black Plague: each row on a black strip of its own length
		else if(mbBlackText)
		{
			tWStringVec vRows;
			mpFont->GetWordWrapRows(750, 16, 15, subTitle.msMessage, &vRows);

			float fY = 500;
			for(size_t i=0; i<vRows.size(); ++i)
			{
				float fLength = mpFont->GetLength(15, vRows[i].c_str());

				mpDrawer->DrawGfxObject(mpBlackGfx, cVector3f(25, fY, 46), cVector2f(fLength, 16), cColor(1, fAlpha));
				mpFont->Draw(cVector3f(25, fY, 47), 15, cColor(1, fAlpha), eFontAlign_Left, _W("%ls"), vRows[i].c_str());

				fY += 16;
			}
		}
		else
		{
			mpFont->DrawWordWrap(cVector3f(25, 500,47),750,16,15,cColor(1,fAlpha),
									eFontAlign_Left,subTitle.msMessage);
			mpFont->DrawWordWrap(cVector3f(25, 500,46),750,16,15,cColor(0,fAlpha),
									eFontAlign_Left,subTitle.msMessage,eFontEffect_Outline,1);
		}
	}
}
void cEffect_SubTitle::Reset()
{
	mlstSubTitles.clear();
}


//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// WAVE GRAVITY
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffect_WaveGravity::cEffect_WaveGravity(cInit *apInit)
{
	mpInit = apInit;
	
	Reset();
}
cEffect_WaveGravity::~cEffect_WaveGravity()
{
}

//-----------------------------------------------------------------------

void cEffect_WaveGravity::SetActive(bool abX)
{
	mbActive = abX;
}

void cEffect_WaveGravity::Setup(float afMaxAngle, float afSwingLength, float afGravitySize, int alDir)
{
	mfMaxAngle = afMaxAngle;
	mfSwingLength = afSwingLength;
	mfSize = afGravitySize;
	mlDir = alDir;
}

void cEffect_WaveGravity::Update(float afTimeStep)
{
	if(mbActive==false) return;

	iPhysicsWorld *pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	
	//////////////////////////////////
	//Set all bodies to not active
	
	cPhysicsBodyIterator it = pWorld->GetBodyIterator();
	while(it.HasNext())
	{
		iPhysicsBody *pBody = it.Next();
		
		//quick fix for oscillation
		if(pBody->GetJointNum()>0 && pBody->GetJoint(0)->GetLimitAutoSleep()) continue;

		if(	pBody->GetMass() != 0)
		{
			pBody->SetEnabled(true);
		}
	}

	
	/////////////////////////////////
	// Update gravity
	mfTime += (k2Pif / mfSwingLength) * afTimeStep;

	float afAngle = mfMaxAngle * sin(mfTime);
	
	cVector3f vDir(0,0,0);
	vDir.y = -cos(afAngle);
	if(mlDir==0)	vDir.x = sin(afAngle);
	else			vDir.z = sin(afAngle);

	vDir = vDir * mfSize;

	pWorld->SetGravity(vDir);
}

void cEffect_WaveGravity::Reset()
{
	mbActive = false;

	mfMaxAngle = 0;
	mfSwingLength = 1;
	mfSize = 9.8f;

	mfTime = 0;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffectHandler::cEffectHandler(cInit *apInit)  : iUpdateable("EffectHandler")
{
	mpInit = apInit;
	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	mpFlash = hplNew( cEffect_Flash,(mpInit, mpDrawer) );
	mpWaveGravity = hplNew( cEffect_WaveGravity,(mpInit) );
	mpSubTitle = hplNew( cEffect_SubTitle,(mpInit,mpDrawer) );
	mpDepthOfField = hplNew( cEffect_DepthOfField,(mpInit) );
	mpSaveEffect = hplNew( cEffect_SaveEffect,(mpInit,mpDrawer) );
	mpShakeScreen = hplNew( cEffect_ShakeScreen,(mpInit) );
	mpUnderwater = hplNew( cEffect_Underwater,(mpInit,mpDrawer) );
	mpImageFlash = hplNew( cEffect_ImageFlash,(mpInit,mpDrawer) );
	mpDreamFilter = hplNew( cEffect_DreamFilter,(mpInit) );
	mpGasMask = hplNew( cEffect_GasMask,(mpInit) );

	Reset();
}

//-----------------------------------------------------------------------

cEffectHandler::~cEffectHandler(void)
{
	hplDelete( mpFlash );
	hplDelete( mpWaveGravity );
	hplDelete( mpSubTitle );
	hplDelete( mpDepthOfField );
	hplDelete( mpSaveEffect );
	hplDelete( mpShakeScreen );
	hplDelete( mpUnderwater );
	hplDelete( mpImageFlash );
	hplDelete( mpDreamFilter );
	hplDelete( mpGasMask );
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cEffectHandler::OnStart()
{

}

//-----------------------------------------------------------------------

void cEffectHandler::Update(float afTimeStep)
{
	mpFlash->Update(afTimeStep);
	mpWaveGravity->Update(afTimeStep);
	mpSubTitle->Update(afTimeStep);
	mpDepthOfField->Update(afTimeStep);
	mpSaveEffect->Update(afTimeStep);
	mpShakeScreen->Update(afTimeStep);
	mpUnderwater->Update(afTimeStep);
	mpImageFlash->Update(afTimeStep);
	mpDreamFilter->Update(afTimeStep);
	mpGasMask->Update(afTimeStep);
}

//-----------------------------------------------------------------------

void cEffectHandler::Reset()
{
	mpFlash->Reset();
	mpWaveGravity->Reset();
	mpSubTitle->Reset();
	mpDepthOfField->Reset();
	mpSaveEffect->Reset();
	mpShakeScreen->Reset();
	mpUnderwater->Reset();
	mpImageFlash->Reset();
	mpDreamFilter->Reset();
	mpGasMask->Reset();
}

//-----------------------------------------------------------------------

void cEffectHandler::OnDraw()
{
	mpFlash->OnDraw();
	mpSubTitle->OnDraw();
	mpSaveEffect->OnDraw();
	mpUnderwater->OnDraw();
	mpImageFlash->OnDraw();
	mpGasMask->OnDraw();
}

//-----------------------------------------------------------------------

void cEffectHandler::OnPostSceneDraw()
{
	mpDreamFilter->OnPostSceneDraw();
}

//-----------------------------------------------------------------------
