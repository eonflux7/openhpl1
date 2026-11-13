// Copyright (C) 2026 - OpenHPL1 contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Reads glTF 2.0 files (.glb, .gltf) into the Collada loader's intermediate
// data. The mapping, and what the converter in tools/model_tool.cpp writes,
// is described in docs/modding.md:
// - glTF nodes are Collada nodes; their names carry the same meaning
//   (_collider_box, _ref_<entity>_<name>, _area_<type>_<name>, lights...).
//   A node's scale is its mvScale; lights, colliders and areas keep it out
//   of their transform like in Collada.
// - Each mesh primitive is a geometry. A mesh is named like a Collada
//   geometry (sub mesh names, physics SubName). The material file is the
//   base colour texture's file name, else the "hpl_material" extra, else
//   the material's name (the extension is replaced by .mat).
// - Tangents are the TANGENT attribute if there is one, else they are made
//   like for Collada.
// - Skins are controllers, with an identity bind shape matrix.
// - KHR_lights_punctual lights are lights.
// - The first animation (or the one called "Default") is the animation; its
//   keys are kept as they are (cColladaAnimation::mvBakedKeys).
// - A root node without a meaning of its own whose children are joints (a
//   Blender armature) is left out; its transform goes to its children.
// Extras: nodes "hpl_id" (Collada id, when not the name), "hpl_joint" (a
// joint no skin uses), "hpl_source_file" (an external file instance),
// "hpl_geometry" (the geometry name of a node without a mesh); lights
// "hpl_type"; materials "hpl_material"; animations "hpl_length" (seconds).

#include "impl/MeshLoaderCollada.h"

#include "system/LowLevelSystem.h"
#include "system/String.h"

#include "graphics/Animation.h"
#include "graphics/AnimationTrack.h"
#include "graphics/Bone.h"
#include "graphics/Skeleton.h"

#include "math/Math.h"

#include "cgltf.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>
#include <set>

namespace hpl
{
	// Reads a string, number, boolean or array (its contents) of a glTF
	// element's extras object.
	static bool GetGltfExtra(const cgltf_extras &aExtras, const char *asKey, tString &asValue)
	{
		if(aExtras.data == nullptr) return false;
		const tString sJson = aExtras.data;
		const tString sKey = tString("\"") + asKey + "\"";

		size_t lPos = 0;
		while((lPos = sJson.find(sKey, lPos)) != tString::npos)
		{
			size_t lValue = lPos + sKey.size();
			while(lValue < sJson.size() && isspace((unsigned char)sJson[lValue])) ++lValue;
			if(lValue >= sJson.size() || sJson[lValue] != ':')
			{
				lPos = lValue;
				continue;
			}
			++lValue;
			while(lValue < sJson.size() && isspace((unsigned char)sJson[lValue])) ++lValue;
			if(lValue >= sJson.size()) return false;

			asValue = "";
			if(sJson[lValue] == '"')
			{
				for(size_t i = lValue + 1; i < sJson.size(); ++i)
				{
					char c = sJson[i];
					if(c == '"') return true;
					if(c == '\\' && i + 1 < sJson.size())
					{
						char e = sJson[++i];
						if(e == 'n')
							c = '\n';
						else if(e == 't')
							c = '\t';
						else if(e == 'u' && i + 4 < sJson.size())
						{
							unsigned lCode = (unsigned)strtoul(sJson.substr(i + 1, 4).c_str(), nullptr, 16);
							i += 4;
							c = lCode < 0x80 ? (char)lCode : '?';
						}
						else
							c = e;
					}
					asValue += c;
				}
				return false;
			}

			//Numbers, booleans and arrays of numbers
			size_t lEnd = lValue;
			if(sJson[lValue] == '[')
			{
				lEnd = sJson.find(']', lValue);
				if(lEnd == tString::npos) return false;
				asValue = sJson.substr(lValue + 1, lEnd - lValue - 1);
				return true;
			}
			while(lEnd < sJson.size() && sJson[lEnd] != ',' && sJson[lEnd] != '}' &&
				!isspace((unsigned char)sJson[lEnd])) ++lEnd;
			asValue = sJson.substr(lValue, lEnd - lValue);
			return true;
		}
		return false;
	}

	static bool GetGltfFloats(const cgltf_extras &aExtras, const char *asKey, tFloatVec &avValues)
	{
		tString sValue;
		if(GetGltfExtra(aExtras, asKey, sValue) == false) return false;
		tString sSeparators = ", \t\r\n";
		avValues.clear();
		cString::GetFloatVec(sValue, avValues, &sSeparators);
		return true;
	}

	// Close enough that the glTF values have not been edited
	static bool MatricesAgree(const cMatrixf &a_mtxA, const cMatrixf &a_mtxB)
	{
		for(int i = 0; i < 16; ++i)
		{
			float fMax = std::max(1.0f, std::max(std::fabs(a_mtxA.v[i]), std::fabs(a_mtxB.v[i])));
			if(std::fabs(a_mtxA.v[i] - a_mtxB.v[i]) > 1e-4f * fMax) return false;
		}
		return true;
	}

	static bool ValuesAgree(float afA, float afB)
	{
		return std::fabs(afA - afB) <= 1e-4f * std::max(1.0f, std::max(std::fabs(afA), std::fabs(afB)));
	}

	static cQuaternion GltfQuaternion(const float *apXYZW)
	{
		cQuaternion q(apXYZW[3], apXYZW[0], apXYZW[1], apXYZW[2]);
		q.Normalise();
		return q;
	}

	// A rotation matrix (with possibly scaled axes) as a quaternion.
	static cQuaternion RotationOf(const cMatrixf &a_mtxA)
	{
		cMatrixf mtxRot = a_mtxA.GetRotation();
		for(int c = 0; c < 3; ++c)
		{
			float fLength = sqrtf(mtxRot.m[0][c] * mtxRot.m[0][c] + mtxRot.m[1][c] * mtxRot.m[1][c] +
				mtxRot.m[2][c] * mtxRot.m[2][c]);
			if(fLength > 0)
				for(int r = 0; r < 3; ++r) mtxRot.m[r][c] /= fLength;
		}
		cQuaternion q;
		q.FromRotationMatrix(mtxRot);
		q.Normalise();
		return q;
	}

	// The local transform of a glTF node as translation, rotation and scale
	class cGltfTrs
	{
	public:
		cVector3f mvTrans;
		cQuaternion mqRot;
		cVector3f mvScale;
	};

