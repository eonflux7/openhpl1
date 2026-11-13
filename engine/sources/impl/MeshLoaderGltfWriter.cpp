// Copyright (C) 2026 - OpenHPL1 contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Writes the Collada loader's intermediate data as glTF 2.0, so that the
// glTF reader (MeshLoaderGltf.cpp) gives the engine the same meshes, worlds
// and animations as the Collada file. Used by tools/model_tool.cpp.
//
// What the engine uses is written, as the loader has made it: a geometry's
// vertices after splitting, triangles left out by the loader stay out, and
// a skin's bind shape matrix is applied to its vertices. Animations are
// written as the keys the engine makes of the Collada channels, as local
// node transforms.

#include "impl/MeshLoaderCollada.h"

#include "system/LowLevelSystem.h"
#include "system/String.h"

#include "graphics/Animation.h"
#include "graphics/AnimationTrack.h"
#include "graphics/Bone.h"
#include "graphics/Skeleton.h"

#include "math/Math.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>

namespace hpl
{

	// JSON AND BUFFER HELPERS

	static tString JsonString(const tString &asText)
	{
		tString sOut = "\"";
		for(unsigned char c : asText)
		{
			if(c == '"')
				sOut += "\\\"";
			else if(c == '\\')
				sOut += "\\\\";
			else if(c == '\n')
				sOut += "\\n";
			else if(c < 0x20)
			{
				char sBuf[8];
				snprintf(sBuf, sizeof(sBuf), "\\u%04x", c);
				sOut += sBuf;
			}
			else
				sOut += (char)c;
		}
		return sOut + "\"";
	}

	static tString JsonFloat(float afX)
	{
		if(std::isfinite(afX) == false) afX = 0;
		char sBuf[32];
		snprintf(sBuf, sizeof(sBuf), "%.9g", afX);
		return sBuf;
	}

	// Bit for bit (a -0 where the Collada data has 0 counts as a difference)
	static bool SameBits(const float *apA, const float *apB, int alCount)
	{
		for(int i = 0; i < alCount; ++i)
		{
			uint32_t lA, lB;
			memcpy(&lA, &apA[i], 4);
			memcpy(&lB, &apB[i], 4);
			if(lA != lB) return false;
		}
		return true;
	}

	// A float as a reader gets it back from JsonFloat
	static float JsonRoundTrip(float afX)
	{
		return (float)atof(JsonFloat(afX).c_str());
	}

	static tString JsonFloats(const float *apValues, size_t alCount)
	{
		tString sOut = "[";
		for(size_t i = 0; i < alCount; ++i)
		{
			if(i) sOut += ",";
			sOut += JsonFloat(apValues[i]);
		}
		return sOut + "]";
	}

	static tString JsonInts(const std::vector<int> &avValues)
	{
		tString sOut = "[";
		for(size_t i = 0; i < avValues.size(); ++i)
		{
			if(i) sOut += ",";
			sOut += cString::ToString(avValues[i]);
		}
		return sOut + "]";
	}

	// An object: "key":value pairs, joined.
	static tString JsonObject(const tStringVec &avMembers)
	{
		tString sOut = "{";
		for(size_t i = 0; i < avMembers.size(); ++i)
		{
			if(i) sOut += ",";
			sOut += avMembers[i];
		}
		return sOut + "}";
	}

	static tString JsonMember(const char *asKey, const tString &asValue)
	{
		return JsonString(asKey) + ":" + asValue;
	}

	static tString JsonArray(const tStringVec &avItems)
	{
		tString sOut = "[";
		for(size_t i = 0; i < avItems.size(); ++i)
		{
			if(i) sOut += ",";
			sOut += avItems[i];
		}
		return sOut + "]";
	}

#define kGltfFloat 5126
#define kGltfUInt 5125
#define kGltfUShort 5123
#define kGltfArrayBuffer 34962
#define kGltfElementBuffer 34963

	class cGltfBuffer
	{
	public:
		std::vector<unsigned char> mvData;
		tStringVec mvViews;
		tStringVec mvAccessors;

		// Adds data and an accessor of it; returns the accessor index.
		int AddAccessor(const void *apData, size_t alBytes, size_t alCount, int alComponentType,
			const char *asType, int alTarget, const tString &asMinMax = "")
		{
			while(mvData.size() % 4) mvData.push_back(0);
			size_t lOffset = mvData.size();
			mvData.insert(mvData.end(), (const unsigned char *)apData, (const unsigned char *)apData + alBytes);

			tStringVec vView;
			vView.push_back(JsonMember("buffer", "0"));
			vView.push_back(JsonMember("byteOffset", cString::ToString((int)lOffset)));
			vView.push_back(JsonMember("byteLength", cString::ToString((int)alBytes)));
			if(alTarget) vView.push_back(JsonMember("target", cString::ToString(alTarget)));
			mvViews.push_back(JsonObject(vView));

			tStringVec vAccessor;
			vAccessor.push_back(JsonMember("bufferView", cString::ToString((int)mvViews.size() - 1)));
			vAccessor.push_back(JsonMember("componentType", cString::ToString(alComponentType)));
			vAccessor.push_back(JsonMember("count", cString::ToString((int)alCount)));
			vAccessor.push_back(JsonMember("type", JsonString(asType)));
			if(asMinMax != "") vAccessor.push_back(asMinMax);
			mvAccessors.push_back(JsonObject(vAccessor));
			return (int)mvAccessors.size() - 1;
		}

