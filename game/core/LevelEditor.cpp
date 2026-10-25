/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/LevelEditor.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/AddonWatcher.h"
#include "entities/GameEntity.h"
#include "player/Player.h"
#include "impl/MeshLoaderCollada.h"

#include <imgui.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace fs = std::filesystem;

//////////////////////////////////////////////////////////////////////////
// HELPERS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

// The rotation of a matrix without its scale, and the scale
static void SplitMatrix(const cMatrixf &a_mtx, cMatrixf &a_mtxRot, cVector3f &avScale)
{
	a_mtxRot = a_mtx.GetRotation();
	for(int c = 0; c < 3; ++c)
	{
		float fLen = std::sqrt(a_mtxRot.m[0][c] * a_mtxRot.m[0][c] + a_mtxRot.m[1][c] * a_mtxRot.m[1][c] +
								a_mtxRot.m[2][c] * a_mtxRot.m[2][c]);
		avScale.v[c] = fLen;
		if(fLen > 1e-12f)
			for(int r = 0; r < 3; ++r) a_mtxRot.m[r][c] /= fLen;
	}
}

// The rotation and position of a matrix, without its scale
static cMatrixf RotationAndPosition(const cMatrixf &a_mtx)
{
	cMatrixf mtxRot;
	cVector3f vScale;
	SplitMatrix(a_mtx, mtxRot, vScale);
	mtxRot.SetTranslation(a_mtx.GetTranslation());
	return mtxRot;
}

// What the world made of a node (cLevelEditor::cLiveTarget::mType)
enum eLive
{
	eLive_Mesh,			// the map's own mesh, or a static one from a file
	eLive_Body,			// the body of a map mesh or collider
	eLive_EntityBody,	// a body of a game entity
	eLive_EntityMesh,	// the mesh of a game entity without bodies
	eLive_Light,
	eLive_Area,
	eLive_Start,
	eLive_Sound,
	eLive_Particles,
	eLive_Billboard,
	eLive_Beam,
};

static cVector3f ToDegrees(const cVector3f &avRad)
{
	cVector3f vDeg(cMath::ToDeg(avRad.x), cMath::ToDeg(avRad.y), cMath::ToDeg(avRad.z));
	// -0 reads badly
	for(int i = 0; i < 3; ++i)
		if(std::fabs(vDeg.v[i]) < 5e-5f) vDeg.v[i] = 0;
	return vDeg;
}

static cVector3f ToRadians(const cVector3f &avDeg)
{
	return cVector3f(cMath::ToRad(avDeg.x), cMath::ToRad(avDeg.y), cMath::ToRad(avDeg.z));
}