	static cGltfTrs GetGltfTrs(const cgltf_node *apNode)
	{
		cGltfTrs trs;
		trs.mvTrans = cVector3f(0, 0, 0);
		trs.mqRot = cQuaternion::Identity;
		trs.mvScale = cVector3f(1, 1, 1);

		if(apNode->has_matrix)
		{
			cMatrixf mtx;
			mtx.FromTranspose(apNode->matrix);
			trs.mvTrans = mtx.GetTranslation();
			for(int c = 0; c < 3; ++c)
			{
				trs.mvScale.v[c] = sqrtf(mtx.m[0][c] * mtx.m[0][c] + mtx.m[1][c] * mtx.m[1][c] +
					mtx.m[2][c] * mtx.m[2][c]);
			}
			cMatrixf mtxRot = mtx.GetRotation();
			if(cMath::Vector3Dot(cMath::Vector3Cross(cVector3f(mtxRot.m[0][0], mtxRot.m[1][0], mtxRot.m[2][0]),
									 cVector3f(mtxRot.m[0][1], mtxRot.m[1][1], mtxRot.m[2][1])),
				   cVector3f(mtxRot.m[0][2], mtxRot.m[1][2], mtxRot.m[2][2])) < 0)
			{
				trs.mvScale.x = -trs.mvScale.x;
				for(int r = 0; r < 3; ++r) mtx.m[r][0] = -mtx.m[r][0];
			}
			trs.mqRot = RotationOf(mtx);
			return trs;
		}

		if(apNode->has_translation)
			trs.mvTrans = cVector3f(apNode->translation[0], apNode->translation[1], apNode->translation[2]);
		if(apNode->has_rotation) trs.mqRot = GltfQuaternion(apNode->rotation);
		if(apNode->has_scale)
			trs.mvScale = cVector3f(apNode->scale[0], apNode->scale[1], apNode->scale[2]);
		return trs;
	}

	static cMatrixf TrMatrix(const cVector3f &avTrans, const cQuaternion &aqRot)
	{
		cMatrixf mtx = cMath::MatrixQuaternion(aqRot);
		mtx.SetTranslation(avTrans);
		return mtx;
	}

	static tString GltfName(const char *asName, const char *asDefault, size_t alIndex)
	{
		if(asName && asName[0]) return asName;
		return asDefault + cString::ToString((int)alIndex);
	}

	static const cgltf_accessor *GetAttribute(const cgltf_primitive &aPrim, cgltf_attribute_type aType, int alIndex)
	{
		for(cgltf_size i = 0; i < aPrim.attributes_count; ++i)
		{
			if(aPrim.attributes[i].type == aType && aPrim.attributes[i].index == alIndex)
				return aPrim.attributes[i].data;
		}
		return nullptr;
	}

	// The values of an animation sampler, read once.
	class cGltfSamplerData
	{
	public:
		tFloatVec mvTimes;
		tFloatVec mvValues;
		int mlComponents;
		cgltf_interpolation_type mInterpolation;

		// The value at a time, of mlComponents floats.
		void Sample(float afTime, float *apOut, bool abQuaternion) const
		{
			const size_t lCount = mvTimes.size();
			const bool bCubic = mInterpolation == cgltf_interpolation_type_cubic_spline;
			const size_t lStride = bCubic ? 3 : 1;
			auto Value = [&](size_t alKey)
			{ return &mvValues[(alKey * lStride + (bCubic ? 1 : 0)) * mlComponents]; };

			if(lCount == 0) return;
			if(afTime <= mvTimes[0] || lCount == 1)
			{
				for(int i = 0; i < mlComponents; ++i) apOut[i] = Value(0)[i];
				return;
			}
			if(afTime >= mvTimes[lCount - 1])
			{
				for(int i = 0; i < mlComponents; ++i) apOut[i] = Value(lCount - 1)[i];
				return;
			}

			size_t lKey = 0;
			while(lKey + 1 < lCount && mvTimes[lKey + 1] < afTime) ++lKey;
			if(mvTimes[lKey + 1] == afTime) ++lKey;

			const float fT0 = mvTimes[lKey];
			if(fT0 == afTime || mInterpolation == cgltf_interpolation_type_step)
			{
				for(int i = 0; i < mlComponents; ++i) apOut[i] = Value(lKey)[i];
				return;
			}

			const float fT1 = mvTimes[lKey + 1];
			const float fDelta = fT1 - fT0;
			const float fT = (afTime - fT0) / fDelta;
			const float *pA = Value(lKey);
			const float *pB = Value(lKey + 1);

			if(bCubic)
			{
				const float *pOutTangent = &mvValues[(lKey * 3 + 2) * mlComponents];
				const float *pInTangent = &mvValues[((lKey + 1) * 3) * mlComponents];
				const float fT2 = fT * fT, fT3 = fT2 * fT;
				for(int i = 0; i < mlComponents; ++i)
				{
					apOut[i] = (2 * fT3 - 3 * fT2 + 1) * pA[i] + (fT3 - 2 * fT2 + fT) * fDelta * pOutTangent[i] +
						(-2 * fT3 + 3 * fT2) * pB[i] + (fT3 - fT2) * fDelta * pInTangent[i];
				}
			}
			else if(abQuaternion)
			{
				cQuaternion qResult = cMath::QuaternionSlerp(fT, GltfQuaternion(pA), GltfQuaternion(pB), true);
				apOut[0] = qResult.v.x;
				apOut[1] = qResult.v.y;
				apOut[2] = qResult.v.z;
				apOut[3] = qResult.w;
				return;
			}
			else
			{
				for(int i = 0; i < mlComponents; ++i) apOut[i] = pA[i] * (1 - fT) + pB[i] * fT;
			}

			if(abQuaternion)
			{
				cQuaternion q = GltfQuaternion(apOut);
				apOut[0] = q.v.x;
				apOut[1] = q.v.y;
				apOut[2] = q.v.z;
				apOut[3] = q.w;
			}
		}
	};

