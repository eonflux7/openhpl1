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
#ifndef GAME_LAMP_APPEARANCE_CONTROLLER_H
#define GAME_LAMP_APPEARANCE_CONTROLLER_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//----------------------------------------

// Appearance settings for world lamps, the glow stick and flares.
class cLampSettings
{
public:
	/// Flames: how much their brightness wavers. 1 is what each kind of flame has, 0 none.
	float mfFlicker = 1.0f;
	/// Flames: how far the place they shine from wanders, which moves their shadows. 1 as above.
	float mfSway = 1.0f;
	/// The colour each kind of lamp has: 0 the original's, 1 all of the kind's own.
	float mfColour = 0.5f;
	/// Falloff by distance (brighter near the lamp) in place of the ramp.
	bool mbFalloff = true;
	/// The distance, as a share of the reach, at which a lamp is as bright as its colour says.
	float mfEvenAt = 0.5f;
	/// How many times brighter than that it gets at most, nearer.
	float mfNearBoost = 2.5f;
	/// Fluorescents: the length of their highlights, in times the tube's. 0: a point, as in the original.
	float mfTube = 1.0f;
	/// The lights that are part of the maps get the falloff too, and the flames among them waver.
	bool mbMapLights = true;
};

//----------------------------------------

// Updates cLightLook from the lamp kind inferred from entity filenames.
// Also handles held glow sticks/flares and map-light falloff; other entities are unchanged.
class cLampAppearanceController : public iUpdateable
{
public:
	cLampAppearanceController(cInit *apInit);
	~cLampAppearanceController() override;

	void Update(float afTimeStep) override;
	void Reset() override;

	void SetSettings(const cLampSettings& aSettings){ mSettings = aSettings; }
	const cLampSettings& GetSettings(){ return mSettings; }

	/// Flames that hold still: for pictures that are to be compared.
	void SetStill(bool abStill){ mbStill = abStill; }
	bool GetStill(){ return mbStill; }

	/// The lights that have a look at the moment, for the panel.
	int GetLampCount(){ return mlLampCount; }

private:
	void SetLook(iLight3D *apLight, int alKind, const cMatrixf *apEntityMtx, bool abFalloff);
	void SetMapLightLook(iLight3D *apLight);

	cInit *mpInit;

	cLampSettings mSettings;
	bool mbStill;
	double mfTime;
	int mlLampCount;
};

//----------------------------------------

#endif // GAME_LAMP_APPEARANCE_CONTROLLER_H
