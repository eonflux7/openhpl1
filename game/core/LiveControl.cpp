/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/LiveControl.h"
#include "core/Editor.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/RefView.h"
#include "core/LevelEditor.h"
#include "core/GameLauncher.h"
#include "player/Player.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

#ifdef WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// Real time a map may take to load or reload before the command fails
static const float kfLoadTimeout = 180.0f;
// Steps after a map has loaded before it counts as settled: bodies come
// to rest, lights fade in
static const int klMapSettleFrames = 30;
static const int klReloadSettleFrames = 10;
static const int klShotSettleFrames = 10;

//-----------------------------------------------------------------------

// Words separated by spaces; "quoted words" may have spaces in them
static tStringVec SplitArgs(const tString &asLine)
{
	tStringVec vArgs;
	tString sArg;
	bool bQuoted = false, bHasArg = false;
	for(char c : asLine)
	{
		if(c == '"')
		{
			bQuoted = !bQuoted;
			bHasArg = true;
		}
		else if((c == ' ' || c == '\t') && bQuoted == false)
		{
			if(bHasArg) vArgs.push_back(sArg);
			sArg = "";
			bHasArg = false;
		}
		else
		{
			sArg += c;
			bHasArg = true;
		}
	}
	if(bHasArg) vArgs.push_back(sArg);
	return vArgs;
}

static bool ToFloat(const tString &asText, float &afValue)
{
	char *pEnd = nullptr;
	afValue = strtof(asText.c_str(), &pEnd);
	return pEnd && pEnd != asText.c_str() && *pEnd == 0;
}

// Three numbers from avArgs[alFirst...]
static bool ToVector(const tStringVec &avArgs, size_t alFirst, cVector3f &avVec)
{
	if(alFirst + 3 > avArgs.size()) return false;
	for(int i = 0; i < 3; ++i)
		if(ToFloat(avArgs[alFirst + i], avVec.v[i]) == false) return false;
	return true;
}

static tString Vec(const cVector3f &avVec)
{
	char sText[96];
	snprintf(sText, sizeof(sText), "%.4f %.4f %.4f", avVec.x, avVec.y, avVec.z);
	return sText;
}

//-----------------------------------------------------------------------

cLiveControl::cLiveControl(cInit *apInit, const tString &asDir) : iUpdateable("LiveControl")
{
	mpInit = apInit;
	msDir = asDir;
	mfScanTime = 0;
	mlLine = 0;
	mbFixedStep = false;
	mlSeededLoad = -1;
	mWait = eWait_None;
	mlFrames = 0;
	mfWaited = 0;
	mlLoadCount = 0;
	mlFailCount = 0;
	mbShot2D = false;
	mbPanelWasOpen = false;
	mbEditorUiWasHidden = false;
	mbSkippedDrawing = false;
	mbQuit = false;

	// Commands and answers of an earlier run are not this one's
	std::error_code err;
	fs::create_directories(fs::u8path(msDir), err);
	for(fs::directory_iterator it(fs::u8path(msDir), err), end; it != end; it.increment(err))
	{
		tString sExt = it->path().extension().u8string();
		if(sExt == ".cmd" || sExt == ".out" || sExt == ".tmp") fs::remove(it->path(), err);
	}

	std::ofstream ready(fs::u8path(msDir) / "ready");
	ready << "pid " << getpid() << "\n";
	ready << "game " << cGameLauncher::GetGameKey(cInit::GetGameId()) << "\n";
	ready << "editdir " << mpInit->msEditDir << "\n";
	Log("Live control: commands from '%s'\n", msDir.c_str());
}

cLiveControl::~cLiveControl()
{
	std::error_code err;
	fs::remove(fs::u8path(msDir) / "ready", err);
}

//-----------------------------------------------------------------------

bool cLiveControl::IsRequested()
{
	return GetDir() != "";
}

tString cLiveControl::GetDir()
{
	const char *pDir = SDL_getenv("OPENHPL_CONTROL");
	return pDir ? tString(pDir) : tString("");
}

