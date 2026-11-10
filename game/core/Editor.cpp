/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/Editor.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/RefView.h"
#include "player/Player.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////
// HELPERS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

namespace
{
	enum eAction
	{
		eAction_None,
		eAction_Save,
		eAction_Revert,
		eAction_Reload,
		eAction_OpenMap,
		eAction_ResetAll,
		eAction_Quit,
	};

	const char *gvKinds[] = {"all", "entity", "static", "mesh", "light", "area", "start", "sound", "particles",
							 "billboard", "beam", "collider", "portal", "node", "joint", "group"};

	ImU32 KindColor(const tString &asKind)
	{
		if(asKind == "entity") return IM_COL32(240, 160, 80, 255);
		if(asKind == "static" || asKind == "mesh") return IM_COL32(150, 175, 205, 255);
		if(asKind == "light") return IM_COL32(250, 220, 90, 255);
		if(asKind == "area") return IM_COL32(110, 210, 120, 255);
		if(asKind == "start") return IM_COL32(220, 120, 230, 255);
		if(asKind == "sound") return IM_COL32(90, 210, 230, 255);
		if(asKind == "particles" || asKind == "billboard" || asKind == "beam") return IM_COL32(240, 130, 160, 255);
		return IM_COL32(160, 160, 160, 255);
	}

	// Kinds that have nothing to click on in the view: drawn as dots
	bool IsHelperKind(const tString &asKind)
	{
		return asKind == "light" || asKind == "area" || asKind == "start" || asKind == "sound" ||
			   asKind == "particles" || asKind == "billboard" || asKind == "beam";
	}

	const ImU32 gvAxisColors[3] = {IM_COL32(230, 70, 70, 255), IM_COL32(90, 210, 80, 255), IM_COL32(80, 130, 245, 255)};
	const ImU32 gHoverColor = IM_COL32(255, 230, 60, 255);

	float Dist2(const ImVec2 &a, const ImVec2 &b)
	{
		return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
	}

	// Distance from a point to a segment, on the screen
	float SegmentDistance(const ImVec2 &avP, const ImVec2 &avA, const ImVec2 &avB)
	{
		ImVec2 ab(avB.x - avA.x, avB.y - avA.y);
		float fLen2 = ab.x * ab.x + ab.y * ab.y;
		float t = fLen2 > 0 ? ((avP.x - avA.x) * ab.x + (avP.y - avA.y) * ab.y) / fLen2 : 0;
		t = cMath::Clamp(t, 0.0f, 1.0f);
		return std::sqrt(Dist2(avP, ImVec2(avA.x + ab.x * t, avA.y + ab.y * t)));
	}

	// Where a line (start, unit direction) comes closest to a ray: the
	// distance along the line; false if they are nearly parallel
	bool ClosestOnLine(const cVector3f &avLineStart, const cVector3f &avLineDir, const cVector3f &avRayStart,
					   const cVector3f &avRayDir, float &afT)
	{
		float fDot = cMath::Vector3Dot(avLineDir, avRayDir);
		float fDenom = 1 - fDot * fDot;
		if(fDenom < 1e-4f) return false;
		cVector3f vW = avLineStart - avRayStart;
		afT = (fDot * cMath::Vector3Dot(vW, avRayDir) - cMath::Vector3Dot(vW, avLineDir)) / fDenom;
		return true;
	}

	bool RayPlane(const cVector3f &avRayStart, const cVector3f &avRayDir, const cVector3f &avPoint,
				  const cVector3f &avNormal, cVector3f &avHit)
	{
		float fDenom = cMath::Vector3Dot(avNormal, avRayDir);
		if(std::fabs(fDenom) < 1e-4f) return false;
		float t = cMath::Vector3Dot(avPoint - avRayStart, avNormal) / fDenom;
		if(t < 0) return false;
		avHit = avRayStart + avRayDir * t;
		return true;
	}

	// A rotation of afAngle around an axis through the origin
	cMatrixf AxisRotation(const cVector3f &avAxis, float afAngle)
	{
		cQuaternion qRot;
		qRot.FromAngleAxis(afAngle, avAxis);
		return cMath::MatrixQuaternion(qRot);
	}

	float Snap(float afValue, float afStep)
	{
		return afStep > 0 ? std::round(afValue / afStep) * afStep : afValue;
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEditor::cEditor(cInit *apInit) : iUpdateable("Editor")
{
	mpInit = apInit;
	mpLevel = apInit->mpLevelEditor;

	mbPlaying = false;
	mbSimulate = false;
	mbReloadOnStop = true;
	mbUiHidden = false;
	mbF8Down = false;
	mlTakeCameraIn = 0;

	mbFlying = false;
	mbOrbiting = false;
	mbPanning = false;
	mfSpeed = 4;

	mTool = eTool_Move;
	mbLocalSpace = false;
	mbSnap = false;
	mfSnapMove = 0.25f;
	mfSnapAngle = 15;
	mfSnapScale = 0.1f;
	mlGizmoDrag = -1;
	mfDragAxisStart = 0;
	mfGizmoLength = 1;
	mbClickPending = false;

	mlNodeVersion = -1;
	mbScrollToSelected = false;
	msFilter[0] = 0;
	mlKindFilter = 0;
	msMapFilter[0] = 0;
	mbShowHelpers = true;
	mbShowLightRadius = true;
	mbShowLog = true;
	mfLeftWidth = 0;
	mfLeftShown = 0;
	mfRightWidth = 0;
	mfLogHeight = 0;
	mlSceneTab = 0;
	mlSelectTab = -1;

	mlConfirmAction = eAction_None;
	mbOpenConfirm = false;

	mlLogRead = 0;
	mfLogTime = 0;
	msLogFilter[0] = 0;
	mbLogErrorsOnly = false;
	mbLogScroll = true;
	mlLogErrors = 0;
	mfMessageTime = 0;

	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	pLowLevel->SetDebugPanel([this](){ Draw(); });
	pLowLevel->SetDebugPanelInput(true);
	mpInit->mpRefView->SetOption("hud", false);
	Log("Editor: started\n");
}

cEditor::~cEditor()
{
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	pLowLevel->SetDebugPanelMouseCapture(false);
	pLowLevel->SetDebugPanel({});
	pLowLevel->SetDebugPanelInput(false);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

bool cEditor::IsRequested(const tString &asCommandLine)
{
	const char *pValue = SDL_getenv("OPENHPL_EDITOR");
	if(pValue && pValue[0] && pValue[0] != '0') return true;
	return (" " + asCommandLine + " ").find(" --editor ") != tString::npos;
}

//-----------------------------------------------------------------------

void cEditor::SetPlaying(bool abX)
{
	if(mbPlaying == abX) return;
	if(abX && mpInit->mpGame->GetScene()->GetWorld3D() == nullptr) return;
	mbPlaying = abX;

	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	mbFlying = mbOrbiting = mbPanning = false;
	pLowLevel->SetDebugPanelMouseCapture(false);
	if(mlGizmoDrag >= 0)
	{
		mlGizmoDrag = -1;
		mpLevel->EndUndo();
	}

	if(mbPlaying)
	{
		// The player walks on from the camera
		mpInit->mpRefView->ReleaseCamera();
		mpInit->mpRefView->SetOption("hud", true);
		pLowLevel->SetDebugPanelInput(false);
		Log("Editor: playing %s\n", mpLevel->GetMap().c_str());
	}
	else
	{
		HoldCamera();
		mpInit->mpRefView->SetOption("hud", false);
		pLowLevel->SetDebugPanelInput(true);
		Log("Editor: stopped playing\n");
		if(mbReloadOnStop && mpInit->mpGame->GetScene()->GetWorld3D()) mpInit->mpMapHandler->RequestReload();
	}
}

//-----------------------------------------------------------------------

void cEditor::Select(const tString &asNode)
{
	tString sError;
	if(asNode != "" && mpLevel->Prepare(sError) == false) return;
	cLevelEditor::cNodeInfo info;
	if(asNode != "" && mpLevel->GetNode(asNode, info) == false) return;
	mpLevel->SetSelected(asNode != "" ? info.msName : "");
	mpLevel->TakeShowRequest();
	mbScrollToSelected = true;
}

void cEditor::Undo()
{
	tString sNode = mpLevel->Undo();
	if(sNode != "") Select(sNode);
}

void cEditor::Redo()
{
	tString sNode = mpLevel->Redo();
	if(sNode != "") Select(sNode);
}

//-----------------------------------------------------------------------

void cEditor::FocusSelected()
{
	cVector3f vCenter;
	float fRadius;
	if(GetSelected() == "" || mpLevel->GetNodeBounds(GetSelected(), vCenter, fRadius) == false) return;
	if(mpLevel->GetNodeKind(GetSelected()) == "light") fRadius = 0.5f;

	cVector3f vPos;
	float fYaw, fPitch;
	mpInit->mpRefView->GetHeldCamera(vPos, fYaw, fPitch);
	// The direction the camera is held at, which a reload may not have
	// given it back yet
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	pCamera->SetYaw(fYaw);
	pCamera->SetPitch(fPitch);
	// Not behind a wall: in front of it, or from another side with more room
	float fWanted = cMath::Max(1.0f, fRadius * 2.0f);
	cVector3f vBest;
	float fBestRoom = -1, fBestYaw = fYaw, fBestPitch = fPitch;
	for(float fTryPitch : {fPitch, -0.35f, -0.8f, 0.0f})
	{
		for(int i = 0; i < 8 && fBestRoom < fWanted * 0.95f; ++i)
		{
			float fTryYaw = fYaw + i * kPif / 4;
			pCamera->SetYaw(fTryYaw);
			pCamera->SetPitch(fTryPitch);
			cVector3f vFrom = mpLevel->GetClearView(GetSelected(), vCenter, vCenter - pCamera->GetForward() * fWanted);
			float fRoom = (vFrom - vCenter).Length();
			if(fRoom > fBestRoom + 0.05f)
			{
				fBestRoom = fRoom;
				vBest = vFrom;
				fBestYaw = fTryYaw;
				fBestPitch = fTryPitch;
			}
		}
	}
	mpInit->mpRefView->HoldCamera(vBest, fBestYaw, fBestPitch);
	mvOrbitCenter = vCenter;
}

//-----------------------------------------------------------------------

void cEditor::Update(float afTimeStep)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	bool bDefault = mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default";

	// F8 reaches the game whoever has the input
	bool bF8 = mpInit->mpGame->GetInput()->GetKeyboard()->KeyIsDown(eKey_F8);
	if(bF8 && mbF8Down == false && bDefault) SetPlaying(!mbPlaying);
	mbF8Down = bF8;

	// A map is held still while it is edited
	bool bStill = mbPlaying == false && mbSimulate == false;
	mpInit->mpMapHandler->SetPaused(bStill);
	if(bDefault) mpInit->mpGame->GetScene()->SetUpdateMap(bStill == false);

	if(bDefault && pWorld && mpInit->mpMapHandler->IsChangingMap() == false)
	{
		tString sError;
		mpLevel->Prepare(sError);

		// A new map: the camera starts where the player does
		const tString &sMap = mpInit->mpMapHandler->GetCurrentMapName();
		if(sMap != msCameraMap)
		{
			msCameraMap = sMap;
			mlTakeCameraIn = 3;
			if(mbPlaying == false) mpInit->mpRefView->ReleaseCamera();
		}
		if(mlTakeCameraIn > 0) --mlTakeCameraIn;
		else if(mbPlaying == false && mpInit->mpRefView->IsHoldingCamera() == false) HoldCamera();
	}

	mfLogTime -= afTimeStep;
	if(mfLogTime <= 0)
	{
		mfLogTime = 0.3f;
		ReadLog();
	}
	if(mfMessageTime > 0) mfMessageTime -= afTimeStep;
}

//-----------------------------------------------------------------------

void cEditor::Reset()
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cEditor::HoldCamera()
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	mpInit->mpRefView->HoldCamera(pCamera->GetPosition(), pCamera->GetYaw(), pCamera->GetPitch());
}

//-----------------------------------------------------------------------

bool cEditor::ToScreen(const cVector3f &avWorld, ImVec2 &avScreen)
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	cVector3f vView = cMath::MatrixMul(pCamera->GetViewMatrix(), avWorld);
	if(vView.z > -pCamera->GetNearClipPlane()) return false;
	cVector3f vClip = cMath::MatrixMulDivideW(pCamera->GetProjectionMatrix(), vView);
	const ImVec2 &vSize = ImGui::GetIO().DisplaySize;
	avScreen = ImVec2((vClip.x * 0.5f + 0.5f) * vSize.x, (0.5f - vClip.y * 0.5f) * vSize.y);
	return true;
}