		int AddFloats(const tFloatVec &avValues, int alComponents, const char *asType, int alTarget, bool abMinMax)
		{
			tString sMinMax;
			size_t lCount = avValues.size() / alComponents;
			if(abMinMax && lCount > 0)
			{
				std::vector<float> vMin(avValues.begin(), avValues.begin() + alComponents);
				std::vector<float> vMax = vMin;
				for(size_t i = 0; i < lCount; ++i)
					for(int c = 0; c < alComponents; ++c)
					{
						vMin[c] = std::min(vMin[c], avValues[i * alComponents + c]);
						vMax[c] = std::max(vMax[c], avValues[i * alComponents + c]);
					}
				sMinMax = JsonMember("min", JsonFloats(vMin.data(), alComponents)) + "," +
					JsonMember("max", JsonFloats(vMax.data(), alComponents));
			}
			return AddAccessor(avValues.data(), avValues.size() * sizeof(float), lCount, kGltfFloat,
				asType, alTarget, sMinMax);
		}
	};

	// glTF stores matrices column by column
	static void MatrixToGltf(const cMatrixf &a_mtxA, float *apOut)
	{
		for(int c = 0; c < 4; ++c)
			for(int r = 0; r < 4; ++r)
				apOut[c * 4 + r] = a_mtxA.m[r][c];
	}

	static bool IsRotation(const cMatrixf &a_mtxA, float afEpsilon)
	{
		for(int i = 0; i < 3; ++i)
			for(int j = 0; j < 3; ++j)
			{
				float fDot = a_mtxA.m[0][i] * a_mtxA.m[0][j] + a_mtxA.m[1][i] * a_mtxA.m[1][j] + a_mtxA.m[2][i] * a_mtxA.m[2][j];
				if(std::fabs(fDot - (i == j ? 1.0f : 0.0f)) > afEpsilon) return false;
			}
		cVector3f vX(a_mtxA.m[0][0], a_mtxA.m[1][0], a_mtxA.m[2][0]);
		cVector3f vY(a_mtxA.m[0][1], a_mtxA.m[1][1], a_mtxA.m[2][1]);
		cVector3f vZ(a_mtxA.m[0][2], a_mtxA.m[1][2], a_mtxA.m[2][2]);
		return cMath::Vector3Dot(cMath::Vector3Cross(vX, vY), vZ) > 0;
	}

	static cQuaternion QuaternionOf(const cMatrixf &a_mtxRot)
	{
		cQuaternion q;
		q.FromRotationMatrix(a_mtxRot.GetRotation());
		q.Normalise();
		return q;
	}

	// Rotation of a matrix whose axes may be scaled
	static cQuaternion NormalisedRotation(const cMatrixf &a_mtxA)
	{
		cMatrixf mtx = a_mtxA.GetRotation();
		for(int c = 0; c < 3; ++c)
		{
			float fLength = sqrtf(mtx.m[0][c] * mtx.m[0][c] + mtx.m[1][c] * mtx.m[1][c] + mtx.m[2][c] * mtx.m[2][c]);
			if(fLength > 0)
				for(int r = 0; r < 3; ++r) mtx.m[r][c] /= fLength;
		}
		return QuaternionOf(mtx);
	}

	bool cMeshLoaderCollada::LoadData(const tString &asFile, cColladaData &aData)
	{
		return FillStructures(asFile, &aData.mvImages, &aData.mvTextures, &aData.mvMaterials,
			&aData.mvLights, &aData.mvGeometries, &aData.mvControllers,
			&aData.mvAnimations, &aData.mScene, false);
	}

	tString cMeshLoaderCollada::GetGeometryMaterialFile(cColladaData &aData, const cColladaGeometry &aGeometry)
	{
		if(aGeometry.msMaterial == "") return "";
		return GetMaterialTextureFile(aGeometry.msMaterial, aData.mvMaterials, aData.mvTextures, aData.mvImages);
	}

	cAnimation *cMeshLoaderCollada::CreateAnimation(cColladaData &aData, bool abRestPoses, cSkeleton **apSkeleton)
	{
		cSkeleton *pSkeleton = nullptr;
		if(aData.mvControllers.empty() == false) pSkeleton = CreateSkeleton(aData.mvControllers, aData.mScene);
		if(apSkeleton) *apSkeleton = pSkeleton;

		if(aData.mvAnimations.empty())
		{
			if(apSkeleton == nullptr && pSkeleton) hplDelete(pSkeleton);
			return nullptr;
		}

		cAnimation *pAnimation = hplNew(cAnimation, ("Default", ""));
		pAnimation->SetAnimationName("Default");
		pAnimation->SetLength(aData.mScene.mfDeltaTime);
		pAnimation->ResizeTracks((int)aData.mvAnimations.size());
		for(size_t i = 0; i < aData.mvAnimations.size(); i++)
		{
			CreateAnimTrack(pAnimation, pSkeleton, aData.mvAnimations[i], &aData.mScene);
		}
		if(abRestPoses && pSkeleton) AddRestPoseTracks(pAnimation, pSkeleton, aData.mScene);

		if(apSkeleton == nullptr && pSkeleton) hplDelete(pSkeleton);
		return pAnimation;
	}

