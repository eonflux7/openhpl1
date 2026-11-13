/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_LEVEL_EDITOR_H
#define GAME_LEVEL_EDITOR_H

#include "core/StdAfx.h"

#include <memory>
#include <tuple>

using namespace hpl;

class cInit;
class iGameEntity;

namespace hpl
{
	class cColladaData;
	class cColladaNode;
}

// Moves, turns and scales the nodes of the loaded map's file (entities,
// static meshes, lights, areas, start positions...) and saves the map with
// the changes, so that it is seen in the game straight away.
//
// The map's file (its .dae, or a .glb standing in for it) is read when the
// editor is first used on a map. Changes are made to that data, and what
// the world made of a node is moved along at once (its meshes, bodies,
// lights, areas, sounds, particles; see SyncLive): a scale shows on meshes
// but not yet in their bodies. Save writes the whole map as
// <edit folder>/<map>.glb and loads the map again
// (cMapHandler::RequestReload), with the player and the camera where they
// were. A reload without a save keeps the unsaved changes and makes them
// again in the new world. The edit folder is an addon of its own with the
// highest priority (cInit::msEditDir: OPENHPL_EDIT_DIR, else the personal
// folder's addons/editor), so the saved map replaces the game's and other
// addons' copies until it is reverted. Next to it, <map>.edits.txt lists
// the nodes changed and their world transforms, for carrying the changes
// back into the map's source (a Blender scene); see docs/level-editor.md.
//
// The editor is used from the F2 panel's Level editor section and from
// cLiveControl's commands. Nodes are named as in the map file, or by the
// name of the entity, area or sound they make ("smalltable1" for
// "_ref_furniture_smalltable01_smalltable1").
class cLevelEditor
{
public:
	cLevelEditor(cInit *apInit);
	~cLevelEditor();

	// A node of the map file, with its world transform
	struct cNodeInfo
	{
		tString msName;
		tString msKind; // entity, static, mesh, light, area, start, sound, particles, ...
		tString msSource; // .ent or mesh file of an entity or static mesh, else ""
		tString msObject; // the name of what the game makes of it, if not msName
		tString msParent; // "" at the top of the scene
		cVector3f mvPos;
		cVector3f mvRot; // Euler angles in degrees, X then Y then Z
		cVector3f mvScale;
		bool mbEdited; // changed since the last save
	};

	// Follows the loaded map; call on every update.
	void Update();

	// Reads the loaded map's file, if that has not been done since it was
	// loaded. False, with asError, if there is no map or it can't be read.
	bool Prepare(tString &asError);
	const tString& GetMap(){ return msMap; }
	// The file the map's data was read from
	const tString& GetMapFile(){ return msMapFile; }
	// The edited map of this map in the edit folder, "" if there is none
	tString GetEditedMapFile();

	void FindNodes(const tString &asFilter, std::vector<cNodeInfo> &avNodes);
	bool GetNode(const tString &asName, cNodeInfo &aInfo);

	// Sets the world transform of a node: the parts given, the others are
	// kept. Children move along.
	bool SetTransform(const tString &asName, const cVector3f *apPos, const cVector3f *apRot,
						const cVector3f *apScale, tString &asError);
	// The world transform without the scale (rotation and position), and
	// the scale; false if there is no such node
	bool GetWorld(const tString &asName, cMatrixf &a_mtxRotPos, cVector3f &avScale);
	// Sets them; the rotation of a_mtxRotPos has to be a pure rotation
	bool SetWorld(const tString &asName, const cMatrixf &a_mtxRotPos, const cVector3f &avScale, tString &asError);

	// A node's transform as the file keeps it, for undoing
	struct cState
	{
		cMatrixf m_mtxTransform; // under the parent
		cVector3f mvScale;
	};
	bool GetState(const tString &asName, cState &aState);
	bool SetState(const tString &asName, const cState &aState);

	// Puts a node, or all nodes ("" ), back as they were read
	void ResetNode(const tString &asName);
	int GetEditCount(){ return (int)m_mapOriginal.size(); }
	// Counts every change, for what is worked out from the nodes
	int GetVersion(){ return mlVersion; }

	// Undo: the changes between BeginUndo() and EndUndo() are one step (a
	// drag of the gizmo); a change made outside of them is a step of its
	// own. Steps are kept over reloads and saves of the map.
	void BeginUndo(const tString &asLabel);
	void EndUndo();
	bool CanUndo(){ return mlUndo > 0; }
	bool CanRedo(){ return mlUndo < mvUndo.size(); }
	const tString& GetUndoLabel(){ static tString sNone; return CanUndo() ? mvUndo[mlUndo - 1].msLabel : sNone; }
	const tString& GetRedoLabel(){ static tString sNone; return CanRedo() ? mvUndo[mlUndo].msLabel : sNone; }
	// The node the step was of, "" if there was none
	tString Undo();
	tString Redo();