	static cGltfSamplerData ReadSampler(const cgltf_animation_sampler *apSampler, int alComponents)
	{
		cGltfSamplerData data;
		data.mlComponents = alComponents;
		data.mInterpolation = apSampler->interpolation;

		data.mvTimes.resize(apSampler->input->count);
		for(cgltf_size i = 0; i < apSampler->input->count; ++i)
			cgltf_accessor_read_float(apSampler->input, i, &data.mvTimes[i], 1);

		data.mvValues.resize(apSampler->output->count * alComponents);
		for(cgltf_size i = 0; i < apSampler->output->count; ++i)
			cgltf_accessor_read_float(apSampler->output, i, &data.mvValues[i * alComponents], alComponents);

		const size_t lNeeded = data.mvTimes.size() * (data.mInterpolation == cgltf_interpolation_type_cubic_spline ? 3 : 1);
		if(apSampler->output->count < lNeeded) data.mvTimes.resize(apSampler->output->count /
			(data.mInterpolation == cgltf_interpolation_type_cubic_spline ? 3 : 1));
		return data;
	}

	// READER

	class cGltfSkinUse
	{
	public:
		tString msGeomId;
		tString msId;
		const cgltf_mesh *mpMesh;
		size_t mlPrim;
		const cgltf_skin *mpSkin;
	};

	class cGltfReader
	{
	public:
		cgltf_data *mpData;
		tString msFile;

		tColladaImageVec *mpImages;
		tColladaTextureVec *mpTextures;
		tColladaMaterialVec *mpMaterials;
		tColladaLightVec *mpLights;
		tColladaGeometryVec *mpGeometries;
		tColladaControllerVec *mpControllers;
		cColladaScene *mpScene;

		std::set<const cgltf_node *> m_setJoints;
		std::map<const cgltf_node *, cColladaNode *> m_mapNodes;
		// Transforms of the left out armature nodes, for their children
		std::map<const cgltf_node *, cMatrixf> m_mapFolded;
		std::map<std::pair<size_t, size_t>, tString> m_mapGeometries;
		std::map<std::pair<tString, size_t>, tString> m_mapControllers;
		std::map<size_t, tString> m_mapMaterials;
		std::map<size_t, tString> m_mapLights;
		std::vector<cGltfSkinUse> mvSkinUses;
		// The nodes made for a node's other mesh primitives, animated with it
		std::map<const cgltf_node *, std::vector<cColladaNode *>> m_mapSiblings;

		size_t Index(const cgltf_node *apNode) const { return (size_t)(apNode - mpData->nodes); }

		bool IsArmature(const cgltf_node *apNode) const
		{
			if(m_setJoints.count(apNode) || apNode->mesh || apNode->light) return false;
			if(apNode->name && apNode->name[0] == '_') return false;
			tString sValue;
			if(GetGltfExtra(apNode->extras, "hpl_source_file", sValue)) return false;
			for(cgltf_size i = 0; i < apNode->children_count; ++i)
				if(m_setJoints.count(apNode->children[i])) return true;
			return false;
		}

		tString GetMaterial(const cgltf_material *apMaterial)
		{
			if(apMaterial == nullptr || mpMaterials == nullptr) return "";
			size_t lIndex = (size_t)(apMaterial - mpData->materials);
			std::map<size_t, tString>::iterator it = m_mapMaterials.find(lIndex);
			if(it != m_mapMaterials.end()) return it->second;

			tString sFile;
			const cgltf_texture *pTexture = apMaterial->has_pbr_metallic_roughness ? apMaterial->pbr_metallic_roughness.base_color_texture.texture : nullptr;
			if(pTexture && pTexture->image)
			{
				const cgltf_image *pImage = pTexture->image;
				if(pImage->uri && strncmp(pImage->uri, "data:", 5) != 0)
				{
					tString sUri = pImage->uri;
					//Undo %20 and the like
					tString sDecoded;
					for(size_t i = 0; i < sUri.size(); ++i)
					{
						if(sUri[i] == '%' && i + 2 < sUri.size())
						{
							sDecoded += (char)strtol(sUri.substr(i + 1, 2).c_str(), nullptr, 16);
							i += 2;
						}
						else
							sDecoded += sUri[i];
					}
					sFile = cString::GetFileName(sDecoded);
				}
				else if(pImage->name && pImage->name[0])
					sFile = pImage->name;
			}
			if(sFile == "" && GetGltfExtra(apMaterial->extras, "hpl_material", sFile) == false)
			{
				if(apMaterial->name) sFile = apMaterial->name;
			}

			tString sId = "material" + cString::ToString((int)lIndex);
			cColladaImage image;
			image.msId = sId + "-image";
			image.msName = sId;
			image.msSource = sFile;
			if(mpImages) mpImages->push_back(image);

			cColladaTexture texture;
			texture.msId = sId + "-fx";
			texture.msImage = image.msId;
			if(mpTextures) mpTextures->push_back(texture);

			cColladaMaterial material;
			material.msId = sId;
			material.msName = apMaterial->name ? apMaterial->name : "";
			material.msTexture = "#" + texture.msId;
			material.mDiffuseColor = cColor(1, 1);
			mpMaterials->push_back(material);

			m_mapMaterials[lIndex] = sId;
			return sId;
		}