// The rotation as a quaternion (x, y, z, w), for the edits file, as the
// glTF writer makes it
static void QuaternionOf(const cMatrixf &a_mtxRot, float avQuat[4])
{
	cQuaternion qRot;
	qRot.FromRotationMatrix(a_mtxRot);
	qRot.Normalise();
	avQuat[0] = qRot.v.x;
	avQuat[1] = qRot.v.y;
	avQuat[2] = qRot.v.z;
	avQuat[3] = qRot.w;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cLevelEditor::cLevelEditor(cInit *apInit)
{
	mpInit = apInit;
	mlMapLoad = -1;
	mbReadFailed = false;
	msFilter[0] = 0;
	msKindFilter[0] = 0;
	mbSaveRequested = false;
	mbRevertRequested = false;
	mbPickRequested = false;
	mbShowRequested = false;
	mbStaticMoved = false;
	mlVersion = 0;
	mlUndo = 0;
	mlUndoDepth = 0;
	mbApplyingUndo = false;
}

cLevelEditor::~cLevelEditor()
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cLevelEditor::PrepareEditDir(const tString &asDir)
{
	std::error_code err;
	fs::path dir = fs::u8path(asDir);
	fs::create_directories(dir, err);
	if(fs::exists(dir / "addon.cfg", err)) return;

	std::ofstream file(dir / "addon.cfg");
	file << "<Addon Name=\"Level editor\" Priority=\"1000\" />\n";
	if(file.good() == false) Warning("Couldn't make the level editor's folder '%s'\n", asDir.c_str());
}

//-----------------------------------------------------------------------

void cLevelEditor::Update()
{
	// A map that was loaded or reloaded is read again when it is used
	const tString &sMap = mpInit->mpMapHandler->GetCurrentMapName();
	int lLoad = mpInit->mpMapHandler->GetLoadCount();
	if(sMap != msMap || lLoad != mlMapLoad)
	{
		bool bSameMap = sMap == msMap;
		// The same map again, unsaved: its changes are made in the new world
		std::vector<std::pair<tString, cState>> vKept;
		if(bSameMap && mpData)
		{
			for(const auto &original : m_mapOriginal)
			{
				cState state;
				if(GetState(original.first, state)) vKept.push_back({original.first, state});
			}
		}
		Clear();
		msMap = sMap;
		mlMapLoad = lLoad;
		if(bSameMap == false)
		{
			msSelected = "";
			ClearUndo();
		}
		else if(msMessage.compare(0, 6, "Saved ") != 0 && msMessage.compare(0, 9, "Reverted ") != 0)
			msMessage = "";

		tString sError;
		if(vKept.empty() == false && Prepare(sError))
		{
			mbApplyingUndo = true; // not a step of its own
			for(const auto &kept : vKept) SetState(kept.first, kept.second);
			mbApplyingUndo = false;
			Log("Level editor: %d unsaved change(s) of %s made again after the reload\n", (int)vKept.size(),
				msMap.c_str());
		}
	}

	if(mpInit->mpGame->GetUpdater()->GetCurrentContainerName() != "Default") return;

	tString sMessage;
	if(mbPickRequested)
	{
		mbPickRequested = false;
		tString sError;
		if(Prepare(sError))
		{
			tString sPicked = Pick();
			if(sPicked != "") msSelected = sPicked;
			else msMessage = "Nothing of the map's file there";
		}
		else msMessage = sError;
	}
	if(mbSaveRequested)
	{
		mbSaveRequested = false;
		Save(msMessage);
	}
	if(mbRevertRequested)
	{
		mbRevertRequested = false;
		Revert(msMessage);
	}
}

//-----------------------------------------------------------------------

bool cLevelEditor::Prepare(tString &asError)
{
	if(mpData) return true;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(msMap == "" || pWorld == nullptr)
	{
		asError = "no map is loaded";
		return false;
	}
	if(mbReadFailed)
	{
		asError = "the map's file '" + msMapFile + "' can't be read";
		return false;
	}

	cFileSearcher *pSearcher = mpInit->mpGame->GetResources()->GetFileSearcher();
	msMapFile = pSearcher->GetFilePath(pWorld->GetFileName());
	std::unique_ptr<cColladaData> pData(new cColladaData());
	cMeshLoaderCollada loader(mpInit->mpGame->GetGraphics()->GetLowLevel());
	if(msMapFile == "" || loader.LoadData(msMapFile, *pData) == false)
	{
		mbReadFailed = true;
		asError = "the map's file '" + pWorld->GetFileName() + "' can't be read";
		return false;
	}
	mpData = std::move(pData);
	Log("Level editor: read '%s'\n", msMapFile.c_str());
	return true;
}

//-----------------------------------------------------------------------

tString cLevelEditor::GetEditedMapFile()
{
	if(mpInit->msEditDir == "" || msMap == "") return "";
	std::error_code err;
	fs::path file = fs::u8path(mpInit->msEditDir) / fs::u8path(msMap + ".glb");
	return fs::exists(file, err) ? file.u8string() : "";
}

//-----------------------------------------------------------------------

tString cLevelEditor::GetKind(const cColladaNode *apNode, const cColladaData &aData)
{
	tString sName = cString::ToLowerCase(apNode->msName);
	auto Starts = [&sName](const char *asPrefix){ return sName.compare(0, strlen(asPrefix), asPrefix) == 0; };

	if(apNode->mbSourceIsFile)
		return Starts("static") ? "static" : "entity";
	if(Starts("_area")) return "area";
	if(Starts("_start")) return "start";
	if(Starts("_sound")) return "sound";
	if(Starts("_ps")) return "particles";
	if(Starts("_bb")) return "billboard";
	if(Starts("_beam")) return "beam";
	if(Starts("_ref")) return "entity";
	if(Starts("_node")) return "node";
	if(Starts("_room") || Starts("_portal") || Starts("_sector")) return "portal";
	if(Starts("_collider") || Starts("_charcollider")) return "collider";
	if(Starts("_joint")) return "joint";
	if(apNode->msSource != "")
	{
		for(const cColladaLight &light : aData.mvLights)
			if(light.msId == apNode->msSource) return "light";
		return "mesh";
	}
	return "group";
}

//-----------------------------------------------------------------------

tString cLevelEditor::GetObjectName(const cColladaNode *apNode)
{
	tString sName = cString::ToLowerCase(apNode->msName);
	for(const char *pPrefix : {"_ref_", "_area_", "_sound_", "_ps_", "_bb_", "_beam_"})
	{
		if(sName.compare(0, strlen(pPrefix), pPrefix) != 0) continue;
		size_t lLast = apNode->msName.rfind('_');
		if(lLast > strlen(pPrefix)) return apNode->msName.substr(lLast + 1);
	}
	return apNode->msName;
}

//-----------------------------------------------------------------------

void cLevelEditor::FindNodes(const tString &asFilter, std::vector<cNodeInfo> &avNodes)
{
	avNodes.clear();
	if(mpData == nullptr) return;

	tString sFilter = cString::ToLowerCase(asFilter);
	for(cColladaNode *pNode : mpData->mScene.mlstNodes)
	{
		if(pNode->msName == "") continue;
		cNodeInfo info;
		FillInfo(pNode, info);
		if(sFilter != "" && cString::ToLowerCase(info.msName).find(sFilter) == tString::npos &&
			cString::ToLowerCase(info.msObject).find(sFilter) == tString::npos &&
			info.msKind != sFilter && cString::ToLowerCase(info.msSource).find(sFilter) == tString::npos)
			continue;
		avNodes.push_back(info);
	}
}

//-----------------------------------------------------------------------

bool cLevelEditor::GetNode(const tString &asName, cNodeInfo &aInfo)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr) return false;
	FillInfo(pNode, aInfo);
	return true;
}

//-----------------------------------------------------------------------

bool cLevelEditor::SetTransform(const tString &asName, const cVector3f *apPos, const cVector3f *apRot,
								const cVector3f *apScale, tString &asError)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr)
	{
		asError = "no node '" + asName + "' in " + msMap;
		return false;
	}
	if(apScale && (apScale->x == 0 || apScale->y == 0 || apScale->z == 0))
	{
		asError = "a scale of 0";
		return false;
	}

	cNodeInfo info;
	FillInfo(pNode, info);
	cVector3f vPos = apPos ? *apPos : info.mvPos;
	cVector3f vScale = apScale ? *apScale : info.mvScale;

	// The rotation is kept exactly when it is not set, not via the angles
	cMatrixf mtxRot;
	if(apRot)
		mtxRot = cMath::MatrixRotate(ToRadians(*apRot), eEulerRotationOrder_XYZ);
	else
	{
		cVector3f vWorldScale;
		SplitMatrix(pNode->m_mtxWorldTransform, mtxRot, vWorldScale);
	}
	mtxRot.SetTranslation(vPos);
	return SetWorld(pNode->msName, mtxRot, vScale, asError);
}

//-----------------------------------------------------------------------

bool cLevelEditor::GetWorld(const tString &asName, cMatrixf &a_mtxRotPos, cVector3f &avScale)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr) return false;
	cVector3f vWorldScale;
	SplitMatrix(pNode->m_mtxWorldTransform, a_mtxRotPos, vWorldScale);
	a_mtxRotPos.SetTranslation(pNode->m_mtxWorldTransform.GetTranslation());
	avScale = pNode->mvScale;
	return true;
}

bool cLevelEditor::SetWorld(const tString &asName, const cMatrixf &a_mtxRotPos, const cVector3f &avScale,
							tString &asError)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr)
	{
		asError = "no node '" + asName + "' in " + msMap;
		return false;
	}
	if(avScale.x == 0 || avScale.y == 0 || avScale.z == 0)
	{
		asError = "a scale of 0";
		return false;
	}

	// The world transform, then the local one under the parent
	cMatrixf mtxWorld = a_mtxRotPos.GetRotation();
	if(IsScaleSeparate(pNode) == false) mtxWorld = cMath::MatrixMul(mtxWorld, cMath::MatrixScale(avScale));
	mtxWorld.SetTranslation(a_mtxRotPos.GetTranslation());

	cMatrixf mtxParent = pNode->pParent ? pNode->pParent->m_mtxWorldTransform : cMatrixf::Identity;
	ChangeNode(pNode, cMath::MatrixMul(cMath::MatrixInverse(mtxParent), mtxWorld), avScale);
	return true;
}

