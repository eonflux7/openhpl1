/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
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
// (cRefShots) by hand: F2, or OPENHPL_REFVIEW=1 to start with it open.
//
// It lists the maps, and for the loaded map the same places and directions
// the reference images are taken from, named as the frames are. Picking one
// puts the camera at the viewpoint (or the player on the start position, or
// the camera at the light), facing that way. The options can make the view
// match the frames: flashlight on, no flicker, nothing drawn in 2D, a
// player that cannot be hurt. The flashlight and the 2D are left as the
// game has them until they are switched.
//
// It is also where the viewpoints are chosen: fly the camera to a spot, open
// the panel and add the viewpoint. They are saved to tools/refviews.txt at
// once, and a map that has any is captured from those alone.
//
// A window of its own, Player, has what changes how the game plays while
// testing: Fly (with the panel closed: the movement keys, Run for speed,
// Jump and Crouch for up and down, looking is free of the player's limits),
// which puts the player where the camera is when it is switched off, and
// the flashlight, flicker, HUD and damage switches. It can also put the
// flashlight, the glow stick and flares in the inventory.
//
// The Shadows window controls shadow-map quality, caching and cost, and the
// lights in view, each of which can be drawn alone or have its shadows
// switched. A test light can be put where the camera is, or carried along.
//
// Another window is for the ambient occlusion: on or off, how good and how
// far it searches, how strongly it darkens, and the occlusion alone.
//
// The last window, Lens and film, is for the depth of field (with a test
// that switches it on at a distance of your choosing: the game only uses
// it now and then), the motion blur and the noise filter's grain.
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

	void SetOpen(bool abOpen);
	bool IsOpen(){ return mbOpen; }
	void ReleaseForDogTest();
	bool EffectsCaptureFailed() const { return mbEffectsCaptureFailed; }

	void Update(float afTimeStep) override;
	void OnDraw() override;
	void Reset() override;

private:
	void Draw();
	void DrawMaps();
	void DrawViews();
	void DrawPlayer();
	void DrawPlayerEffects();
	void DrawEffectsLab();
	void UpdatePlayerEffects(float afTimeStep);
	cPlayerEffectState EffectPreviewState() const;
	void CheckEffectsCapture(bool abCondition, const char *asMessage);
	void UpdateEffectsChecks();
	void DrawRenderer();
	void DrawShadows();
	void DrawAmbientOcclusion();
	void DrawLensAndFilm();
	void DrawAntiAliasing();
	void DrawShading();
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

	enum eEffectPreview
	{
		ePreview_None,
		ePreview_Adaptation,
		ePreview_Health,
		ePreview_DoubleVision,
		ePreview_Blood,
		ePreview_Ice,
		ePreview_Panic,
		ePreview_Flash,
		ePreview_Save,
		ePreview_Death,
		ePreview_Underwater,
		ePreview_Combined,
		ePreview_Last
	};
	int mlEffectPreview = ePreview_None;
	float mfEffectPreviewAmount = 0.75f;
	float mfEffectPreviewClock = 0;
	unsigned int mlEffectPreviewBloodSeed = 17;
	bool mbEffectPreviewAnimate = true;
	bool mbEffectTour = false;
	bool mbEffectsLabFullPanel = false;
	int mlEffectAction = 0;
	float mfEffectBlackFade = 0;
	// Headless captures use the same preview as the panel, at fixed amounts.
	tString msEffectsCapture;
	int mlEffectsCaptureFrame = 0;
	int mlEffectsCaptureIndex = 0;
	float mfEffectsCaptureHealth = -1;
	float mfEffectsCapturePanicFov = 0;
	float mfEffectsCaptureBloodSeed = 0;
	int mlEffectsCheckFrame = 0;
	bool mbEffectsCaptureFailed = false;
	bool mbEffectsCaptureHealthGap = false;

	cInit *mpInit;
	bool mbOpen;
	//The anti-aliasing window: folded, and frames left to put it in its corner
	bool mbAntiAliasingFolded = true;
	int mlAntiAliasingPlace = 0;

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
	float mfMapWaitElapsedSeconds;

	// The options change nothing before the panel has been used
	bool mbUsed;
	bool mbFlashLight;
	bool mbNoFlicker;
	bool mbHide2D;
	bool mbGodMode;
	bool mbSkippingDrawing;
	// The flashlight is switched to mbFlashLight on the next step
	bool mbSetFlashLight;
	// Items to put in the inventory on the next step
	enum { eGiveItem_Flashlight = 1, eGiveItem_GlowStick = 2, eGiveItem_Flare = 4 };
	int mlGiveItems;

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