		// Creates the geometry of a mesh primitive. Returns its id, "" if it
		// cannot be used.
		tString GetGeometry(const cgltf_mesh *apMesh, size_t alPrim)
		{
			size_t lMesh = (size_t)(apMesh - mpData->meshes);
			std::pair<size_t, size_t> key(lMesh, alPrim);
			std::map<std::pair<size_t, size_t>, tString>::iterator it = m_mapGeometries.find(key);
			if(it != m_mapGeometries.end()) return it->second;
			m_mapGeometries[key] = "";

			const cgltf_primitive &prim = apMesh->primitives[alPrim];
			tString sMeshName = GltfName(apMesh->name, "mesh", lMesh);
			if(alPrim > 0) sMeshName += "_" + cString::ToString((int)alPrim);

			const cgltf_accessor *pPos = GetAttribute(prim, cgltf_attribute_type_position, 0);
			if(pPos == nullptr)
			{
				Warning("glTF mesh '%s' has no positions\n", sMeshName.c_str());
				return "";
			}
			if(prim.type != cgltf_primitive_type_triangles &&
				prim.type != cgltf_primitive_type_triangle_strip &&
				prim.type != cgltf_primitive_type_triangle_fan)
			{
				Warning("glTF mesh '%s' is not made of triangles\n", sMeshName.c_str());
				return "";
			}

			cColladaGeometry geom;
			geom.msId = "mesh" + cString::ToString((int)lMesh) + "-" + cString::ToString((int)alPrim);
			geom.msName = sMeshName;
			geom.msMaterial = GetMaterial(prim.material);

			const cgltf_accessor *pNorm = GetAttribute(prim, cgltf_attribute_type_normal, 0);
			const cgltf_accessor *pTex = GetAttribute(prim, cgltf_attribute_type_texcoord, 0);

			geom.mvVertexVec.resize(pPos->count);
			for(cgltf_size i = 0; i < pPos->count; ++i)
			{
				cVertex &vtx = geom.mvVertexVec[i];
				vtx.col = cColor(1, 1);
				float vVal[3] = {0, 0, 0};
				cgltf_accessor_read_float(pPos, i, vVal, 3);
				vtx.pos = cVector3f(vVal[0], vVal[1], vVal[2]);
				vtx.norm = cVector3f(0, 0, 0);
				if(pNorm && i < pNorm->count)
				{
					cgltf_accessor_read_float(pNorm, i, vVal, 3);
					vtx.norm = cVector3f(vVal[0], vVal[1], vVal[2]);
				}
				vtx.tex = cVector3f(0, 0, 0);
				if(pTex && i < pTex->count)
				{
					cgltf_accessor_read_float(pTex, i, vVal, 2);
					//glTF and the engine both have v=0 at the top of the image
					vtx.tex = cVector3f(vVal[0], vVal[1], 0);
				}
			}

			//Triangles
			tUIntVec vIndices;
			const size_t lCount = prim.indices ? prim.indices->count : pPos->count;
			auto Index = [&](size_t i)
			{ return prim.indices ? (unsigned)cgltf_accessor_read_index(prim.indices, i) : (unsigned)i; };
			if(prim.type == cgltf_primitive_type_triangles)
			{
				for(size_t i = 0; i + 2 < lCount; i += 3)
					for(int c = 0; c < 3; ++c) vIndices.push_back(Index(i + c));
			}
			else if(prim.type == cgltf_primitive_type_triangle_strip)
			{
				for(size_t i = 0; i + 2 < lCount; ++i)
				{
					if(i % 2 == 0)
					{
						vIndices.push_back(Index(i));
						vIndices.push_back(Index(i + 1));
					}
					else
					{
						vIndices.push_back(Index(i + 1));
						vIndices.push_back(Index(i));
					}
					vIndices.push_back(Index(i + 2));
				}
			}
			else
			{
				for(size_t i = 1; i + 1 < lCount; ++i)
				{
					vIndices.push_back(Index(0));
					vIndices.push_back(Index(i));
					vIndices.push_back(Index(i + 1));
				}
			}
			for(unsigned lIdx : vIndices)
			{
				if(lIdx >= geom.mvVertexVec.size())
				{
					Warning("glTF mesh '%s' has an index out of range\n", sMeshName.c_str());
					return "";
				}
			}
			geom.mvIndexVec = vIndices;

			if(pNorm == nullptr)
			{
				Warning("glTF mesh '%s' has no normals, making smooth ones\n", sMeshName.c_str());
				for(size_t i = 0; i + 2 < vIndices.size(); i += 3)
				{
					cVertex &a = geom.mvVertexVec[vIndices[i]];
					cVertex &b = geom.mvVertexVec[vIndices[i + 1]];
					cVertex &c = geom.mvVertexVec[vIndices[i + 2]];
					cVector3f vNormal = cMath::Vector3Cross(b.pos - a.pos, c.pos - a.pos);
					a.norm += vNormal;
					b.norm += vNormal;
					c.norm += vNormal;
				}
				for(cVertex &vtx : geom.mvVertexVec) vtx.norm.Normalise();
			}

			//Each vertex is its own Collada position (for the skin weights)
			geom.mvExtraVtxVec.resize(geom.mvVertexVec.size());
			for(size_t i = 0; i < geom.mvVertexVec.size(); ++i)
				geom.mvExtraVtxVec[i].push_back(cColladaExtraVtx((int)i, (int)i, (int)i, (int)i));

			//Tangents from the file (the engine's bitangent is
			//cross(normal, tangent)*w as in glTF), else made like Collada's.
			const cgltf_accessor *pTangent = GetAttribute(prim, cgltf_attribute_type_tangent, 0);
			if(pTangent && pTangent->count == geom.mvVertexVec.size())
			{
				geom.mvTangents.resize(pTangent->count * 4);
				for(cgltf_size i = 0; i < pTangent->count; ++i)
					cgltf_accessor_read_float(pTangent, i, &geom.mvTangents[i * 4], 4);
			}
			else
			{
				cMeshLoaderCollada::CreateTangents(geom);
			}

			mpGeometries->push_back(geom);
			m_mapGeometries[key] = geom.msId;
			return geom.msId;
		}

		// A geometry without data, so special nodes made as empties (Blender)
		// are found like Collada's _ref, _bb, _ps and _sound geometries.
		tString AddEmptyGeometry(const tString &asId, const tString &asName)
		{
			cColladaGeometry geom;
			geom.msId = asId;
			geom.msName = asName;
			mpGeometries->push_back(geom);
			return asId;
		}

		// The id of the controller of a skinned geometry. The controllers are
		// made once all nodes are there (CreateControllers).
		tString GetController(const tString &asGeomId, const cgltf_mesh *apMesh, size_t alPrim, const cgltf_skin *apSkin)
		{
			size_t lSkin = (size_t)(apSkin - mpData->skins);
			std::pair<tString, size_t> key(asGeomId, lSkin);
			std::map<std::pair<tString, size_t>, tString>::iterator it = m_mapControllers.find(key);
			if(it != m_mapControllers.end()) return it->second;

			tString sId = asGeomId + "-skin" + cString::ToString((int)lSkin);
			m_mapControllers[key] = sId;
			cGltfSkinUse use = {asGeomId, sId, apMesh, alPrim, apSkin};
			mvSkinUses.push_back(use);
			return sId;
		}

		void CreateControllers()
		{
			//In the order of the skins, as the converter writes them
			std::stable_sort(mvSkinUses.begin(), mvSkinUses.end(), [](const cGltfSkinUse &a, const cGltfSkinUse &b)
				{ return a.mpSkin < b.mpSkin; });
			for(const cGltfSkinUse &use : mvSkinUses) CreateController(use);
		}