//-----------------------------------------------------------------------

bool cLevelEditor::GetState(const tString &asName, cState &aState)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr) return false;
	aState.m_mtxTransform = pNode->m_mtxTransform;
	aState.mvScale = pNode->mvScale;
	return true;
}

bool cLevelEditor::SetState(const tString &asName, const cState &aState)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr) return false;
	ChangeNode(pNode, aState.m_mtxTransform, aState.mvScale);
	return true;
}

//-----------------------------------------------------------------------

void cLevelEditor::GetChildren(const tString &asName, tStringVec &avChildren)
{
	avChildren.clear();
	if(mpData == nullptr) return;
	cColladaNode *pNode = asName == "" ? &mpData->mScene.mRoot : FindNode(asName);
	if(pNode == nullptr) return;
	for(cColladaNode *pChild : pNode->mlstChildren)
		if(pChild->msName != "") avChildren.push_back(pChild->msName);
}

tString cLevelEditor::GetNodeKind(const tString &asName)
{
	cColladaNode *pNode = FindNode(asName);
	return pNode ? GetKind(pNode, *mpData) : "";
}

//-----------------------------------------------------------------------

bool cLevelEditor::GetNodeBox(const tString &asName, cVector3f &avMin, cVector3f &avMax)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr) return false;

	bool bFound = false;
	auto AddBox = [&](const cVector3f &avBoxMin, const cVector3f &avBoxMax)
	{
		if(bFound == false)
		{
			avMin = avBoxMin;
			avMax = avBoxMax;
			bFound = true;
			return;
		}
		for(int i = 0; i < 3; ++i)
		{
			avMin.v[i] = cMath::Min(avMin.v[i], avBoxMin.v[i]);
			avMax.v[i] = cMath::Max(avMax.v[i], avBoxMax.v[i]);
		}
	};

	std::vector<cLiveTarget> vTargets;
	if(LiveWorldValid()) FindTargets(pNode, vTargets);
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	for(const cLiveTarget &target : vTargets)
	{
		if(target.mType == eLive_Area)
		{
			cAreaEntity *pArea = pWorld->GetAreaEntity(target.msName);
			if(pArea == nullptr) continue;
			cVector3f vHalf = pArea->mvSize * 0.5f;
			for(int lCorner = 0; lCorner < 8; ++lCorner)
			{
				cVector3f vCorner((lCorner & 1) ? vHalf.x : -vHalf.x, (lCorner & 2) ? vHalf.y : -vHalf.y,
								(lCorner & 4) ? vHalf.z : -vHalf.z);
				cVector3f vPoint = cMath::MatrixMul(pArea->m_mtxTransform, vCorner);
				AddBox(vPoint, vPoint);
			}
			continue;
		}
		// Meshes; bodies only for what has no mesh; a light's sphere is
		// all of what it lights, not where it is
		if(target.mType != eLive_Mesh && target.mType != eLive_EntityMesh && target.mType != eLive_Body &&
			target.mType != eLive_EntityBody)
			continue;
		if((target.mType == eLive_Body || target.mType == eLive_EntityBody) && bFound) continue;
		iEntity3D *pEntity = GetTargetEntity(target);
		if(pEntity == nullptr) continue;
		cBoundingVolume *pBV = pEntity->GetBoundingVolume();
		AddBox(pBV->GetMin(), pBV->GetMax());
	}
	if(bFound == false)
	{
		cVector3f vPos = pNode->m_mtxWorldTransform.GetTranslation();
		AddBox(vPos - cVector3f(0.15f), vPos + cVector3f(0.15f));
	}
	return true;
}

iLight3D* cLevelEditor::GetNodeLight(const tString &asName)
{
	cColladaNode *pNode = FindNode(asName);
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pNode == nullptr || pWorld == nullptr || GetKind(pNode, *mpData) != "light") return nullptr;
	return pWorld->GetLight(GetLightName(pNode->msName));
}

//-----------------------------------------------------------------------

void cLevelEditor::ResetNode(const tString &asName)
{
	cColladaNode *pNamed = asName != "" ? FindNode(asName) : nullptr;
	if(asName != "" && pNamed == nullptr) return;
	BeginUndo(pNamed ? "Reset " + GetObjectName(pNamed) : tString("Reset all"));
	// Put back as read: ChangeNode() then forgets the node as changed
	std::vector<std::pair<tString, cOriginal>> vReset;
	for(const auto &original : m_mapOriginal)
		if(pNamed == nullptr || original.first == pNamed->msName) vReset.push_back(original);
	for(const auto &reset : vReset)
	{
		cColladaNode *pNode = FindNode(reset.first);
		if(pNode) ChangeNode(pNode, reset.second.m_mtxTransform, reset.second.mvScale);
		m_mapOriginal.erase(reset.first);
	}
	EndUndo();
}

//-----------------------------------------------------------------------

