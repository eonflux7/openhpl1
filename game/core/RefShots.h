/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_REF_SHOTS_H
#define GAME_REF_SHOTS_H

#include "core/StdAfx.h"

using namespace hpl;

class cInit;

//----------------------------------------

// Reference images for checking renderer changes, enabled with
// OPENHPL_REFSHOTS:
//   OPENHPL_REFSHOTS=1            every map in maps/ and the test map
//   OPENHPL_REFSHOTS=office,worm  maps whose name contains one of these,
//                                  in any case
// A map's frames are taken from its viewpoints in tools/refviews.txt
// (chosen by hand in the F2 panel, cRefView): the camera is put at each one
// and a frame saved as <OPENHPL_REFSHOTS_DIR>/<map>-<viewpoint>.png.
// A map without viewpoints gets automatic places instead: the player is put
// on every start position in turn, then the camera alone at some of the
// map's lights, and a frame is saved looking in four directions from each,
// as <map>-<place>-<angle>.png. Then the game quits.
//
// The frames show the 3D view only, with the flashlight on; nothing the
// game draws in 2D (HUD, fades, effects, menus). The player cannot be hurt.
// After the last map the first one is loaded again for the 2D screens: the
// HUD, inventory, notebook, keypad, a message and the main menu, saved as
// <OPENHPL_REFSHOTS_DIR>/2d-<screen>.png.
// The run uses a fixed time step and a fixed random seed, so that two runs
// of the same build give the same frames; tools/refshots.py runs it headless
// and compares the result with an earlier run. Progress is logged with a
// "REFSHOTS:" prefix to hpl.log, and the exit code is 1 if a map failed to
// load or the player died.
class cRefShots : public iUpdateable
{
public:
	cRefShots(cInit *apInit, const tString &asSpec);

	// A place to look from
	struct cView
	{
		tString msName;
		bool mbStartPos; // else the camera is moved, the player stays
		cVector3f mvPos;
		// A viewpoint from the file: one direction, the camera's
		bool mbManual = false;
		float mfYaw = 0;
		float mfPitch = 0;
		bool mbFlashLight = true;
	};
	typedef std::map<tString, std::vector<cView> > tViewMap;

	// Directions looked in from each automatic place, a quarter turn apart
	static const int klAngles = 4;
	static int GetAngleNum(const cView &aView){ return aView.mbManual ? 1 : klAngles; }

	// The viewpoints file: tools/refviews.txt of the source tree the game
	// was built from, or OPENHPL_REFVIEWS_FILE. One line per viewpoint:
	// map (lower case, no extension), name, x y z, yaw, pitch, flashlight
	// (1 or 0; on if left out).
	static tString GetViewsFile();
	static void LoadViewsFile(tViewMap &aViews);
	static bool SaveViewsFile(const tViewMap &aViews);

	// The maps in maps/ and the test map, sorted. cRefView uses these too.
	static void FindMaps(cInit *apInit, tStringVec &avMaps);
	// The places of a loaded map: its viewpoints from the file, or without
	// any its start positions, then some of its lights. Returns the number
	// of lights the map has.
	static size_t FindViews(cWorld3D *apWorld, const tString &asMap, std::vector<cView> &avViews,
							size_t &alStarts);
	// The name the frames of a map are saved under
	static tString GetFrameName(const tString &asMap, const cView &aView, int alAngle);

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
		eStep_2D,
		eStep_Done,
	};

	void Fail(const char *asFormat, ...);
	void NextView();
	void NextMap();
	void Finish();
	void SetView();
	void Update2D();

	cInit *mpInit;

	tString msDir;
	tStringVec mvMaps;
	size_t mlMap;
	std::vector<cView> mvViews;
	size_t mlView;
	int mlAngle;
	float mfStartYaw;

	eStep mStep;
	int mlFrame;
	int mlShots;
	int mlFailures;

	// The 2D screens are being shot, on the first map again
	bool mb2D;
	size_t ml2DScreen;

	tString msDeadOn;

	bool mbPrevLimitFPS;
	bool mbPrevSubtitles;
};

//----------------------------------------

#endif // GAME_REF_SHOTS_H