		void CreateController(const cGltfSkinUse &aUse)
		{
			const cgltf_skin *apSkin = aUse.mpSkin;
			const cgltf_mesh *apMesh = aUse.mpMesh;
			const size_t alPrim = aUse.mlPrim;

			cColladaController ctrl;
			ctrl.msId = aUse.msId;
			ctrl.msTarget = aUse.msGeomId;
			ctrl.m_mtxBindShapeMatrix = cMatrixf::Identity;

			for(cgltf_size j = 0; j < apSkin->joints_count; ++j)
			{
				std::map<const cgltf_node *, cColladaNode *>::iterator nodeIt = m_mapNodes.find(apSkin->joints[j]);
				ctrl.mvJoints.push_back(nodeIt != m_mapNodes.end() ? nodeIt->second->msId : GltfName(apSkin->joints[j]->name, "node", Index(apSkin->joints[j])));

				cMatrixf mtxInvBind = cMatrixf::Identity;
				if(apSkin->inverse_bind_matrices && j < apSkin->inverse_bind_matrices->count)
				{
					float vMtx[16];
					cgltf_accessor_read_float(apSkin->inverse_bind_matrices, j, vMtx, 16);
					mtxInvBind.FromTranspose(vMtx);
				}
				ctrl.mvMatrices.push_back(mtxInvBind);
			}

			//Weights
			const cgltf_primitive &prim = apMesh->primitives[alPrim];
			const cgltf_accessor *pPos = GetAttribute(prim, cgltf_attribute_type_position, 0);
			ctrl.mvPairs.resize(pPos ? pPos->count : 0);
			for(int lSet = 0;; ++lSet)
			{
				const cgltf_accessor *pJoints = GetAttribute(prim, cgltf_attribute_type_joints, lSet);
				const cgltf_accessor *pWeights = GetAttribute(prim, cgltf_attribute_type_weights, lSet);
				if(pJoints == nullptr || pWeights == nullptr) break;

				for(size_t v = 0; v < ctrl.mvPairs.size() && v < pJoints->count && v < pWeights->count; ++v)
				{
					cgltf_uint vJoint[4] = {0, 0, 0, 0};
					float vWeight[4] = {0, 0, 0, 0};
					cgltf_accessor_read_uint(pJoints, v, vJoint, 4);
					cgltf_accessor_read_float(pWeights, v, vWeight, 4);
					for(int c = 0; c < 4; ++c)
					{
						if(vWeight[c] <= 0 || vJoint[c] >= ctrl.mvJoints.size()) continue;
						ctrl.mvPairs[v].push_back(cColladaJointPair((int)vJoint[c], (int)ctrl.mvWeights.size()));
						ctrl.mvWeights.push_back(vWeight[c]);
					}
				}
			}

			if(mpControllers) mpControllers->push_back(ctrl);
		}

		tString GetLight(const cgltf_light *apLight)
		{
			size_t lIndex = (size_t)(apLight - mpData->lights);
			std::map<size_t, tString>::iterator it = m_mapLights.find(lIndex);
			if(it != m_mapLights.end()) return it->second;

			cColladaLight light;
			light.msId = "light" + cString::ToString((int)lIndex);
			light.msName = GltfName(apLight->name, "light", lIndex);

			if(GetGltfExtra(apLight->extras, "hpl_type", light.msType) == false)
			{
				if(apLight->type == cgltf_light_type_spot)
					light.msType = "spot";
				else if(apLight->type == cgltf_light_type_directional)
					light.msType = "directional";
				else
					light.msType = "point";
			}

			if(apLight->intensity > 100)
			{
				Warning("glTF light '%s' has intensity %g, using its colour only (see docs/modding.md)\n",
					light.msName.c_str(), apLight->intensity);
			}
			light.mDiffuseColor = cMeshLoaderCollada::GltfLightColor(apLight->color, apLight->intensity);
			light.mfAngle = apLight->type == cgltf_light_type_spot ? cMeshLoaderCollada::GltfSpotAngle(apLight->spot_outer_cone_angle) : 0;

			//The Collada values of a converted file, while they agree
			tFloatVec vExact;
			if(GetGltfFloats(apLight->extras, "hpl_color", vExact) && vExact.size() == 3 &&
				ValuesAgree(vExact[0], light.mDiffuseColor.r) && ValuesAgree(vExact[1], light.mDiffuseColor.g) &&
				ValuesAgree(vExact[2], light.mDiffuseColor.b))
			{
				light.mDiffuseColor = cColor(vExact[0], vExact[1], vExact[2], 1);
			}
			if(GetGltfFloats(apLight->extras, "hpl_angle", vExact) && vExact.size() == 1 && ValuesAgree(vExact[0], light.mfAngle))
				light.mfAngle = vExact[0];

			if(mpLights) mpLights->push_back(light);
			m_mapLights[lIndex] = light.msId;
			return light.msId;
		}

		// Sets a node's transform from its glTF transform, with the
		// transform of a left out parent (armature) put in front.
		void SetTransform(cColladaNode *apNode, const cgltf_node *apGltfNode, const cMatrixf *apPre)
		{
			cGltfTrs trs = GetGltfTrs(apGltfNode);
			const bool bSeparate = cMeshLoaderCollada::ScaleIsSeparate(apNode, mpLights);
			apNode->mvScale = trs.mvScale;

			cMatrixf mtx;
			if(apGltfNode->has_matrix == false)
			{
				const float vIdentity[4] = {0, 0, 0, 1}, vOne[3] = {1, 1, 1}, vZero[3] = {0, 0, 0};
				mtx = cMeshLoaderCollada::GltfTransform(apGltfNode->has_translation ? apGltfNode->translation : vZero,
					apGltfNode->has_rotation ? apGltfNode->rotation : vIdentity,
					apGltfNode->has_scale ? apGltfNode->scale : vOne, bSeparate);

				//A converted file also has the Collada transform, which is used
				//as long as the glTF one agrees with it (it has not been edited)
				tFloatVec vExact;
				if(GetGltfFloats(apGltfNode->extras, "hpl_matrix", vExact) && vExact.size() == 16)
				{
					cMatrixf mtxExact(vExact.data());
					if(MatricesAgree(mtx, mtxExact)) mtx = mtxExact;
				}
			}
			else
			{
				//Kept as it is (it may be sheared), the Collada scale from the extras
				tString sScale;
				if(GetGltfExtra(apGltfNode->extras, "hpl_scale", sScale))
				{
					tFloatVec vScale;
					tString sSeparators = ", \t\n";
					cString::GetFloatVec(sScale, vScale, &sSeparators);
					if(vScale.size() == 3) apNode->mvScale = cVector3f(vScale[0], vScale[1], vScale[2]);
				}
				mtx.FromTranspose(apGltfNode->matrix);
				if(bSeparate) mtx = cMath::MatrixMul(mtx, cMath::MatrixInverse(cMath::MatrixScale(apNode->mvScale)));
			}
			if(apPre) mtx = cMath::MatrixMul(*apPre, mtx);

			apNode->m_mtxTransform = mtx;
			apNode->m_mtxWorldTransform = cMath::MatrixMul(apNode->pParent->m_mtxWorldTransform, mtx);
		}