	bool cMeshLoaderCollada::SaveGltf(cColladaData &aData, const tString &asFile, const tString &asSourceName,
		tStringVec *apWarnings)
	{
		auto Warn = [&](const tString &asText)
		{ if(apWarnings) apWarnings->push_back(asText); };

		cGltfBuffer buffer;

		// Nodes, in the order of the scene
		std::vector<cColladaNode *> vNodes;
		std::map<cColladaNode *, int> mapNodeIndex;
		std::vector<cColladaNode *> vStack(aData.mScene.mRoot.mlstChildren.rbegin(), aData.mScene.mRoot.mlstChildren.rend());
		while(vStack.empty() == false)
		{
			cColladaNode *pNode = vStack.back();
			vStack.pop_back();
			mapNodeIndex[pNode] = (int)vNodes.size();
			vNodes.push_back(pNode);
			vStack.insert(vStack.end(), pNode->mlstChildren.rbegin(), pNode->mlstChildren.rend());
		}

		auto NodeById = [&](const tString &asId) -> int
		{
			cColladaNode *pNode = aData.mScene.GetNode(asId);
			if(pNode == nullptr) return -1;
			std::map<cColladaNode *, int>::iterator it = mapNodeIndex.find(pNode);
			return it == mapNodeIndex.end() ? -1 : it->second;
		};

		std::set<tString> setSkinJoints;
		for(cColladaController &ctrl : aData.mvControllers)
			setSkinJoints.insert(ctrl.mvJoints.begin(), ctrl.mvJoints.end());

		{
			std::set<tString> setNames;
			for(cColladaNode *pNode : vNodes)
			{
				if(setNames.insert(pNode->msName).second == false)
					Warn("node name '" + pNode->msName + "' is used more than once (Blender renames one)");
			}
		}

		// Materials
		tStringVec vMaterials, vTextures, vImages;
		std::map<tString, int> mapMaterialIndex, mapImageIndex;
		auto GetMaterial = [&](const cColladaGeometry &aGeom) -> int
		{
			if(aGeom.msMaterial == "") return -1;
			std::map<tString, int>::iterator it = mapMaterialIndex.find(aGeom.msMaterial);
			if(it != mapMaterialIndex.end()) return it->second;

			tString sFile = GetGeometryMaterialFile(aData, aGeom);
			tString sName = sFile != "" ? cString::SetFileExt(sFile, "") : aGeom.msMaterial;

			tStringVec vPbr;
			if(sFile != "")
			{
				std::map<tString, int>::iterator imgIt = mapImageIndex.find(sFile);
				int lImage;
				if(imgIt == mapImageIndex.end())
				{
					lImage = (int)vImages.size();
					tString sUri;
					for(unsigned char c : sFile)
					{
						if(c == ' ' || c == '%' || c == '#' || c >= 0x80)
						{
							char sBuf[4];
							snprintf(sBuf, sizeof(sBuf), "%%%02X", c);
							sUri += sBuf;
						}
						else
							sUri += (char)c;
					}
					vImages.push_back(JsonObject({JsonMember("uri", JsonString(sUri)),
						JsonMember("name", JsonString(cString::SetFileExt(sFile, "")))}));
					vTextures.push_back(JsonObject({JsonMember("source", cString::ToString(lImage))}));
					mapImageIndex[sFile] = lImage;
				}
				else
					lImage = imgIt->second;
				vPbr.push_back(JsonMember("baseColorTexture", JsonObject({JsonMember("index", cString::ToString(lImage))})));
			}
			vPbr.push_back(JsonMember("metallicFactor", "0"));
			vPbr.push_back(JsonMember("roughnessFactor", "1"));

			int lIndex = (int)vMaterials.size();
			vMaterials.push_back(JsonObject({JsonMember("name", JsonString(sName)),
				JsonMember("pbrMetallicRoughness", JsonObject(vPbr)),
				JsonMember("extras", JsonObject({JsonMember("hpl_material", JsonString(sFile))}))}));
			mapMaterialIndex[aGeom.msMaterial] = lIndex;
			return lIndex;
		};

		// Meshes (one per geometry and bind shape)
		tStringVec vMeshes;
		std::map<std::pair<tString, const cColladaController *>, int> mapMeshIndex;
		auto GetMesh = [&](cColladaGeometry *apGeom, cColladaController *apCtrl) -> int
		{
			std::pair<tString, const cColladaController *> key(apGeom->msId, apCtrl);
			std::map<std::pair<tString, const cColladaController *>, int>::iterator it = mapMeshIndex.find(key);
			if(it != mapMeshIndex.end()) return it->second;
			mapMeshIndex[key] = -1;
			if(apGeom->mvVertexVec.empty() || apGeom->mvIndexVec.empty()) return -1;

			const size_t lVtxNum = apGeom->mvVertexVec.size();
			cMatrixf mtxBind = apCtrl ? apCtrl->m_mtxBindShapeMatrix : cMatrixf::Identity;
			cMatrixf mtxBindRot = mtxBind.GetRotation();

			//The tangents are written as the engine has them: made from the
			//vertices before the bind shape matrix is applied.
			const bool bTangents = apGeom->mvTangents.size() == lVtxNum * 4;
			tFloatVec vPos, vNorm, vTex, vTangents;
			vPos.reserve(lVtxNum * 3);
			vNorm.reserve(lVtxNum * 3);
			vTex.reserve(lVtxNum * 2);
			for(size_t i = 0; i < lVtxNum; ++i)
			{
				const cVertex &vtx = apGeom->mvVertexVec[i];
				cVector3f vP = vtx.pos, vN = vtx.norm, vT(0, 0, 0);
				if(bTangents) vT = cVector3f(apGeom->mvTangents[i * 4], apGeom->mvTangents[i * 4 + 1], apGeom->mvTangents[i * 4 + 2]);
				if(apCtrl)
				{
					//As cVertexBufferVBO::Transform, but not normalised: the
					//engine normalises them when it applies the (identity)
					//bind shape matrix of the glTF skin, and normalising twice
					//can change the last bit.
					vP = cMath::MatrixMul(mtxBind, vP);
					vN = cMath::MatrixMul(mtxBindRot, vN);
					vT = cMath::MatrixMul(mtxBindRot, vT);
				}
				vPos.push_back(vP.x);
				vPos.push_back(vP.y);
				vPos.push_back(vP.z);
				vNorm.push_back(vN.x);
				vNorm.push_back(vN.y);
				vNorm.push_back(vN.z);
				vTex.push_back(vtx.tex.x);
				vTex.push_back(vtx.tex.y);
				if(bTangents)
				{
					vTangents.push_back(vT.x);
					vTangents.push_back(vT.y);
					vTangents.push_back(vT.z);
					vTangents.push_back(apGeom->mvTangents[i * 4 + 3]);
				}
			}

			tStringVec vAttributes;
			vAttributes.push_back(JsonMember("POSITION", cString::ToString(buffer.AddFloats(vPos, 3, "VEC3", kGltfArrayBuffer, true))));
			vAttributes.push_back(JsonMember("NORMAL", cString::ToString(buffer.AddFloats(vNorm, 3, "VEC3", kGltfArrayBuffer, false))));
			vAttributes.push_back(JsonMember("TEXCOORD_0", cString::ToString(buffer.AddFloats(vTex, 2, "VEC2", kGltfArrayBuffer, false))));
			if(bTangents)
				vAttributes.push_back(JsonMember("TANGENT", cString::ToString(buffer.AddFloats(vTangents, 4, "VEC4", kGltfArrayBuffer, false))));

			if(apCtrl)
			{
				//The influences of each vertex, as the loader gives them to the sub mesh
				std::vector<std::vector<std::pair<int, float>>> vInfluences(lVtxNum);
				for(size_t j = 0; j < apCtrl->mvPairs.size() && j < apGeom->mvExtraVtxVec.size(); ++j)
				{
					for(const cColladaExtraVtx &extra : apGeom->mvExtraVtxVec[j])
					{
						if(extra.mlNewVtx < 0 || (size_t)extra.mlNewVtx >= lVtxNum) continue;
						for(const cColladaJointPair &pair : apCtrl->mvPairs[j])
						{
							if(pair.mlWeight < 0 || (size_t)pair.mlWeight >= apCtrl->mvWeights.size()) continue;
							vInfluences[extra.mlNewVtx].push_back(std::pair<int, float>(pair.mlJoint, apCtrl->mvWeights[pair.mlWeight]));
						}
					}
				}
				size_t lMax = 1;
				for(auto &vList : vInfluences) lMax = std::max(lMax, vList.size());
				const size_t lSets = (lMax + 3) / 4;
				for(size_t lSet = 0; lSet < lSets; ++lSet)
				{
					std::vector<uint16_t> vJoints(lVtxNum * 4, 0);
					tFloatVec vWeights(lVtxNum * 4, 0.0f);
					for(size_t v = 0; v < lVtxNum; ++v)
						for(size_t c = 0; c < 4; ++c)
						{
							size_t lInf = lSet * 4 + c;
							if(lInf >= vInfluences[v].size()) break;
							vJoints[v * 4 + c] = (uint16_t)vInfluences[v][lInf].first;
							vWeights[v * 4 + c] = vInfluences[v][lInf].second;
						}
					tString sSet = cString::ToString((int)lSet);
					vAttributes.push_back(JsonMember(("JOINTS_" + sSet).c_str(), cString::ToString(buffer.AddAccessor(vJoints.data(), vJoints.size() * 2, lVtxNum, kGltfUShort, "VEC4", kGltfArrayBuffer))));
					vAttributes.push_back(JsonMember(("WEIGHTS_" + sSet).c_str(), cString::ToString(buffer.AddFloats(vWeights, 4, "VEC4", kGltfArrayBuffer, false))));
				}
			}

			int lIndices;
			if(lVtxNum > 65535)
			{
				std::vector<uint32_t> vIdx(apGeom->mvIndexVec.begin(), apGeom->mvIndexVec.end());
				lIndices = buffer.AddAccessor(vIdx.data(), vIdx.size() * 4, vIdx.size(), kGltfUInt, "SCALAR", kGltfElementBuffer);
			}
			else
			{
				std::vector<uint16_t> vIdx(apGeom->mvIndexVec.begin(), apGeom->mvIndexVec.end());
				lIndices = buffer.AddAccessor(vIdx.data(), vIdx.size() * 2, vIdx.size(), kGltfUShort, "SCALAR", kGltfElementBuffer);
			}

			tStringVec vPrim;
			vPrim.push_back(JsonMember("attributes", JsonObject(vAttributes)));
			vPrim.push_back(JsonMember("indices", cString::ToString(lIndices)));
			int lMaterial = GetMaterial(*apGeom);
			if(lMaterial >= 0) vPrim.push_back(JsonMember("material", cString::ToString(lMaterial)));

			int lIndex = (int)vMeshes.size();
			vMeshes.push_back(JsonObject({JsonMember("name", JsonString(apGeom->msName)),
				JsonMember("primitives", JsonArray({JsonObject(vPrim)}))}));
			mapMeshIndex[key] = lIndex;
			return lIndex;
		};

		// Skins
		tStringVec vSkins;
		std::map<const cColladaController *, int> mapSkinIndex;
		auto GetSkin = [&](cColladaController *apCtrl) -> int
		{
			std::map<const cColladaController *, int>::iterator it = mapSkinIndex.find(apCtrl);
			if(it != mapSkinIndex.end()) return it->second;

			std::vector<int> vJoints;
			tFloatVec vMatrices;
			for(size_t j = 0; j < apCtrl->mvJoints.size(); ++j)
			{
				int lNode = NodeById(apCtrl->mvJoints[j]);
				if(lNode < 0)
				{
					Warn("skin '" + apCtrl->msId + "' uses the missing joint '" + apCtrl->mvJoints[j] + "'");
					mapSkinIndex[apCtrl] = -1;
					return -1;
				}
				vJoints.push_back(lNode);
				float vMtx[16];
				MatrixToGltf(j < apCtrl->mvMatrices.size() ? apCtrl->mvMatrices[j] : cMatrixf::Identity, vMtx);
				vMatrices.insert(vMatrices.end(), vMtx, vMtx + 16);
			}
			int lIndex = (int)vSkins.size();
			vSkins.push_back(JsonObject({JsonMember("joints", JsonInts(vJoints)),
				JsonMember("inverseBindMatrices", cString::ToString(buffer.AddFloats(vMatrices, 16, "MAT4", 0, false)))}));
			mapSkinIndex[apCtrl] = lIndex;
			return lIndex;
		};

		// Lights
		tStringVec vLights;
		std::map<tString, int> mapLightIndex;
		auto GetLightIndex = [&](cColladaLight &aLight) -> int
		{
			std::map<tString, int>::iterator it = mapLightIndex.find(aLight.msId);
			if(it != mapLightIndex.end()) return it->second;

			float vColor[3] = {aLight.mDiffuseColor.r, aLight.mDiffuseColor.g, aLight.mDiffuseColor.b};
			float fIntensity = std::max(vColor[0], std::max(vColor[1], vColor[2]));
			if(fIntensity > 1)
			{
				for(float &c : vColor) c /= fIntensity;
			}
			else
				fIntensity = 1;

			tStringVec vLight, vExtras;
			vLight.push_back(JsonMember("name", JsonString(aLight.msName)));
			vLight.push_back(JsonMember("color", JsonFloats(vColor, 3)));
			vLight.push_back(JsonMember("intensity", JsonFloat(fIntensity)));

			//The exact Collada values where the reader would round them
			float vReadColor[3] = {JsonRoundTrip(vColor[0]), JsonRoundTrip(vColor[1]), JsonRoundTrip(vColor[2])};
			cColor colRead = GltfLightColor(vReadColor, JsonRoundTrip(fIntensity));
			if(colRead.r != aLight.mDiffuseColor.r || colRead.g != aLight.mDiffuseColor.g || colRead.b != aLight.mDiffuseColor.b)
			{
				float vExact[3] = {aLight.mDiffuseColor.r, aLight.mDiffuseColor.g, aLight.mDiffuseColor.b};
				vExtras.push_back(JsonMember("hpl_color", JsonFloats(vExact, 3)));
			}

			if(aLight.msType == "spot")
			{
				vLight.push_back(JsonMember("type", JsonString("spot")));
				float fOuter = cMath::ToRad(aLight.mfAngle) * 0.5f;
				if(fOuter <= 0 || fOuter > kPi2f)
				{
					Warn("light '" + aLight.msName + "' has the cone angle " + cString::ToString(aLight.mfAngle));
					fOuter = cMath::Clamp(fOuter, 0.0001f, kPi2f);
				}
				vLight.push_back(JsonMember("spot", JsonObject({JsonMember("innerConeAngle", "0"), JsonMember("outerConeAngle", JsonFloat(fOuter))})));
				if(GltfSpotAngle(JsonRoundTrip(fOuter)) != aLight.mfAngle)
					vExtras.push_back(JsonMember("hpl_angle", JsonFloats(&aLight.mfAngle, 1)));
			}
			else
			{
				vLight.push_back(JsonMember("type", JsonString("point")));
				if(aLight.msType != "point") vExtras.push_back(JsonMember("hpl_type", JsonString(aLight.msType)));
			}
			if(vExtras.empty() == false) vLight.push_back(JsonMember("extras", JsonObject(vExtras)));
			int lIndex = (int)vLights.size();
			vLights.push_back(JsonObject(vLight));
			mapLightIndex[aLight.msId] = lIndex;
			return lIndex;
		};

		// Meshes, lights and skins in the order of the loader's data: the
		// reader makes them in the order of the file, and the loader makes sub
		// meshes, lights and bind poses in that order.
		for(cColladaGeometry &geom : aData.mvGeometries)
		{
			for(cColladaNode *pNode : vNodes)
			{
				if(pNode->mbSourceIsFile || pNode->msSource == "") continue;
				if(pNode->msSource == geom.msId) GetMesh(&geom, nullptr);
				for(cColladaController &ctrl : aData.mvControllers)
					if(ctrl.msId == pNode->msSource && ctrl.msTarget == geom.msId) GetMesh(&geom, &ctrl);
			}
		}
		for(cColladaController &ctrl : aData.mvControllers)
		{
			cColladaGeometry *pGeom = GetGeometry(ctrl.msTarget, aData.mvGeometries);
			std::map<std::pair<tString, const cColladaController *>, int>::iterator it =
				mapMeshIndex.find(std::pair<tString, const cColladaController *>(pGeom ? pGeom->msId : "", &ctrl));
			if(it != mapMeshIndex.end() && it->second >= 0) GetSkin(&ctrl);
		}
		for(cColladaLight &light : aData.mvLights)
		{
			for(cColladaNode *pNode : vNodes)
				if(pNode->mbSourceIsFile == false && pNode->msSource == light.msId)
				{
					GetLightIndex(light);
					break;
				}
		}

		// The nodes
		tStringVec vNodeJson;
		std::vector<int> vRoots;
		for(cColladaNode *pRoot : aData.mScene.mRoot.mlstChildren) vRoots.push_back(mapNodeIndex[pRoot]);

		for(cColladaNode *pNode : vNodes)
		{
			tStringVec vMembers, vExtras;
			vMembers.push_back(JsonMember("name", JsonString(pNode->msName)));
			if(pNode->msId != pNode->msName) vExtras.push_back(JsonMember("hpl_id", JsonString(pNode->msId)));
			if(pNode->msType == "JOINT" && setSkinJoints.count(pNode->msId) == 0)
				vExtras.push_back(JsonMember("hpl_joint", "true"));

			std::vector<int> vChildren;
			for(cColladaNode *pChild : pNode->mlstChildren) vChildren.push_back(mapNodeIndex[pChild]);
			if(vChildren.empty() == false) vMembers.push_back(JsonMember("children", JsonInts(vChildren)));

			//Source
			if(pNode->msSource != "")
			{
				cColladaGeometry *pGeom = GetGeometry(pNode->msSource, aData.mvGeometries);
				cColladaLight *pLight = GetLight(pNode->msSource, aData.mvLights);
				cColladaController *pCtrl = nullptr;
				for(cColladaController &ctrl : aData.mvControllers)
					if(ctrl.msId == pNode->msSource) pCtrl = &ctrl;

				if(pNode->mbSourceIsFile)
				{
					vExtras.push_back(JsonMember("hpl_source_file", JsonString(pNode->msSource)));
				}
				else if(pCtrl)
				{
					cColladaGeometry *pSkinGeom = GetGeometry(pCtrl->msTarget, aData.mvGeometries);
					int lMesh = pSkinGeom ? GetMesh(pSkinGeom, pCtrl) : -1;
					int lSkin = lMesh >= 0 ? GetSkin(pCtrl) : -1;
					if(lMesh >= 0) vMembers.push_back(JsonMember("mesh", cString::ToString(lMesh)));
					if(lSkin >= 0) vMembers.push_back(JsonMember("skin", cString::ToString(lSkin)));
					if(lMesh < 0) Warn("skin '" + pCtrl->msId + "' of node '" + pNode->msName + "' has no geometry");
				}
				else if(pGeom)
				{
					int lMesh = GetMesh(pGeom, nullptr);
					if(lMesh >= 0)
						vMembers.push_back(JsonMember("mesh", cString::ToString(lMesh)));
					else if(pGeom->msName != pNode->msName)
						vExtras.push_back(JsonMember("hpl_geometry", JsonString(pGeom->msName)));
				}
				else if(pLight)
				{
					vMembers.push_back(JsonMember("extensions", JsonObject({JsonMember("KHR_lights_punctual", JsonObject({JsonMember("light", cString::ToString(GetLightIndex(*pLight)))}))})));
				}
				else
					Warn("node '" + pNode->msName + "' uses the missing source '" + pNode->msSource + "'");
			}
			else if(pNode->msName.size() && pNode->msName[0] == '_')
			{
				//The reader makes empty geometries for special nodes without
				//meshes (Blender empties); this one had none.
				vExtras.push_back(JsonMember("hpl_geometry", "false"));
			}

			//Transform: the scale is mvScale. Lights, colliders and areas do not
			//have it in their matrix.
			const bool bSeparate = ScaleIsSeparate(pNode, &aData.mvLights);
			const cVector3f vScale = pNode->mvScale;
			cMatrixf mtxRot = pNode->m_mtxTransform.GetRotation();
			bool bTrs = true;
			if(bSeparate == false)
			{
				for(int c = 0; c < 3; ++c)
				{
					if(std::fabs(vScale.v[c]) < 1e-12f)
					{
						bTrs = false;
						break;
					}
					for(int r = 0; r < 3; ++r) mtxRot.m[r][c] /= vScale.v[c];
				}
			}
			if(bTrs && IsRotation(mtxRot, 1e-3f) == false) bTrs = false;

			if(bTrs)
			{
				cVector3f vTrans = pNode->m_mtxTransform.GetTranslation();
				cQuaternion qRot = QuaternionOf(mtxRot);
				float vT[3] = {vTrans.x, vTrans.y, vTrans.z};
				float vR[4] = {qRot.v.x, qRot.v.y, qRot.v.z, qRot.w};
				float vS[3] = {vScale.x, vScale.y, vScale.z};
				if(vTrans != cVector3f(0, 0, 0)) vMembers.push_back(JsonMember("translation", JsonFloats(vT, 3)));
				if(qRot.v != cVector3f(0, 0, 0)) vMembers.push_back(JsonMember("rotation", JsonFloats(vR, 4)));
				if(vScale != cVector3f(1, 1, 1)) vMembers.push_back(JsonMember("scale", JsonFloats(vS, 3)));

				//The exact Collada matrix where the reader would round it
				//(physics settles differently on the smallest difference)
				const float vIdentity[4] = {0, 0, 0, 1};
				float vReadT[3], vReadR[4], vReadS[3];
				for(int i = 0; i < 3; ++i)
				{
					vReadT[i] = JsonRoundTrip(vT[i]);
					vReadS[i] = JsonRoundTrip(vS[i]);
				}
				for(int i = 0; i < 4; ++i) vReadR[i] = JsonRoundTrip(vR[i]);
				cMatrixf mtxRead = GltfTransform(vReadT, qRot.v != cVector3f(0, 0, 0) ? vReadR : vIdentity, vReadS, bSeparate);
				if(SameBits(mtxRead.v, pNode->m_mtxTransform.v, 16) == false)
					vExtras.push_back(JsonMember("hpl_matrix", JsonFloats(pNode->m_mtxTransform.v, 16)));
			}
			else
			{
				Warn("node '" + pNode->msName + "' has a transform that is not translation, rotation and scale");
				cMatrixf mtxFull = pNode->m_mtxTransform;
				if(bSeparate) mtxFull = cMath::MatrixMul(mtxFull, cMath::MatrixScale(vScale));
				float vM[16];
				MatrixToGltf(mtxFull, vM);
				vMembers.push_back(JsonMember("matrix", JsonFloats(vM, 16)));
				float vS[3] = {vScale.x, vScale.y, vScale.z};
				vExtras.push_back(JsonMember("hpl_scale", JsonFloats(vS, 3)));
			}

			if(vExtras.empty() == false) vMembers.push_back(JsonMember("extras", JsonObject(vExtras)));
			vNodeJson.push_back(JsonObject(vMembers));
		}

		// The animation, as the engine's keys
		tStringVec vAnimations;
		{
			cSkeleton *pSkeleton = nullptr;
			cAnimation *pAnimation = CreateAnimation(aData, false, &pSkeleton);
			if(pAnimation)
			{
				tStringVec vSamplers, vChannels;
				for(int t = 0; t < pAnimation->GetTrackNum(); ++t)
				{
					cAnimationTrack *pTrack = pAnimation->GetTrack(t);
					if(pTrack == nullptr || pTrack->GetKeyFrameNum() == 0) continue;
					int lNode = NodeById(pTrack->GetName());
					if(lNode < 0)
					{
						Warn(tString("animation of the missing node '") + pTrack->GetName() + "'");
						continue;
					}

					//Bone keys are relative to the bind transform
					cMatrixf mtxBind = cMatrixf::Identity;
					if(pSkeleton)
					{
						cBone *pBone = pSkeleton->GetBoneByName(pTrack->GetName());
						if(pBone) mtxBind = pBone->GetLocalTransform();
						if(IsRotation(mtxBind.GetRotation(), 1e-3f) == false)
							Warn(tString("bone '") + pTrack->GetName() + "' has a scaled bind transform");
					}

					tFloatVec vTimes, vTrans, vRot, vEngineTrans, vEngineRot;
					cQuaternion qPrev = cQuaternion::Identity;
					for(int k = 0; k < pTrack->GetKeyFrameNum(); ++k)
					{
						cKeyFrame *pKey = pTrack->GetKeyFrame(k);
						vTimes.push_back(pKey->time);
						vEngineTrans.insert(vEngineTrans.end(), {pKey->trans.x, pKey->trans.y, pKey->trans.z});
						vEngineRot.insert(vEngineRot.end(), {pKey->rotation.v.x, pKey->rotation.v.y, pKey->rotation.v.z, pKey->rotation.w});

						cVector3f vT = pKey->trans;
						cQuaternion qR = pKey->rotation;
						if(pSkeleton)
						{
							vT = mtxBind.GetTranslation() + pKey->trans;
							qR = NormalisedRotation(cMath::MatrixMul(mtxBind.GetRotation(), cMath::MatrixQuaternion(pKey->rotation)));
						}
						if(k > 0 && cMath::QuaternionDot(qPrev, qR) < 0) qR = qR * -1.0f;
						qPrev = qR;

						vTrans.push_back(vT.x);
						vTrans.push_back(vT.y);
						vTrans.push_back(vT.z);
						vRot.push_back(qR.v.x);
						vRot.push_back(qR.v.y);
						vRot.push_back(qR.v.z);
						vRot.push_back(qR.w);
					}

					int lTimes = buffer.AddFloats(vTimes, 1, "SCALAR", 0, true);
					int lTrans = buffer.AddFloats(vTrans, 3, "VEC3", 0, false);
					int lRot = buffer.AddFloats(vRot, 4, "VEC4", 0, false);

					//The engine's keys as they are: going through the local
					//transforms rounds them, and physics bodies moved by
					//animations settle differently on the smallest difference.
					//Used while the hash of the keys above still matches.
					tString sExact = JsonObject({JsonMember("hpl_engine_translation", cString::ToString(buffer.AddFloats(vEngineTrans, 3, "VEC3", 0, false))),
						JsonMember("hpl_engine_rotation", cString::ToString(buffer.AddFloats(vEngineRot, 4, "VEC4", 0, false))),
						JsonMember("hpl_hash", JsonString(GltfKeyHash(vTimes, vTrans, vRot)))});

					for(int lPath = 0; lPath < 2; ++lPath)
					{
						vChannels.push_back(JsonObject({JsonMember("sampler", cString::ToString((int)vSamplers.size())),
							JsonMember("target", JsonObject({JsonMember("node", cString::ToString(lNode)), JsonMember("path", JsonString(lPath == 0 ? "translation" : "rotation"))}))}));
						tStringVec vSampler;
						vSampler.push_back(JsonMember("input", cString::ToString(lTimes)));
						vSampler.push_back(JsonMember("output", cString::ToString(lPath == 0 ? lTrans : lRot)));
						vSampler.push_back(JsonMember("interpolation", JsonString("LINEAR")));
						if(lPath == 1) vSampler.push_back(JsonMember("extras", sExact));
						vSamplers.push_back(JsonObject(vSampler));
					}
				}
				if(vChannels.empty() == false)
				{
					vAnimations.push_back(JsonObject({JsonMember("name", JsonString("Default")),
						JsonMember("channels", JsonArray(vChannels)),
						JsonMember("samplers", JsonArray(vSamplers)),
						JsonMember("extras", JsonObject({JsonMember("hpl_length", JsonFloat(aData.mScene.mfDeltaTime))}))}));
				}
				hplDelete(pAnimation);
			}
			if(pSkeleton) hplDelete(pSkeleton);
		}

		// The document
		const tString sType = cString::ToLowerCase(cString::GetFileExt(asFile));
		const bool bBinary = sType != "gltf";
		while(buffer.mvData.size() % 4) buffer.mvData.push_back(0);

		tString sBinName = cString::SetFileExt(cString::GetFileName(asFile), "bin");
		tStringVec vBuffer;
		vBuffer.push_back(JsonMember("byteLength", cString::ToString((int)buffer.mvData.size())));
		if(bBinary == false) vBuffer.push_back(JsonMember("uri", JsonString(sBinName)));

		tStringVec vDoc;
		vDoc.push_back(JsonMember("asset", JsonObject({JsonMember("version", JsonString("2.0")), JsonMember("generator", JsonString("OpenHPL1 model tool")), JsonMember("extras", JsonObject({JsonMember("hpl_source", JsonString(asSourceName))}))})));
		if(vLights.empty() == false)
		{
			vDoc.push_back(JsonMember("extensionsUsed", JsonArray({JsonString("KHR_lights_punctual")})));
			vDoc.push_back(JsonMember("extensions", JsonObject({JsonMember("KHR_lights_punctual", JsonObject({JsonMember("lights", JsonArray(vLights))}))})));
		}
		vDoc.push_back(JsonMember("scene", "0"));
		vDoc.push_back(JsonMember("scenes", JsonArray({JsonObject({JsonMember("nodes", JsonInts(vRoots))})})));
		if(vNodeJson.empty() == false) vDoc.push_back(JsonMember("nodes", JsonArray(vNodeJson)));
		if(vMeshes.empty() == false) vDoc.push_back(JsonMember("meshes", JsonArray(vMeshes)));
		if(vMaterials.empty() == false) vDoc.push_back(JsonMember("materials", JsonArray(vMaterials)));
		if(vTextures.empty() == false) vDoc.push_back(JsonMember("textures", JsonArray(vTextures)));
		if(vImages.empty() == false) vDoc.push_back(JsonMember("images", JsonArray(vImages)));
		if(vSkins.empty() == false) vDoc.push_back(JsonMember("skins", JsonArray(vSkins)));
		if(vAnimations.empty() == false) vDoc.push_back(JsonMember("animations", JsonArray(vAnimations)));
		if(buffer.mvAccessors.empty() == false)
		{
			vDoc.push_back(JsonMember("accessors", JsonArray(buffer.mvAccessors)));
			vDoc.push_back(JsonMember("bufferViews", JsonArray(buffer.mvViews)));
			vDoc.push_back(JsonMember("buffers", JsonArray({JsonObject(vBuffer)})));
		}
		tString sJson = JsonObject(vDoc);

		FILE *pFile = fopen(asFile.c_str(), "wb");
		if(pFile == nullptr)
		{
			Error("Couldn't write '%s'\n", asFile.c_str());
			return false;
		}
		bool bOk = true;
		if(bBinary)
		{
			while(sJson.size() % 4) sJson += ' ';
			const bool bBin = buffer.mvData.empty() == false;
			uint32_t vHeader[3] = {0x46546C67, 2, (uint32_t)(12 + 8 + sJson.size() + (bBin ? 8 + buffer.mvData.size() : 0))};
			uint32_t vJsonChunk[2] = {(uint32_t)sJson.size(), 0x4E4F534A};
			bOk = fwrite(vHeader, 4, 3, pFile) == 3 && fwrite(vJsonChunk, 4, 2, pFile) == 2 &&
				fwrite(sJson.data(), 1, sJson.size(), pFile) == sJson.size();
			if(bBin)
			{
				uint32_t vBinChunk[2] = {(uint32_t)buffer.mvData.size(), 0x004E4942};
				bOk = bOk && fwrite(vBinChunk, 4, 2, pFile) == 2 &&
					fwrite(buffer.mvData.data(), 1, buffer.mvData.size(), pFile) == buffer.mvData.size();
			}
		}
		else
		{
			bOk = fwrite(sJson.data(), 1, sJson.size(), pFile) == sJson.size();
			if(buffer.mvData.empty() == false)
			{
				tString sBinPath = cString::SetFileExt(asFile, "bin");
				FILE *pBin = fopen(sBinPath.c_str(), "wb");
				bOk = bOk && pBin && fwrite(buffer.mvData.data(), 1, buffer.mvData.size(), pBin) == buffer.mvData.size();
				if(pBin) fclose(pBin);
			}
		}
		if(fclose(pFile) != 0) bOk = false;
		if(bOk == false) Error("Couldn't write '%s'\n", asFile.c_str());
		return bOk;
	}
} // namespace hpl
