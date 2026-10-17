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
#include "ui/GraphicsPresets.h"

#include "core/Init.h"
#include "core/LampLooks.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/EffectHandler.h"

#include <cmath>

//-----------------------------------------------------------------------

bool GetImprovedOption(cInit *apInit, eImprovedOption aOption)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();
	switch(aOption)
	{
	case eImprovedOption_Shading:		return pLowLevel->GetShading().mbImproved;
	case eImprovedOption_Flashlight:	return apInit->mpPlayer->GetFlashLight()->GetSettings().mbImproved;
	case eImprovedOption_Halos:			return pLowLevel->GetEffects().mbHalos;
	case eImprovedOption_Shafts:		return pLowLevel->GetEffects().mbShafts;
	case eImprovedOption_Particles:		return pLowLevel->GetEffects().mbParticles;
	default:							return apInit->mpLampLooks->GetSettings().mbImproved;
	}
}

void SetImprovedOption(cInit *apInit, eImprovedOption aOption, bool abX)
{
	iLowLevelGraphics *pLowLevel = apInit->mpGame->GetGraphics()->GetLowLevel();
	if(aOption == eImprovedOption_Shading)
	{
		cShadingSettings shading = pLowLevel->GetShading();
		shading.mbImproved = abX;
		pLowLevel->SetShading(shading);
	}
	else if(aOption == eImprovedOption_Flashlight)
	{
		cFlashlightSettings flashlight = apInit->mpPlayer->GetFlashLight()->GetSettings();
		flashlight.mbImproved = abX;
		apInit->mpPlayer->GetFlashLight()->SetSettings(flashlight);
	}
	else if(aOption == eImprovedOption_Halos || aOption == eImprovedOption_Shafts || aOption == eImprovedOption_Particles)
	{
		cEffectSettings effects = pLowLevel->GetEffects();
		if(aOption == eImprovedOption_Halos)		effects.mbHalos = abX;
		else if(aOption == eImprovedOption_Shafts)	effects.mbShafts = abX;
		else										effects.mbParticles = abX;
		pLowLevel->SetEffects(effects);
	}
	else
	{
		cLampSettings lamps = apInit->mpLampLooks->GetSettings();
		lamps.mbImproved = abX;
		apInit->mpLampLooks->SetSettings(lamps);
	}
}

//-----------------------------------------------------------------------

int GetShadowOption(cInit *apInit)
{
	cRenderer3D *pRenderer = apInit->mpGame->GetGraphics()->GetRenderer3D();
	eRendererShowShadows show = pRenderer->GetShowShadows();
	if(show == eRendererShowShadows_None || pRenderer->GetShadowType() == eShadowType_Volumes) return (int)show;
	return show == eRendererShowShadows_All ? 3 : 4;
}

void SetShadowOption(cInit *apInit, int alOption)
{
	cRenderer3D *pRenderer = apInit->mpGame->GetGraphics()->GetRenderer3D();
	pRenderer->SetShadowType(alOption >= 3 ? eShadowType_Maps : eShadowType_Volumes);
	if(alOption >= 3)	pRenderer->SetShowShadows(alOption == 3 ? eRendererShowShadows_All : eRendererShowShadows_Static);
	else				pRenderer->SetShowShadows((eRendererShowShadows)alOption);
}

//-----------------------------------------------------------------------

namespace {

	// Off is 0 for the options with qualities, else the quality + 1
	struct cPresetValues
	{
		bool mbImproved;		// the Lighting page and the player effects
		bool mbLinearLighting;
		int mlBloom;			// 0 off, else 1 + eBloomType
		int mlDepthOfField;		// 0 off, else 1 + eDepthOfFieldType
		int mlAmbientOcclusion;
		int mlSmaa;
		int mlShadows;			// GetShadowOption()
		int mlAnisotropy;		// 1 is trilinear alone
	};

	// Motion blur stays off and the render scale at 100% in all of them, as in
	// the original. Resolution, gamma, field of view, the noise filter, frame
	// limit and VSync are the player's own and not part of a preset.
	const cPresetValues gvPresets[eGraphicsPreset_Custom] = {
		// Improved High: Bokeh depth of field, high ambient occlusion and
		// edge smoothing, soft shadows
		{true,  true,  1 + eBloomType_Soft,     1 + eDepthOfFieldType_Bokeh,    1 + 2, 1 + 2, 3, 16},
		// Improved Medium: lower ambient occlusion and edge smoothing
		{true,  true,  1 + eBloomType_Soft,     1 + eDepthOfFieldType_Bokeh,    1 + 0, 1 + 1, 3, 8},
		// Improved Low: the improved looks with the original's shadows and
		// depth of field, without ambient occlusion
		{true,  true,  1 + eBloomType_Soft,     1 + eDepthOfFieldType_Original, 0,     1 + 0, 0, 4},
		// Original: as the 2007 game drew it
		{false, false, 1 + eBloomType_Original, 1 + eDepthOfFieldType_Original, 0,     0,     0, 1},
	};

	int ClampAnisotropy(cInit *apInit, int alX)
	{
		int lMax = cMath::Max(apInit->mpGame->GetGraphics()->GetLowLevel()->GetCaps(eGraphicCaps_MaxAnisotropicFiltering), 1);
		return cMath::Min(alX, lMax);
	}