		void AddNode(const cgltf_node *apGltfNode, cColladaNode *apParent, const cMatrixf *apPre)
		{
			if(IsArmature(apGltfNode) && apParent == &mpScene->mRoot && apPre == nullptr)
			{
				cGltfTrs trs = GetGltfTrs(apGltfNode);
				cMatrixf mtxArmature = cMath::MatrixMul(TrMatrix(trs.mvTrans, trs.mqRot), cMath::MatrixScale(trs.mvScale));
				for(cgltf_size i = 0; i < apGltfNode->children_count; ++i)
				{
					m_mapFolded[apGltfNode->children[i]] = mtxArmature;
				}
				for(cgltf_size i = 0; i < apGltfNode->children_count; ++i)
				{
					AddNode(apGltfNode->children[i], apParent, &m_mapFolded[apGltfNode->children[i]]);
				}
				return;
			}

			cColladaNode *pNode = apParent->CreateChild();
			mpScene->mlstNodes.push_back(pNode);
			m_mapNodes[apGltfNode] = pNode;

			pNode->msName = GltfName(apGltfNode->name, "node", Index(apGltfNode));
			if(GetGltfExtra(apGltfNode->extras, "hpl_id", pNode->msId) == false) pNode->msId = pNode->msName;

			tString sValue;
			bool bJoint = m_setJoints.count(apGltfNode) ||
				(GetGltfExtra(apGltfNode->extras, "hpl_joint", sValue) && sValue == "true");
			pNode->msType = bJoint ? "JOINT" : "NODE";

			//Source
			if(apGltfNode->mesh && mpGeometries)
			{
				//Skins are controllers, which need geometries (as in Collada)
				tString sGeom = GetGeometry(apGltfNode->mesh, 0);
				if(sGeom != "")
				{
					pNode->msSource = apGltfNode->skin && mpControllers ? GetController(sGeom, apGltfNode->mesh, 0, apGltfNode->skin) : sGeom;
				}
			}
			else if(apGltfNode->light)
			{
				pNode->msSource = GetLight(apGltfNode->light);
			}
			else if(GetGltfExtra(apGltfNode->extras, "hpl_source_file", sValue))
			{
				pNode->msSource = sValue;
				pNode->mbSourceIsFile = true;
			}
			else if(mpGeometries &&
				(GetGltfExtra(apGltfNode->extras, "hpl_geometry", sValue) == false || sValue != "false") &&
				(cString::Sub(pNode->msName, 0, 4) == "_ref" || cString::Sub(pNode->msName, 0, 3) == "_bb" ||
					cString::Sub(pNode->msName, 0, 4) == "_ps" || cString::Sub(pNode->msName, 0, 6) == "_sound" ||
					cString::Sub(pNode->msName, 0, 5) == "_beam"))
			{
				tString sGeomName;
				if(GetGltfExtra(apGltfNode->extras, "hpl_geometry", sGeomName) == false) sGeomName = pNode->msName;
				pNode->msSource = AddEmptyGeometry("node" + cString::ToString((int)Index(apGltfNode)) + "-empty", sGeomName);
			}

			SetTransform(pNode, apGltfNode, apPre);

			//More materials than one: the other primitives become nodes next to this
			if(apGltfNode->mesh && mpGeometries && apGltfNode->mesh->primitives_count > 1)
			{
				if(pNode->msName[0] == '_')
				{
					Warning("glTF node '%s' has more than one material, only the first is used\n", pNode->msName.c_str());
				}
				else
				{
					for(cgltf_size p = 1; p < apGltfNode->mesh->primitives_count; ++p)
					{
						tString sGeom = GetGeometry(apGltfNode->mesh, p);
						if(sGeom == "") continue;

						cColladaNode *pSibling = apParent->CreateChild();
						mpScene->mlstNodes.push_back(pSibling);
						pSibling->msName = pNode->msName + "_" + cString::ToString((int)p);
						pSibling->msId = pSibling->msName;
						pSibling->msType = "NODE";
						pSibling->msSource = apGltfNode->skin && mpControllers ? GetController(sGeom, apGltfNode->mesh, p, apGltfNode->skin) : sGeom;
						SetTransform(pSibling, apGltfNode, apPre);
						m_mapSiblings[apGltfNode].push_back(pSibling);
					}
				}
			}

			for(cgltf_size i = 0; i < apGltfNode->children_count; ++i)
			{
				AddNode(apGltfNode->children[i], pNode, nullptr);
			}
		}