//-----------------------------------------------------------------------

void cLiveControl::Reset()
{
	// Changing map resets every updateable; a command waits on
}

//-----------------------------------------------------------------------

void cLiveControl::Update(float afTimeStep)
{
	if(mbQuit) return;

	// Same map, same random numbers, for frames that can be compared
	int lLoad = mpInit->mpMapHandler->GetLoadCount();
	if(mbFixedStep && lLoad != mlSeededLoad) cMath::Randomize(1);
	mlSeededLoad = lLoad;

	if(msId != "")
	{
		if(Continue(afTimeStep)) RunLines();
		return;
	}

	// The folder is looked at 20 times a second
	mfScanTime += afTimeStep;
	if(mfScanTime < 0.05f) return;
	mfScanTime = 0;

	if(ReadNextCommand()) RunLines();
}

//-----------------------------------------------------------------------

bool cLiveControl::ReadNextCommand()
{
	std::error_code err;
	tStringVec vFiles;
	for(fs::directory_iterator it(fs::u8path(msDir), err), end; it != end; it.increment(err))
	{
		if(it->path().extension() == ".cmd") vFiles.push_back(it->path().u8string());
	}
	if(vFiles.empty()) return false;
	std::sort(vFiles.begin(), vFiles.end());

	fs::path file = fs::u8path(vFiles[0]);
	mvLines.clear();
	{
		std::ifstream in(file);
		tString sLine;
		while(std::getline(in, sLine))
		{
			while(sLine != "" && (sLine.back() == '\r' || sLine.back() == '\n')) sLine.pop_back();
			if(sLine != "") mvLines.push_back(sLine);
		}
	}
	fs::remove(file, err);

	msId = file.stem().u8string();
	mlLine = 0;
	mOut.str("");
	mOut.clear();
	msError = "";
	mWait = eWait_None;
	if(mvLines.empty()) Fail("empty command");
	return true;
}

//-----------------------------------------------------------------------

void cLiveControl::RunLines()
{
	// The lines of a file run one after the other, without a frame between
	// one that is done and the next
	while(msError == "" && mlLine < mvLines.size())
	{
		const tString &sLine = mvLines[mlLine++];
		Log("Live control: %s: %s\n", msId.c_str(), sLine.c_str());
		mWait = eWait_None;
		mfWaited = 0;

		tStringVec vArgs = SplitArgs(sLine);
		if(vArgs.empty()) continue;
		// A script line is passed on as it is
		if(vArgs[0] == "script")
		{
			tString sRest = cString::Sub(sLine, (int)(sLine.find("script") + 6));
			while(sRest != "" && sRest[0] == ' ') sRest.erase(0, 1);
			vArgs.resize(1);
			vArgs.push_back(sRest);
		}
		if(Start(vArgs) == false) return; // Continue() goes on with it
	}
	Answer();
	if(mbQuit) mpInit->mpGame->Exit();
}

//-----------------------------------------------------------------------

bool cLiveControl::InWorld()
{
	return mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default" &&
		   mpInit->mpGame->GetScene()->GetWorld3D() != nullptr &&
		   mpInit->mpMapHandler->GetCurrentMapName() != "";
}

//-----------------------------------------------------------------------

void cLiveControl::WriteNode(const tString &asName)
{
	cLevelEditor::cNodeInfo info;
	if(mpInit->mpLevelEditor->GetNode(asName, info) == false) return;
	mOut << info.msName << "\t" << info.msKind << "\tpos " << Vec(info.mvPos) << "\trot " << Vec(info.mvRot)
		 << "\tscale " << Vec(info.mvScale);
	if(info.msObject != "") mOut << "\tname " << info.msObject;
	if(info.msSource != "") mOut << "\tfile " << info.msSource;
	if(info.msParent != "") mOut << "\tparent " << info.msParent;
	if(info.mbEdited) mOut << "\tedited";
	mOut << "\n";
}

//-----------------------------------------------------------------------