void cEditor::MouseRay(const ImVec2 &avMouse, cVector3f &avStart, cVector3f &avDir)
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	const ImVec2 &vSize = ImGui::GetIO().DisplaySize;
	const cMatrixf &mtxProj = pCamera->GetProjectionMatrix();
	float fX = (avMouse.x / vSize.x) * 2 - 1;
	float fY = 1 - (avMouse.y / vSize.y) * 2;
	cVector3f vView(fX / mtxProj.m[0][0], fY / mtxProj.m[1][1], -1);
	cMatrixf mtxCamera = cMath::MatrixInverse(pCamera->GetViewMatrix());
	avDir = cMath::MatrixMul(mtxCamera.GetRotation(), vView);
	avDir.Normalise();
	avStart = pCamera->GetPosition();
}

//-----------------------------------------------------------------------

tString cEditor::PickAt(const ImVec2 &avMouse)
{
	// The dots of lights, areas and the like first: nothing else shows them
	if(mbShowHelpers)
	{
		float fBest = 9 * 9;
		tString sBest;
		for(const cCachedNode &node : mvNodes)
		{
			if(IsHelperKind(node.msKind) == false) continue;
			ImVec2 vScreen;
			if(ToScreen(node.mvPos, vScreen) == false) continue;
			float fDist = Dist2(vScreen, avMouse);
			if(fDist < fBest)
			{
				fBest = fDist;
				sBest = node.msName;
			}
		}
		if(sBest != "") return sBest;
	}

	cVector3f vStart, vDir;
	MouseRay(avMouse, vStart, vDir);
	return mpLevel->Pick(vStart, vStart + vDir * 200.0f);
}

//-----------------------------------------------------------------------

void cEditor::UpdateNodeCache()
{
	if(mlNodeVersion == mpLevel->GetVersion() && msNodeMap == mpLevel->GetMap()) return;
	mlNodeVersion = mpLevel->GetVersion();
	msNodeMap = mpLevel->GetMap();

	std::vector<cLevelEditor::cNodeInfo> vInfo;
	mpLevel->FindNodes("", vInfo);
	mvNodes.clear();
	mvNodes.reserve(vInfo.size());
	for(const cLevelEditor::cNodeInfo &info : vInfo)
	{
		cCachedNode node;
		node.msName = info.msName;
		node.msLabel = info.msObject != "" ? info.msObject : info.msName;
		node.msKind = info.msKind;
		node.mvPos = info.mvPos;
		node.mbEdited = info.mbEdited;
		mvNodes.push_back(node);
	}
}

//-----------------------------------------------------------------------

void cEditor::ReadLog()
{
	if(mpInit->msLogFile == "") return;
	std::ifstream file(fs::u8path(mpInit->msLogFile), std::ios::binary);
	if(file.good() == false) return;
	file.seekg(0, std::ios::end);
	unsigned long long lSize = (unsigned long long)file.tellg();
	if(lSize < mlLogRead) mlLogRead = 0; // a new file
	if(lSize == mlLogRead) return;

	file.seekg((std::streamoff)mlLogRead);
	std::string sText((size_t)(lSize - mlLogRead), '\0');
	file.read(&sText[0], (std::streamsize)sText.size());
	// Whole lines only
	size_t lEnd = sText.rfind('\n');
	if(lEnd == std::string::npos) return;
	mlLogRead += lEnd + 1;

	size_t lStart = 0;
	while(lStart <= lEnd)
	{
		size_t lLineEnd = sText.find('\n', lStart);
		tString sLine = sText.substr(lStart, lLineEnd - lStart);
		if(sLine.size() && sLine.back() == '\r') sLine.pop_back();
		tString sLower = cString::ToLowerCase(sLine);
		if(sLower.find("error") != tString::npos || sLower.find("couldn't") != tString::npos) ++mlLogErrors;
		mvLog.push_back(sLine);
		lStart = lLineEnd + 1;
	}
	if(mvLog.size() > 5000) mvLog.erase(mvLog.begin(), mvLog.begin() + (mvLog.size() - 5000));
}

//-----------------------------------------------------------------------

void cEditor::LoadMap(const tString &asMap)
{
	mpInit->mpRefView->RequestMap(asMap, "");
	msMessage = "Loading " + asMap;
	mfMessageTime = 4;
	mlLogErrors = 0;
}

void cEditor::RequestAction(int alAction)
{
	// Unsaved changes would be lost
	bool bAsk = (alAction == eAction_OpenMap || alAction == eAction_Quit || alAction == eAction_ResetAll) &&
				mpLevel->GetEditCount() > 0;
	if(alAction == eAction_Revert) bAsk = true;
	if(bAsk)
	{
		// Opened where the popup is drawn: popups are named by the window
		mlConfirmAction = alAction;
		mbOpenConfirm = true;
		return;
	}
	DoAction(alAction);
}

