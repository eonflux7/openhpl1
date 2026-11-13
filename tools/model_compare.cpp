// Copyright (C) 2026 - OpenHPL1 contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model_compare.h"

#include "hpl.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

using namespace hpl;

// Differences between what the engine reads from two files

static bool gbExact = false;

void SetModelCompareExact(bool abX)
{
	gbExact = abX;
}

static bool Near(float afA, float afB, float afTolerance)
{
	if(gbExact) return afA == afB;
	return std::fabs(afA - afB) <= afTolerance * std::max(1.0f, std::max(std::fabs(afA), std::fabs(afB)));
}

static bool Near(const cVector3f &avA, const cVector3f &avB, float afTolerance)
{
	return Near(avA.x, avB.x, afTolerance) && Near(avA.y, avB.y, afTolerance) && Near(avA.z, avB.z, afTolerance);
}

static bool Near(const cMatrixf &a_mtxA, const cMatrixf &a_mtxB, float afTolerance)
{
	for(int i = 0; i < 16; ++i)
		if(Near(a_mtxA.v[i], a_mtxB.v[i], afTolerance) == false) return false;
	return true;
}

static bool Near(const cQuaternion &aqA, const cQuaternion &aqB)
{
	if(gbExact)
		return (aqA.w == aqB.w && aqA.v == aqB.v) || (aqA.w == -aqB.w && aqA.v == aqB.v * -1.0f);
	float fDot = std::fabs(cMath::QuaternionDot(aqA, aqB));
	return 1.0f - fDot < 2e-6f;
}

static tString Str(const cVector3f &avV)
{
	return avV.ToString();
}

std::vector<cColladaNode *> ModelSceneNodes(cColladaScene &aScene)
{
	std::vector<cColladaNode *> vNodes;
	std::vector<cColladaNode *> vStack(aScene.mRoot.mlstChildren.rbegin(), aScene.mRoot.mlstChildren.rend());
	while(vStack.empty() == false)
	{
		cColladaNode *pNode = vStack.back();
		vStack.pop_back();
		vNodes.push_back(pNode);
		vStack.insert(vStack.end(), pNode->mlstChildren.rbegin(), pNode->mlstChildren.rend());
	}
	return vNodes;
}

cColladaGeometry *ModelFindGeometry(cColladaData &aData, const tString &asId)
{
	for(cColladaGeometry &geom : aData.mvGeometries)
		if(geom.msId == asId) return &geom;
	return nullptr;
}

static cColladaController *FindController(cColladaData &aData, const tString &asId)
{
	for(cColladaController &ctrl : aData.mvControllers)
		if(ctrl.msId == asId) return &ctrl;
	return nullptr;
}

static cColladaLight *FindLight(cColladaData &aData, const tString &asId)
{
	for(cColladaLight &light : aData.mvLights)
		if(light.msId == asId) return &light;
	return nullptr;
}

// A geometry as the engine uses it: vertices with the bind shape applied
// (as cVertexBufferVBO::Transform), and the bone influences of each vertex.
class cEffectiveGeometry
{
public:
	tString msName;
	tString msMaterial;
	tVertexVec mvVertices;
	tFloatVec mvTangents;
	tUIntVec mvIndices;
	std::vector<std::vector<std::pair<tString, float>>> mvInfluences;
};