bool cLevelEditor::Save(tString &asMessage)
{
	if(mpData == nullptr || m_mapOriginal.empty())
	{
		asMessage = "Nothing to save";
		return false;
	}
	if(mpInit->msEditDir == "")
	{
		asMessage = "No edit folder: addons are off (OPENHPL_ADDONS=0)";
		return false;
	}

	fs::path dir = fs::u8path(mpInit->msEditDir);
	PrepareEditDir(mpInit->msEditDir);
	tString sFile = (dir / fs::u8path(msMap + ".glb")).u8string();
	tString sTemp = sFile + ".part";

	// Written beside it first: a reload never sees half a file
	tStringVec vWarnings;
	cMeshLoaderCollada loader(mpInit->mpGame->GetGraphics()->GetLowLevel());
	if(loader.SaveGltf(*mpData, sTemp, cString::GetFileName(msMapFile), &vWarnings) == false)
	{
		asMessage = "Couldn't write '" + sTemp + "'";
		return false;
	}
	for(const tString &sWarning : vWarnings)
		Warning("Level editor: %s: %s\n", msMap.c_str(), sWarning.c_str());

	// The changes, added to the ones saved before, by node. A node changed
	// again keeps where it was before the first change.
	tString sEditsFile = (dir / fs::u8path(msMap + ".edits.txt")).u8string();
	std::map<tString, tString> mapLines;
	// The map the first changes were made to, not the edited one
	tString sFrom = msMapFile;
	{
		std::ifstream in(fs::u8path(sEditsFile));
		tString sLine;
		while(std::getline(in, sLine))
		{
			if(sLine.compare(0, 7, "# from ") == 0) sFrom = sLine.substr(7);
			if(sLine == "" || sLine[0] == '#') continue;
			mapLines[sLine.substr(0, sLine.find('\t'))] = sLine;
		}
	}
	for(const auto &original : m_mapOriginal)
	{
		cColladaNode *pNode = FindNode(original.first);
		if(pNode == nullptr) continue;
		tString sWas = original.second.msWorld;
		auto it = mapLines.find(pNode->msName);
		if(it != mapLines.end() && it->second.find("\twas\t") != tString::npos)
			sWas = it->second.substr(it->second.find("\twas\t") + 5);
		mapLines[pNode->msName] = pNode->msName + "\t" + GetWorldText(pNode) + "\twas\t" + sWas;
	}
	{
		std::ofstream out(fs::u8path(sEditsFile));
		out << "# " << msMap << ": nodes changed in OpenHPL1's level editor\n";
		out << "# from " << sFrom << "\n";
		out << "# World transforms in the map's (glTF) space, +Y up, metres; after \"was\" where\n";
		out << "# the node was before it was first changed:\n";
		out << "# node<TAB>x y z<TAB>rotation x y z w<TAB>scale x y z<TAB>was<TAB>x y z<TAB>...\n";
		for(const auto &line : mapLines) out << line.second << "\n";
	}

	std::error_code err;
	fs::rename(fs::u8path(sTemp), fs::u8path(sFile), err);
	if(err)
	{
		asMessage = "Couldn't replace '" + sFile + "': " + err.message();
		return false;
	}

	int lNodes = (int)m_mapOriginal.size();
	m_mapOriginal.clear();
	Log("Level editor: saved %d changed node(s) of %s to '%s'\n", lNodes, msMap.c_str(), sFile.c_str());
	asMessage = "Saved " + cString::ToString(lNodes) + " node(s) to " + sFile;

	if(mpInit->mpAddonWatcher) mpInit->mpAddonWatcher->Accept();
	mpInit->mpMapHandler->RequestReload();
	return true;
}

//-----------------------------------------------------------------------

bool cLevelEditor::Revert(tString &asMessage)
{
	tString sFile = GetEditedMapFile();
	if(sFile == "")
	{
		asMessage = "The map has no saved edits";
		return false;
	}
	std::error_code err;
	fs::path file = fs::u8path(sFile);
	fs::path edits = file.parent_path() / fs::u8path(msMap + ".edits.txt");
	fs::remove(file, err);
	if(err)
	{
		asMessage = "Couldn't remove '" + sFile + "': " + err.message();
		return false;
	}
	fs::remove(edits, err);

	m_mapOriginal.clear();
	Log("Level editor: reverted %s, removed '%s'\n", msMap.c_str(), sFile.c_str());
	asMessage = "Reverted " + msMap;
	if(mpInit->mpAddonWatcher) mpInit->mpAddonWatcher->Accept();
	mpInit->mpMapHandler->RequestReload();
	return true;
}

//-----------------------------------------------------------------------

namespace
{
	// The closest body a ray hits
	class cClosestHit : public iPhysicsRayCallback
	{
	public:
		iPhysicsBody *mpBody = nullptr;
		float mfDist = 0;
		cVector3f mvPoint;
		iPhysicsBody *mpIgnore = nullptr;

		bool BeforeIntersect(iPhysicsBody *apBody) override
		{
			// Not through what can't be touched: script areas and the like
			return apBody != mpIgnore && apBody->IsCharacter() == false && apBody->GetCollide();
		}
		bool OnIntersect(iPhysicsBody *apBody, cPhysicsRayParams *apParams) override
		{
			if(mpBody == nullptr || apParams->mfDist < mfDist)
			{
				mpBody = apBody;
				mfDist = apParams->mfDist;
				mvPoint = apParams->mvPoint;
			}
			return true;
		}
	};
}

tString cLevelEditor::Pick()
{
	cCamera3D *pCamera = mpInit->mpPlayer->GetCamera();
	cVector3f vStart = pCamera->GetPosition();
	return Pick(vStart, vStart + pCamera->GetForward() * 100.0f);
}

tString cLevelEditor::Pick(const cVector3f &vStart, const cVector3f &vEnd)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld == nullptr || mpData == nullptr) return "";

	cClosestHit hit;
	hit.mpIgnore = mpInit->mpPlayer->GetCharacterBody() ? mpInit->mpPlayer->GetCharacterBody()->GetBody() : nullptr;
	pWorld->GetPhysicsWorld()->CastRay(&hit, vStart, vEnd, true, false, true);
	if(hit.mpBody == nullptr) return "";

	// A body of a game entity: the entity
	tGameEntityIterator entityIt = mpInit->mpMapHandler->GetGameEntityIterator();
	while(entityIt.HasNext())
	{
		iGameEntity *pEntity = entityIt.Next();
		for(int i = 0; i < pEntity->GetBodyNum(); ++i)
		{
			if(pEntity->GetBody(i) != hit.mpBody) continue;
			cColladaNode *pNode = FindNode(pEntity->GetName());
			if(pNode) return pNode->msName;
			break;
		}
	}

	// A body of the map's own mesh or collider has its name
	cColladaNode *pBodyNode = FindNode(hit.mpBody->GetName());
	if(pBodyNode && cString::ToLowerCase(pBodyNode->msName) == cString::ToLowerCase(hit.mpBody->GetName()))
		return pBodyNode->msName;

	// Else the smallest mesh of the map's file around the point hit
	tString sBest;
	float fBestVolume = 0;
	cMeshEntityIterator meshIt = pWorld->GetMeshEntityIterator();
	while(meshIt.HasNext())
	{
		cMeshEntity *pMesh = static_cast<cMeshEntity *>(meshIt.Next());
		cColladaNode *pNode = FindNode(pMesh->GetName());
		if(pNode == nullptr) continue;
		cBoundingVolume *pBV = pMesh->GetBoundingVolume();
		cVector3f vMin = pBV->GetMin() - cVector3f(0.02f), vMax = pBV->GetMax() + cVector3f(0.02f);
		if(hit.mvPoint.x < vMin.x || hit.mvPoint.y < vMin.y || hit.mvPoint.z < vMin.z ||
			hit.mvPoint.x > vMax.x || hit.mvPoint.y > vMax.y || hit.mvPoint.z > vMax.z)
			continue;
		cVector3f vSize = vMax - vMin;
		float fVolume = vSize.x * vSize.y * vSize.z;
		if(sBest == "" || fVolume < fBestVolume)
		{
			sBest = pNode->msName;
			fBestVolume = fVolume;
		}
	}
	return sBest;
}