void cEditor::DoAction(int alAction)
{
	tString sMessage;
	switch(alAction)
	{
	case eAction_Save: mpLevel->Save(sMessage); break;
	case eAction_Revert: mpLevel->Revert(sMessage); break;
	case eAction_Reload:
		mpInit->mpMapHandler->RequestReload();
		sMessage = "Reloading " + mpLevel->GetMap();
		break;
	case eAction_OpenMap: LoadMap(msConfirmMap); break;
	case eAction_ResetAll:
		mpLevel->ResetNode("");
		sMessage = "Unsaved changes undone";
		break;
	case eAction_Quit: mpInit->mpGame->Exit(); break;
	default: break;
	}
	if(sMessage != "")
	{
		msMessage = sMessage;
		mfMessageTime = 5;
	}
}

//-----------------------------------------------------------------------

void cEditor::Draw()
{
	ImGuiIO &io = ImGui::GetIO();
	if(mbPlaying)
	{
		DrawPlayingNote();
		if(mpInit->mpRefView->IsOpen()) mpInit->mpRefView->DrawWindows();
		return;
	}
	if(mbUiHidden)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);
		if(mpInit->mpRefView->IsOpen()) mpInit->mpRefView->DrawWindows();
		return;
	}

	UpdateNodeCache();
	const float fEm = ImGui::GetFontSize();
	if(mfLeftWidth <= 0) mfLeftWidth = fEm * 20;
	if(mfRightWidth <= 0) mfRightWidth = fEm * 23;
	if(mfLogHeight <= 0) mfLogHeight = fEm * 11;

	DrawMenuBar();
	float fTop = ImGui::GetFrameHeight();
	float fLogHeight = mbShowLog ? mfLogHeight : 0;
	ImVec2 vSize = io.DisplaySize;
	float fMiddle = vSize.y - fTop - fLogHeight;

	// A narrow window keeps half of its width for the view
	mfLeftShown = cMath::Min(mfLeftWidth, vSize.x * 0.24f);
	float fRight = cMath::Min(mfRightWidth, vSize.x * 0.26f);
	DrawScenePanel(ImVec2(0, fTop), ImVec2(mfLeftShown, fMiddle));
	DrawInspector(ImVec2(vSize.x - fRight, fTop), ImVec2(fRight, fMiddle));
	if(mbShowLog) DrawLog(ImVec2(0, vSize.y - fLogHeight), ImVec2(vSize.x, fLogHeight));

	DrawViewport();
	DrawConfirmPopup();
	UpdateShortcuts();

	if(mpInit->mpRefView->IsOpen()) mpInit->mpRefView->DrawWindows();
}

//-----------------------------------------------------------------------

void cEditor::DrawMenuBar()
{
	if(ImGui::BeginMainMenuBar() == false) return;
	bool bMap = mpLevel->GetMap() != "";
	bool bWorld = mpInit->mpGame->GetScene()->GetWorld3D() != nullptr;

	if(ImGui::BeginMenu("File"))
	{
		if(ImGui::MenuItem("Open map...")) mlSceneTab = 1, mlSelectTab = 1;
		if(ImGui::MenuItem("Save", "Ctrl+S", false, mpLevel->GetEditCount() > 0)) RequestAction(eAction_Save);
		if(ImGui::MenuItem("Reload", "Ctrl+R", false, bWorld)) RequestAction(eAction_Reload);
		if(ImGui::MenuItem("Revert to the game's map", nullptr, false, mpLevel->GetEditedMapFile() != ""))
			RequestAction(eAction_Revert);
		ImGui::Separator();
		if(ImGui::MenuItem("Quit", "")) RequestAction(eAction_Quit);
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Edit"))
	{
		tString sUndo = "Undo " + mpLevel->GetUndoLabel(), sRedo = "Redo " + mpLevel->GetRedoLabel();
		if(ImGui::MenuItem(sUndo.c_str(), "Ctrl+Z", false, mpLevel->CanUndo())) Undo();
		if(ImGui::MenuItem(sRedo.c_str(), "Ctrl+Y", false, mpLevel->CanRedo())) Redo();
		ImGui::Separator();
		if(ImGui::MenuItem("Reset selected", nullptr, false, GetSelected() != "")) mpLevel->ResetNode(GetSelected());
		if(ImGui::MenuItem("Reset all", nullptr, false, mpLevel->GetEditCount() > 0)) RequestAction(eAction_ResetAll);
		if(ImGui::MenuItem("Deselect", "Esc", false, GetSelected() != "")) Select("");
		ImGui::Separator();
		ImGui::MenuItem("Snap", "G", &mbSnap);
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
		ImGui::DragFloat("Move step", &mfSnapMove, 0.01f, 0.01f, 10, "%.2f m");
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
		ImGui::DragFloat("Angle step", &mfSnapAngle, 0.5f, 1, 90, "%.0f deg");
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
		ImGui::DragFloat("Scale step", &mfSnapScale, 0.01f, 0.01f, 1, "%.2f");
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("View"))
	{
		if(ImGui::MenuItem("Focus selected", "F", false, GetSelected() != "")) FocusSelected();
		ImGui::MenuItem("Helpers (lights, areas, sounds...)", nullptr, &mbShowHelpers);
		ImGui::MenuItem("Light radius of the selected light", nullptr, &mbShowLightRadius);
		ImGui::MenuItem("Log", nullptr, &mbShowLog);
		ImGui::Separator();
		bool bPanel = mpInit->mpRefView->IsOpen();
		if(ImGui::MenuItem("Debug panels", "F2", &bPanel)) mpInit->mpRefView->SetOpen(bPanel);
		if(ImGui::BeginMenu("Debug windows"))
		{
			mpInit->mpRefView->DrawWindowMenuItems();
			ImGui::EndMenu();
		}
		iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
		bool bStats = pLowLevel->GetDebugOverlayActive();
		if(ImGui::MenuItem("Frame times", "F3", &bStats)) pLowLevel->SetDebugOverlayActive(bStats);
		ImGui::Separator();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
		ImGui::DragFloat("Camera speed", &mfSpeed, 0.1f, 0.2f, 50, "%.1f m/s");
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Play"))
	{
		if(ImGui::MenuItem("Play from the camera", "F8", false, bWorld)) SetPlaying(true);
		ImGui::MenuItem("Simulate while editing", nullptr, &mbSimulate);
		ImGui::MenuItem("Reload when play stops", nullptr, &mbReloadOnStop);
		ImGui::EndMenu();
	}
	if(ImGui::BeginMenu("Help"))
	{
		ImGui::TextUnformatted("Right mouse button held: look; WASD move, Q/E down/up,\n"
							   "Shift faster, the wheel sets the speed.\n"
							   "Wheel: forward and back.  Middle button: sideways.\n"
							   "Alt + left button: around the selection.\n"
							   "Click: select.  W move, E turn, R scale, Q select only.\n"
							   "X: world or local axes.  G: snap.  F: focus.\n"
							   "Ctrl+Z / Ctrl+Y undo and redo, Ctrl+S save, Ctrl+R reload.\n"
							   "F8: play from the camera, and back.  F2: debug panels.");
		ImGui::EndMenu();
	}

	DrawToolbar();

	// The map, on the right
	tString sMap = bMap ? mpLevel->GetMap() : tString("no map");
	if(mpLevel->GetEditCount() > 0) sMap += "  *" + cString::ToString(mpLevel->GetEditCount()) + " unsaved";
	else if(mpLevel->GetEditedMapFile() != "") sMap += "  (edited copy)";
	// Where there is room for it
	float fWidth = ImGui::CalcTextSize(sMap.c_str()).x;
	ImGui::SameLine(cMath::Max(ImGui::GetWindowWidth() - fWidth - ImGui::GetFontSize(),
							   ImGui::GetCursorPosX() + ImGui::GetFontSize()));
	if(mpLevel->GetEditCount() > 0) ImGui::TextColored(ImVec4(1, 0.8f, 0.4f, 1), "%s", sMap.c_str());
	else ImGui::TextUnformatted(sMap.c_str());
	if(ImGui::IsItemHovered() && mpLevel->GetMapFile() != "")
		ImGui::SetTooltip("Read from %s%s%s", mpLevel->GetMapFile().c_str(),
						  mpLevel->GetEditedMapFile() != "" ? "\nSaved edits: " : "",
						  mpLevel->GetEditedMapFile().c_str());

	ImGui::EndMainMenuBar();
}

//-----------------------------------------------------------------------

void cEditor::DrawToolbar()
{
	ImGui::Separator();
	auto ToolButton = [this](const char *asLabel, eTool aTool, const char *asTip)
	{
		bool bOn = mTool == aTool;
		if(bOn) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.85f, 0.3f, 1));
		if(ImGui::MenuItem(asLabel, nullptr, bOn)) mTool = aTool;
		if(bOn) ImGui::PopStyleColor();
		if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s", asTip);
	};
	ToolButton("Select", eTool_Select, "Q: select only");
	ToolButton("Move", eTool_Move, "W: move along the axes, or in the view's plane from the middle");
	ToolButton("Rotate", eTool_Rotate, "E: turn around the axes");
	ToolButton("Scale", eTool_Scale, "R: scale along the node's axes, or all from the middle");
	ImGui::Separator();
	if(ImGui::MenuItem(mbLocalSpace ? "Local" : "World")) mbLocalSpace = !mbLocalSpace;
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("X: the gizmo's axes, the world's or the node's own");
	ImGui::MenuItem("Snap", nullptr, &mbSnap);
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("G: steps of %.2f m, %.0f degrees, %.2f (Edit menu)", mfSnapMove, mfSnapAngle, mfSnapScale);
	ImGui::Separator();
	ImGui::MenuItem("Simulate", nullptr, &mbSimulate);
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("Physics, scripts and entities run while editing");
	if(ImGui::MenuItem("Play")) SetPlaying(true);
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("F8: the player starts at the camera; F8 comes back");
}

