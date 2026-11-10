// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
#ifndef GAME_GAME_MIST_H
#define GAME_GAME_MIST_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//////////////////////////////////////////////////////////////////////////
// Where the mist is (cRenderer3D::RenderMist): only where the maps
// already have damp air. A low layer lies over the water of liquid areas,
// and around the particle systems that let out mist and fog there is a
// pocket of it, and a thinner one around steam. A pocket is as wide as
// the particles reach, with room around them, and grows while they
// spread. Smoke and dust leave the air as it is.
//////////////////////////////////////////////////////////////////////////

class cGameMist : public iMistSource
{
public:
	cGameMist(cInit *apInit);

	void GetMistVolumes(cCamera3D *apCamera, std::vector<cMistVolume> &avVolumes) override;

	enum eKind
	{
		eKind_None,
		eKind_Mist,
		eKind_Steam,
	};
	/// What a particle system's file (its type) lets out.
	static eKind KindOf(const tString &asDataName);

private:
	class cPocket
	{
	public:
		tString msName;
		cVector3f mvMin, mvMax;
	};

	cInit *mpInit;
	cWorld3D *mpWorld = nullptr;
	std::map<cParticleSystem3D*, cPocket> m_mapPockets;
};

#endif // GAME_GAME_MIST_H