bool cLiveControl::Start(const tStringVec &avArgs)
{
	const tString &sCmd = avArgs[0];
	cRefView *pView = mpInit->mpRefView;
	cLevelEditor *pEditor = mpInit->mpLevelEditor;
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	tString sError;

	auto NeedWorld = [this]()
	{
		if(InWorld()) return true;
		Fail("no map is being played (in '" + mpInit->mpGame->GetUpdater()->GetCurrentContainerName() + "')");
		return false;
	};
	auto NeedData = [&]()
	{
		if(NeedWorld() == false) return false;
		if(pEditor->Prepare(sError)) return true;
		Fail(sError);
		return false;
	};

	if(sCmd == "status")
	{
		mOut << "game " << cGameLauncher::GetGameKey(cInit::GetGameId()) << "\n";
		mOut << "state " << mpInit->mpGame->GetUpdater()->GetCurrentContainerName() << "\n";
		mOut << "map " << mpInit->mpMapHandler->GetCurrentMapName() << "\n";
		if(pEditor->GetMapFile() != "") mOut << "mapfile " << pEditor->GetMapFile() << "\n";
		mOut << "camera " << Vec(pCamera->GetPosition()) << " " << pCamera->GetYaw() << " " << pCamera->GetPitch()
			 << "\n";
		mOut << "held " << (pView->IsHoldingCamera() ? 1 : 0) << "\n";
		mOut << "edits " << pEditor->GetEditCount() << "\n";
		mOut << "editdir " << mpInit->msEditDir << "\n";
		cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
		iPhysicsWorld *pPhysics = pWorld ? pWorld->GetPhysicsWorld() : nullptr;
		if(pPhysics)
		{
			// Bodies with no mass do not move; the others move unless they
			// rest (sleeping or frozen) or are disabled
			int lBodies = 0, lStatic = 0, lMoving = 0;
			cPhysicsBodyIterator it = pPhysics->GetBodyIterator();
			while(it.HasNext())
			{
				iPhysicsBody *pBody = it.Next();
				++lBodies;
				if(pBody->GetMass() == 0) ++lStatic;
				else if(pBody->GetEnabled() && !pBody->IsSleeping() && !pBody->IsFrozen()) ++lMoving;
			}
			mOut << "bodies " << lBodies << " static " << lStatic << " moving " << lMoving << "\n";
		}
		tString sEdited = pEditor->GetEditedMapFile();
		if(sEdited != "") mOut << "editedmap " << sEdited << "\n";
		if(mpInit->mpEditor)
		{
			mOut << "editor playing " << (mpInit->mpEditor->IsPlaying() ? 1 : 0) << " simulating "
				 << (mpInit->mpEditor->IsSimulating() ? 1 : 0) << "\n";
			mOut << "selected " << mpInit->mpEditor->GetSelected() << "\n";
			mOut << "undo " << (pEditor->CanUndo() ? pEditor->GetUndoLabel() : tString("")) << "\n";
			mOut << "redo " << (pEditor->CanRedo() ? pEditor->GetRedoLabel() : tString("")) << "\n";
		}
		return true;
	}
	if(sCmd == "maps")
	{
		tString sFilter = avArgs.size() > 1 ? cString::ToLowerCase(avArgs[1]) : "";
		for(const tString &sFile : pView->GetMaps())
		{
			tString sName = cString::ToLowerCase(cString::SetFileExt(sFile, ""));
			if(sFilter == "" || sName.find(sFilter) != tString::npos) mOut << sName << "\n";
		}
		return true;
	}
	if(sCmd == "map")
	{
		if(avArgs.size() < 2) return Fail("map <name> [start]");
		const tString &sContainer = mpInit->mpGame->GetUpdater()->GetCurrentContainerName();
		if(sContainer != "Default" && sContainer != "MainMenu")
			return Fail("can't change map from '" + sContainer + "'");
		if(pView->IsMapPending()) return Fail("a map is being loaded");
		pView->ReleaseCamera();
		pView->RequestMap(avArgs[1], avArgs.size() > 2 ? avArgs[2] : "");
		msWaitMap = cString::ToLowerCase(cString::SetFileExt(avArgs[1], ""));
		mlLoadCount = mpInit->mpMapHandler->GetLoadCount();
		mlFrames = -1;
		mWait = eWait_Map;
		return false;
	}
	if(sCmd == "reload")
	{
		if(NeedWorld() == false) return true;
		mlLoadCount = mpInit->mpMapHandler->GetLoadCount();
		mlFailCount = mpInit->mpMapHandler->GetReloadFailCount();
		mpInit->mpMapHandler->RequestReload();
		mlFrames = -1;
		mWait = eWait_Reload;
		return false;
	}
	if(sCmd == "views")
	{
		if(NeedWorld() == false) return true;
		for(const cRefShots::cView &view : pView->GetViews())
		{
			const char *pKind = view.mbManual ? "viewpoint" : view.mbStartPos ? "start" : "light";
			mOut << view.msName << "\t" << pKind << "\t" << Vec(view.mvPos);
			if(view.mbManual)
				mOut << "\t" << view.mfYaw << " " << view.mfPitch << "\tflashlight " << (view.mbFlashLight ? 1 : 0);
			mOut << "\n";
		}
		return true;
	}
	if(sCmd == "view")
	{
		if(avArgs.size() < 2) return Fail("view <name>");
		if(NeedWorld() == false) return true;
		for(const cRefShots::cView &view : pView->GetViews())
		{
			if(view.msName != avArgs[1]) continue;
			if(view.mbStartPos)
			{
				pView->ReleaseCamera();
				mpInit->mpPlayer->SetStartPos(view.msName);
			}
			else
			{
				pView->HoldCamera(view.mvPos, view.mbManual ? view.mfYaw : 0, view.mbManual ? view.mfPitch : 0);
				if(view.mbManual) pView->SetOption("flashlight", view.mbFlashLight);
			}
			mOut << "camera " << Vec(pCamera->GetPosition()) << " " << pCamera->GetYaw() << " "
				 << pCamera->GetPitch() << "\n";
			return true;
		}
		return Fail("no place '" + avArgs[1] + "' in this map (see views)");
	}
	if(sCmd == "camera")
	{
		if(avArgs.size() > 1)
		{
			if(NeedWorld() == false) return true;
			cVector3f vPos;
			float fYaw = pCamera->GetYaw(), fPitch = pCamera->GetPitch();
			if(ToVector(avArgs, 1, vPos) == false ||
				(avArgs.size() > 4 && (avArgs.size() < 6 || ToFloat(avArgs[4], fYaw) == false ||
				 ToFloat(avArgs[5], fPitch) == false)))
				return Fail("camera [x y z [yaw pitch]]");
			pView->HoldCamera(vPos, fYaw, fPitch);
		}
		mOut << "camera " << Vec(pCamera->GetPosition()) << " " << pCamera->GetYaw() << " " << pCamera->GetPitch()
			 << "\n";
		return true;
	}
	if(sCmd == "look")
	{
		const char *pUsage = "look <node> [distance [yaw pitch]]";
		if(avArgs.size() < 2) return Fail(pUsage);
		if(NeedData() == false) return true;
		cVector3f vCenter;
		float fRadius = 0;
		if(pEditor->GetNodeBounds(avArgs[1], vCenter, fRadius) == false)
			return Fail("no node '" + avArgs[1] + "'");
		float fDist = cMath::Max(1.0f, fRadius * 2.2f);
		// From the side the camera looks from now, a little from above
		float fYaw = pCamera->GetYaw(), fPitch = -0.35f;
		if(avArgs.size() > 2 && ToFloat(avArgs[2], fDist) == false) return Fail(pUsage);
		if(avArgs.size() > 3 && (avArgs.size() < 5 || ToFloat(avArgs[3], fYaw) == false ||
								 ToFloat(avArgs[4], fPitch) == false))
			return Fail(pUsage);

		// Without a direction given, turned around the node until nothing
		// is in the way, or to where the least is
		bool bTurn = avArgs.size() < 5;
		cVector3f vBest;
		float fBestYaw = fYaw, fBestDist = -1;
		for(int i = 0; i < (bTurn ? 8 : 1); ++i)
		{
			float fTryYaw = fYaw + i * (k2Pif / 8);
			pView->HoldCamera(vCenter, fTryYaw, fPitch);
			cVector3f vFrom = pEditor->GetClearView(avArgs[1], vCenter, vCenter - pCamera->GetForward() * fDist);
			float fClear = (vFrom - vCenter).Length();
			if(fClear > fBestDist + 0.01f)
			{
				vBest = vFrom;
				fBestYaw = fTryYaw;
				fBestDist = fClear;
			}
			if(fClear >= fDist * 0.9f) break;
		}
		fYaw = fBestYaw;
		pView->HoldCamera(vBest, fYaw, fPitch);
		mOut << "camera " << Vec(pCamera->GetPosition()) << " " << fYaw << " " << fPitch << "\n";
		return true;
	}
	if(sCmd == "release")
	{
		pView->ReleaseCamera();
		return true;
	}
	if(sCmd == "option" && avArgs.size() > 2 && avArgs[1] == "fixedstep")
	{
		// One logic step per frame and the same random numbers from each
		// map load, as for the reference images
		mbFixedStep = avArgs[2] != "0";
		mpInit->mpGame->GetLogicTimer()->SetFixedStep(mbFixedStep);
		// A live view runs at the game's speed, however fast frames are drawn
		mpInit->mpGame->GetLogicTimer()->SetPaceFixedStep(mbFixedStep);
		mpInit->mpMapHandler->SetReloadSeed(mbFixedStep ? 1 : -1);
		if(mbFixedStep) mpInit->mpGame->SetLimitFPS(true);
		return true;
	}
	if(sCmd == "option" && avArgs.size() > 2 && avArgs[1] == "pace")
	{
		// With a fixed step: 0 draws frames as fast as they can be drawn
		// (one step each), keeping the GPU busy for steady timings
		// (tools/framebench.py); 1 runs at the game's speed
		mpInit->mpGame->GetLogicTimer()->SetPaceFixedStep(mbFixedStep && avArgs[2] != "0");
		return true;
	}
	if(sCmd == "option")
	{
		if(avArgs.size() < 3 || pView->SetOption(avArgs[1], avArgs[2] != "0") == false)
			return Fail("option <flashlight|flicker|hud|godmode|dof|fixedstep> <0|1>");
		return true;
	}
	if(sCmd == "shot")
	{
		if(avArgs.size() < 2) return Fail("shot <file.png> [frames] [2d] [panel]");
		msShotFile = avArgs[1];
		mlFrames = klShotSettleFrames;
		mbShot2D = false;
		bool bPanel = false;
		for(size_t i = 2; i < avArgs.size(); ++i)
		{
			if(avArgs[i] == "2d") mbShot2D = true;
			else if(avArgs[i] == "panel") bPanel = true;
			else mlFrames = cString::ToInt(avArgs[i].c_str(), klShotSettleFrames);
		}
		std::error_code err;
		fs::remove(fs::u8path(msShotFile), err);
		fs::create_directories(fs::u8path(msShotFile).parent_path(), err);

		// The panel's windows are drawn into the frame: closed meanwhile,
		// unless they are what is to be seen
		mbPanelWasOpen = pView->IsOpen();
		if(mpInit->mpEditor)
		{
			// The editor's panels are the panel; the F2 windows are seen if open
			mbEditorUiWasHidden = mpInit->mpEditor->IsUiHidden();
			mpInit->mpEditor->SetUiHidden(bPanel == false);
			if(bPanel) mbPanelWasOpen = false;
			else pView->SetOpen(false);
		}
		else
		{
			if(bPanel) mbPanelWasOpen = false;
			pView->SetOpen(bPanel);
		}
		cGraphicsDrawer *pDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
		mbSkippedDrawing = pDrawer->GetSkipDrawing();
		if(mbShot2D == false) pDrawer->SetSkipDrawing(true);
		mWait = eWait_Shot;
		return false;
	}
	if(sCmd == "nodes")
	{
		if(NeedData() == false) return true;
		std::vector<cLevelEditor::cNodeInfo> vNodes;
		pEditor->FindNodes(avArgs.size() > 1 ? avArgs[1] : "", vNodes);
		for(const cLevelEditor::cNodeInfo &node : vNodes) WriteNode(node.msName);
		return true;
	}
	if(sCmd == "node")
	{
		if(avArgs.size() < 2) return Fail("node <name>");
		if(NeedData() == false) return true;
		cLevelEditor::cNodeInfo info;
		if(pEditor->GetNode(avArgs[1], info) == false) return Fail("no node '" + avArgs[1] + "'");
		WriteNode(info.msName);
		return true;
	}
	if(sCmd == "set")
	{
		const char *pUsage = "set <node> [pos x y z] [rot x y z] [scale x y z] [move x y z] [turn x y z]";
		if(avArgs.size() < 6) return Fail(pUsage);
		if(NeedData() == false) return true;
		cLevelEditor::cNodeInfo info;
		if(pEditor->GetNode(avArgs[1], info) == false) return Fail("no node '" + avArgs[1] + "'");

		cVector3f vPos = info.mvPos, vRot = info.mvRot, vScale = info.mvScale;
		bool bPos = false, bRot = false, bScale = false;
		for(size_t i = 2; i < avArgs.size(); i += 4)
		{
			cVector3f vValue;
			if(ToVector(avArgs, i + 1, vValue) == false) return Fail(pUsage);
			const tString &sWhat = avArgs[i];
			if(sWhat == "pos") { vPos = vValue; bPos = true; }
			else if(sWhat == "move") { vPos += vValue; bPos = true; }
			else if(sWhat == "rot") { vRot = vValue; bRot = true; }
			else if(sWhat == "turn") { vRot += vValue; bRot = true; }
			else if(sWhat == "scale") { vScale = vValue; bScale = true; }
			else return Fail(pUsage);
		}
		if(pEditor->SetTransform(info.msName, bPos ? &vPos : nullptr, bRot ? &vRot : nullptr,
								 bScale ? &vScale : nullptr, sError) == false)
			return Fail(sError);
		WriteNode(info.msName);
		return true;
	}
	if(sCmd == "reset")
	{
		if(NeedData() == false) return true;
		pEditor->ResetNode(avArgs.size() > 1 ? avArgs[1] : "");
		mOut << "edits " << pEditor->GetEditCount() << "\n";
		return true;
	}
	if(sCmd == "save" || sCmd == "revert")
	{
		if(NeedData() == false) return true;
		mlLoadCount = mpInit->mpMapHandler->GetLoadCount();
		mlFailCount = mpInit->mpMapHandler->GetReloadFailCount();
		tString sMessage;
		bool bDone = sCmd == "save" ? pEditor->Save(sMessage) : pEditor->Revert(sMessage);
		if(bDone == false) return Fail(sMessage);
		mOut << sMessage << "\n";
		mlFrames = -1;
		mWait = eWait_Reload;
		return false;
	}
	if(sCmd == "pick")
	{
		if(NeedData() == false) return true;
		tString sNode = pEditor->Pick();
		if(sNode == "") return Fail("the middle of the screen shows nothing of the map's file");
		pEditor->SetSelected(sNode);
		WriteNode(sNode);
		return true;
	}
	if(sCmd == "window")
	{
		// Resizes the window as a drag of its edge does
		if(avArgs.size() < 3) return Fail("window <width> <height>");
		SDL_Window *pWindow = SDL_GL_GetCurrentWindow();
		int lW = cString::ToInt(avArgs[1].c_str(), 0), lH = cString::ToInt(avArgs[2].c_str(), 0);
		if(pWindow == nullptr || lW < 64 || lH < 64 || SDL_SetWindowSize(pWindow, lW, lH) == false)
			return Fail("can't resize the window to " + avArgs[1] + " x " + avArgs[2]);
		SDL_SyncWindow(pWindow);
		mlFrames = 3;
		mWait = eWait_Frames;
		return false;
	}
	if(sCmd == "mouse")
	{
		// The editor's pointer, as if moved and clicked: what an agent can
		// do with the window (events go in ImGui's queue, one a frame)
		const char *pUsage = "mouse <x> <y> [move|press|release|click] [left|right|middle]";
		if(mpInit->mpEditor == nullptr) return Fail("'mouse' needs the editor (start the game with --editor)");
		if(avArgs.size() < 3) return Fail(pUsage);
		tString sAction = avArgs.size() > 3 ? avArgs[3] : "move";
		tString sButton = avArgs.size() > 4 ? avArgs[4] : "left";
		int lButton = sButton == "right" ? ImGuiMouseButton_Right :
					  (sButton == "middle" ? ImGuiMouseButton_Middle : ImGuiMouseButton_Left);
		if(sAction != "move" && sAction != "press" && sAction != "release" && sAction != "click") return Fail(pUsage);
		ImGuiIO &io = ImGui::GetIO();
		io.AddMousePosEvent(cString::ToFloat(avArgs[1].c_str(), 0), cString::ToFloat(avArgs[2].c_str(), 0));
		if(sAction == "press" || sAction == "click") io.AddMouseButtonEvent(lButton, true);
		if(sAction == "release" || sAction == "click") io.AddMouseButtonEvent(lButton, false);
		mlFrames = 3;
		mWait = eWait_Frames;
		return false;
	}
	if(sCmd == "select" || sCmd == "undo" || sCmd == "redo" || sCmd == "play" || sCmd == "simulate")
	{
		cEditor *pWindow = mpInit->mpEditor;
		if(pWindow == nullptr) return Fail("'" + sCmd + "' needs the editor (start the game with --editor)");
		if(sCmd == "play" || sCmd == "simulate")
		{
			if(avArgs.size() < 2) return Fail(sCmd + " <0|1>");
			if(sCmd == "play")
			{
				if(NeedWorld() == false) return true;
				pWindow->SetPlaying(avArgs[1] != "0");
			}
			else pWindow->SetSimulating(avArgs[1] != "0");
			return true;
		}
		if(NeedData() == false) return true;
		if(sCmd == "undo" || sCmd == "redo")
		{
			if(sCmd == "undo" ? pEditor->CanUndo() == false : pEditor->CanRedo() == false)
				return Fail("nothing to " + sCmd);
			mOut << (sCmd == "undo" ? pEditor->GetUndoLabel() : pEditor->GetRedoLabel()) << "\n";
			if(sCmd == "undo") pWindow->Undo();
			else pWindow->Redo();
			return true;
		}
		if(avArgs.size() > 1)
		{
			cLevelEditor::cNodeInfo info;
			if(pEditor->GetNode(avArgs[1], info) == false) return Fail("no node '" + avArgs[1] + "'");
			pWindow->Select(info.msName);
			if(avArgs.size() > 2 && avArgs[2] == "focus") pWindow->FocusSelected();
		}
		mOut << pWindow->GetSelected() << "\n";
		return true;
	}
	if(sCmd == "script")
	{
		if(NeedWorld() == false) return true;
		iScript *pScript = mpInit->mpGame->GetScene()->GetWorld3D()->GetScript();
		if(avArgs.size() < 2 || avArgs[1] == "") return Fail("script <command>");
		if(pScript == nullptr || pScript->Run(avArgs[1]) == false)
			return Fail("the map's script couldn't run '" + avArgs[1] + "' (see hpl.log)");
		return true;
	}
	if(sCmd == "wait")
	{
		mlFrames = avArgs.size() > 1 ? cString::ToInt(avArgs[1].c_str(), 1) : 1;
		mWait = eWait_Frames;
		return false;
	}
	if(sCmd == "quit")
	{
		mbQuit = true;
		return true;
	}

	Fail("unknown command '" + sCmd + "' (see docs/level-editor.md)");
	return true;
}