//-----------------------------------------------------------------------

bool cLevelEditor::GetNodeBounds(const tString &asName, cVector3f &avCenter, float &afRadius)
{
	cColladaNode *pNode = FindNode(asName);
	if(pNode == nullptr) return false;

	// What the world made of it, as it was loaded
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	tString sObject = GetObjectName(pNode);
	iGameEntity *pEntity = mpInit->mpMapHandler->GetGameEntity(sObject, false);
	cMeshEntity *pMesh = pEntity ? pEntity->GetMeshEntity() : nullptr;
	if(pMesh == nullptr && pWorld) pMesh = pWorld->GetMeshEntity(sObject);
	if(pMesh)
	{
		cBoundingVolume *pBV = pMesh->GetBoundingVolume();
		avCenter = (pBV->GetMin() + pBV->GetMax()) * 0.5f;
		afRadius = cMath::Max(0.1f, (pBV->GetMax() - pBV->GetMin()).Length() * 0.5f);
		return true;
	}

	avCenter = pNode->m_mtxWorldTransform.GetTranslation();
	afRadius = 0.5f;
	return true;
}

//-----------------------------------------------------------------------

cVector3f cLevelEditor::GetClearView(const tString &asName, const cVector3f &avTarget, const cVector3f &avFrom)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld == nullptr) return avFrom;

	// The node's own bodies are looked through
	cColladaNode *pNode = FindNode(asName);
	iGameEntity *pEntity = pNode ? mpInit->mpMapHandler->GetGameEntity(GetObjectName(pNode), false) : nullptr;
	class cWallHit : public cClosestHit
	{
	public:
		iGameEntity *mpEntity = nullptr;
		tString msOwnBody;
		bool BeforeIntersect(iPhysicsBody *apBody) override
		{
			if(msOwnBody != "" && apBody->GetName() == msOwnBody) return false;
			if(mpEntity)
				for(int i = 0; i < mpEntity->GetBodyNum(); ++i)
					if(mpEntity->GetBody(i) == apBody) return false;
			return cClosestHit::BeforeIntersect(apBody);
		}
	} hit;
	hit.mpEntity = pEntity;
	// A map mesh's body has its name
	if(pNode) hit.msOwnBody = pNode->msName;
	hit.mpIgnore = mpInit->mpPlayer->GetCharacterBody() ? mpInit->mpPlayer->GetCharacterBody()->GetBody() : nullptr;
	pWorld->GetPhysicsWorld()->CastRay(&hit, avTarget, avFrom, true, false, true);
	if(hit.mpBody == nullptr) return avFrom;

	// A little in front of the wall
	cVector3f vDir = avFrom - avTarget;
	float fLength = vDir.Length();
	if(fLength < 1e-4f) return avFrom;
	return avTarget + vDir * (cMath::Max(0.05f, hit.mfDist - 0.15f) / fLength);
}

//-----------------------------------------------------------------------

