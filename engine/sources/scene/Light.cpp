// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/Light.h"

#include "system/LowLevelSystem.h"
#include "math/Math.h"

#include "scene/World3D.h"
#include "scene/SoundEntity.h"


namespace hpl {

	iLight::iLight() : mDiffuseColor(0), mSpecularColor(0),mfIntensity(1),
		mbCastShadows(false), mfFarAttenuation(0), mfNearAttenuation(0),
		mfSourceRadius(10), mbAffectMaterial(true)
	{
		mfFadeTime=0;

		mbFlickering = false;
		mbFlickerFade = false;
		mfFlickerOnFadeLength = 0;
		mfFlickerOffFadeLength = 0;

		mpWorld3D = nullptr;

		mfFlickerStateLength = 0;

		mfFadeTime =0;
	}

	void iLight::SetDiffuseColor(cColor aColor)
	{
		mDiffuseColor = aColor;

		OnSetDiffuse();
	}

	void iLight::UpdateLight(float afTimeStep)
	{
		// Fade
		if(mfFadeTime>0)
		{

			mfFarAttenuation += mfRadiusAdd*afTimeStep;
			mDiffuseColor.r += mColAdd.r*afTimeStep;
			mDiffuseColor.g += mColAdd.g*afTimeStep;
			mDiffuseColor.b += mColAdd.b*afTimeStep;
			mDiffuseColor.a += mColAdd.a*afTimeStep;
			SetDiffuseColor(mDiffuseColor);

			mfFadeTime-=afTimeStep;

			//Set the dest values.
			if(mfFadeTime<=0)
			{
				mfFadeTime =0;
				SetDiffuseColor(mDestCol);
				mfFarAttenuation = mfDestRadius;
			}
		}

		// Flickering
		if(mbFlickering && mfFadeTime<=0)
		{
			//On
			if(mbFlickerOn)
			{
				if(mfFlickerTime >= mfFlickerStateLength)
				{
					mbFlickerOn = false;
					if(!mbFlickerFade)
					{
						SetDiffuseColor(mFlickerOffColor);
						mfFarAttenuation = mfFlickerOffRadius;
					}
					else
					{
						FadeTo(mFlickerOffColor,mfFlickerOffRadius,mfFlickerOffFadeLength);
					}
					//Sound
					if(msFlickerOffSound!=""){
						cSoundEntity *pSound = mpWorld3D->CreateSoundEntity("FlickerOff",
							msFlickerOffSound,true);
						if(pSound) pSound->SetPosition(GetLightPosition());
					}

					OnFlickerOff();

					mfFlickerTime =0;
					mfFlickerStateLength = cMath::RandRectf(mfFlickerOffMinLength,mfFlickerOffMaxLength);
				}
			}
			//Off
			else
			{
				if(mfFlickerTime >= mfFlickerStateLength)
				{
					mbFlickerOn = true;
					if(!mbFlickerFade)
					{
						SetDiffuseColor(mFlickerOnColor);
						mfFarAttenuation = mfFlickerOnRadius;
					}
					else
					{
						FadeTo(mFlickerOnColor,mfFlickerOnRadius,mfFlickerOnFadeLength);
					}
					if(msFlickerOnSound!=""){
						cSoundEntity *pSound = mpWorld3D->CreateSoundEntity("FlickerOn",
															msFlickerOnSound,true);
						if(pSound) pSound->SetPosition(GetLightPosition());
					}

					OnFlickerOn();

					mfFlickerTime =0;
					mfFlickerStateLength = cMath::RandRectf(mfFlickerOnMinLength,mfFlickerOnMaxLength);
				}
			}

			mfFlickerTime += afTimeStep;
		}

	}

	void iLight::FadeTo(const cColor& aCol, float afRadius, float afTime)
	{
		if(afTime<=0) afTime = 0.0001f;

		mfFadeTime = afTime;

		mColAdd.r = (aCol.r - mDiffuseColor.r)/afTime;
		mColAdd.g = (aCol.g - mDiffuseColor.g)/afTime;
		mColAdd.b = (aCol.b - mDiffuseColor.b)/afTime;
		mColAdd.a = (aCol.a - mDiffuseColor.a)/afTime;

		mfRadiusAdd = (afRadius - mfFarAttenuation)/afTime;

		mfDestRadius = afRadius;
		mDestCol = aCol;
	}

	bool iLight::IsFading()
	{
		return mfFadeTime != 0;
	}

	void iLight::SetFlickerActive(bool abX)
	{
		mbFlickering = abX;
	}

	void iLight::SetFlicker(const cColor& aOffCol, float afOffRadius,
							float afOnMinLength, float afOnMaxLength,const tString &asOnSound,const tString &asOnPS,
							float afOffMinLength, float afOffMaxLength,const tString &asOffSound,const tString &asOffPS,
							bool abFade, float afOnFadeLength, float afOffFadeLength)
	{
		mFlickerOffColor = aOffCol;
		mfFlickerOffRadius = afOffRadius;

		mfFlickerOnMinLength = afOnMinLength;
		mfFlickerOnMaxLength = afOnMaxLength;
		msFlickerOnSound = asOnSound;
		msFlickerOnPS = asOnPS;

		mfFlickerOffMinLength = afOffMinLength;
		mfFlickerOffMaxLength = afOffMaxLength;
		msFlickerOffSound = asOffSound;
		msFlickerOffPS = asOffPS;

		mbFlickerFade = abFade;

		mfFlickerOnFadeLength = afOnFadeLength;
		mfFlickerOffFadeLength = afOffFadeLength;

		mFlickerOnColor = mDiffuseColor;
		mfFlickerOnRadius = mfFarAttenuation;

		mbFlickerOn = true;
		mfFlickerTime =0;

		mfFadeTime =0;

		mfFlickerStateLength = cMath::RandRectf(mfFlickerOnMinLength,mfFlickerOnMaxLength);
	}
}
