/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_REF_VIEW_H
#define GAME_REF_VIEW_H

#include "core/StdAfx.h"
#include "core/RefShots.h"

using namespace hpl;

class cInit;

//----------------------------------------

// A Dear ImGui panel for looking at the places of the reference images
// (cRefShots) by hand: F2, or OVERTURE_REFVIEW=1 to start with it open.
//
// It lists the maps, and for the loaded map the same places and directions
// the reference images are taken from, named as the frames are. Picking one
// puts the camera at the viewpoint (or the player on the start position, or
// the camera at the light), facing that way. The options make the view
// match the frames: flashlight on, no flicker, nothing drawn in 2D, a
// player that cannot be hurt.
//
// It is also where the viewpoints are chosen: fly the camera to a spot, open
// the panel and add the viewpoint. They are saved to tools/refviews.txt at
// once, and a map that has any is captured from those alone.
//
// A window of its own, Player, has what changes how the game plays while
// testing: Fly (with the panel closed: the movement keys, Run for speed,
// Jump and Crouch for up and down, looking is free of the player's limits),
// which puts the player where the camera is when it is switched off, and
// the flashlight, flicker, HUD and damage switches.
//
// A second window is for the shadows: which kind (the original volumes or
// shadow maps), how the maps are made and read, what they cost, and the
// lights in view, each of which can be drawn alone or have its shadows
// switched. A test light can be put where the camera is, or carried along.
//
// While the panel is open it has the mouse and the keyboard, and the view
// is kept exactly where the frame was taken from. Closing it (F2) gives the
// input back to the game: the player can look and walk around from there.
// At a light the camera stays at the light until the view is released.
class cRefView : public iUpdateable
{
public:
	cRefView(cInit *apInit);
	~cRefView() override;

	void SetOpen(bool abX);
	bool IsOpen(){ return mbOpen; }

	void Update(float afTimeStep) override;
	void Reset() override;

private:
	void Draw();
	void DrawMaps();
	void DrawViews();
	void DrawPlayer();
	void DrawRenderer();
	void DrawShadows();
	void DrawShadowLights();
	void UpdateTestLight();

	void DrawViewpoints();
	void UpdateFly(float afTimeStep);
	void PutPlayerAtCamera();
	void AddViewpoint();
	void DeleteViewpoint();

	void LoadMap(const tString &asMap);
	void GoTo(size_t alView, int alAngle);
	void Release();
	void SetView();
	void ApplyOptions();
	bool CanLoadMap();

	cInit *mpInit;
	bool mbOpen;

	tStringVec mvMaps;
	char msMapFilter[64];

	// The loaded map and its places
	tString msViewsMap;
	std::vector<cRefShots::cView> mvViews;
	size_t mlStarts;
	size_t mlLights;

	// The place the view is at, if mbAtView
	bool mbAtView;
	size_t mlView;
	int mlAngle;
	float mfStartYaw;
	// The direction is kept, while the panel is open
	bool mbHoldDirection;

	// What the panel asks for is done in Update(), not while drawing
	tString msLoadMap;
	int mlGoToView;
	int mlGoToAngle;
	// The map being loaded, and the time waited for it
	tString msWaitMap;
	float mfWaitTime;

	// The options change nothing before the panel has been used
	bool mbUsed;
	bool mbFlashLight;
	bool mbNoFlicker;
	bool mbHide2D;
	bool mbGodMode;
	bool mbSkippingDrawing;
	bool mbLinearLighting;
	bool mbSetLinearLighting;
	// The flashlight is switched to mbFlashLight on the next step
	bool mbSetFlashLight;

	// Flying: the camera goes where the keys take it and the player's body
	// stays behind, switched off
	bool mbFly;
	bool mbFlying;
	cVector3f mvFlyPos;
	float mfFlySpeed;
	// The player's limits on looking up and down, back when flying ends
	cVector2f mvWalkPitchLimits;
	// Steps left before the camera's position is taken over
	int mlFlySeed;

	// The light picked in the shadow window, by name: lights come and go
	tString msShadowLight;
	bool mbShadowAllLights;

	// A point light of the panel's own, in the map that is loaded
	bool mbPlaceTestLight;
	bool mbRemoveTestLight;
	bool mbCarryTestLight;
	float mfTestLightRadius;
	float mfTestLightBrightness;

	char msViewpointName[64];
	bool mbAddViewpoint;
	bool mbDeleteViewpoint;
	tString msViewpointMessage;
};

//----------------------------------------

#endif // GAME_REF_VIEW_H
