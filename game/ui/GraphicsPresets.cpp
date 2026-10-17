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
#include "core/LampAppearanceController.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/EffectHandler.h"

#include <cmath>

//-----------------------------------------------------------------------

namespace {
	struct cPresetValues
	{
		bool mbDepthOfField;
		int mlAmbientOcclusion; // 0 off, otherwise quality + 1
		int mlSmaa; // 0 off, otherwise quality + 1
		int mlAnisotropy;
	};

	const cPresetValues gvPresets[eGraphicsPreset_Custom] = {
		{true,  3, 3, 16},
		{true,  1, 2, 8},
		{false, 0, 1, 4},
	};

	int ClampAnisotropy(cInit *apInit, int alAnisotropy)
	{
		int lMax = cMath::Max(apInit->mpGame->GetGraphics()->GetLowLevel()->GetCaps(eGraphicCaps_MaxAnisotropicFiltering), 1);
		return cMath::Min(alAnisotropy, lMax);
	}
}

const wchar_t *GetGraphicsPresetName(eGraphicsPreset aPreset)
{
	static const wchar_t *vNames[eGraphicsPreset_LastEnum] = {L"High", L"Medium", L"Low", L"Custom"};
	return vNames[aPreset];
}

void ApplyGraphicsPreset(cInit *apInit, eGraphicsPreset aPreset)
{
	if(aPreset < 0 || aPreset >= eGraphicsPreset_Custom) return;
	const cPresetValues &values = gvPresets[aPreset];
	cGraphics *pGraphics = apInit->mpGame->GetGraphics();
	iLowLevelGraphics *pLowLevel = pGraphics->GetLowLevel();
	cRendererPostEffects *pPostEffects = pGraphics->GetRendererPostEffects();

	pGraphics->SetRenderScale(1.0f);
	pPostEffects->SetActive(true);
	pPostEffects->SetMotionBlurActive(false);
	pPostEffects->SetBloomActive(true);
	apInit->mpEffectHandler->GetDepthOfField()->SetDisabled(!values.mbDepthOfField);
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
	pGraphics->GetRenderer3D()->SetShowShadows(eRendererShowShadows_All);
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
	bool bDoF = !apInit->mpEffectHandler->GetDepthOfField()->IsDisabled();
	int lAO = pPostEffects->GetAmbientOcclusionActive() ? 1 + pPostEffects->GetAmbientOcclusionSettings().GetQuality() : 0;
	int lSmaa = pLowLevel->GetSmaa().mbActive ? 1 + pLowLevel->GetSmaa().GetQuality() : 0;
	int lAnisotropy = cMath::Max((int)pMaterialManager->GetTextureAnisotropy(), 1);

	if(!pPostEffects->GetActive() || !pPostEffects->GetBloomActive() || pPostEffects->GetMotionBlurActive() ||
		std::fabs(pGraphics->GetRenderScale() - 1.0f) > 0.01f ||
		pMaterialManager->GetTextureFilter() != eTextureFilter_Trilinear ||
		pGraphics->GetRenderer3D()->GetShowShadows() != eRendererShowShadows_All)
		return eGraphicsPreset_Custom;

	for(int i=0; i<eGraphicsPreset_Custom; ++i)
	{
		const cPresetValues &values = gvPresets[i];
		if(bDoF == values.mbDepthOfField && lAO == values.mlAmbientOcclusion &&
			lSmaa == values.mlSmaa && lAnisotropy == ClampAnisotropy(apInit, values.mlAnisotropy))
			return (eGraphicsPreset)i;
	}
	return eGraphicsPreset_Custom;
}