void cLevelEditor::Draw()
{
	const float fEm = ImGui::GetFontSize();
	if(msMap == "")
	{
		ImGui::TextDisabled("No map loaded");
		return;
	}

	tString sError;
	if(mpData == nullptr)
	{
		ImGui::TextWrapped("Moves the nodes of %s's file and saves the map as an addon.", msMap.c_str());
		if(ImGui::Button("Read the map") && Prepare(sError) == false) msMessage = sError;
		if(msMessage != "") ImGui::TextWrapped("%s", msMessage.c_str());
		return;
	}

	ImGui::TextWrapped("%s", cString::GetFileName(msMapFile).c_str());
	if(ImGui::IsItemHovered()) ImGui::SetTooltip("%s", msMapFile.c_str());

	ImGui::SetNextItemWidth(fEm * 14);
	ImGui::InputTextWithHint("##nodefilter", "filter: name, kind, file", msFilter, sizeof(msFilter));
	ImGui::SameLine();
	if(ImGui::Button("Pick")) mbPickRequested = true;
	if(ImGui::IsItemHovered())
		ImGui::SetTooltip("Selects what the middle of the screen shows");

	std::vector<cNodeInfo> vNodes;
	FindNodes(msFilter, vNodes);
	if(ImGui::BeginListBox("##nodes", ImVec2(-1, fEm * 8)))
	{
		for(const cNodeInfo &node : vNodes)
		{
			tString sLabel = (node.msObject != "" ? node.msObject : node.msName) + "  (" + node.msKind + ")" +
							 (node.mbEdited ? " *" : "");
			if(ImGui::Selectable(sLabel.c_str(), node.msName == msSelected)) msSelected = node.msName;
			if(node.msName == msSelected && ImGui::IsWindowAppearing()) ImGui::SetScrollHereY();
		}
		ImGui::EndListBox();
	}

	cNodeInfo info;
	if(msSelected != "" && GetNode(msSelected, info))
	{
		ImGui::Text("%s: %s", info.msKind.c_str(), (info.msObject != "" ? info.msObject : info.msName).c_str());
		if(info.msObject != "") ImGui::TextDisabled("%s", info.msName.c_str());
		if(info.msSource != "") ImGui::TextDisabled("%s", info.msSource.c_str());

		cVector3f vPos = info.mvPos, vRot = info.mvRot, vScale = info.mvScale;
		bool bPos = ImGui::DragFloat3("Position", vPos.v, 0.01f, 0, 0, "%.3f");
		bool bRot = ImGui::DragFloat3("Rotation", vRot.v, 0.5f, 0, 0, "%.1f");
		bool bScale = ImGui::DragFloat3("Scale", vScale.v, 0.005f, 0, 0, "%.3f");
		if(bPos || bRot || bScale)
		{
			if(SetTransform(info.msName, bPos ? &vPos : nullptr, bRot ? &vRot : nullptr,
							bScale ? &vScale : nullptr, sError) == false)
				msMessage = sError;
		}
		ImGui::BeginDisabled(info.mbEdited == false);
		if(ImGui::Button("Reset node")) ResetNode(info.msName);
		ImGui::EndDisabled();
	}

	ImGui::Separator();
	ImGui::Text("%d unsaved node(s)", GetEditCount());
	ImGui::BeginDisabled(GetEditCount() == 0);
	if(ImGui::Button("Save and reload")) mbSaveRequested = true;
	ImGui::SameLine();
	if(ImGui::Button("Reset all")) ResetNode("");
	ImGui::EndDisabled();
	tString sEdited = GetEditedMapFile();
	ImGui::SameLine();
	ImGui::BeginDisabled(sEdited == "");
	if(ImGui::Button("Revert")) mbRevertRequested = true;
	ImGui::EndDisabled();
	if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Removes the saved map from %s", mpInit->msEditDir.c_str());
	if(msMessage != "") ImGui::TextWrapped("%s", msMessage.c_str());
	ImGui::TextWrapped("Changes show at once (a scale not yet in the physics). Saving writes the map and "
						"reloads it.");
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cLevelEditor::Clear()
{
	mpData.reset();
	msMapFile = "";
	mbReadFailed = false;
	m_mapOriginal.clear();
	m_mapLive.clear();
	mbStaticMoved = false;
	++mlVersion;
}

//-----------------------------------------------------------------------

cColladaNode* cLevelEditor::FindNode(const tString &asName)
{
	if(mpData == nullptr || asName == "") return nullptr;
	for(cColladaNode *pNode : mpData->mScene.mlstNodes)
		if(pNode->msName == asName) return pNode;
	// Names are found in any case, as the engine's are, and by what the
	// nodes make
	tString sLower = cString::ToLowerCase(asName);
	for(cColladaNode *pNode : mpData->mScene.mlstNodes)
		if(cString::ToLowerCase(pNode->msName) == sLower) return pNode;
	for(cColladaNode *pNode : mpData->mScene.mlstNodes)
		if(cString::ToLowerCase(GetObjectName(pNode)) == sLower) return pNode;
	return nullptr;
}

//-----------------------------------------------------------------------

bool cLevelEditor::IsScaleSeparate(cColladaNode *apNode)
{
	return cMeshLoaderCollada::ScaleIsSeparate(apNode, &mpData->mvLights);
}

//-----------------------------------------------------------------------

void cLevelEditor::FillInfo(cColladaNode *apNode, cNodeInfo &aInfo)
{
	aInfo.msName = apNode->msName;
	aInfo.msKind = GetKind(apNode, *mpData);
	aInfo.msSource = apNode->mbSourceIsFile ? cString::GetFileName(apNode->msSource) : "";
	aInfo.msObject = GetObjectName(apNode);
	if(aInfo.msObject == aInfo.msName) aInfo.msObject = "";
	// "_ref_<file>_<name>": the file is <file>.ent
	if(aInfo.msKind == "entity" && aInfo.msSource == "" && aInfo.msObject != "")
		aInfo.msSource = apNode->msName.substr(5, apNode->msName.size() - 5 - aInfo.msObject.size() - 1) + ".ent";
	aInfo.msParent = apNode->pParent && apNode->pParent != &mpData->mScene.mRoot ? apNode->pParent->msName : "";
	aInfo.mvPos = apNode->m_mtxWorldTransform.GetTranslation();

	cMatrixf mtxRot;
	cVector3f vScale;
	SplitMatrix(apNode->m_mtxWorldTransform, mtxRot, vScale);
	aInfo.mvRot = ToDegrees(cMath::MatrixToEulerAngles(mtxRot, eEulerRotationOrder_XYZ));
	aInfo.mvScale = apNode->mvScale;
	aInfo.mbEdited = m_mapOriginal.find(apNode->msName) != m_mapOriginal.end();
}

//-----------------------------------------------------------------------

tString cLevelEditor::GetWorldText(cColladaNode *apNode)
{
	cMatrixf mtxRot;
	cVector3f vScale;
	SplitMatrix(apNode->m_mtxWorldTransform, mtxRot, vScale);
	if(IsScaleSeparate(apNode)) vScale = apNode->mvScale;
	cVector3f vPos = apNode->m_mtxWorldTransform.GetTranslation();
	float vQuat[4];
	QuaternionOf(mtxRot, vQuat);

	char sText[256];
	snprintf(sText, sizeof(sText), "%.6g %.6g %.6g\t%.7g %.7g %.7g %.7g\t%.6g %.6g %.6g", vPos.x, vPos.y, vPos.z,
			 vQuat[0], vQuat[1], vQuat[2], vQuat[3], vScale.x, vScale.y, vScale.z);
	return sText;
}

//-----------------------------------------------------------------------

void cLevelEditor::UpdateWorldTransforms(cColladaNode *apNode)
{
	cMatrixf mtxParent = apNode->pParent ? apNode->pParent->m_mtxWorldTransform : cMatrixf::Identity;
	apNode->m_mtxWorldTransform = cMath::MatrixMul(mtxParent, apNode->m_mtxTransform);
	for(cColladaNode *pChild : apNode->mlstChildren) UpdateWorldTransforms(pChild);
}

//-----------------------------------------------------------------------

void cLevelEditor::ChangeNode(cColladaNode *apNode, const cMatrixf &a_mtxTransform, const cVector3f &avScale)
{
	if(m_mapOriginal.find(apNode->msName) == m_mapOriginal.end())
		m_mapOriginal[apNode->msName] = cOriginal{apNode->m_mtxTransform, apNode->mvScale, GetWorldText(apNode)};

	cState before{apNode->m_mtxTransform, apNode->mvScale};
	CaptureLive(apNode);
	apNode->m_mtxTransform = a_mtxTransform;
	apNode->mvScale = avScale;
	UpdateWorldTransforms(apNode);
	SyncLive(apNode);
	++mlVersion;
	if(mbApplyingUndo == false) AddToUndo(apNode, before);

	// Nothing is changed any more: as it was read
	cOriginal &original = m_mapOriginal[apNode->msName];
	if(original.mvScale == avScale && original.m_mtxTransform == a_mtxTransform)
		m_mapOriginal.erase(apNode->msName);
}

//-----------------------------------------------------------------------

bool cLevelEditor::LiveWorldValid()
{
	return mpData && mpInit->mpGame->GetScene()->GetWorld3D() &&
		   mpInit->mpMapHandler->GetLoadCount() == mlMapLoad && mpInit->mpMapHandler->GetCurrentMapName() == msMap &&
		   mpInit->mpMapHandler->IsChangingMap() == false;
}

//-----------------------------------------------------------------------

void cLevelEditor::CaptureLive(cColladaNode *apNode)
{
	if(LiveWorldValid() == false) return;
	if(apNode->msName != "" && m_mapLive.find(apNode->msName) == m_mapLive.end())
	{
		cLiveNode &live = m_mapLive[apNode->msName];
		live.m_mtxStartWorld = apNode->m_mtxWorldTransform;
		live.mvStartScale = apNode->mvScale;
		FindTargets(apNode, live.mvTargets);
	}
	for(cColladaNode *pChild : apNode->mlstChildren) CaptureLive(pChild);
}

//-----------------------------------------------------------------------

void cLevelEditor::SyncLive(cColladaNode *apNode)
{
	if(LiveWorldValid() == false) return;
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	cPortalContainer *pContainer = pWorld->GetPortalContainer();

	auto it = m_mapLive.find(apNode->msName);
	if(it != m_mapLive.end())
	{
		const cLiveNode &live = it->second;
		// From where it was to where it is: all of it for what is drawn,
		// without a change of scale for bodies and the rest
		cMatrixf mtxMove = cMath::MatrixMul(apNode->m_mtxWorldTransform, cMath::MatrixInverse(live.m_mtxStartWorld));
		cMatrixf mtxRigid = cMath::MatrixMul(RotationAndPosition(apNode->m_mtxWorldTransform),
											 cMath::MatrixInverse(RotationAndPosition(live.m_mtxStartWorld)));

		for(const cLiveTarget &target : live.mvTargets)
		{
			if(target.mType == eLive_Area)
			{
				cAreaEntity *pArea = pWorld->GetAreaEntity(target.msName);
				if(pArea == nullptr) continue;
				pArea->m_mtxTransform = cMath::MatrixMul(mtxRigid, target.m_mtxStart);
				for(int i = 0; i < 3; ++i)
					if(live.mvStartScale.v[i] != 0)
						pArea->mvSize.v[i] = target.mvStartSize.v[i] * apNode->mvScale.v[i] / live.mvStartScale.v[i];
				continue;
			}
			if(target.mType == eLive_Start)
			{
				cStartPosEntity *pStart = pWorld->GetStartPosEntity(target.msName);
				if(pStart) pStart->SetMatrix(cMath::MatrixMul(mtxRigid, target.m_mtxStart));
				continue;
			}

			iEntity3D *pEntity = GetTargetEntity(target);
			if(pEntity == nullptr) continue;
			bool bDrawn = target.mType == eLive_Mesh || target.mType == eLive_EntityMesh;
			iRenderable *pRenderable = nullptr;
			if(target.mType == eLive_Mesh) pRenderable = pWorld->GetMeshEntity(target.msName);
			else if(target.mType == eLive_Light) pRenderable = pWorld->GetLight(target.msName);
			if(pRenderable)
			{
				if(pRenderable->IsStatic())
				{
					pContainer->MakeDynamic(pRenderable);
					mbStaticMoved = true;
				}
			}
			pEntity->SetMatrix(cMath::MatrixMul(bDrawn ? mtxMove : mtxRigid, target.m_mtxStart));

			// A light's radius is in the scale
			if(target.mType == eLive_Light && live.mvStartScale.x > 0)
				pWorld->GetLight(target.msName)->SetFarAttenuation(
					target.mvStartSize.x * cMath::Abs(apNode->mvScale.x / live.mvStartScale.x));
		}
	}

	for(cColladaNode *pChild : apNode->mlstChildren) SyncLive(pChild);

	// What was static has gone from the static lights' shadow casters
	if(mbStaticMoved)
	{
		mbStaticMoved = false;
		cLight3DListIterator lightIt = pWorld->GetLightIterator();
		while(lightIt.HasNext())
		{
			iLight3D *pLight = lightIt.Next();
			if(pLight->IsStatic() == false) continue;
			pLight->ClearCasters(true);
			pLight->SetAllStaticCastersAdded(false);
		}
	}
}

//-----------------------------------------------------------------------

iGameEntity* cLevelEditor::GetGameEntity(cColladaNode *apNode)
{
	iGameEntity *pEntity = mpInit->mpMapHandler->GetGameEntity(GetObjectName(apNode), false);
	if(pEntity == nullptr) pEntity = mpInit->mpMapHandler->GetGameEntity(apNode->msName, false);
	return pEntity;
}

//-----------------------------------------------------------------------

tString cLevelEditor::GetLightName(const tString &asNodeName)
{
	// [dynamic_]<name> or [dynamic_]<file>_<name>, as the map loader has it
	tStringVec vParams;
	tString sSeparator = "_";
	cString::GetStringVec(asNodeName, vParams, &sSeparator);
	if(vParams.empty()) return asNodeName;
	size_t lFirst = cString::ToLowerCase(vParams[0]) == "dynamic" ? 1 : 0;
	if(vParams.size() >= 2 + lFirst) return vParams.back();
	return lFirst < vParams.size() ? vParams[lFirst] : asNodeName;
}

//-----------------------------------------------------------------------

void cLevelEditor::FindTargets(cColladaNode *apNode, std::vector<cLiveTarget> &avTargets)
{
	avTargets.clear();
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld == nullptr) return;

	tString sKind = GetKind(apNode, *mpData);
	tString sObject = GetObjectName(apNode);
	auto Add = [&avTargets](int alType, const tString &asName, int alIndex, const cMatrixf &a_mtxStart)
	{
		avTargets.push_back(cLiveTarget{alType, asName, alIndex, a_mtxStart, cVector3f(0)});
	};
	// A game entity: its bodies, or its mesh if it has none
	auto AddGameEntity = [&](iGameEntity *apEntity)
	{
		if(apEntity == nullptr) return;
		for(int i = 0; i < apEntity->GetBodyNum(); ++i)
			if(apEntity->GetBody(i)) Add(eLive_EntityBody, apEntity->GetName(), i, apEntity->GetBody(i)->GetLocalMatrix());
		if(apEntity->GetBodyNum() == 0 && apEntity->GetMeshEntity())
			Add(eLive_EntityMesh, apEntity->GetName(), 0, apEntity->GetMeshEntity()->GetLocalMatrix());
	};

	if(sKind == "entity")
		AddGameEntity(GetGameEntity(apNode));
	else if(sKind == "static" || sKind == "mesh" || sKind == "collider")
	{
		cMeshEntity *pMesh = pWorld->GetMeshEntity(apNode->msName);
		if(pMesh) Add(eLive_Mesh, apNode->msName, 0, pMesh->GetLocalMatrix());
		iPhysicsBody *pBody = pWorld->GetPhysicsWorld()->GetBody(apNode->msName);
		if(pBody) Add(eLive_Body, apNode->msName, 0, pBody->GetLocalMatrix());
	}
	else if(sKind == "light")
	{
		iLight3D *pLight = pWorld->GetLight(GetLightName(apNode->msName));
		if(pLight)
		{
			Add(eLive_Light, pLight->GetName(), 0, pLight->GetLocalMatrix());
			avTargets.back().mvStartSize.x = pLight->GetFarAttenuation();
		}
	}
	else if(sKind == "area")
	{
		cAreaEntity *pArea = pWorld->GetAreaEntity(sObject);
		if(pArea)
		{
			Add(eLive_Area, sObject, 0, pArea->m_mtxTransform);
			avTargets.back().mvStartSize = pArea->mvSize;
		}
		AddGameEntity(mpInit->mpMapHandler->GetGameEntity(sObject, false));
	}
	else if(sKind == "start")
	{
		tString sName = cString::Sub(apNode->msName, 7);
		cStartPosEntity *pStart = pWorld->GetStartPosEntity(sName);
		if(pStart) Add(eLive_Start, sName, 0, pStart->GetLocalMatrix());
	}
	else if(sKind == "sound")
	{
		cSoundEntity *pSound = pWorld->GetSoundEntity(sObject);
		if(pSound) Add(eLive_Sound, sObject, 0, pSound->GetLocalMatrix());
	}
	else if(sKind == "particles")
	{
		cParticleSystem3D *pPS = pWorld->GetParticleSystem(sObject);
		if(pPS) Add(eLive_Particles, sObject, 0, pPS->GetLocalMatrix());
	}
	else if(sKind == "billboard")
	{
		cBillboard *pBillboard = pWorld->GetBillboard(sObject);
		if(pBillboard) Add(eLive_Billboard, sObject, 0, pBillboard->GetLocalMatrix());
	}
	else if(sKind == "beam")
	{
		cBeam *pBeam = pWorld->GetBeam(sObject);
		if(pBeam) Add(eLive_Beam, sObject, 0, pBeam->GetLocalMatrix());
	}
}