//-----------------------------------------------------------------------

bool cLiveControl::Continue(float afTimeStep)
{
	mfWaited += afTimeStep;
	cMapHandler *pMaps = mpInit->mpMapHandler;

	switch(mWait)
	{
	case eWait_Map:
		if(mlFrames < 0)
		{
			bool bLoaded = mpInit->mpRefView->IsMapPending() == false && pMaps->GetCurrentMapName() == msWaitMap &&
						   pMaps->GetLoadCount() != mlLoadCount && InWorld();
			if(bLoaded) mlFrames = klMapSettleFrames;
			else if(mfWaited > kfLoadTimeout)
				return Fail(msWaitMap + " did not load (see hpl.log)");
			else if(mpInit->mpRefView->IsMapPending() == false && pMaps->GetLoadCount() == mlLoadCount &&
					mfWaited > 1.0f)
				return Fail(msWaitMap + " was not loaded: no such map, or the game can't change map now");
			return false;
		}
		if(--mlFrames > 0) return false;
		mOut << "map " << pMaps->GetCurrentMapName() << "\n";
		return true;

	case eWait_Reload:
		if(mlFrames < 0)
		{
			if(pMaps->GetReloadFailCount() != mlFailCount)
				return Fail("the map's file can't be read, the loaded map is kept (see hpl.log)");
			if(pMaps->GetLoadCount() != mlLoadCount && InWorld()) mlFrames = klReloadSettleFrames;
			else if(mfWaited > kfLoadTimeout) return Fail("the map was not reloaded");
			return false;
		}
		if(--mlFrames > 0) return false;
		mOut << "map " << pMaps->GetCurrentMapName() << "\n";
		return true;

	case eWait_Frames:
		return --mlFrames <= 0;

	case eWait_Shot:
		if(--mlFrames > 0) return false;
		// This step's frame is drawn after the update, then saved
		mpInit->mpGame->GetGraphics()->GetLowLevel()->SaveNextFrameToPNG(msShotFile);
		mWait = eWait_ShotSaved;
		mlFrames = 0;
		return false;

	case eWait_ShotSaved:
	{
		std::error_code err;
		if(fs::exists(fs::u8path(msShotFile), err))
		{
			EndShot();
			mOut << msShotFile << "\n";
			return true;
		}
		if(++mlFrames > 30)
		{
			EndShot();
			return Fail("the frame was not saved to '" + msShotFile + "'");
		}
		return false;
	}

	case eWait_None:
		break;
	}
	return true;
}