//-----------------------------------------------------------------------

void cEditor::DrawScenePanel(const ImVec2 &avPos, const ImVec2 &avSize)
{
	ImGui::SetNextWindowPos(avPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(avSize, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetFontSize() * 8, avSize.y),
										ImVec2(ImGui::GetIO().DisplaySize.x * 0.45f, avSize.y));
	ImGuiWindowFlags lFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
							  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if(ImGui::Begin("Scene##editor", nullptr, lFlags))
	{
		// Dragged wider or narrower at its right edge
		if(std::fabs(ImGui::GetWindowWidth() - avSize.x) > 0.5f) mfLeftWidth = ImGui::GetWindowWidth();
		if(ImGui::BeginTabBar("scenetabs"))
		{
			ImGuiTabItemFlags lOutliner = mlSelectTab == 0 ? ImGuiTabItemFlags_SetSelected : 0;
			ImGuiTabItemFlags lMaps = mlSelectTab == 1 ? ImGuiTabItemFlags_SetSelected : 0;
			mlSelectTab = -1;
			if(ImGui::BeginTabItem("Outliner", nullptr, lOutliner))
			{
				DrawOutliner();
				ImGui::EndTabItem();
			}
			if(ImGui::BeginTabItem("Maps", nullptr, lMaps))
			{
				DrawMaps();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

//-----------------------------------------------------------------------

void cEditor::DrawOutliner()
{
	const float fEm = ImGui::GetFontSize();
	if(mpLevel->GetMap() == "")
	{
		ImGui::TextDisabled("No map loaded: open one in Maps");
		return;
	}
	if(mvNodes.empty())
	{
		ImGui::TextDisabled("Reading %s...", mpLevel->GetMap().c_str());
		return;
	}

	ImGui::SetNextItemWidth(-fEm * 6.5f);
	ImGui::InputTextWithHint("##filter", "filter: name, file", msFilter, sizeof(msFilter));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1);
	ImGui::Combo("##kind", &mlKindFilter, gvKinds, IM_ARRAYSIZE(gvKinds));

	if(ImGui::BeginChild("nodes", ImVec2(0, 0), ImGuiChildFlags_None))
	{
		if(msFilter[0] == 0 && mlKindFilter == 0)
		{
			// The tree of the map's file
			tStringVec vTop;
			mpLevel->GetChildren("", vTop);
			for(const tString &sName : vTop) DrawOutlinerNode(sName);
		}
		else
		{
			// A list of what matches
			std::vector<cLevelEditor::cNodeInfo> vFound;
			mpLevel->FindNodes(msFilter, vFound);
			std::vector<const cLevelEditor::cNodeInfo *> vShown;
			for(const cLevelEditor::cNodeInfo &info : vFound)
				if(mlKindFilter == 0 || info.msKind == gvKinds[mlKindFilter]) vShown.push_back(&info);

			ImGuiListClipper clipper;
			clipper.Begin((int)vShown.size());
			if(mbScrollToSelected)
				for(size_t i = 0; i < vShown.size(); ++i)
					if(vShown[i]->msName == GetSelected()) clipper.IncludeItemByIndex((int)i);
			while(clipper.Step())
			{
				for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
				{
					const cLevelEditor::cNodeInfo &info = *vShown[i];
					ImGui::PushID(info.msName.c_str());
					ImGui::PushStyleColor(ImGuiCol_Text, KindColor(info.msKind));
					ImGui::TextUnformatted(info.msKind.substr(0, 1).c_str());
					ImGui::PopStyleColor();
					ImGui::SameLine();
					tString sLabel = (info.msObject != "" ? info.msObject : info.msName) + (info.mbEdited ? " *" : "");
					bool bSelected = info.msName == GetSelected();
					if(ImGui::Selectable(sLabel.c_str(), bSelected, ImGuiSelectableFlags_AllowDoubleClick))
					{
						Select(info.msName);
						if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) FocusSelected();
					}
					if(ImGui::IsItemHovered())
						ImGui::SetTooltip("%s (%s)%s%s", info.msName.c_str(), info.msKind.c_str(),
										  info.msSource != "" ? "\n" : "", info.msSource.c_str());
					if(bSelected && mbScrollToSelected)
					{
						ImGui::SetScrollHereY();
						mbScrollToSelected = false;
					}
					ImGui::PopID();
				}
			}
		}
		mbScrollToSelected = false;
	}
	ImGui::EndChild();
}

void cEditor::DrawOutlinerNode(const tString &asName)
{
	tStringVec vChildren;
	mpLevel->GetChildren(asName, vChildren);
	const cCachedNode *pNode = nullptr;
	for(const cCachedNode &node : mvNodes)
		if(node.msName == asName) pNode = &node;
	if(pNode == nullptr) return;

	bool bSelected = asName == GetSelected();
	ImGuiTreeNodeFlags lFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
								ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if(vChildren.empty()) lFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if(bSelected) lFlags |= ImGuiTreeNodeFlags_Selected;

	// Open down to what was picked in the view
	if(mbScrollToSelected && GetSelected() != "" && vChildren.empty() == false)
	{
		tStringVec vStack = vChildren;
		while(vStack.empty() == false)
		{
			tString sName = vStack.back();
			vStack.pop_back();
			if(sName == GetSelected())
			{
				ImGui::SetNextItemOpen(true);
				break;
			}
			tStringVec vMore;
			mpLevel->GetChildren(sName, vMore);
			vStack.insert(vStack.end(), vMore.begin(), vMore.end());
		}
	}

	ImGui::PushStyleColor(ImGuiCol_Text, KindColor(pNode->msKind));
	tString sLabel = pNode->msLabel + (pNode->mbEdited ? " *" : "") + "##" + asName;
	bool bOpen = ImGui::TreeNodeEx(sLabel.c_str(), lFlags);
	ImGui::PopStyleColor();
	if(ImGui::IsItemClicked() && ImGui::IsItemToggledOpen() == false) Select(asName);
	if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		Select(asName);
		FocusSelected();
	}
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s (%s)", asName.c_str(), pNode->msKind.c_str());
	if(bSelected && mbScrollToSelected) ImGui::SetScrollHereY();

	if(bOpen && vChildren.empty() == false)
	{
		for(const tString &sChild : vChildren) DrawOutlinerNode(sChild);
		ImGui::TreePop();
	}
}

//-----------------------------------------------------------------------

void cEditor::DrawMaps()
{
	ImGui::SetNextItemWidth(-1);
	ImGui::InputTextWithHint("##mapfilter", "filter", msMapFilter, sizeof(msMapFilter));
	tString sFilter = cString::ToLowerCase(msMapFilter);
	tString sCurrent = cString::ToLowerCase(cString::SetFileExt(mpLevel->GetMap(), ""));
	if(ImGui::BeginChild("maps", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 1.2f)))
	{
		for(const tString &sMap : mpInit->mpRefView->GetMaps())
		{
			tString sName = cString::SetFileExt(sMap, "");
			if(sFilter != "" && cString::ToLowerCase(sName).find(sFilter) == tString::npos) continue;
			bool bCurrent = cString::ToLowerCase(sName) == sCurrent;
			if(ImGui::Selectable(sName.c_str(), bCurrent, ImGuiSelectableFlags_AllowDoubleClick) &&
			   ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				msConfirmMap = sMap;
				RequestAction(eAction_OpenMap);
			}
		}
	}
	ImGui::EndChild();
	ImGui::TextDisabled("Double-click opens a map");
}