//-----------------------------------------------------------------------

iEntity3D* cLevelEditor::GetTargetEntity(const cLiveTarget &aTarget)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	if(pWorld == nullptr) return nullptr;
	switch(aTarget.mType)
	{
	case eLive_Mesh: return pWorld->GetMeshEntity(aTarget.msName);
	case eLive_Body: return pWorld->GetPhysicsWorld()->GetBody(aTarget.msName);
	case eLive_EntityBody:
	case eLive_EntityMesh:
	{
		// Entities can go during a game: looked up each time
		iGameEntity *pEntity = mpInit->mpMapHandler->GetGameEntity(aTarget.msName, false);
		if(pEntity == nullptr) return nullptr;
		if(aTarget.mType == eLive_EntityMesh) return pEntity->GetMeshEntity();
		return aTarget.mlIndex < pEntity->GetBodyNum() ? pEntity->GetBody(aTarget.mlIndex) : nullptr;
	}
	case eLive_Light: return pWorld->GetLight(aTarget.msName);
	case eLive_Sound: return pWorld->GetSoundEntity(aTarget.msName);
	case eLive_Particles: return pWorld->GetParticleSystem(aTarget.msName);
	case eLive_Billboard: return pWorld->GetBillboard(aTarget.msName);
	case eLive_Beam: return pWorld->GetBeam(aTarget.msName);
	default: return nullptr;
	}
}