//-----------------------------------------------------------------------

void cLiveControl::EndShot()
{
	mpInit->mpGame->GetGraphics()->GetDrawer()->SetSkipDrawing(mbSkippedDrawing);
	if(mbPanelWasOpen) mpInit->mpRefView->SetOpen(true);
	mbPanelWasOpen = false;
	if(mpInit->mpEditor) mpInit->mpEditor->SetUiHidden(mbEditorUiWasHidden);
}

//-----------------------------------------------------------------------

bool cLiveControl::Fail(const tString &asMessage)
{
	if(msError == "") msError = asMessage;
	return true;
}

//-----------------------------------------------------------------------

void cLiveControl::Answer()
{
	fs::path dir = fs::u8path(msDir);
	fs::path temp = dir / fs::u8path(msId + ".tmp");
	{
		// An error first, then what the lines before it gave
		std::ofstream out(temp, std::ios::binary);
		if(msError != "") out << "error: " << msError << "\n";
		else out << "ok\n";
		out << mOut.str();
	}
	std::error_code err;
	fs::rename(temp, dir / fs::u8path(msId + ".out"), err);
	if(err) Warning("Live control: couldn't answer %s: %s\n", msId.c_str(), err.message().c_str());
	if(msError != "") Log("Live control: %s failed: %s\n", msId.c_str(), msError.c_str());

	msId = "";
	mWait = eWait_None;
}

//-----------------------------------------------------------------------