		void AddAnimation(tColladaAnimationVec &avAnimations)
		{
			if(mpData->animations_count == 0) return;

			size_t lAnim = 0;
			for(cgltf_size i = 0; i < mpData->animations_count; ++i)
			{
				if(mpData->animations[i].name && tString(mpData->animations[i].name) == "Default") lAnim = i;
			}
			if(mpData->animations_count > 1)
			{
				Warning("'%s' has %d animations, only '%s' is used\n", msFile.c_str(), (int)mpData->animations_count,
					GltfName(mpData->animations[lAnim].name, "animation", lAnim).c_str());
			}
			const cgltf_animation &anim = mpData->animations[lAnim];

			float fLength = 0;
			std::map<const cgltf_node *, std::pair<const cgltf_animation_sampler *, const cgltf_animation_sampler *>> mapTargets;
			for(cgltf_size i = 0; i < anim.channels_count; ++i)
			{
				const cgltf_animation_channel &channel = anim.channels[i];
				if(channel.target_node == nullptr || channel.sampler == nullptr) continue;
				if(channel.sampler->input->count > 0)
				{
					float fLast = 0;
					cgltf_accessor_read_float(channel.sampler->input, channel.sampler->input->count - 1, &fLast, 1);
					if(fLast > fLength) fLength = fLast;
				}
				if(channel.target_path == cgltf_animation_path_type_translation)
					mapTargets[channel.target_node].first = channel.sampler;
				else if(channel.target_path == cgltf_animation_path_type_rotation)
					mapTargets[channel.target_node].second = channel.sampler;
			}

			tString sLength;
			if(GetGltfExtra(anim.extras, "hpl_length", sLength)) fLength = cString::ToFloat(sLength.c_str(), fLength);

			mpScene->mfStartTime = 0;
			mpScene->mfEndTime = fLength;
			mpScene->mfDeltaTime = fLength;

			for(cgltf_size n = 0; n < mpData->nodes_count; ++n)
			{
				const cgltf_node *pGltfNode = &mpData->nodes[n];
				auto targetIt = mapTargets.find(pGltfNode);
				if(targetIt == mapTargets.end()) continue;

				std::map<const cgltf_node *, cColladaNode *>::iterator nodeIt = m_mapNodes.find(pGltfNode);
				if(nodeIt == m_mapNodes.end())
				{
					Warning("Animation of the left out glTF node '%s' is not used\n",
						GltfName(pGltfNode->name, "node", n).c_str());
					continue;
				}

				cGltfSamplerData transData, rotData;
				bool bTrans = targetIt->second.first != nullptr;
				bool bRot = targetIt->second.second != nullptr;
				std::set<float> setTimes;
				if(bTrans)
				{
					transData = ReadSampler(targetIt->second.first, 3);
					setTimes.insert(transData.mvTimes.begin(), transData.mvTimes.end());
				}
				if(bRot)
				{
					rotData = ReadSampler(targetIt->second.second, 4);
					setTimes.insert(rotData.mvTimes.begin(), rotData.mvTimes.end());
				}

				cGltfTrs rest = GetGltfTrs(pGltfNode);
				std::map<const cgltf_node *, cMatrixf>::iterator foldIt = m_mapFolded.find(pGltfNode);

				//The engine's keys of an unedited converted file
				tFloatVec vExactTrans, vExactRot;
				if(bTrans && bRot && transData.mvTimes == rotData.mvTimes && foldIt == m_mapFolded.end())
				{
					const cgltf_extras &extras = targetIt->second.second->extras;
					tString sHash, sTransIdx, sRotIdx;
					if(GetGltfExtra(extras, "hpl_hash", sHash) && GetGltfExtra(extras, "hpl_engine_translation", sTransIdx) &&
						GetGltfExtra(extras, "hpl_engine_rotation", sRotIdx) &&
						sHash == cMeshLoaderCollada::GltfKeyHash(rotData.mvTimes, transData.mvValues, rotData.mvValues))
					{
						size_t lTransIdx = (size_t)cString::ToInt(sTransIdx.c_str(), -1);
						size_t lRotIdx = (size_t)cString::ToInt(sRotIdx.c_str(), -1);
						const size_t lKeys = rotData.mvTimes.size();
						if(lTransIdx < mpData->accessors_count && lRotIdx < mpData->accessors_count &&
							mpData->accessors[lTransIdx].count == lKeys && mpData->accessors[lRotIdx].count == lKeys)
						{
							vExactTrans.resize(lKeys * 3);
							vExactRot.resize(lKeys * 4);
							for(size_t k = 0; k < lKeys; ++k)
							{
								cgltf_accessor_read_float(&mpData->accessors[lTransIdx], k, &vExactTrans[k * 3], 3);
								cgltf_accessor_read_float(&mpData->accessors[lRotIdx], k, &vExactRot[k * 4], 4);
							}
						}
					}
				}

				cColladaAnimation colladaAnim;
				colladaAnim.msId = nodeIt->second->msId + "-anim";
				colladaAnim.msTargetNode = nodeIt->second->msId;
				size_t lKey = 0;
				for(float fTime : setTimes)
				{
					cColladaBakedKey key;
					if(vExactRot.empty() == false)
					{
						key.mbExact = true;
						key.mvExactTrans = cVector3f(vExactTrans[lKey * 3], vExactTrans[lKey * 3 + 1], vExactTrans[lKey * 3 + 2]);
						key.mqExactRot = cQuaternion(vExactRot[lKey * 4 + 3], vExactRot[lKey * 4], vExactRot[lKey * 4 + 1], vExactRot[lKey * 4 + 2]);
					}
					++lKey;
					key.mfTime = fTime;
					key.mvTrans = rest.mvTrans;
					key.mqRot = rest.mqRot;
					if(bTrans)
					{
						float vVal[3];
						transData.Sample(fTime, vVal, false);
						key.mvTrans = cVector3f(vVal[0], vVal[1], vVal[2]);
					}
					if(bRot)
					{
						float vVal[4];
						rotData.Sample(fTime, vVal, true);
						key.mqRot = GltfQuaternion(vVal);
					}
					if(foldIt != m_mapFolded.end())
					{
						cMatrixf mtxKey = cMath::MatrixMul(foldIt->second, TrMatrix(key.mvTrans, key.mqRot));
						key.mvTrans = mtxKey.GetTranslation();
						key.mqRot = RotationOf(mtxKey);
					}
					colladaAnim.mvBakedKeys.push_back(key);
				}
				if(colladaAnim.mvBakedKeys.empty()) continue;
				avAnimations.push_back(colladaAnim);
				for(cColladaNode *pSibling : m_mapSiblings[pGltfNode])
				{
					colladaAnim.msId = pSibling->msId + "-anim";
					colladaAnim.msTargetNode = pSibling->msId;
					avAnimations.push_back(colladaAnim);
				}
			}
		}
	};

	// LOADER