static cEffectiveGeometry GetEffective(cMeshLoaderCollada &aLoader, cColladaData &aData, cColladaGeometry &aGeom,
	cColladaController *apCtrl)
{
	cEffectiveGeometry out;
	out.msName = aGeom.msName;
	out.msMaterial = aLoader.GetGeometryMaterialFile(aData, aGeom);
	out.mvVertices = aGeom.mvVertexVec;
	out.mvTangents = aGeom.mvTangents;
	out.mvIndices = aGeom.mvIndexVec;
	if(apCtrl)
	{
		cMatrixf mtxBind = apCtrl->m_mtxBindShapeMatrix;
		cMatrixf mtxRot = mtxBind.GetRotation();
		for(size_t i = 0; i < out.mvVertices.size(); ++i)
		{
			cVertex &vtx = out.mvVertices[i];
			vtx.pos = cMath::MatrixMul(mtxBind, vtx.pos);
			vtx.norm = cMath::MatrixMul(mtxRot, vtx.norm);
			vtx.norm.Normalise();
			if(i * 4 + 3 < out.mvTangents.size())
			{
				cVector3f vTan(out.mvTangents[i * 4], out.mvTangents[i * 4 + 1], out.mvTangents[i * 4 + 2]);
				vTan = cMath::MatrixMul(mtxRot, vTan);
				vTan.Normalise();
				out.mvTangents[i * 4] = vTan.x;
				out.mvTangents[i * 4 + 1] = vTan.y;
				out.mvTangents[i * 4 + 2] = vTan.z;
			}
		}

		out.mvInfluences.resize(out.mvVertices.size());
		for(size_t j = 0; j < apCtrl->mvPairs.size() && j < aGeom.mvExtraVtxVec.size(); ++j)
			for(const cColladaExtraVtx &extra : aGeom.mvExtraVtxVec[j])
			{
				if(extra.mlNewVtx < 0 || (size_t)extra.mlNewVtx >= out.mvVertices.size()) continue;
				for(const cColladaJointPair &pair : apCtrl->mvPairs[j])
				{
					tString sJoint = pair.mlJoint >= 0 && (size_t)pair.mlJoint < apCtrl->mvJoints.size() ? apCtrl->mvJoints[pair.mlJoint] : "?";
					float fWeight = pair.mlWeight >= 0 && (size_t)pair.mlWeight < apCtrl->mvWeights.size() ? apCtrl->mvWeights[pair.mlWeight] : 0;
					out.mvInfluences[extra.mlNewVtx].push_back(std::pair<tString, float>(sJoint, fWeight));
				}
			}
		//The engine normalises each vertex's weights (cSubMesh::CompileBonePairs)
		for(auto &vList : out.mvInfluences)
		{
			std::sort(vList.begin(), vList.end());
			float fTotal = 0;
			for(const auto &inf : vList) fTotal += inf.second;
			if(fTotal > 0)
				for(auto &inf : vList) inf.second /= fTotal;
		}
	}
	return out;
}

static void CompareGeometry(const tString &asNode, const cEffectiveGeometry &aA, const cEffectiveGeometry &aB, cModelDiff &aDiff)
{
	tString sWhere = "node '" + asNode + "' geometry '" + aA.msName + "': ";
	if(aA.msName != aB.msName) aDiff.Error(sWhere + "name is '" + aB.msName + "'");
	if(cString::ToLowerCase(cString::SetFileExt(aA.msMaterial, "")) != cString::ToLowerCase(cString::SetFileExt(aB.msMaterial, "")))
		aDiff.Error(sWhere + "material '" + aA.msMaterial + "' is '" + aB.msMaterial + "'");
	if(aA.mvVertices.size() != aB.mvVertices.size() || aA.mvIndices != aB.mvIndices)
	{
		aDiff.Error(sWhere + cString::ToString((int)aA.mvVertices.size()) + " vertices/" +
			cString::ToString((int)aA.mvIndices.size()) + " indices are " +
			cString::ToString((int)aB.mvVertices.size()) + "/" + cString::ToString((int)aB.mvIndices.size()) +
			(aA.mvIndices.size() == aB.mvIndices.size() ? " (other order)" : ""));
		return;
	}

	int lPos = 0, lNorm = 0, lTex = 0, lTan = 0, lInf = 0;
	tString sFirstInf;
	auto Influences = [](const std::vector<std::pair<tString, float>> &avList)
	{
		tString sOut;
		for(const auto &inf : avList) sOut += (sOut == "" ? "" : ", ") + inf.first + " " + cString::ToString(inf.second);
		return "(" + sOut + ")";
	};
	for(size_t i = 0; i < aA.mvVertices.size(); ++i)
	{
		if(Near(aA.mvVertices[i].pos, aB.mvVertices[i].pos, 1e-4f) == false) ++lPos;
		if(Near(aA.mvVertices[i].norm, aB.mvVertices[i].norm, 1e-4f) == false) ++lNorm;
		//The third texture coordinate is not used
		if(Near(aA.mvVertices[i].tex.x, aB.mvVertices[i].tex.x, 1e-5f) == false ||
			Near(aA.mvVertices[i].tex.y, aB.mvVertices[i].tex.y, 1e-5f) == false) ++lTex;
		if(i * 4 + 3 < aA.mvTangents.size() && i * 4 + 3 < aB.mvTangents.size())
		{
			cVector3f vA(aA.mvTangents[i * 4], aA.mvTangents[i * 4 + 1], aA.mvTangents[i * 4 + 2]);
			cVector3f vB(aB.mvTangents[i * 4], aB.mvTangents[i * 4 + 1], aB.mvTangents[i * 4 + 2]);
			if(Near(vA, vB, 2e-3f) == false || (aA.mvTangents[i * 4 + 3] < 0) != (aB.mvTangents[i * 4 + 3] < 0)) ++lTan;
		}
		if(i < aA.mvInfluences.size() || i < aB.mvInfluences.size())
		{
			bool bSame = i < aA.mvInfluences.size() && i < aB.mvInfluences.size() &&
				aA.mvInfluences[i].size() == aB.mvInfluences[i].size();
			for(size_t k = 0; bSame && k < aA.mvInfluences[i].size(); ++k)
			{
				if(aA.mvInfluences[i][k].first != aB.mvInfluences[i][k].first ||
					Near(aA.mvInfluences[i][k].second, aB.mvInfluences[i][k].second, 1e-5f) == false) bSame = false;
			}
			if(bSame == false && lInf++ == 0 && i < aA.mvInfluences.size() && i < aB.mvInfluences.size())
				sFirstInf = ", first: vertex " + cString::ToString((int)i) + " " + Influences(aA.mvInfluences[i]) +
					" is " + Influences(aB.mvInfluences[i]);
		}
	}
	if(lPos) aDiff.Error(sWhere + cString::ToString(lPos) + " positions differ");
	if(lNorm) aDiff.Error(sWhere + cString::ToString(lNorm) + " normals differ");
	if(lTex) aDiff.Error(sWhere + cString::ToString(lTex) + " texture coordinates differ");
	if(lTan) aDiff.Error(sWhere + cString::ToString(lTan) + " tangents differ");
	if(lInf) aDiff.Error(sWhere + cString::ToString(lInf) + " vertices have other bone weights" + sFirstInf);
}

