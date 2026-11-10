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

// The Dear ImGui debug panel: F2, or OPENHPL_REFVIEW=1 to start with it
// open. A menu bar along the top switches its windows on and off (in the
// level editor they are in its View menu instead):
//
// - Render: the renderer's settings, kept in settings.cfg, a tab each for
//   the lighting and bloom, shading, shadows (quality, caching, the lights
//   in view and a test light), ambient occlusion, lens and film (depth of
//   field, motion blur, grain) and anti-aliasing.
// - Move (RefViewTools.cpp): Fly (with the panel closed: the movement keys,
//   Run for speed, Jump and Crouch for up and down), the camera's position
//   and a place to type one, every map, the loaded map's start positions,
//   and bookmarks of camera places in any map, kept until the game quits.
// - God (RefViewTools.cpp): cannot be hurt, heal, the flashlight, flicker
//   and HUD switches, and items: every item entity in the game's files,
//   quest items (type Normal) apart from the rest, given under a name of
//   one's choosing, and the items lying in the loaded map, taken as if
//   picked up. The player effects can be previewed there too.
// - Views: the places the reference images (cRefShots) are taken from, named
//   as the frames are, and the viewpoints: fly the camera to a spot and add
//   one. They are saved to tools/refviews.txt at once, and a map that has
//   any is captured from those alone.
// - Level editor: cLevelEditor's node edits.
//
// The options change nothing until something in the panel is used: the
// flashlight and the 2D are left as the game has them until switched.
//
// While the panel is open it has the mouse and the keyboard, and a place
// picked in Views is held exactly where the frame was taken from. Closing
// it (F2) gives the input back to the game: the player can look and walk
// around from there. At a light the camera stays at the light until the
// view is released.
class cRefView : public iUpdateable
{
public:
	cRefView(cInit *apInit);
	~cRefView() override;

	void SetOpen(bool abOpen);
	bool IsOpen(){ return mbOpen; }
	void ReleaseForDogTest();
	bool EffectsCaptureFailed() const { return mbEffectsCaptureFailed; }

	// For cLiveControl: what the panel does, asked for from outside
	void RequestMap(const tString &asMap, const tString &asStart)
	{
		msLoadMap = cString::SetFileExt(asMap, "dae");
		msLoadStart = asStart;
	}
	// A map asked for has not been loaded yet
	bool IsMapPending();
	const tStringVec& GetMaps();
	// The places of the loaded map, as the panel lists them
	const std::vector<cRefShots::cView>& GetViews(){ return mvViews; }
	// Keeps the camera at a place, looking that way, until released; the
	// player's body is left where it is (as when flying)
	void HoldCamera(const cVector3f &avPos, float afYaw, float afPitch);
	bool IsHoldingCamera(){ return mbHoldCamera; }
	void GetHeldCamera(cVector3f &avPos, float &afYaw, float &afPitch)
	{
		avPos = mvHoldPos;
		afYaw = mfHoldYaw;
		afPitch = mfHoldPitch;
	}
	// The player walks on from the camera
	void ReleaseCamera();
	// flashlight, flicker, hud, godmode, dof; false if there is no such option
	bool SetOption(const tString &asName, bool abValue);

	// The panel's windows, inside the level editor's frame (cEditor has the
	// ImGui panel then, and SetOpen() only shows or hides them)
	void DrawWindows(){ Draw(); }
	// Toggles for the panel's windows, for a menu (the editor's View menu)
	void DrawWindowMenuItems();

	void Update(float afTimeStep) override;
	void OnDraw() override;
	void Reset() override;

private:
	static void Tip(const char *apText);
	bool BeginWindow(const char *asName, bool *apShow, float afX, float afY, float afWidthEm, float afHeightEm);
	void Draw();
	void DrawMenuBar();
	void DrawRenderWindow();
	void DrawMoveWindow();
	void DrawGodWindow();
	void DrawViewsWindow();
	void DrawEditorWindow();
	void DrawMaps();
	void DrawViews();
	void DrawStartPositions();
	void DrawBookmarks();
	void DrawItems(bool abQuest);
	void DrawMapItems();
	void FindItemFiles();
	float ItemMessageHeight();
	void UpdateMoveAndGod();
	void MoveCameraTo(const cVector3f &avPos, float afYaw, float afPitch);
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

	// The panel's windows (F2 shows those that are on)
	bool mbShowRender = true;
	bool mbShowMove = true;
	bool mbShowGod = true;
	bool mbShowViews = false;
	bool mbShowEditor = false;

	// Quick move: done in Update(), not while drawing
	tString msGoToStart;
	int mlGoToBookmark = -1;
	bool mbBookmarkMapAsked = false;
	bool mbGoToCoords = false;
	float mvCoords[3] = {0, 0, 0};
	// Camera places kept for this session, in any map
	struct cBookmark
	{
		tString msName;
		tString msMap;
		cVector3f mvPos;
		float mfYaw;
		float mfPitch;
	};
	std::vector<cBookmark> mvBookmarks;
	char msBookmarkName[48] = {0};
	bool mbAddBookmark = false;

	// God: every item entity in the game's files, found the first time the
	// window lists them. Quest items are those of type Normal: the scripts
	// know them by the name they are given in the map.
	struct cItemFile
	{
		tString msFile;   // item_key01.ent
		int mlType; // eGameItemType
		tString msName;   // the name it is given when spawned
		tString msLabel;  // its name in the game's language, if it has one
		bool mbQuest;
	};
	std::vector<cItemFile> mvItemFiles;
	bool mbItemFilesFound = false;
	char msItemFilter[64] = {0};
	char msSpawnName[64] = {0};
	int mlSpawnItem = -1;
	bool mbSpawnItem = false;
	tString msTakeItem; // an item in the map, taken into the inventory
	bool mbHeal = false;
	tString msItemMessage;

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
	tString msLoadStart; // "" for the map's first start position
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

	// cLiveControl's camera
	bool mbHoldCamera = false;
	cVector3f mvHoldPos;
	float mfHoldYaw = 0;
	float mfHoldPitch = 0;

	char msViewpointName[64];
	bool mbAddViewpoint;
	bool mbDeleteViewpoint;
	tString msViewpointMessage;
};

//----------------------------------------

#endif // GAME_REF_VIEW_H