//-----------------------------------------------------------------------

void cEditor::DrawInspector(const ImVec2 &avPos, const ImVec2 &avSize)
{
	ImGui::SetNextWindowPos(avPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(avSize, ImGuiCond_Always);
	ImGuiWindowFlags lFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
							  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if(ImGui::Begin("Inspector##editor", nullptr, lFlags) == false)
	{
		ImGui::End();
		return;
	}
	const float fEm = ImGui::GetFontSize();
	tString sError;

	cLevelEditor::cNodeInfo info;
	if(GetSelected() != "" && mpLevel->GetNode(GetSelected(), info))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, KindColor(info.msKind));
		ImGui::TextWrapped("%s", (info.msObject != "" ? info.msObject : info.msName).c_str());
		ImGui::PopStyleColor();
		ImGui::TextDisabled("%s%s", info.msKind.c_str(), info.mbEdited ? ", changed" : "");
		if(ImGui::BeginTable("nodeinfo", 2))
		{
			ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
			auto Row = [](const char *asKey, const tString &asValue)
			{
				if(asValue == "") return;
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextDisabled("%s", asKey);
				ImGui::TableNextColumn();
				ImGui::TextWrapped("%s", asValue.c_str());
			};
			Row("Node", info.msName);
			Row("File", info.msSource);
			Row("Parent", info.msParent);
			ImGui::EndTable();
		}
		ImGui::Separator();

		// The transform: each drag is one step to undo
		cVector3f vPos = info.mvPos, vRot = info.mvRot, vScale = info.mvScale;
		bool bLight = info.msKind == "light";
		bool bArea = info.msKind == "area";
		ImGui::PushItemWidth(-fEm * 4.5f);
		bool bPos = ImGui::DragFloat3("Position", vPos.v, 0.01f, 0, 0, "%.3f");
		if(ImGui::IsItemActivated()) mpLevel->BeginUndo("Move " + (info.msObject != "" ? info.msObject : info.msName));
		if(ImGui::IsItemDeactivated()) mpLevel->EndUndo();
		bool bRot = ImGui::DragFloat3("Rotation", vRot.v, 0.5f, 0, 0, "%.1f");
		if(ImGui::IsItemActivated()) mpLevel->BeginUndo("Turn " + (info.msObject != "" ? info.msObject : info.msName));
		if(ImGui::IsItemDeactivated()) mpLevel->EndUndo();
		bool bScale = ImGui::DragFloat3(bLight ? "Scale*" : (bArea ? "Size*" : "Scale"), vScale.v, 0.005f, 0, 0, "%.3f");
		if(ImGui::IsItemActivated()) mpLevel->BeginUndo("Scale " + (info.msObject != "" ? info.msObject : info.msName));
		if(ImGui::IsItemDeactivated()) mpLevel->EndUndo();
		if(ImGui::IsItemHovered() && (bLight || bArea))
			ImGui::SetTooltip(bLight ? "The light's radius is x, as the map loader reads it"
									 : "The area's size, times the box it was made from");
		ImGui::PopItemWidth();
		if(bPos || bRot || bScale)
		{
			if(mpLevel->SetTransform(info.msName, bPos ? &vPos : nullptr, bRot ? &vRot : nullptr,
									 bScale ? &vScale : nullptr, sError) == false)
			{
				msMessage = sError;
				mfMessageTime = 4;
			}
		}

		if(ImGui::Button("Focus")) FocusSelected();
		ImGui::SameLine();
		ImGui::BeginDisabled(info.mbEdited == false);
		if(ImGui::Button("Reset")) mpLevel->ResetNode(info.msName);
		ImGui::EndDisabled();
		ImGui::SameLine();
		if(ImGui::Button("Copy name")) ImGui::SetClipboardText(info.msName.c_str());

		iLight3D *pLight = bLight ? mpLevel->GetNodeLight(info.msName) : nullptr;
		if(pLight && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			cColor color = pLight->GetDiffuseColor();
			ImVec4 vColor(color.r, color.g, color.b, 1);
			ImGui::ColorButton("##color", vColor, ImGuiColorEditFlags_NoTooltip);
			ImGui::SameLine();
			ImGui::Text("%.2f %.2f %.2f  x %.2f", color.r, color.g, color.b, color.a);
			ImGui::Text("Radius %.2f m, %s", pLight->GetFarAttenuation(),
						pLight->GetLightType() == eLight3DType_Spot ? "spot" : "point");
			ImGui::Text("Shadows %s, %s", pLight->GetCastShadows() ? "on" : "off",
						pLight->IsStatic() ? "static" : "dynamic");
		}
	}
	else
	{
		ImGui::TextDisabled("Nothing selected");
		ImGui::TextWrapped("Click something in the view, or a node in the Outliner.");
	}

	// The map, at the bottom
	ImGui::Dummy(ImVec2(0, fEm * 0.5f));
	if(ImGui::CollapsingHeader("Map", ImGuiTreeNodeFlags_DefaultOpen) && mpLevel->GetMap() != "")
	{
		ImGui::TextWrapped("%s", cString::GetFileName(mpLevel->GetMapFile()).c_str());
		if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s", mpLevel->GetMapFile().c_str());
		ImGui::Text("%d unsaved change(s)", mpLevel->GetEditCount());
		ImGui::BeginDisabled(mpLevel->GetEditCount() == 0);
		if(ImGui::Button("Save")) RequestAction(eAction_Save);
		ImGui::SameLine();
		if(ImGui::Button("Reset all")) RequestAction(eAction_ResetAll);
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(mpLevel->GetEditedMapFile() == "");
		if(ImGui::Button("Revert")) RequestAction(eAction_Revert);
		ImGui::EndDisabled();
		if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip("Removes the saved map: the game's (or an addon's) is used again");
		ImGui::TextDisabled("Saved to %s", mpInit->msEditDir != "" ? mpInit->msEditDir.c_str() : "(no edit folder)");
	}
	ImGui::End();
}

//-----------------------------------------------------------------------

void cEditor::DrawLog(const ImVec2 &avPos, const ImVec2 &avSize)
{
	ImGui::SetNextWindowPos(avPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(avSize, ImGuiCond_Always);
	ImGuiWindowFlags lFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
							  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
							  ImGuiWindowFlags_NoBringToFrontOnFocus;
	if(ImGui::Begin("Log##editor", nullptr, lFlags))
	{
		ImGui::Text("hpl.log");
		ImGui::SameLine();
		if(mlLogErrors > 0) ImGui::TextColored(ImVec4(1, 0.45f, 0.4f, 1), "%d error(s)", mlLogErrors);
		else ImGui::TextDisabled("no errors");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
		ImGui::InputTextWithHint("##logfilter", "filter", msLogFilter, sizeof(msLogFilter));
		ImGui::SameLine();
		ImGui::Checkbox("Errors only", &mbLogErrorsOnly);
		ImGui::SameLine();
		ImGui::Checkbox("Follow", &mbLogScroll);
		ImGui::SameLine();
		if(ImGui::SmallButton("Clear"))
		{
			mvLog.clear();
			mlLogErrors = 0;
		}
		if(msMessage != "" && mfMessageTime > 0)
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.6f, 0.9f, 1, 1), "%s", msMessage.c_str());
		}

		if(ImGui::BeginChild("logtext", ImVec2(0, 0), ImGuiChildFlags_Borders))
		{
			tString sFilter = cString::ToLowerCase(msLogFilter);
			std::vector<const tString *> vShown;
			vShown.reserve(mvLog.size());
			for(const tString &sLine : mvLog)
			{
				if(sFilter == "" && mbLogErrorsOnly == false)
				{
					vShown.push_back(&sLine);
					continue;
				}
				tString sLower = cString::ToLowerCase(sLine);
				if(sFilter != "" && sLower.find(sFilter) == tString::npos) continue;
				if(mbLogErrorsOnly && sLower.find("error") == tString::npos && sLower.find("warning") == tString::npos &&
				   sLower.find("couldn't") == tString::npos)
					continue;
				vShown.push_back(&sLine);
			}
			ImGuiListClipper clipper;
			clipper.Begin((int)vShown.size());
			while(clipper.Step())
			{
				for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
				{
					tString sLower = cString::ToLowerCase(*vShown[i]);
					ImVec4 vColor(0.82f, 0.82f, 0.82f, 1);
					if(sLower.find("error") != tString::npos || sLower.find("couldn't") != tString::npos)
						vColor = ImVec4(1, 0.45f, 0.4f, 1);
					else if(sLower.find("warning") != tString::npos) vColor = ImVec4(1, 0.8f, 0.35f, 1);
					else if(sLower.compare(0, 12, "level editor") == 0 || sLower.compare(0, 6, "editor") == 0)
						vColor = ImVec4(0.6f, 0.9f, 1, 1);
					ImGui::TextColored(vColor, "%s", vShown[i]->c_str());
				}
			}
			if(mbLogScroll && ImGui::GetScrollY() < ImGui::GetScrollMaxY() - 1) ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

//-----------------------------------------------------------------------

void cEditor::DrawPlayingNote()
{
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetFontSize() * 0.5f),
							ImGuiCond_Always, ImVec2(0.5f, 0));
	ImGui::SetNextWindowBgAlpha(0.6f);
	ImGui::Begin("##playing", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
				 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
	ImGui::Text("Playing %s.  F8: back to the editor%s", mpLevel->GetMap().c_str(),
				mbReloadOnStop ? " (the map is reloaded)" : "");
	ImGui::End();
}