//-----------------------------------------------------------------------

void cLevelEditor::BeginUndo(const tString &asLabel)
{
	if(mlUndoDepth++ == 0)
	{
		mOpenStep.msLabel = asLabel;
		mOpenStep.mvNodes.clear();
	}
}

void cLevelEditor::EndUndo()
{
	if(mlUndoDepth == 0 || --mlUndoDepth > 0) return;

	// What is where it started is no step
	cUndoStep step;
	step.msLabel = mOpenStep.msLabel;
	for(auto &node : mOpenStep.mvNodes)
	{
		cColladaNode *pNode = FindNode(std::get<0>(node));
		if(pNode == nullptr) continue;
		std::get<2>(node) = cState{pNode->m_mtxTransform, pNode->mvScale};
		if(std::get<1>(node).m_mtxTransform == std::get<2>(node).m_mtxTransform &&
			std::get<1>(node).mvScale == std::get<2>(node).mvScale)
			continue;
		step.mvNodes.push_back(node);
	}
	mOpenStep.mvNodes.clear();
	if(step.mvNodes.empty()) return;

	mvUndo.resize(mlUndo);
	mvUndo.push_back(step);
	// Enough for an afternoon
	if(mvUndo.size() > 500) mvUndo.erase(mvUndo.begin());
	mlUndo = mvUndo.size();
}

void cLevelEditor::AddToUndo(cColladaNode *apNode, const cState &aBefore)
{
	bool bOwnStep = mlUndoDepth == 0;
	if(bOwnStep) BeginUndo("Change " + GetObjectName(apNode));
	bool bKnown = false;
	for(const auto &node : mOpenStep.mvNodes)
		if(std::get<0>(node) == apNode->msName) bKnown = true;
	if(bKnown == false) mOpenStep.mvNodes.push_back(std::make_tuple(apNode->msName, aBefore, aBefore));
	if(bOwnStep) EndUndo();
}

tString cLevelEditor::Undo()
{
	if(CanUndo() == false || mpData == nullptr) return "";
	--mlUndo;
	ApplyStep(mvUndo[mlUndo], true);
	return std::get<0>(mvUndo[mlUndo].mvNodes.front());
}

tString cLevelEditor::Redo()
{
	if(CanRedo() == false || mpData == nullptr) return "";
	ApplyStep(mvUndo[mlUndo], false);
	++mlUndo;
	return std::get<0>(mvUndo[mlUndo - 1].mvNodes.front());
}

void cLevelEditor::ApplyStep(const cUndoStep &aStep, bool abUndo)
{
	mbApplyingUndo = true;
	for(const auto &node : aStep.mvNodes)
	{
		cColladaNode *pNode = FindNode(std::get<0>(node));
		const cState &state = abUndo ? std::get<1>(node) : std::get<2>(node);
		if(pNode) ChangeNode(pNode, state.m_mtxTransform, state.mvScale);
	}
	mbApplyingUndo = false;
}

void cLevelEditor::ClearUndo()
{
	mvUndo.clear();
	mlUndo = 0;
	mlUndoDepth = 0;
	mOpenStep.mvNodes.clear();
}

//-----------------------------------------------------------------------