	bool ImprovedMatches(cInit *apInit, bool abImproved)
	{
		for(int i=0; i<eImprovedOption_LastEnum; ++i)
			if(GetImprovedOption(apInit, (eImprovedOption)i) != abImproved) return false;
		return apInit->mpGame->GetGraphics()->GetLowLevel()->GetPlayerEffectSettings().mbImproved == abImproved;
	}
}

const wchar_t *GetGraphicsPresetName(eGraphicsPreset aPreset)
{
	static const wchar_t *vNames[eGraphicsPreset_LastEnum] = {
		L"Improved High", L"Improved Medium", L"Improved Low", L"Original", L"Custom"};
	return vNames[aPreset];
}

void ApplyGraphicsPreset(cInit *apInit, eGraphicsPreset aPreset)
{
	if(aPreset < 0 || aPreset >= eGraphicsPreset_Custom) return;
	const cPresetValues &values = gvPresets[aPreset];

	cGraphics *pGraphics = apInit->mpGame->GetGraphics();
	iLowLevelGraphics *pLowLevel = pGraphics->GetLowLevel();
	cRendererPostEffects *pPostEffects = pGraphics->GetRendererPostEffects();

	for(int i=0; i<eImprovedOption_LastEnum; ++i)
		SetImprovedOption(apInit, (eImprovedOption)i, values.mbImproved);
	cPlayerEffectSettings feedback = pLowLevel->GetPlayerEffectSettings();
	feedback.mbImproved = values.mbImproved;
	pLowLevel->SetPlayerEffectSettings(feedback);

	pGraphics->SetLinearLighting(values.mbLinearLighting);
	pGraphics->SetRenderScale(1.0f);
	pPostEffects->SetActive(true);
	pPostEffects->SetMotionBlurActive(false);

	pPostEffects->SetBloomActive(values.mlBloom > 0);
	if(values.mlBloom > 0) pPostEffects->SetBloomType((eBloomType)(values.mlBloom - 1));

	cEffect_DepthOfField *pDoF = apInit->mpEffectHandler->GetDepthOfField();
	pDoF->SetDisabled(values.mlDepthOfField == 0);
	if(values.mlDepthOfField > 0) pPostEffects->SetDepthOfFieldType((eDepthOfFieldType)(values.mlDepthOfField - 1));

	pPostEffects->SetAmbientOcclusionActive(values.mlAmbientOcclusion > 0);
	if(values.mlAmbientOcclusion > 0)
	{
		cAmbientOcclusionSettings occlusion = pPostEffects->GetAmbientOcclusionSettings();
		occlusion.SetQuality(values.mlAmbientOcclusion - 1);
		pPostEffects->SetAmbientOcclusionSettings(occlusion);
	}

	cSmaaSettings smaa = pLowLevel->GetSmaa();
	smaa.mbActive = values.mlSmaa > 0;
	if(values.mlSmaa > 0) smaa.SetQuality(values.mlSmaa - 1);
	pLowLevel->SetSmaa(smaa);

	SetShadowOption(apInit, values.mlShadows);

	cMaterialManager *pMaterialManager = apInit->mpGame->GetResources()->GetMaterialManager();
	pMaterialManager->SetTextureFilter(eTextureFilter_Trilinear);
	pMaterialManager->SetTextureAnisotropy((float)ClampAnisotropy(apInit, values.mlAnisotropy));
}

eGraphicsPreset GetGraphicsPreset(cInit *apInit)
{
	cGraphics *pGraphics = apInit->mpGame->GetGraphics();
	iLowLevelGraphics *pLowLevel = pGraphics->GetLowLevel();
	cRendererPostEffects *pPostEffects = pGraphics->GetRendererPostEffects();
	cMaterialManager *pMaterialManager = apInit->mpGame->GetResources()->GetMaterialManager();

	int lBloom = pPostEffects->GetBloomActive() ? 1 + (int)pPostEffects->GetBloomType() : 0;
	int lDoF = apInit->mpEffectHandler->GetDepthOfField()->IsDisabled() ? 0 : 1 + (int)pPostEffects->GetDepthOfFieldType();
	// A quality set by hand in the F2 panel is none of the presets'
	int lAO = pPostEffects->GetAmbientOcclusionActive() ? 1 + pPostEffects->GetAmbientOcclusionSettings().GetQuality() : 0;
	int lSmaa = pLowLevel->GetSmaa().mbActive ? 1 + pLowLevel->GetSmaa().GetQuality() : 0;
	int lAnisotropy = cMath::Max((int)pMaterialManager->GetTextureAnisotropy(), 1);
	bool bTrilinear = pMaterialManager->GetTextureFilter() == eTextureFilter_Trilinear;

	if(	pPostEffects->GetActive()==false || pPostEffects->GetMotionBlurActive() ||
		std::fabs(pGraphics->GetRenderScale() - 1.0f) > 0.01f || !bTrilinear)
	{
		return eGraphicsPreset_Custom;
	}

	for(int i=0; i<eGraphicsPreset_Custom; ++i)
	{
		const cPresetValues &values = gvPresets[i];
		if(	ImprovedMatches(apInit, values.mbImproved) &&
			pGraphics->GetLinearLighting() == values.mbLinearLighting &&
			lBloom == values.mlBloom && lDoF == values.mlDepthOfField &&
			lAO == values.mlAmbientOcclusion && lSmaa == values.mlSmaa &&
			GetShadowOption(apInit) == values.mlShadows &&
			lAnisotropy == ClampAnisotropy(apInit, values.mlAnisotropy))
		{
			return (eGraphicsPreset)i;
		}
	}
	return eGraphicsPreset_Custom;
}