//-----------------------------------------------------------------------

void cEditor::DrawConfirmPopup()
{
	if(mbOpenConfirm)
	{
		mbOpenConfirm = false;
		ImGui::OpenPopup("Confirm##editor");
	}
	if(ImGui::BeginPopupModal("Confirm##editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const char *pText = "";
		switch(mlConfirmAction)
		{
		case eAction_Revert: pText = "Remove the saved map from the edit folder and load the game's?"; break;
		case eAction_OpenMap: pText = "Open another map? Unsaved changes are lost."; break;
		case eAction_ResetAll: pText = "Undo all unsaved changes?"; break;
		case eAction_Quit: pText = "Quit? Unsaved changes are lost."; break;
		default: break;
		}
		ImGui::TextUnformatted(pText);
		if(mpLevel->GetEditCount() > 0 && mlConfirmAction != eAction_ResetAll)
			ImGui::TextDisabled("%d unsaved change(s)", mpLevel->GetEditCount());
		if(ImGui::Button("Yes", ImVec2(ImGui::GetFontSize() * 6, 0)))
		{
			DoAction(mlConfirmAction);
			mlConfirmAction = eAction_None;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("No", ImVec2(ImGui::GetFontSize() * 6, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			mlConfirmAction = eAction_None;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

//-----------------------------------------------------------------------

void cEditor::UpdateShortcuts()
{
	ImGuiIO &io = ImGui::GetIO();
	if(io.WantTextInput || mbFlying || ImGui::IsPopupOpen("Confirm##editor")) return;

	if(io.KeyCtrl)
	{
		if(ImGui::IsKeyPressed(ImGuiKey_Z, false))
		{
			if(io.KeyShift) Redo();
			else Undo();
		}
		if(ImGui::IsKeyPressed(ImGuiKey_Y, false)) Redo();
		if(ImGui::IsKeyPressed(ImGuiKey_S, false) && mpLevel->GetEditCount() > 0) RequestAction(eAction_Save);
		if(ImGui::IsKeyPressed(ImGuiKey_R, false)) RequestAction(eAction_Reload);
		return;
	}
	if(mlGizmoDrag >= 0) return;
	if(ImGui::IsKeyPressed(ImGuiKey_Q, false)) mTool = eTool_Select;
	if(ImGui::IsKeyPressed(ImGuiKey_W, false)) mTool = eTool_Move;
	if(ImGui::IsKeyPressed(ImGuiKey_E, false)) mTool = eTool_Rotate;
	if(ImGui::IsKeyPressed(ImGuiKey_R, false)) mTool = eTool_Scale;
	if(ImGui::IsKeyPressed(ImGuiKey_X, false)) mbLocalSpace = !mbLocalSpace;
	if(ImGui::IsKeyPressed(ImGuiKey_G, false)) mbSnap = !mbSnap;
	if(ImGui::IsKeyPressed(ImGuiKey_F, false)) FocusSelected();
	if(ImGui::IsKeyPressed(ImGuiKey_Escape, false)) Select("");
}

//-----------------------------------------------------------------------

void cEditor::DrawViewport()
{
	if(mpInit->mpGame->GetScene()->GetWorld3D() == nullptr ||
	   mpInit->mpGame->GetUpdater()->GetCurrentContainerName() != "Default")
		return;

	ImDrawList *pList = ImGui::GetBackgroundDrawList();
	UpdateCamera(ImGui::GetIO().DeltaTime);
	if(mbShowHelpers) DrawHelpers(pList);
	DrawSelection(pList);
	bool bGizmo = UpdateGizmo(pList);
	if(bGizmo == false) UpdateViewportInput();
	DrawStatus();
}

//-----------------------------------------------------------------------

void cEditor::UpdateCamera(float afTimeStep)
{
	ImGuiIO &io = ImGui::GetIO();
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	bool bOverView = io.WantCaptureMouse == false;
	if(mpInit->mpRefView->IsHoldingCamera() == false) return;

	cVector3f vPos;
	float fYaw, fPitch;
	mpInit->mpRefView->GetHeldCamera(vPos, fYaw, fPitch);
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	bool bChanged = false;

	// Right button: look and fly
	if(mbFlying == false && bOverView && mlGizmoDrag < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		mbFlying = true;
		pLowLevel->SetDebugPanelMouseCapture(true);
	}
	if(mbFlying && ImGui::IsMouseDown(ImGuiMouseButton_Right) == false)
	{
		mbFlying = false;
		pLowLevel->SetDebugPanelMouseCapture(false);
	}
	if(mbFlying)
	{
		cVector2f vMotion = pLowLevel->TakeDebugPanelMouseMotion();
		fYaw -= vMotion.x * 0.0025f;
		fPitch = cMath::Clamp(fPitch - vMotion.y * 0.0025f, cMath::ToRad(-89), cMath::ToRad(89));
		if(io.MouseWheel != 0) mfSpeed = cMath::Clamp(mfSpeed * std::pow(1.25f, io.MouseWheel), 0.2f, 50.0f);

		// The direction the view has after the turn
		pCamera->SetYaw(fYaw);
		pCamera->SetPitch(fPitch);
		cVector3f vMove(0, 0, 0);
		if(ImGui::IsKeyDown(ImGuiKey_W)) vMove += pCamera->GetForward();
		if(ImGui::IsKeyDown(ImGuiKey_S)) vMove -= pCamera->GetForward();
		if(ImGui::IsKeyDown(ImGuiKey_D)) vMove += pCamera->GetRight();
		if(ImGui::IsKeyDown(ImGuiKey_A)) vMove -= pCamera->GetRight();
		if(ImGui::IsKeyDown(ImGuiKey_E) || ImGui::IsKeyDown(ImGuiKey_Space)) vMove.y += 1;
		if(ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) vMove.y -= 1;
		float fSpeed = mfSpeed * (io.KeyShift ? 4.0f : 1.0f);
		vPos += vMove * fSpeed * afTimeStep;
		bChanged = true;
	}
	else if(bOverView)
	{
		// The wheel: forward and back
		if(io.MouseWheel != 0)
		{
			vPos += pCamera->GetForward() * io.MouseWheel * cMath::Max(0.25f, mfSpeed * 0.15f);
			bChanged = true;
		}
		// Middle button: sideways and up
		if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) mbPanning = true;
		// Alt and the left button: around the selection
		if(io.KeyAlt && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			mbOrbiting = true;
			cVector3f vCenter;
			float fRadius;
			if(GetSelected() != "" && mpLevel->GetNodeBounds(GetSelected(), vCenter, fRadius))
				mvOrbitCenter = vCenter;
			else
				mvOrbitCenter = vPos + pCamera->GetForward() * 4.0f;
		}
	}
	if(mbPanning)
	{
		if(ImGui::IsMouseDown(ImGuiMouseButton_Middle) == false) mbPanning = false;
		else
		{
			float fScale = cMath::Max(0.002f, mfSpeed * 0.0015f);
			vPos -= pCamera->GetRight() * io.MouseDelta.x * fScale;
			vPos += pCamera->GetUp() * io.MouseDelta.y * fScale;
			bChanged = true;
		}
	}
	if(mbOrbiting)
	{
		if(ImGui::IsMouseDown(ImGuiMouseButton_Left) == false) mbOrbiting = false;
		else
		{
			float fDist = (vPos - mvOrbitCenter).Length();
			fYaw -= io.MouseDelta.x * 0.005f;
			fPitch = cMath::Clamp(fPitch - io.MouseDelta.y * 0.005f, cMath::ToRad(-89), cMath::ToRad(89));
			pCamera->SetYaw(fYaw);
			pCamera->SetPitch(fPitch);
			vPos = mvOrbitCenter - pCamera->GetForward() * fDist;
			bChanged = true;
		}
	}

	if(bChanged) mpInit->mpRefView->HoldCamera(vPos, fYaw, fPitch);
}

//-----------------------------------------------------------------------

void cEditor::UpdateViewportInput()
{
	ImGuiIO &io = ImGui::GetIO();
	bool bOverView = io.WantCaptureMouse == false;
	if(mbFlying || mbOrbiting || mbPanning) return;

	// A click selects, unless the mouse moved on: then it was a drag
	if(bOverView && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && io.KeyAlt == false)
	{
		mbClickPending = true;
		mvClickMouse = io.MousePos;
	}
	if(mbClickPending && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		mbClickPending = false;
		if(Dist2(io.MousePos, mvClickMouse) < 25)
		{
			tString sPicked = PickAt(io.MousePos);
			Select(sPicked);
			if(sPicked == "")
			{
				msMessage = "Nothing of the map's file there";
				mfMessageTime = 2;
			}
		}
	}
	if(bOverView && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && GetSelected() != "") FocusSelected();
}

//-----------------------------------------------------------------------

void cEditor::DrawHelpers(ImDrawList *apList)
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	ImVec2 vMouse = ImGui::GetIO().MousePos;
	for(const cCachedNode &node : mvNodes)
	{
		if(IsHelperKind(node.msKind) == false) continue;
		if((node.mvPos - pCamera->GetPosition()).Length() > 60) continue;
		ImVec2 vScreen;
		if(ToScreen(node.mvPos, vScreen) == false) continue;
		ImU32 lColor = KindColor(node.msKind);
		bool bSelected = node.msName == GetSelected();
		float fRadius = bSelected ? 6.0f : 4.5f;
		apList->AddCircleFilled(vScreen, fRadius + 1.5f, IM_COL32(0, 0, 0, 160));
		apList->AddCircleFilled(vScreen, fRadius, lColor);
		if(bSelected || Dist2(vScreen, vMouse) < 81)
			apList->AddText(ImVec2(vScreen.x + 8, vScreen.y - 7), lColor, node.msLabel.c_str());
	}
}

//-----------------------------------------------------------------------

void cEditor::DrawSelection(ImDrawList *apList)
{
	if(GetSelected() == "") return;
	cVector3f vMin, vMax;
	if(mpLevel->GetNodeBox(GetSelected(), vMin, vMax) == false) return;

	// The box: its edges whose ends are both in front
	cVector3f vCorners[8];
	for(int i = 0; i < 8; ++i)
		vCorners[i] = cVector3f((i & 1) ? vMax.x : vMin.x, (i & 2) ? vMax.y : vMin.y, (i & 4) ? vMax.z : vMin.z);
	const int vEdges[12][2] = {{0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7}};
	ImU32 lColor = IM_COL32(255, 200, 60, 230);
	for(const auto &edge : vEdges)
	{
		ImVec2 vA, vB;
		if(ToScreen(vCorners[edge[0]], vA) && ToScreen(vCorners[edge[1]], vB)) apList->AddLine(vA, vB, lColor, 1.5f);
	}

	// A light's reach
	iLight3D *pLight = mbShowLightRadius ? mpLevel->GetNodeLight(GetSelected()) : nullptr;
	if(pLight)
	{
		cVector3f vCenter = pLight->GetLightPosition();
		float fRadius = pLight->GetFarAttenuation();
		for(int lAxis = 0; lAxis < 3; ++lAxis)
		{
			ImVec2 vPoints[49];
			int lCount = 0;
			bool bAll = true;
			for(int i = 0; i <= 48; ++i)
			{
				float fAngle = i * k2Pif / 48;
				cVector3f vOffset(0, 0, 0);
				vOffset.v[(lAxis + 1) % 3] = std::cos(fAngle) * fRadius;
				vOffset.v[(lAxis + 2) % 3] = std::sin(fAngle) * fRadius;
				if(ToScreen(vCenter + vOffset, vPoints[lCount])) ++lCount;
				else bAll = false;
			}
			if(bAll) apList->AddPolyline(vPoints, lCount, IM_COL32(250, 220, 90, 110), ImDrawFlags_None, 1.0f);
		}
	}
}

//-----------------------------------------------------------------------

void cEditor::DrawStatus()
{
	ImGuiIO &io = ImGui::GetIO();
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	cVector3f vPos = pCamera->GetPosition();
	char sText[256];
	snprintf(sText, sizeof(sText), "%.2f %.2f %.2f   %.1f m/s%s%s   %.0f fps", vPos.x, vPos.y, vPos.z, mfSpeed,
			 mbSimulate ? "   simulating" : "   still", mbSnap ? "   snap" : "", io.Framerate);
	float fBottom = io.DisplaySize.y - (mbShowLog ? mfLogHeight : 0) - ImGui::GetFontSize() * 1.6f;
	ImVec2 vText(mfLeftShown + ImGui::GetFontSize() * 0.6f, fBottom);
	ImDrawList *pList = ImGui::GetBackgroundDrawList();
	ImVec2 vSize = ImGui::CalcTextSize(sText);
	pList->AddRectFilled(ImVec2(vText.x - 4, vText.y - 2), ImVec2(vText.x + vSize.x + 4, vText.y + vSize.y + 2),
						 IM_COL32(0, 0, 0, 140));
	pList->AddText(vText, IM_COL32(220, 220, 220, 255), sText);
}

//-----------------------------------------------------------------------

cVector3f cEditor::GizmoAxis(int alAxis)
{
	cVector3f vAxis(0, 0, 0);
	vAxis.v[alAxis] = 1;
	// Scaling is along the node's own axes
	if(mbLocalSpace || mTool == eTool_Scale)
	{
		cMatrixf mtxWorld;
		cVector3f vScale;
		if(mlGizmoDrag >= 0) mtxWorld = m_mtxDragStart;
		else if(mpLevel->GetWorld(GetSelected(), mtxWorld, vScale) == false) return vAxis;
		vAxis = cVector3f(mtxWorld.m[0][alAxis], mtxWorld.m[1][alAxis], mtxWorld.m[2][alAxis]);
		vAxis.Normalise();
	}
	return vAxis;
}

int cEditor::GizmoHit(const ImVec2 &avMouse)
{
	cMatrixf mtxWorld;
	cVector3f vScale;
	if(mpLevel->GetWorld(GetSelected(), mtxWorld, vScale) == false) return -1;
	cVector3f vOrigin = mtxWorld.GetTranslation();
	ImVec2 vCenter;
	if(ToScreen(vOrigin, vCenter) == false) return -1;

	const float fPick = 7;
	if(Dist2(avMouse, vCenter) < 10 * 10 && mTool != eTool_Rotate) return 3;
	float fBest = fPick;
	int lBest = -1;
	for(int lAxis = 0; lAxis < 3; ++lAxis)
	{
		cVector3f vAxis = GizmoAxis(lAxis);
		if(mTool == eTool_Rotate)
		{
			// The ring around the axis
			cVector3f vU = cMath::Vector3Cross(vAxis, std::fabs(vAxis.y) < 0.9f ? cVector3f(0, 1, 0) : cVector3f(1, 0, 0));
			vU.Normalise();
			cVector3f vV = cMath::Vector3Cross(vAxis, vU);
			ImVec2 vPrev;
			bool bPrev = false;
			for(int i = 0; i <= 48; ++i)
			{
				float fAngle = i * k2Pif / 48;
				ImVec2 vPoint;
				bool bIn = ToScreen(vOrigin + (vU * std::cos(fAngle) + vV * std::sin(fAngle)) * mfGizmoLength, vPoint);
				if(bIn && bPrev)
				{
					float fDist = SegmentDistance(avMouse, vPrev, vPoint);
					if(fDist < fBest)
					{
						fBest = fDist;
						lBest = lAxis;
					}
				}
				vPrev = vPoint;
				bPrev = bIn;
			}
		}
		else
		{
			ImVec2 vEnd;
			if(ToScreen(vOrigin + vAxis * mfGizmoLength, vEnd) == false) continue;
			float fDist = SegmentDistance(avMouse, vCenter, vEnd);
			if(fDist < fBest)
			{
				fBest = fDist;
				lBest = lAxis;
			}
		}
	}
	return lBest;
}

//-----------------------------------------------------------------------

void cEditor::DrawGizmo(ImDrawList *apList, int alHover)
{
	cMatrixf mtxWorld;
	cVector3f vScale;
	if(mpLevel->GetWorld(GetSelected(), mtxWorld, vScale) == false) return;
	cVector3f vOrigin = mtxWorld.GetTranslation();
	ImVec2 vCenter;
	if(ToScreen(vOrigin, vCenter) == false) return;

	int lActive = mlGizmoDrag >= 0 ? mlGizmoDrag : alHover;
	for(int lAxis = 0; lAxis < 3; ++lAxis)
	{
		ImU32 lColor = lActive == lAxis ? gHoverColor : gvAxisColors[lAxis];
		cVector3f vAxis = GizmoAxis(lAxis);
		if(mTool == eTool_Rotate)
		{
			cVector3f vU = cMath::Vector3Cross(vAxis, std::fabs(vAxis.y) < 0.9f ? cVector3f(0, 1, 0) : cVector3f(1, 0, 0));
			vU.Normalise();
			cVector3f vV = cMath::Vector3Cross(vAxis, vU);
			ImVec2 vPoints[65];
			int lCount = 0;
			for(int i = 0; i <= 64; ++i)
			{
				float fAngle = i * k2Pif / 64;
				if(ToScreen(vOrigin + (vU * std::cos(fAngle) + vV * std::sin(fAngle)) * mfGizmoLength, vPoints[lCount]))
					++lCount;
			}
			if(lCount > 1) apList->AddPolyline(vPoints, lCount, lColor, ImDrawFlags_None, lActive == lAxis ? 3.0f : 2.0f);
			continue;
		}
		ImVec2 vEnd;
		if(ToScreen(vOrigin + vAxis * mfGizmoLength, vEnd) == false) continue;
		apList->AddLine(vCenter, vEnd, lColor, lActive == lAxis ? 3.5f : 2.5f);
		if(mTool == eTool_Scale)
			apList->AddRectFilled(ImVec2(vEnd.x - 5, vEnd.y - 5), ImVec2(vEnd.x + 5, vEnd.y + 5), lColor);
		else
		{
			// An arrow head
			ImVec2 vDir(vEnd.x - vCenter.x, vEnd.y - vCenter.y);
			float fLen = std::sqrt(vDir.x * vDir.x + vDir.y * vDir.y);
			if(fLen > 1)
			{
				vDir = ImVec2(vDir.x / fLen, vDir.y / fLen);
				ImVec2 vSide(-vDir.y * 5, vDir.x * 5);
				ImVec2 vTip(vEnd.x + vDir.x * 12, vEnd.y + vDir.y * 12);
				apList->AddTriangleFilled(vTip, ImVec2(vEnd.x + vSide.x, vEnd.y + vSide.y),
										  ImVec2(vEnd.x - vSide.x, vEnd.y - vSide.y), lColor);
			}
		}
	}
	if(mTool != eTool_Rotate)
	{
		ImU32 lColor = lActive == 3 ? gHoverColor : IM_COL32(235, 235, 235, 255);
		apList->AddCircle(vCenter, 7, lColor, 0, 2.0f);
	}
}

//-----------------------------------------------------------------------

bool cEditor::UpdateGizmo(ImDrawList *apList)
{
	ImGuiIO &io = ImGui::GetIO();
	if(GetSelected() == "" || mTool == eTool_Select) return false;
	cMatrixf mtxWorld;
	cVector3f vScale;
	if(mpLevel->GetWorld(GetSelected(), mtxWorld, vScale) == false) return false;

	// About a hundred pixels long, wherever it is
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	cVector3f vOrigin = mtxWorld.GetTranslation();
	float fDist = cMath::Max(0.05f, (vOrigin - pCamera->GetPosition()).Length());
	float fPixelsPerMetre = io.DisplaySize.y / (2 * fDist * std::tan(pCamera->GetScaledFOV() * 0.5f));
	if(mlGizmoDrag < 0) mfGizmoLength = ImGui::GetFontSize() * 6.5f / fPixelsPerMetre;

	bool bOverView = io.WantCaptureMouse == false;
	int lHover = (mlGizmoDrag < 0 && bOverView && mbFlying == false && mbOrbiting == false && mbPanning == false)
				 ? GizmoHit(io.MousePos) : -1;

	cVector3f vRayStart, vRayDir;
	MouseRay(io.MousePos, vRayStart, vRayDir);

	// Starts a drag
	if(lHover >= 0 && io.KeyAlt == false && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		mlGizmoDrag = lHover;
		m_mtxDragStart = mtxWorld;
		mvDragScale = vScale;
		mvDragMouse = io.MousePos;
		mvDragPlanePoint = vOrigin;
		mfDragAxisStart = 0;
		const char *vNames[] = {"Move ", "Turn ", "Scale "};
		cLevelEditor::cNodeInfo info;
		mpLevel->GetNode(GetSelected(), info);
		mpLevel->BeginUndo(vNames[mTool - eTool_Move] + (info.msObject != "" ? info.msObject : info.msName));

		if(mTool != eTool_Rotate && mlGizmoDrag < 3)
			ClosestOnLine(vOrigin, GizmoAxis(mlGizmoDrag), vRayStart, vRayDir, mfDragAxisStart);
		else if(mTool == eTool_Move)
			RayPlane(vRayStart, vRayDir, vOrigin, pCamera->GetForward(), mvDragPlanePoint);
		else if(mTool == eTool_Rotate)
		{
			if(RayPlane(vRayStart, vRayDir, vOrigin, GizmoAxis(mlGizmoDrag), mvDragPlanePoint) == false)
				mvDragPlanePoint = vOrigin;
		}
	}

	if(mlGizmoDrag >= 0)
	{
		if(ImGui::IsMouseDown(ImGuiMouseButton_Left) == false)
		{
			mlGizmoDrag = -1;
			mpLevel->EndUndo();
			mbClickPending = false;
		}
		else
		{
			cVector3f vStartPos = m_mtxDragStart.GetTranslation();
			cMatrixf mtxNew = m_mtxDragStart;
			cVector3f vNewScale = mvDragScale;
			bool bApply = false;

			if(mTool == eTool_Move)
			{
				cVector3f vNewPos = vStartPos;
				if(mlGizmoDrag < 3)
				{
					cVector3f vAxis = GizmoAxis(mlGizmoDrag);
					float t;
					if(ClosestOnLine(vStartPos, vAxis, vRayStart, vRayDir, t))
					{
						float fDelta = t - mfDragAxisStart;
						if(mbSnap) fDelta = Snap(fDelta, mfSnapMove);
						vNewPos = vStartPos + vAxis * fDelta;
						bApply = true;
					}
				}
				else
				{
					cVector3f vHit;
					if(RayPlane(vRayStart, vRayDir, vStartPos, pCamera->GetForward(), vHit))
					{
						cVector3f vDelta = vHit - mvDragPlanePoint;
						if(mbSnap)
							for(int i = 0; i < 3; ++i) vDelta.v[i] = Snap(vDelta.v[i], mfSnapMove);
						vNewPos = vStartPos + vDelta;
						bApply = true;
					}
				}
				mtxNew.SetTranslation(vNewPos);
			}
			else if(mTool == eTool_Rotate)
			{
				cVector3f vAxis = GizmoAxis(mlGizmoDrag);
				cVector3f vHit;
				if(RayPlane(vRayStart, vRayDir, vStartPos, vAxis, vHit))
				{
					cVector3f vFrom = mvDragPlanePoint - vStartPos, vTo = vHit - vStartPos;
					if(vFrom.Length() > 1e-4f && vTo.Length() > 1e-4f)
					{
						float fAngle = std::atan2(cMath::Vector3Dot(cMath::Vector3Cross(vFrom, vTo), vAxis),
												  cMath::Vector3Dot(vFrom, vTo));
						if(mbSnap) fAngle = cMath::ToRad(Snap(cMath::ToDeg(fAngle), mfSnapAngle));
						mtxNew = cMath::MatrixMul(AxisRotation(vAxis, fAngle), m_mtxDragStart.GetRotation());
						mtxNew.SetTranslation(vStartPos);
						bApply = true;
					}
				}
			}
			else if(mTool == eTool_Scale)
			{
				float fFactor = 1;
				if(mlGizmoDrag < 3)
				{
					float t;
					if(ClosestOnLine(vStartPos, GizmoAxis(mlGizmoDrag), vRayStart, vRayDir, t))
						fFactor = 1 + (t - mfDragAxisStart) / cMath::Max(1e-3f, mfGizmoLength);
				}
				else
					fFactor = 1 + (io.MousePos.x - mvDragMouse.x - (io.MousePos.y - mvDragMouse.y)) / 150.0f;
				fFactor = cMath::Max(0.01f, fFactor);
				for(int i = 0; i < 3; ++i)
				{
					if(mlGizmoDrag < 3 && i != mlGizmoDrag) continue;
					float fValue = mvDragScale.v[i] * fFactor;
					if(mbSnap) fValue = cMath::Max(mfSnapScale, Snap(fValue, mfSnapScale));
					vNewScale.v[i] = fValue;
				}
				bApply = true;
			}

			if(bApply)
			{
				tString sError;
				mpLevel->SetWorld(GetSelected(), mtxNew, vNewScale, sError);
			}
		}
	}

	DrawGizmo(apList, lHover);
	return mlGizmoDrag >= 0 || lHover >= 0;
}

//-----------------------------------------------------------------------
