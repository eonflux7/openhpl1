/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_EDITOR_H
#define GAME_EDITOR_H

#include "core/StdAfx.h"
#include "core/LevelEditor.h"

#include <imgui.h>

using namespace hpl;

class cInit;

// The level editor's window: the game started with --editor (or
// OPENHPL_EDITOR=1) opens a map in an editor instead of the menus.
//
// The world is drawn by the game's renderer, with its lights, shadows and
// effects, and held still: no physics, no scripts' timers, no entities
// moving (Simulate lets it run). Around it are Dear ImGui panels: the
// map's nodes as a tree (Outliner) and the maps (Maps), the selected
// node (Inspector), and hpl.log (Log). The view is flown with the right
// mouse button held (WASD, Q/E down and up, Shift faster, the wheel for
// the speed), turned around the selection with Alt and the left button,
// and moved sideways with the middle one. A click selects what is under
// the pointer; the gizmo moves (W), turns (E) and scales (R) it, in the
// world's axes or the node's own (the space toggle), with snapping.
//
// The nodes are changed through cLevelEditor: the world shows a change at
// once, Ctrl+Z / Ctrl+Y undo and redo, Ctrl+S saves the map to the edit
// folder (an addon) and reloads it. Play (F8) puts the player where the
// camera is and gives the game the input; F8 again comes back, and the
// map is reloaded to how it was (unsaved changes are kept).
//
// cLiveControl drives the same editor (select, undo, redo, play,
// simulate, and every level editor command), so what an agent does is
// seen in the window.
class cEditor : public iUpdateable
{
public:
	cEditor(cInit *apInit);
	~cEditor() override;

	// --editor in the command line, or OPENHPL_EDITOR set
	static bool IsRequested(const tString &asCommandLine);

	bool IsPlaying(){ return mbPlaying; }
	void SetPlaying(bool abX);
	bool IsSimulating(){ return mbSimulate; }
	void SetSimulating(bool abX){ mbSimulate = abX; }
	// The panels are not drawn (frames for cLiveControl)
	void SetUiHidden(bool abX){ mbUiHidden = abX; }
	bool IsUiHidden(){ return mbUiHidden; }

	void Select(const tString &asNode);
	const tString& GetSelected(){ return mpLevel->GetSelected(); }
	// Undoes or redoes a step, and selects its node
	void Undo();
	void Redo();
	// The camera on the selection
	void FocusSelected();

	void Update(float afTimeStep) override;
	void Reset() override;

private:
	enum eTool { eTool_Select, eTool_Move, eTool_Rotate, eTool_Scale };

	void Draw();
	void DrawMenuBar();
	void DrawToolbar();
	void DrawScenePanel(const ImVec2 &avPos, const ImVec2 &avSize);
	void DrawOutliner();
	void DrawOutlinerNode(const tString &asName);
	void DrawMaps();
	void DrawInspector(const ImVec2 &avPos, const ImVec2 &avSize);
	void DrawLog(const ImVec2 &avPos, const ImVec2 &avSize);
	void DrawViewport();
	void DrawHelpers(ImDrawList *apList);
	void DrawSelection(ImDrawList *apList);
	void DrawStatus();
	void DrawConfirmPopup();
	void DrawPlayingNote();

	// The camera
	void UpdateCamera(float afTimeStep);
	void HoldCamera();
	// Mouse and keys over the 3D view
	void UpdateViewportInput();
	void UpdateShortcuts();

	// The gizmo: true while it has the mouse
	bool UpdateGizmo(ImDrawList *apList);
	void DrawGizmo(ImDrawList *apList, int alHover);
	cVector3f GizmoAxis(int alAxis);
	int GizmoHit(const ImVec2 &avMouse);

	// Projection: false behind the camera
	bool ToScreen(const cVector3f &avWorld, ImVec2 &avScreen);
	void MouseRay(const ImVec2 &avMouse, cVector3f &avStart, cVector3f &avDir);
	tString PickAt(const ImVec2 &avMouse);
	void ReadLog();
	void UpdateNodeCache();
	void LoadMap(const tString &asMap);
	void RequestAction(int alAction);
	void DoAction(int alAction);

	cInit *mpInit;
	cLevelEditor *mpLevel;

	bool mbPlaying;
	bool mbSimulate;
	bool mbReloadOnStop;
	bool mbUiHidden;
	bool mbF8Down;
	// Frames after a map change before the camera is taken from the player
	int mlTakeCameraIn;
	tString msCameraMap;

	// The camera, flown with the mouse
	bool mbFlying;
	bool mbOrbiting;
	bool mbPanning;
	float mfSpeed;
	cVector3f mvOrbitCenter;

	// Selection and gizmo
	eTool mTool;
	bool mbLocalSpace;
	bool mbSnap;
	float mfSnapMove;
	float mfSnapAngle;
	float mfSnapScale;
	int mlGizmoDrag; // the part dragged: 0-2 axes, 3 the middle; -1 none
	cMatrixf m_mtxDragStart;
	cVector3f mvDragScale;
	cVector3f mvDragPlanePoint;
	float mfDragAxisStart;
	ImVec2 mvDragMouse;
	float mfGizmoLength;
	ImVec2 mvClickMouse;
	bool mbClickPending;

	// What the panels show of the map, worked out when it changes
	struct cCachedNode
	{
		tString msName;
		tString msLabel;
		tString msKind;
		cVector3f mvPos;
		bool mbEdited;
	};
	std::vector<cCachedNode> mvNodes;
	int mlNodeVersion;
	tString msNodeMap;
	bool mbScrollToSelected;
	char msFilter[64];
	int mlKindFilter;
	char msMapFilter[64];
	bool mbShowHelpers;
	bool mbShowLightRadius;
	bool mbShowLog;
	float mfLeftWidth; // as dragged
	float mfLeftShown; // as drawn
	float mfRightWidth;
	float mfLogHeight;
	int mlSceneTab;
	int mlSelectTab;

	// Something that loses unsaved changes waits for a yes
	int mlConfirmAction;
	bool mbOpenConfirm;
	tString msConfirmMap;

	// hpl.log
	std::vector<tString> mvLog;
	unsigned long long mlLogRead;
	float mfLogTime;
	char msLogFilter[64];
	bool mbLogErrorsOnly;
	bool mbLogScroll;
	int mlLogErrors;

	tString msMessage;
	float mfMessageTime;
};

#endif // GAME_EDITOR_H