	bool cMeshLoaderCollada::FillStructuresFromGltf(const tString &asFile,
		tColladaImageVec *apColladaImageVec,
		tColladaTextureVec *apColladaTextureVec,
		tColladaMaterialVec *apColladaMaterialVec,
		tColladaLightVec *apColladaLightVec,
		tColladaGeometryVec *apColladaGeometryVec,
		tColladaControllerVec *apColladaControllerVec,
		tColladaAnimationVec *apColladaAnimVec,
		cColladaScene *apColladaScene)
	{
		Log("Loading glTF '%s'\n", asFile.c_str());

		cgltf_options options = {};
		cgltf_data *pData = nullptr;
		cgltf_result result = cgltf_parse_file(&options, asFile.c_str(), &pData);
		if(result == cgltf_result_success) result = cgltf_load_buffers(&options, pData, asFile.c_str());
		if(result == cgltf_result_success) result = cgltf_validate(pData);
		if(result != cgltf_result_success)
		{
			Error("Couldn't load glTF file '%s' (error %d)!\n", asFile.c_str(), (int)result);
			if(pData) cgltf_free(pData);
			return false;
		}

		//The structures are used by the loader's code even if not wanted
		tColladaMaterialVec vMaterials;
		tColladaLightVec vLights;

		cGltfReader reader;
		reader.mpData = pData;
		reader.msFile = asFile;
		reader.mpImages = apColladaImageVec;
		reader.mpTextures = apColladaTextureVec;
		reader.mpMaterials = apColladaMaterialVec ? apColladaMaterialVec : &vMaterials;
		reader.mpLights = apColladaLightVec ? apColladaLightVec : &vLights;
		reader.mpGeometries = apColladaGeometryVec;
		reader.mpControllers = apColladaControllerVec;
		reader.mpScene = apColladaScene;

		cColladaScene tempScene;
		if(reader.mpScene == nullptr) reader.mpScene = &tempScene;
		reader.mpScene->mfStartTime = 0;
		reader.mpScene->mfEndTime = 0;
		reader.mpScene->mfDeltaTime = 0;

		for(cgltf_size s = 0; s < pData->skins_count; ++s)
			for(cgltf_size j = 0; j < pData->skins[s].joints_count; ++j)
				reader.m_setJoints.insert(pData->skins[s].joints[j]);

		//Geometries and lights in the order of the file (the loader makes
		//sub meshes and lights in this order). Lights before the nodes, so
		//that ScaleIsSeparate knows the light nodes.
		if(apColladaGeometryVec)
		{
			for(cgltf_size m = 0; m < pData->meshes_count; ++m)
				for(cgltf_size n = 0; n < pData->nodes_count; ++n)
					if(pData->nodes[n].mesh == &pData->meshes[m])
					{
						reader.GetGeometry(&pData->meshes[m], 0);
						break;
					}
		}
		for(cgltf_size l = 0; l < pData->lights_count; ++l)
			for(cgltf_size n = 0; n < pData->nodes_count; ++n)
				if(pData->nodes[n].light == &pData->lights[l])
				{
					reader.GetLight(&pData->lights[l]);
					break;
				}

		const cgltf_scene *pScene = pData->scene ? pData->scene : (pData->scenes_count ? &pData->scenes[0] : nullptr);
		if(pScene)
		{
			for(cgltf_size i = 0; i < pScene->nodes_count; ++i)
				reader.AddNode(pScene->nodes[i], &reader.mpScene->mRoot, nullptr);
		}
		else
		{
			for(cgltf_size n = 0; n < pData->nodes_count; ++n)
				if(pData->nodes[n].parent == nullptr) reader.AddNode(&pData->nodes[n], &reader.mpScene->mRoot, nullptr);
		}

		reader.CreateControllers();

		if(apColladaAnimVec) reader.AddAnimation(*apColladaAnimVec);

		cgltf_free(pData);
		return true;
	}

	tString cMeshLoaderCollada::GltfKeyHash(const tFloatVec &avTimes, const tFloatVec &avTrans, const tFloatVec &avRot)
	{
		uint64_t lHash = 14695981039346656037ull;
		for(const tFloatVec *pVec : {&avTimes, &avTrans, &avRot})
		{
			for(float fX : *pVec)
			{
				uint32_t lBits;
				memcpy(&lBits, &fX, 4);
				for(int i = 0; i < 4; ++i)
				{
					lHash ^= (lBits >> (i * 8)) & 0xff;
					lHash *= 1099511628211ull;
				}
			}
		}
		char sBuf[17];
		snprintf(sBuf, sizeof(sBuf), "%016llx", (unsigned long long)lHash);
		return sBuf;
	}

	cMatrixf cMeshLoaderCollada::GltfTransform(const float *apTrans, const float *apRotation, const float *apScale,
		bool abScaleIsSeparate)
	{
		cMatrixf mtx = TrMatrix(cVector3f(apTrans[0], apTrans[1], apTrans[2]), GltfQuaternion(apRotation));
		if(abScaleIsSeparate == false) mtx = cMath::MatrixMul(mtx, cMath::MatrixScale(cVector3f(apScale[0], apScale[1], apScale[2])));
		return mtx;
	}

	cColor cMeshLoaderCollada::GltfLightColor(const float *apColor, float afIntensity)
	{
		//The engine's colour is the brightness. An intensity in physical
		//units (Blender's watts converted to candela) is left out.
		if(afIntensity > 100) afIntensity = 1;
		return cColor(apColor[0] * afIntensity, apColor[1] * afIntensity, apColor[2] * afIntensity, 1);
	}

	float cMeshLoaderCollada::GltfSpotAngle(float afOuterConeAngle)
	{
		return cMath::ToDeg(afOuterConeAngle * 2);
	}

	cAnimationTrack *cMeshLoaderCollada::CreateBakedAnimTrack(cAnimation *apAnimation, cSkeleton *apSkeleton,
		cColladaAnimation &aAnim)
	{
		cBone *pBone = nullptr;
		if(apSkeleton)
		{
			pBone = apSkeleton->GetBoneByName(aAnim.msTargetNode);
			if(pBone == nullptr)
			{
				Error("Couldn't find bone '%s'\n", aAnim.msTargetNode.c_str());
				return nullptr;
			}
		}

		//The keys are local transforms. A bone's keys are relative to its bind
		//transform: the engine puts the key's rotation after the bind rotation
		//and adds the translations (see cNode3D::UpdateMatrix).
		cMatrixf mtxInvBindRot = cMatrixf::Identity;
		cVector3f vBindTrans(0, 0, 0);
		if(pBone)
		{
			mtxInvBindRot = cMath::MatrixInverse(pBone->GetLocalTransform().GetRotation());
			vBindTrans = pBone->GetLocalTransform().GetTranslation();
		}

		cAnimationTrack *pTrack = apAnimation->CreateTrack(aAnim.msTargetNode,
			eAnimTransformFlag_Rotate |
				eAnimTransformFlag_Translate |
				eAnimTransformFlag_Scale);
		for(const cColladaBakedKey &key : aAnim.mvBakedKeys)
		{
			cKeyFrame *pFrame = pTrack->CreateKeyFrame(key.mfTime);
			pFrame->scale = 1;
			if(key.mbExact)
			{
				pFrame->trans = key.mvExactTrans;
				pFrame->rotation = key.mqExactRot;
			}
			else if(pBone)
			{
				pFrame->trans = key.mvTrans - vBindTrans;
				pFrame->rotation.FromRotationMatrix(cMath::MatrixMul(mtxInvBindRot, cMath::MatrixQuaternion(key.mqRot)));
				pFrame->rotation.Normalise();
			}
			else
			{
				pFrame->trans = key.mvTrans;
				pFrame->rotation = key.mqRot;
			}
		}

		return pTrack;
	}
} // namespace hpl