	// The node's children, or the nodes at the top of the scene for ""
	void GetChildren(const tString &asName, tStringVec &avChildren);
	// The node's kind and what it makes, without the rest of cNodeInfo
	tString GetNodeKind(const tString &asName);
	// The box in the world around what a node shows (its meshes, area,
	// light), else a small one at the node; false if there is no such node
	bool GetNodeBox(const tString &asName, cVector3f &avMin, cVector3f &avMax);
	// The light a node makes, if it makes one
	iLight3D* GetNodeLight(const tString &asName);

	// Writes the map with the changes to the edit folder and reloads it.
	bool Save(tString &asMessage);
	// Removes the map from the edit folder and reloads it.
	bool Revert(tString &asMessage);

	// The node under the middle of the screen, "" if none: the closest
	// surface the physics ray hits, then the smallest mesh around it.
	tString Pick();
	// The same along a line in the world
	tString Pick(const cVector3f &avStart, const cVector3f &avEnd);
	// The middle and the radius of what a node shows, for looking at it
	bool GetNodeBounds(const tString &asName, cVector3f &avCenter, float &afRadius);
	// Where a camera can look at a node from: avFrom, or nearer the node
	// if a wall is in between
	cVector3f GetClearView(const tString &asName, const cVector3f &avTarget, const cVector3f &avFrom);

	// The F2 panel's Level editor section
	void Draw();
	// Requests from the window are done in the update
	void SetSelected(const tString &asName){ msSelected = asName; mbShowRequested = true; }
	// True once after a node was selected from outside the panel (pick):
	// the panel opens its section
	bool TakeShowRequest(){ bool bShow = mbShowRequested; mbShowRequested = false; return bShow; }
	const tString& GetSelected(){ return msSelected; }

	static tString GetKind(const cColladaNode *apNode, const cColladaData &aData);
	// The name of the entity, area or sound a node makes: the last part of
	// "_ref_<file>_<name>" and the like, else the node's name
	static tString GetObjectName(const cColladaNode *apNode);
	// Makes the edit folder an addon (addon.cfg) if it is not one yet
	static void PrepareEditDir(const tString &asDir);

private:
	void Clear();
	cColladaNode* FindNode(const tString &asName);
	void FillInfo(cColladaNode *apNode, cNodeInfo &aInfo);
	// "x y z<TAB>qx qy qz qw<TAB>sx sy sz": the world transform
	tString GetWorldText(cColladaNode *apNode);
	void UpdateWorldTransforms(cColladaNode *apNode);
	bool IsScaleSeparate(cColladaNode *apNode);
	// The node gets a new local transform and scale; the world follows
	void ChangeNode(cColladaNode *apNode, const cMatrixf &a_mtxTransform, const cVector3f &avScale);

	// What the world made of the nodes, and where it was when first moved
	struct cLiveTarget
	{
		int mType;
		tString msName;
		int mlIndex; // body of a game entity
		cMatrixf m_mtxStart;
		cVector3f mvStartSize; // of an area
	};
	struct cLiveNode
	{
		cMatrixf m_mtxStartWorld;
		cVector3f mvStartScale;
		std::vector<cLiveTarget> mvTargets;
	};
	// Before a node and its children change
	void CaptureLive(cColladaNode *apNode);
	// After: moves what they made
	void SyncLive(cColladaNode *apNode);
	// What the world made of a node, now
	void FindTargets(cColladaNode *apNode, std::vector<cLiveTarget> &avTargets);
	iEntity3D* GetTargetEntity(const cLiveTarget &aTarget);
	iGameEntity* GetGameEntity(cColladaNode *apNode);
	// The world is the one the captures are of
	bool LiveWorldValid();
	static tString GetLightName(const tString &asNodeName);
	std::map<tString, cLiveNode> m_mapLive;

	struct cUndoStep
	{
		tString msLabel;
		// Node, before, after
		std::vector<std::tuple<tString, cState, cState>> mvNodes;
	};
	std::vector<cUndoStep> mvUndo;
	size_t mlUndo; // steps done; the rest can be redone
	int mlUndoDepth; // BeginUndo()s open
	cUndoStep mOpenStep;
	bool mbApplyingUndo;
	void AddToUndo(cColladaNode *apNode, const cState &aBefore);
	void ApplyStep(const cUndoStep &aStep, bool abUndo);
	void ClearUndo();
	// Static lights gather their shadow casters again
	bool mbStaticMoved;
	int mlVersion;

	cInit *mpInit;

	// The map the data is for; reloads are told apart by the load count
	tString msMap;
	int mlMapLoad;
	tString msMapFile;
	std::unique_ptr<cColladaData> mpData;
	bool mbReadFailed;

	// The edited nodes as they were read: local transform and scale
	struct cOriginal
	{
		cMatrixf m_mtxTransform;
		cVector3f mvScale;
		tString msWorld; // the world transform, as the edits file has it
	};
	std::map<tString, cOriginal> m_mapOriginal;

	// The window
	tString msSelected;
	char msFilter[64];
	char msKindFilter[32];
	bool mbSaveRequested;
	bool mbRevertRequested;
	bool mbPickRequested;
	bool mbShowRequested;
	tString msMessage;
};

#endif // GAME_LEVEL_EDITOR_H