static void CompareAnimation(cMeshLoaderCollada &aLoader, cColladaData &aA, cColladaData &aB, cModelDiff &aDiff)
{
	cSkeleton *pSkelA = nullptr, *pSkelB = nullptr;
	cAnimation *pAnimA = aLoader.CreateAnimation(aA, true, &pSkelA);
	cAnimation *pAnimB = aLoader.CreateAnimation(aB, true, &pSkelB);

	if((pSkelA == nullptr) != (pSkelB == nullptr)) aDiff.Error("skeleton only in one file");
	if(pSkelA && pSkelB)
	{
		if(pSkelA->GetBoneNum() != pSkelB->GetBoneNum())
			aDiff.Error("bones: " + cString::ToString(pSkelA->GetBoneNum()) + " are " + cString::ToString(pSkelB->GetBoneNum()));
		for(int i = 0; i < pSkelA->GetBoneNum(); ++i)
		{
			cBone *pA = pSkelA->GetBoneByIndex(i);
			cBone *pB = pSkelB->GetBoneByName(pA->GetName());
			if(pB == nullptr)
			{
				aDiff.Error("bone '" + pA->GetName() + "' is missing");
				continue;
			}
			if(Near(pA->GetLocalTransform(), pB->GetLocalTransform(), 1e-4f) == false)
				aDiff.Error("bone '" + pA->GetName() + "' has another bind transform");
			if((pA->GetParent() ? pA->GetParent()->GetName() : "") != (pB->GetParent() ? pB->GetParent()->GetName() : ""))
				aDiff.Error("bone '" + pA->GetName() + "' has another parent");
		}
	}

	if((pAnimA == nullptr) != (pAnimB == nullptr)) aDiff.Error("animation only in one file");
	if(pAnimA && pAnimB)
	{
		if(Near(pAnimA->GetLength(), pAnimB->GetLength(), 1e-5f) == false)
			aDiff.Error("animation length " + cString::ToString(pAnimA->GetLength()) + " is " + cString::ToString(pAnimB->GetLength()));
		if(pAnimA->GetTrackNum() != pAnimB->GetTrackNum())
			aDiff.Error("animation tracks: " + cString::ToString(pAnimA->GetTrackNum()) + " are " + cString::ToString(pAnimB->GetTrackNum()));
		for(int t = 0; t < pAnimA->GetTrackNum(); ++t)
		{
			cAnimationTrack *pA = pAnimA->GetTrack(t);
			if(pA == nullptr) continue;
			cAnimationTrack *pB = pAnimB->GetTrackByName(pA->GetName());
			tString sWhere = tString("track '") + pA->GetName() + "': ";
			if(pB == nullptr)
			{
				aDiff.Error(sWhere + "missing");
				continue;
			}
			if(pA->GetKeyFrameNum() != pB->GetKeyFrameNum())
			{
				//Resampled (Blender exports a key per frame): compare the motion at
				//the first file's keys
				float fMaxTrans = 0, fMaxAngle = 0;
				for(int k = 0; k < pA->GetKeyFrameNum(); ++k)
				{
					cKeyFrame *pKA = pA->GetKeyFrame(k);
					cKeyFrame keyB = pB->GetInterpolatedKeyFrame(pKA->time);
					fMaxTrans = std::max(fMaxTrans, cMath::Vector3Dist(pKA->trans, keyB.trans));
					float fDot = std::min(1.0f, std::fabs(cMath::QuaternionDot(pKA->rotation, keyB.rotation)));
					fMaxAngle = std::max(fMaxAngle, cMath::ToDeg(2 * std::acos(fDot)));
				}
				tString sText = sWhere + cString::ToString(pA->GetKeyFrameNum()) + " keys are " +
					cString::ToString(pB->GetKeyFrameNum()) + ", at most " + cString::ToString(fMaxTrans) +
					" apart and " + cString::ToString(fMaxAngle) + " degrees";
				if(fMaxTrans <= 1e-3f && fMaxAngle <= 0.5f)
					aDiff.Note(sText);
				else
					aDiff.Error(sText);
				continue;
			}
			int lBad = 0;
			tString sFirst;
			for(int k = 0; k < pA->GetKeyFrameNum(); ++k)
			{
				cKeyFrame *pKA = pA->GetKeyFrame(k), *pKB = pB->GetKeyFrame(k);
				bool bTime = Near(pKA->time, pKB->time, 1e-5f);
				bool bTrans = Near(pKA->trans, pKB->trans, 1e-4f);
				bool bRot = Near(pKA->rotation, pKB->rotation);
				if(bTime && bTrans && bRot) continue;
				if(lBad++ == 0)
				{
					sFirst = "key " + cString::ToString(k) + (bTime ? "" : " time") + (bTrans ? "" : " translation " + Str(pKA->trans) + " is " + Str(pKB->trans)) + (bRot ? "" : " rotation");
				}
			}
			if(lBad) aDiff.Error(sWhere + cString::ToString(lBad) + " keys differ, first: " + sFirst);
		}
	}

	if(pAnimA) hplDelete(pAnimA);
	if(pAnimB) hplDelete(pAnimB);
	if(pSkelA) hplDelete(pSkelA);
	if(pSkelB) hplDelete(pSkelB);
}

