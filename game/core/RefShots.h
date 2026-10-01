/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_REF_SHOTS_H
#define GAME_REF_SHOTS_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//----------------------------------------

// Reference images for checking renderer changes, enabled with
// OVERTURE_REFSHOTS:
//   OVERTURE_REFSHOTS=1            every map in maps/ and the test map
//   OVERTURE_REFSHOTS=office,worm  maps whose name contains one of these,
//                                  in any case
// For each map the player is put on every start position in turn, then the
// camera alone is put at some of the map's lights. At each of these places a
// frame is saved looking in four directions, as
// <OVERTURE_REFSHOTS_DIR>/<map>-<place>-<angle>.png. Then the game quits.
//
// The frames show the 3D view only, with the flashlight on; nothing the
// game draws in 2D (HUD, fades, effects, menus). The player cannot be hurt.
// The run uses a fixed time step and a fixed random seed, so that two runs
// of the same build give the same frames; tools/refshots.py runs it headless
// and compares the result with an earlier run. Progress is logged with a
// "REFSHOTS:" prefix to hpl.log, and the exit code is 1 if a map failed to
// load or the player died.
class cRefShots : public iUpdateable
{
public:
	cRefShots(cInit *apInit, const tString &asSpec);

	static bool IsRequested();
	static tString GetSpec();

	const tString& GetFirstMap();
	bool Failed(){ return mlFailures > 0; }

	void Update(float afTimeStep) override;
	void Reset() override;

private:
	enum eStep
	{
		eStep_ChangeMap,
		eStep_WaitForMap,
		eStep_SettleMap,
		eStep_SettleStart,
		eStep_Shoot,
		eStep_Done,
	};

	void Fail(const char *asFormat, ...);
	void NextView();
	void NextMap();
	void Finish();
	void SetView();

	cInit *mpInit;

	tString msDir;
	tStringVec mvMaps;
	size_t mlMap;
	// A place to look around from
	struct cView
	{
		tString msName;
		bool mbStartPos; // else the camera is moved, the player stays
		cVector3f mvPos;
	};
	std::vector<cView> mvViews;
	size_t mlView;
	int mlAngle;
	float mfStartYaw;

	eStep mStep;
	int mlFrame;
	int mlShots;
	int mlFailures;

	tString msDeadOn;

	bool mbPrevLimitFPS;
	bool mbPrevSubtitles;
};

//----------------------------------------

#endif // GAME_REF_SHOTS_H
