// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GRAPHICS_PRESETS_H
#define GAME_GRAPHICS_PRESETS_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

// Sets of graphics options. Custom is any other combination: it is what
// GetGraphicsPreset() returns, never something to apply.
enum eGraphicsPreset
{
	eGraphicsPreset_ImprovedHigh,
	eGraphicsPreset_ImprovedMedium,
	eGraphicsPreset_ImprovedLow,
	eGraphicsPreset_Custom,
	eGraphicsPreset_LastEnum
};

const wchar_t *GetGraphicsPresetName(eGraphicsPreset aPreset);
// Applies at once; none of the options it sets needs a restart.
void ApplyGraphicsPreset(cInit *apInit, eGraphicsPreset aPreset);
// The preset the current options are, or Custom.
eGraphicsPreset GetGraphicsPreset(cInit *apInit);

#endif // GAME_GRAPHICS_PRESETS_H
