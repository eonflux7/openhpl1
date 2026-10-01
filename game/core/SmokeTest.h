/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_SMOKE_TEST_H
#define GAME_SMOKE_TEST_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//----------------------------------------

// Automated run through the game, enabled with OVERTURE_SMOKE:
//   OVERTURE_SMOKE=1            every map in maps/
//   OVERTURE_SMOKE=office,worm  maps whose name contains one of these
// For each map: change to it the way scripts do, run some frames, save,
// load that save, run some more frames. Then quit. Progress and failures
// are logged with a "SMOKE:" prefix to hpl.log, and the exit code is 1 if
// a map or its level script failed to load. OVERTURE_SMOKE_FRAMES sets the
// frames per step (default 120, two seconds of game time).
class cSmokeTest : public iUpdateable
{
public:
	cSmokeTest(cInit *apInit, const tString &asSpec);

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
		eStep_RunMap,
		eStep_RunLoaded,
		eStep_Done,
	};

	void Fail(const char *asFormat, ...);
	void NextMap();
	tWString SaveFile();

	cInit *mpInit;

	tStringVec mvMaps;
	size_t mlMap;
	eStep mStep;
	int mlFrame;
	int mlFramesPerStep;
	int mlFailures;
};

//----------------------------------------

#endif // GAME_SMOKE_TEST_H
