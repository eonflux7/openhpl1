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
#ifndef GAME_GRAPHICS_PRESETS_H
#define GAME_GRAPHICS_PRESETS_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

// The Lighting page's options: each is the original's or the improved one,
// with its settings in the F2 panel.
enum eImprovedOption
{
	eImprovedOption_Shading,
	eImprovedOption_Flashlight,
	eImprovedOption_Lamps,
	eImprovedOption_Halos,
	eImprovedOption_Shafts,
	eImprovedOption_Particles,
	eImprovedOption_LastEnum
};

bool GetImprovedOption(cInit *apInit, eImprovedOption aOption);
void SetImprovedOption(cInit *apInit, eImprovedOption aOption, bool abX);

// The Shadows option: On, Only Static, Off (shadow volumes, the original),
// then Soft and Soft, Only Static (shadow maps).
static const int glShadowOptionNum = 5;
int GetShadowOption(cInit *apInit);
void SetShadowOption(cInit *apInit, int alOption);

// Sets of graphics options. Custom is any other combination: it is what
// GetGraphicsPreset() returns, never something to apply.
enum eGraphicsPreset
{
	eGraphicsPreset_ImprovedHigh,
	eGraphicsPreset_ImprovedMedium,
	eGraphicsPreset_ImprovedLow,
	eGraphicsPreset_Original,
	eGraphicsPreset_Custom,
	eGraphicsPreset_LastEnum
};

const wchar_t *GetGraphicsPresetName(eGraphicsPreset aPreset);
// Applies at once; none of the options it sets needs a restart.
void ApplyGraphicsPreset(cInit *apInit, eGraphicsPreset aPreset);
// The preset the current options are, or Custom.
eGraphicsPreset GetGraphicsPreset(cInit *apInit);

#endif // GAME_GRAPHICS_PRESETS_H