// What a node's source is to the loader
tString ModelSourceKind(cColladaData &aData, cColladaNode *apNode)
{
	if(apNode->msSource == "") return "none";
	if(apNode->mbSourceIsFile) return "file";
	if(cColladaGeometry *pGeom = ModelFindGeometry(aData, apNode->msSource))
		return pGeom->mvVertexVec.empty() ? "empty geometry" : "geometry";
	if(FindController(aData, apNode->msSource)) return "skin";
	if(FindLight(aData, apNode->msSource)) return "light";
	return "missing";
}

void CompareModelData(cMeshLoaderCollada &aLoader, cColladaData &aA, cColladaData &aB, cModelDiff &aDiff)
{
	std::vector<cColladaNode *> vA = ModelSceneNodes(aA.mScene), vB = ModelSceneNodes(aB.mScene);
	std::map<tString, cColladaNode *> mapB;
	for(cColladaNode *pNode : vB) mapB.insert(std::pair<tString, cColladaNode *>(pNode->msId, pNode));

	std::set<cColladaNode *> setMatched;
	for(cColladaNode *pA : vA)
	{
		std::map<tString, cColladaNode *>::iterator it = mapB.find(pA->msId);
		tString sWhere = "node '" + pA->msName + "': ";
		if(it == mapB.end())
		{
			aDiff.Error(sWhere + "missing");
			continue;
		}
		cColladaNode *pB = it->second;
		setMatched.insert(pB);

		if(pA->msName != pB->msName) aDiff.Error(sWhere + "name is '" + pB->msName + "'");
		//The loader only tells joints from other nodes
		if((pA->msType == "JOINT") != (pB->msType == "JOINT"))
			aDiff.Error(sWhere + "type " + pA->msType + " is " + pB->msType);
		//A joint's own transform is only used for bones without animation
		//tracks (their bind pose comes from the skin), and a skinned mesh's
		//place in the tree and transform are not used: these are notes.
		const bool bMinor = pA->msType == "JOINT" || ModelSourceKind(aA, pA) == "skin";
		auto Report = [&](const tString &asText)
		{ if(bMinor) aDiff.Note(asText); else aDiff.Error(asText); };
		tString sParentA = pA->pParent ? pA->pParent->msName : "", sParentB = pB->pParent ? pB->pParent->msName : "";
		if(sParentA != sParentB) Report(sWhere + "parent '" + sParentA + "' is '" + sParentB + "'");
		if(Near(pA->mvScale, pB->mvScale, 1e-5f) == false) aDiff.Error(sWhere + "scale " + Str(pA->mvScale) + " is " + Str(pB->mvScale));
		if(Near(pA->m_mtxTransform, pB->m_mtxTransform, 1e-4f) == false)
			Report(sWhere + "local transform " + pA->m_mtxTransform.ToString() + " is " + pB->m_mtxTransform.ToString());
		if(Near(pA->m_mtxWorldTransform, pB->m_mtxWorldTransform, 1e-4f) == false) Report(sWhere + "world transform differs");

		tString sKindA = ModelSourceKind(aA, pA), sKindB = ModelSourceKind(aB, pB);
		if(sKindA == "empty geometry" && sKindB == "none" && pA->msName.size() && pA->msName[0] != '_')
		{
			aDiff.Note(sWhere + "had an empty geometry");
			continue;
		}
		if(sKindA != sKindB)
		{
			aDiff.Error(sWhere + "source " + sKindA + " is " + sKindB);
			continue;
		}

		if(sKindA == "file" && pA->msSource != pB->msSource)
			aDiff.Error(sWhere + "file '" + pA->msSource + "' is '" + pB->msSource + "'");
		else if(sKindA == "empty geometry")
		{
			cColladaGeometry *pGA = ModelFindGeometry(aA, pA->msSource), *pGB = ModelFindGeometry(aB, pB->msSource);
			if(pGA->msName != pGB->msName) aDiff.Error(sWhere + "geometry name '" + pGA->msName + "' is '" + pGB->msName + "'");
		}
		else if(sKindA == "geometry")
		{
			CompareGeometry(pA->msName, GetEffective(aLoader, aA, *ModelFindGeometry(aA, pA->msSource), nullptr),
				GetEffective(aLoader, aB, *ModelFindGeometry(aB, pB->msSource), nullptr), aDiff);
		}
		else if(sKindA == "skin")
		{
			cColladaController *pCA = FindController(aA, pA->msSource), *pCB = FindController(aB, pB->msSource);
			cColladaGeometry *pGA = ModelFindGeometry(aA, pCA->msTarget), *pGB = ModelFindGeometry(aB, pCB->msTarget);
			//The joints of B may be more (Blender lists all of an armature's bones)
			bool bJointsOk = true;
			for(size_t j = 0; j < pCA->mvJoints.size(); ++j)
			{
				auto it = std::find(pCB->mvJoints.begin(), pCB->mvJoints.end(), pCA->mvJoints[j]);
				if(it == pCB->mvJoints.end())
				{
					aDiff.Error(sWhere + "skin joint '" + pCA->mvJoints[j] + "' is missing");
					bJointsOk = false;
					continue;
				}
				size_t k = (size_t)(it - pCB->mvJoints.begin());
				if(j < pCA->mvMatrices.size() && k < pCB->mvMatrices.size() && Near(pCA->mvMatrices[j], pCB->mvMatrices[k], 1e-4f) == false)
					aDiff.Error(sWhere + "inverse bind matrix of '" + pCA->mvJoints[j] + "' differs");
			}
			if(bJointsOk && pCA->mvJoints != pCB->mvJoints)
				aDiff.Note(sWhere + "skin joints are " + cString::ToString((int)pCB->mvJoints.size()) + " in another order");
			if(pGA && pGB)
				CompareGeometry(pA->msName, GetEffective(aLoader, aA, *pGA, pCA), GetEffective(aLoader, aB, *pGB, pCB), aDiff);
			else if(pGA || pGB)
				aDiff.Error(sWhere + "skin geometry only in one file");
		}
		else if(sKindA == "light")
		{
			cColladaLight *pLA = FindLight(aA, pA->msSource), *pLB = FindLight(aB, pB->msSource);
			if(pLA->msType != pLB->msType) aDiff.Error(sWhere + "light type " + pLA->msType + " is " + pLB->msType);
			if(Near(pLA->mDiffuseColor.r, pLB->mDiffuseColor.r, 1e-5f) == false || Near(pLA->mDiffuseColor.g, pLB->mDiffuseColor.g, 1e-5f) == false ||
				Near(pLA->mDiffuseColor.b, pLB->mDiffuseColor.b, 1e-5f) == false || Near(pLA->mDiffuseColor.a, pLB->mDiffuseColor.a, 1e-5f) == false)
				aDiff.Error(sWhere + "light colour " + pLA->mDiffuseColor.ToString() + " is " + pLB->mDiffuseColor.ToString());
			if(pLA->msType == "spot" && Near(pLA->mfAngle, pLB->mfAngle, 1e-4f) == false)
				aDiff.Error(sWhere + "light angle " + cString::ToString(pLA->mfAngle) + " is " + cString::ToString(pLB->mfAngle));
			if(pLA->msName != pLB->msName) aDiff.Note(sWhere + "light name '" + pLA->msName + "' is '" + pLB->msName + "'");
		}
	}
	for(cColladaNode *pB : vB)
		if(setMatched.count(pB) == 0) aDiff.Error("node '" + pB->msName + "' is new");

	//The loader makes sub meshes, lights and bind poses in these orders
	auto GeometryOrder = [](cColladaData &aData)
	{
		tStringVec vNames;
		for(cColladaGeometry &geom : aData.mvGeometries)
			if(geom.mvVertexVec.empty() == false) vNames.push_back(geom.msName);
		return vNames;
	};
	auto LightOrder = [](cColladaData &aData)
	{
		tStringVec vNames;
		for(cColladaNode *pNode : ModelSceneNodes(aData.mScene))
			for(size_t i = 0; i < aData.mvLights.size(); ++i)
				if(aData.mvLights[i].msId == pNode->msSource) vNames.push_back(cString::ToString((int)i) + pNode->msName);
		std::sort(vNames.begin(), vNames.end(), [](const tString &a, const tString &b)
			{ return cString::ToInt(a.c_str(), 0) < cString::ToInt(b.c_str(), 0); });
		for(tString &sName : vNames) sName = sName.substr(sName.find_first_not_of("0123456789"));
		return vNames;
	};
	auto ControllerOrder = [](cColladaData &aData)
	{
		tStringVec vNames;
		for(cColladaController &ctrl : aData.mvControllers)
		{
			cColladaGeometry *pGeom = ModelFindGeometry(aData, ctrl.msTarget);
			vNames.push_back(pGeom ? pGeom->msName : ctrl.msTarget);
		}
		return vNames;
	};
	if(GeometryOrder(aA) != GeometryOrder(aB)) aDiff.Error("geometries are in another order");
	if(LightOrder(aA) != LightOrder(aB)) aDiff.Error("lights are in another order");
	if(ControllerOrder(aA) != ControllerOrder(aB)) aDiff.Error("skins are in another order");

	CompareAnimation(aLoader, aA, aB, aDiff);
}

bool CompareModelFiles(cMeshLoaderCollada &aLoader, const tString &asA, const tString &asB, cModelDiff &aDiff)
{
	cColladaData dataA, dataB;
	if(aLoader.LoadData(asA, dataA) == false)
	{
		aDiff.Error("'" + asA + "' could not be read");
		return false;
	}
	if(aLoader.LoadData(asB, dataB) == false)
	{
		aDiff.Error("'" + asB + "' could not be read");
		return false;
	}
	CompareModelData(aLoader, dataA, dataB, aDiff);
	return aDiff.mvErrors.empty();
}
