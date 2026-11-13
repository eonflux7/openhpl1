/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_LIVE_CONTROL_H
#define GAME_LIVE_CONTROL_H

#include "core/StdAfx.h"

#include <sstream>

using namespace hpl;

class cInit;

// Commands to the running game through files, for tools and agents
// (tools/levelview.py): OPENHPL_CONTROL=<folder> turns it on, and
// developer mode with it.
//
// A command is a file <id>.cmd in the folder (written under another name
// and renamed, so it is never read half written). The game reads the
// files in the order of their names, one at a time, deletes each and
// answers with <id>.out: "ok" or "error: <why>" on the first line, then
// what was asked for. Commands that take time (loading a map, a frame to be
// saved) are answered when they are done. A file may have several lines:
// they run one after the other with no frames between them, until one
// fails, and are answered together; with "option fixedstep 1" that makes
// the same frames each time. The folder also gets "ready",
// with the process id and the game, while the game runs.
//
// The commands (docs/level-editor.md has the details):
//   status                      map, camera, edits
//   maps [filter]               the maps that can be loaded
//   map <name> [start]          loads a map, answers once it has settled
//   reload                      loads the map again from its files
//   views                       the map's viewpoints and start positions
//   view <name>                 the camera to a viewpoint or start position
//   camera [x y z [yaw pitch]]  where the camera is; or holds it there
//   look <node> [distance]      holds the camera looking at a node
//   release                     lets go of the camera: the player walks on
//   option <name> <0|1>         flashlight, flicker, hud, godmode, dof;
//                               fixedstep: one logic step a frame and the
//                               same random numbers after each map load
//   shot <file.png> [frames] [2d] [panel]  saves a frame after some
//                               frames (10), of the 3D view only unless
//                               "2d"; "panel" opens the F2 panel for it
//   nodes [filter]              the nodes of the map's file
//   node <name>                 one node
//   set <node> [pos x y z] [rot x y z] [scale x y z] [move x y z]
//               [turn x y z]    changes a node (degrees; turn adds)
//   reset [node]                undoes unsaved changes
//   save                        saves the changed map and reloads it
//   revert                      removes the saved map and reloads
//   pick                        the node in the middle of the screen
//   select [node [focus]]       the editor's selection (with --editor)
//   undo, redo                  the editor's steps
//   play <0|1>, simulate <0|1>  the editor's Play and Simulate
//   window <width> <height>     resizes the window (in its own units)
//   mouse <x> <y> [move|press|release|click] [left|right|middle]
//                               the editor's pointer, in window pixels
//   script <command>            runs a line in the map's script
//   wait <frames>
//   quit
class cLiveControl : public iUpdateable
{
public:
	cLiveControl(cInit *apInit, const tString &asDir);
	~cLiveControl() override;

	static bool IsRequested();
	static tString GetDir();

	void Update(float afTimeStep) override;
	void Reset() override;

private:
	enum eWait
	{
		eWait_None,
		eWait_Map,
		eWait_Reload,
		eWait_Frames,
		eWait_Shot,
		eWait_ShotSaved,
	};

	// Reads the next command file; false if there is none
	bool ReadNextCommand();
	// Starts the lines that are left, answers once they are done
	void RunLines();
	// Starts the current command; false if it is not done yet
	bool Start(const tStringVec &avArgs);
	// Goes on with a command that waits; false while it still does
	bool Continue(float afTimeStep);
	void Answer();
	// Makes the answer an error; true, as for a command that is done
	bool Fail(const tString &asMessage);
	void EndShot();

	bool InWorld();
	void WriteNode(const tString &asName);

	cInit *mpInit;
	tString msDir;
	float mfScanTime;

	// The command being done
	tString msId;
	tStringVec mvLines;
	size_t mlLine;
	std::ostringstream mOut;
	tString msError;
	eWait mWait;
	int mlFrames;
	float mfWaited;
	tString msWaitMap;
	int mlLoadCount;
	int mlFailCount;
	tString msShotFile;
	bool mbShot2D;
	bool mbPanelWasOpen;
	bool mbEditorUiWasHidden;
	bool mbSkippedDrawing;
	bool mbQuit;
	bool mbFixedStep;
	int mlSeededLoad;
};

#endif // GAME_LIVE_CONTROL_H
