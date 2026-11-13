// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
//
// Addons and glTF models (docs/modding.md), without game data: the file
// searcher's priorities, addon folders and .zip files, mesh caches of addon
// files, and the glTF reader and writer against the Collada loader.
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// FillStructures with caches, as engine_regressions.cpp does
#define private public
#include "impl/MeshLoaderCollada.h"
#undef private
#include "hpl.h"
#include "impl/LowLevelResourcesSDL.h"
#include "model_compare.h"
#include "miniz.h"

using namespace hpl;
namespace fs = std::filesystem;

static void Check(bool abResult, const std::string &asMessage)
{
	if(!abResult) throw std::runtime_error(asMessage);
}

static bool Near(float a, float b)
{
	return std::fabs(a - b) < 0.0001f;
}

static void Write(const fs::path &aPath, const std::string &asText)
{
	fs::create_directories(aPath.parent_path());
	std::ofstream(aPath, std::ios::binary) << asText;
}

static bool SamePath(const tString &asA, const fs::path &aB)
{
	std::error_code err;
	return asA != "" && fs::equivalent(fs::u8path(asA), aB, err);
}

static void TestFileSearcher(const fs::path &aDir)
{
	cLowLevelResourcesSDL lowLevel(nullptr);
	hpl::cFileSearcher searcher(&lowLevel);
	const fs::path addon = aDir / "searcher" / "addon", game = aDir / "searcher" / "game";
	Write(addon / "Shared.txt", "addon");
	Write(addon / "Model.glb", "");
	Write(addon / "older.dae", "");
	Write(game / "shared.txt", "game");
	Write(game / "model.dae", "");
	Write(game / "both.dae", "");
	Write(game / "both.gltf", "");
	Write(game / "older.glb", "");
	searcher.AddDirectory(addon.string(), "*.*");
	searcher.AddDirectory(game.string(), "*.*");

	Check(SamePath(searcher.GetFilePath("SHARED.txt"), addon / "Shared.txt"), "earlier directory wins, case-insensitive");
	Check(SamePath(searcher.GetHiddenFilePath("shared.txt"), game / "shared.txt"), "hidden file is remembered");
	Check(SamePath(searcher.GetFilePath("model.dae"), game / "model.dae"), "no alternatives before they are set");

	searcher.SetAlternativeExtensions("dae", {"glb", "gltf"});
	Check(SamePath(searcher.GetFilePath("model.dae"), addon / "Model.glb"), "an addon's glb answers for the game's dae");
	Check(SamePath(searcher.GetExactFilePath("model.dae"), game / "model.dae"), "exact lookup ignores alternatives");
	Check(SamePath(searcher.GetFilePath("both.dae"), game / "both.gltf"), "in one directory the alternative wins");
	Check(SamePath(searcher.GetFilePath("older.dae"), addon / "older.dae"), "an earlier directory's dae beats a later glb");
	Check(searcher.GetFilePath("missing.dae") == "", "missing file");

	//A tree added before the game: files and folders made later are found
	//after a rescan, still before the game's
	const fs::path tree = aDir / "searcher" / "tree";
	Write(tree / "readme.txt", "");
	Write(tree / ".hidden" / "shared.txt", "");
	hpl::cFileSearcher treeSearcher(&lowLevel);
	treeSearcher.AddDirectoryTree(tree.string(), "*.*");
	treeSearcher.AddDirectory(game.string(), "*.*");
	treeSearcher.SetAlternativeExtensions("dae", {"glb", "gltf"});
	Check(SamePath(treeSearcher.GetFilePath("shared.txt"), game / "shared.txt"), "hidden folders are left out");
	Write(tree / "maps" / "model.glb", "");
	Check(SamePath(treeSearcher.GetFilePath("model.dae"), game / "model.dae"), "new file unknown before a rescan");
	treeSearcher.Rescan();
	Check(SamePath(treeSearcher.GetFilePath("model.dae"), tree / "maps" / "model.glb"), "rescan finds a new folder's file first");
	Check(SamePath(treeSearcher.GetFilePath("both.dae"), game / "both.gltf"), "rescan keeps the alternatives");

	Check(iResourceManager::LoadedFileName("crate.dae", "/addon/models/Crate.glb") == "Crate.dae",
		"a model used for another keeps the requested type in its name");
	Check(iResourceManager::LoadedFileName("crate.DAE", "models/Crate.dae") == "Crate.dae", "same type: the file's name");
}

static void TestAddons(const fs::path &aDir)
{
	const fs::path root = aDir / "addons", unpack = aDir / "unpacked";
	Write(root / "b_textures" / "addon.cfg", "<Addon Name=\"B textures\" />");
	Write(root / "b_textures" / "textures" / "wall.jpg", "b");
	Write(root / "a_models" / "models" / "deep" / "crate.glb", "a");
	Write(root / "a_models" / "textures" / "wall.jpg", "a");
	Write(root / "a_models" / ".git" / "hidden.glb", "");
	Write(root / "off" / "addon.cfg", "<Addon Enabled=\"false\" />");
	Write(root / "bp_only" / "addon.cfg", "<Addon Games=\"blackplague, requiem\" />");
	Write(root / "notes.txt", "not an addon");

	const fs::path zip = root / "zipped.zip";
	const char *pCfg = "<Addon Name=\"Zipped\" Priority=\"5\" />";
	Check(mz_zip_add_mem_to_archive_file_in_place(zip.string().c_str(), "zipped/addon.cfg", pCfg, strlen(pCfg), nullptr, 0, MZ_DEFAULT_COMPRESSION) &&
			mz_zip_add_mem_to_archive_file_in_place(zip.string().c_str(), "zipped/models/crate.glb", "zip", 3, nullptr, 0, MZ_DEFAULT_COMPRESSION),
		"write addon zip");
	//A name that would leave the unpack folder (if miniz lets it be written)
	mz_zip_add_mem_to_archive_file_in_place(zip.string().c_str(), "zipped/../../escaped.txt", "x", 1, nullptr, 0, MZ_DEFAULT_COMPRESSION);

	tAddonVec vAddons = cAddons::Find({root.string(), (aDir / "missing").string()}, "overture", unpack.string(), "");
	Check(vAddons.size() == 3, "addons found: folders and .zip, not disabled or other games' ones");
	Check(vAddons[0].msName == "zipped" && vAddons[0].mlPriority == 5 && vAddons[0].msTitle == "Zipped", "higher priority first");
	Check(vAddons[1].msName == "a_models" && vAddons[2].msName == "b_textures" && vAddons[2].msTitle == "B textures", "then by name");
	Check(fs::exists(fs::u8path(vAddons[0].msPath) / "models" / "crate.glb") &&
			fs::u8path(vAddons[0].msPath).filename() == "zipped",
		"zip unpacked, its single folder used");
	Check(!fs::exists(aDir / "escaped.txt") && !fs::exists(unpack / "escaped.txt"), "zip entries stay in the unpack folder");

	const tString sUnpacked = vAddons[0].msPath;
	Write(fs::u8path(sUnpacked) / "marker.txt", "kept");
	tAddonVec vAgain = cAddons::Find({root.string()}, "overture", unpack.string(), "");
	Check(vAgain[0].msPath == sUnpacked && fs::exists(fs::u8path(sUnpacked) / "marker.txt"), "unchanged zip is not unpacked again");

	Check(cAddons::Find({root.string()}, "blackplague", unpack.string(), "").size() == 4, "game filter");
	tAddonVec vOnly = cAddons::Find({root.string()}, "overture", unpack.string(), "B_Textures");
	Check(vOnly.size() == 1 && vOnly[0].msName == "b_textures", "only the listed addons");
	Check(cAddons::Find({(root / "b_textures").string()}, "overture", unpack.string(), "").size() == 1,
		"a folder with addon.cfg is an addon itself");

	//Into the resources, before the game's folders
	cLowLevelResourcesSDL lowLevel(nullptr);
	cResources resources(&lowLevel, nullptr);
	const fs::path game = aDir / "addon_game";
	Write(game / "crate.dae", "game");
	Write(game / "wall.jpg", "game");
	cAddons::AddToResources(&resources, vAddons);
	resources.AddResourceDir(game.string());
	resources.GetFileSearcher()->SetAlternativeExtensions("dae", {"glb", "gltf"});
	hpl::cFileSearcher *pSearcher = resources.GetFileSearcher();
	Check(SamePath(pSearcher->GetFilePath("crate.dae"), fs::u8path(sUnpacked) / "models" / "crate.glb"), "highest priority addon's model used");
	Check(SamePath(pSearcher->GetFilePath("wall.jpg"), root / "a_models" / "textures" / "wall.jpg"), "subfolders indexed, name order");
	Check(pSearcher->GetFilePath("hidden.glb") == "", "hidden folders left out");
	cAddons::LogReplacedFiles(&resources, vAddons);
}

static void TestAddonCaches(const fs::path &aDir)
{
	//A Collada file outside the game's folder: its cache goes to its own folder
	cMeshLoaderCollada loader(nullptr);
	const fs::path source = fs::absolute(aDir / "addon_cache" / "models" / "thing.dae");
	const fs::path writeDir = aDir / "addon_cache" / "write";
	Write(source, "<COLLADA><asset><up_axis>Y_UP</up_axis></asset></COLLADA>");
	fs::create_directories(writeDir);
	iMeshLoader::SetCacheWriteDir(writeDir.string() + "/");
	tColladaImageVec images;
	Check(loader.FillStructures(source.string(), &images, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true),
		"read addon Collada file");
	iMeshLoader::SetCacheWriteDir("");

	Check(!fs::exists(writeDir / "thing.collcach"), "addon cache not next to the game's caches");
	bool bFound = false;
	for(auto &entry : fs::recursive_directory_iterator(writeDir / "external"))
		if(entry.path().filename() == "thing.collcach") bFound = true;
	Check(bFound, "addon cache in its own folder");
}

// A Collada file with what the loader's naming conventions and data cover
static const char *kColladaFixture = R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.0">
<asset><up_axis>Y_UP</up_axis></asset>
<library_lights>
 <light id="lamp-lib" name="lamp"><technique_common><point><color>2 1 0.5</color></point></technique_common></light>
 <light id="spot-lib" name="spotlight"><technique_common><spot><color>0.2 0.4 0.6</color><falloff_angle>60</falloff_angle></spot></technique_common></light>
</library_lights>
<library_images><image id="crate-image" name="crate"><init_from>file://C|/work/textures/crate.jpg</init_from></image></library_images>
<library_materials><material id="crate-mat" name="crate"><instance_effect url="#crate-fx"/></material></library_materials>
<library_effects><effect id="crate-fx"><profile_COMMON>
 <newparam sid="crate-surface"><surface type="2D"><init_from>crate-image</init_from></surface></newparam>
 <newparam sid="crate-sampler"><sampler2D><source>crate-surface</source></sampler2D></newparam>
 <technique sid="common"><lambert><diffuse><texture texture="crate-sampler" texcoord="UVSET0"/></diffuse></lambert></technique>
</profile_COMMON></effect></library_effects>
<library_geometries>
 <geometry id="boxShape-lib" name="boxShape"><mesh>
  <source id="box-pos"><float_array count="12">0 0 0 1 0 0 1 1 0 0 1 0</float_array><technique_common><accessor stride="3" count="4"/></technique_common></source>
  <source id="box-norm"><float_array count="3">0 0 1</float_array><technique_common><accessor stride="3" count="1"/></technique_common></source>
  <source id="box-uv"><float_array count="8">0 0 1 0 1 1 0 1</float_array><technique_common><accessor stride="2" count="4"/></technique_common></source>
  <vertices id="box-vtx"><input semantic="POSITION" source="#box-pos"/></vertices>
  <triangles material="crate-mat" count="2"><input semantic="VERTEX" source="#box-vtx" offset="0"/><input semantic="NORMAL" source="#box-norm" offset="1"/><input semantic="TEXCOORD" source="#box-uv" offset="2"/>
   <p>0 0 0 1 0 1 2 0 2 0 0 0 2 0 2 3 0 3</p></triangles>
 </mesh></geometry>
 <geometry id="colShape-lib" name="_collider_box_1Shape"><mesh>
  <source id="col-pos"><float_array count="9">-1 -1 -1 1 -1 1 1 1 -1</float_array><technique_common><accessor stride="3" count="3"/></technique_common></source>
  <source id="col-norm"><float_array count="3">0 1 0</float_array><technique_common><accessor stride="3" count="1"/></technique_common></source>
  <vertices id="col-vtx"><input semantic="POSITION" source="#col-pos"/></vertices>
  <triangles count="1"><input semantic="VERTEX" source="#col-vtx" offset="0"/><input semantic="NORMAL" source="#col-norm" offset="1"/><p>0 0 1 0 2 0</p></triangles>
 </mesh></geometry>
 <geometry id="skinShape-lib" name="skinShape"><mesh>
  <source id="skin-pos"><float_array count="9">0 0 0 2 0 0 0 1 0.5</float_array><technique_common><accessor stride="3" count="3"/></technique_common></source>
  <source id="skin-norm"><float_array count="3">0 0 1</float_array><technique_common><accessor stride="3" count="1"/></technique_common></source>
  <source id="skin-uv"><float_array count="6">0 0 1 0 0 1</float_array><technique_common><accessor stride="2" count="3"/></technique_common></source>
  <vertices id="skin-vtx"><input semantic="POSITION" source="#skin-pos"/></vertices>
  <triangles material="crate-mat" count="1"><input semantic="VERTEX" source="#skin-vtx" offset="0"/><input semantic="NORMAL" source="#skin-norm" offset="1"/><input semantic="TEXCOORD" source="#skin-uv" offset="2"/>
   <p>0 0 0 1 0 1 2 0 2</p></triangles>
 </mesh></geometry>
</library_geometries>
<library_controllers><controller id="skin-ctrl"><skin source="#skinShape-lib">
 <bind_shape_matrix>0 -1 0 0 1 0 0 0.5 0 0 1 0 0 0 0 1</bind_shape_matrix>
 <source id="skin-joints"><Name_array count="2">joint1 joint2</Name_array></source>
 <source id="skin-matrices"><float_array count="32">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 1 0 0 -1 0 1 0 0 0 0 1 0 0 0 0 1</float_array></source>
 <source id="skin-weights"><float_array count="3">1 0.5 0.5</float_array></source>
 <joints><input semantic="JOINT" source="#skin-joints"/><input semantic="INV_BIND_MATRIX" source="#skin-matrices"/></joints>
 <vertex_weights count="3"><input semantic="JOINT" source="#skin-joints" offset="0"/><input semantic="WEIGHT" source="#skin-weights" offset="1"/>
  <vcount>1 2 1</vcount><v>0 0 0 1 1 2 1 0</v></vertex_weights>
</skin></controller></library_controllers>
<library_animations>
 <animation id="joint2-rotateZ"><source id="rz-t"><float_array count="3">0 0.5 1</float_array></source><source id="rz-v"><float_array count="3">0 45 90</float_array></source>
  <sampler id="rz-s"><input semantic="INPUT" source="#rz-t"/><input semantic="OUTPUT" source="#rz-v"/></sampler><channel source="#rz-s" target="joint2/rotateZ.ANGLE"/></animation>
 <animation id="joint2-translate"><source id="tr-t"><float_array count="2">0 1</float_array></source><source id="tr-v"><float_array count="6">1 0 0 1 0.5 0</float_array></source>
  <sampler id="tr-s"><input semantic="INPUT" source="#tr-t"/><input semantic="OUTPUT" source="#tr-v"/></sampler><channel source="#tr-s" target="joint2/translate"/></animation>
</library_animations>
<library_visual_scenes><visual_scene id="scene">
 <node id="box" name="box"><translate sid="translate">1 2 3</translate><rotate sid="rotateY">0 1 0 30</rotate><rotate sid="rotateX">1 0 0 0.05</rotate><scale sid="scale">2 2 2</scale><instance_geometry url="#boxShape-lib"/>
  <node id="_collider_box_1" name="_collider_box_1"><translate>0 0.5 0</translate><scale sid="scale">1 0.5 3</scale><instance_geometry url="#colShape-lib"/></node>
 </node>
 <node id="lamp1" name="lamp1"><translate>0 3 0</translate><scale sid="scale">4 0.5 1</scale><instance_light url="#lamp-lib"/></node>
 <node id="spot1_id" name="spot1"><rotate sid="rotateX">1 0 0 -90</rotate><instance_light url="#spot-lib"/></node>
 <node id="_ref_crate_crate1" name="_ref_crate_crate1"><translate>0 0 5</translate></node>
 <node id="joint1" name="joint1" type="JOINT"><translate sid="translate">0 0 0</translate>
  <node id="joint2" name="joint2" type="JOINT"><translate sid="translate">1 0 0</translate><rotate sid="rotateZ">0 0 1 0</rotate></node>
 </node>
 <node id="skin" name="skin"><instance_controller url="#skin-ctrl"/></node>
 <extra><technique profile="MAYA"><start_time>0</start_time><end_time>1.25</end_time></technique></extra>
</visual_scene></library_visual_scenes>
</COLLADA>
)";

static cColladaNode *NodeNamed(cColladaData &aData, const tString &asName)
{
	for(cColladaNode *pNode : ModelSceneNodes(aData.mScene))
		if(pNode->msName == asName) return pNode;
	return nullptr;
}

static void TestGltfRoundTrip(const fs::path &aDir)
{
	cMeshLoaderCollada loader(nullptr);
	const fs::path dae = aDir / "gltf" / "fixture.dae";
	Write(dae, kColladaFixture);

	cColladaData collada;
	Check(loader.LoadData(dae.string(), collada), "read Collada fixture");
	//The two channels of joint2 are one animation of that node
	Check(collada.mvGeometries.size() == 3 && collada.mvControllers.size() == 1 && collada.mvAnimations.size() == 1,
		"Collada fixture contents");

	for(const char *sExt : {"glb", "gltf"})
	{
		const fs::path out = aDir / "gltf" / (std::string("fixture.") + sExt);
		tStringVec vWarnings;
		Check(loader.SaveGltf(collada, out.string(), "fixture.dae", &vWarnings), std::string("write ") + sExt);
		Check(vWarnings.empty(), "no conversion warnings" + (vWarnings.empty() ? std::string() : ": " + vWarnings[0]));

		cColladaData gltf;
		Check(loader.LoadData(out.string(), gltf), std::string("read ") + sExt + " back");
		cModelDiff diff;
		CompareModelData(loader, collada, gltf, diff);
		Check(diff.mvErrors.empty(), std::string(sExt) + " reads as the Collada file" + (diff.mvErrors.empty() ? "" : ": " + diff.mvErrors[0]));

		cColladaNode *pCollider = NodeNamed(gltf, "_collider_box_1");
		Check(pCollider && pCollider->mvScale == cVector3f(1, 0.5f, 3) && Near(pCollider->m_mtxTransform.m[2][2], 1),
			"collider keeps its scale out of its transform");
		cColladaNode *pRef = NodeNamed(gltf, "_ref_crate_crate1");
		Check(pRef && pRef->msSource == "", "reference without geometry stays without one");
		cColladaNode *pBox = NodeNamed(gltf, "box");
		Check(pBox && loader.GetGeometryMaterialFile(gltf, *ModelFindGeometry(gltf, pBox->msSource)) == "crate.jpg",
			"material from the texture file name");
		Check(NodeNamed(gltf, "spot1") && NodeNamed(gltf, "spot1")->msId == "spot1_id", "Collada id kept");
		Check(Near(gltf.mScene.mfDeltaTime, 1.25f), "animation length kept");
	}

	//The skin's bind shape is applied to the glTF vertices
	cColladaData gltf;
	loader.LoadData((aDir / "gltf" / "fixture.glb").string(), gltf);
	cColladaNode *pSkin = NodeNamed(gltf, "skin");
	Check(pSkin && gltf.mvControllers.size() == 1 && gltf.mvControllers[0].m_mtxBindShapeMatrix == cMatrixf::Identity,
		"glTF skin has no bind shape matrix");
	const cColladaGeometry *pSkinGeom = ModelFindGeometry(gltf, gltf.mvControllers[0].msTarget);
	Check(pSkinGeom && pSkinGeom->mvVertexVec.size() == 3 && Near(pSkinGeom->mvVertexVec[1].pos.y, 2.5f),
		"bind shape applied to the vertices");
}

// A glTF file made like Blender's: an armature node, a mesh with two
// materials, an empty, a light in watts and a mesh without normals.
static void TestBlenderStyleGltf(const fs::path &aDir)
{
	std::vector<unsigned char> vBin;
	std::vector<std::string> vViews, vAccessors;
	auto Add = [&](const void *apData, size_t alBytes, size_t alCount, int alType, const char *asType, const std::string &asExtra)
	{
		while(vBin.size() % 4) vBin.push_back(0);
		size_t lOffset = vBin.size();
		vBin.insert(vBin.end(), (const unsigned char *)apData, (const unsigned char *)apData + alBytes);
		vViews.push_back("{\"buffer\":0,\"byteOffset\":" + std::to_string(lOffset) + ",\"byteLength\":" + std::to_string(alBytes) + "}");
		vAccessors.push_back("{\"bufferView\":" + std::to_string(vViews.size() - 1) + ",\"componentType\":" + std::to_string(alType) +
			",\"count\":" + std::to_string(alCount) + ",\"type\":\"" + asType + "\"" + asExtra + "}");
		return std::to_string(vAccessors.size() - 1);
	};
	const float vTriPos[] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
	const float vNorm[] = {0, 0, 1, 0, 0, 1, 0, 0, 1};
	const float vUv[] = {0, 0, 1, 0, 0, 1};
	const unsigned short vJoints[] = {0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0};
	const float vWeights[] = {1, 0, 0, 0, 0.5f, 0.5f, 0, 0, 1, 0, 0, 0};
	const float vIbm[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -2, 0, 1};
	const float vTimes[] = {0, 2};
	const float vRot[] = {0, 0, 0, 1, 0, 0, 0.70710677f, 0.70710677f};
	const std::string sMinMax = ",\"min\":[0,0,0],\"max\":[1,1,0]";
	std::string aPos = Add(vTriPos, sizeof(vTriPos), 3, 5126, "VEC3", sMinMax);
	std::string aNorm = Add(vNorm, sizeof(vNorm), 3, 5126, "VEC3", "");
	std::string aUv = Add(vUv, sizeof(vUv), 3, 5126, "VEC2", "");
	std::string aJoints = Add(vJoints, sizeof(vJoints), 3, 5123, "VEC4", "");
	std::string aWeights = Add(vWeights, sizeof(vWeights), 3, 5126, "VEC4", "");
	std::string aIbm = Add(vIbm, sizeof(vIbm), 2, 5126, "MAT4", "");
	std::string aTimes = Add(vTimes, sizeof(vTimes), 2, 5126, "SCALAR", ",\"min\":[0],\"max\":[2]");
	std::string aRot = Add(vRot, sizeof(vRot), 2, 5126, "VEC4", "");

	const fs::path bin = aDir / "blender" / "scene.bin";
	fs::create_directories(bin.parent_path());
	std::ofstream(bin, std::ios::binary).write((const char *)vBin.data(), (std::streamsize)vBin.size());

	auto Join = [](const std::vector<std::string> &avItems)
	{
		std::string sOut;
		for(size_t i = 0; i < avItems.size(); ++i) sOut += (i ? "," : "") + avItems[i];
		return sOut;
	};
	std::string sJson = std::string("{\"asset\":{\"version\":\"2.0\",\"generator\":\"Khronos glTF Blender I/O\"},") +
		"\"extensionsUsed\":[\"KHR_lights_punctual\"]," +
		"\"extensions\":{\"KHR_lights_punctual\":{\"lights\":[{\"name\":\"Lamp\",\"type\":\"point\",\"color\":[1,0.5,0.25],\"intensity\":54.35}," +
		"{\"name\":\"Watts\",\"type\":\"point\",\"color\":[1,1,1],\"intensity\":54351}]}}," +
		"\"scene\":0,\"scenes\":[{\"nodes\":[0,4,5,6,7]}]," +
		"\"nodes\":[" +
		"{\"name\":\"Armature\",\"translation\":[0,1,0],\"children\":[1,3]}," +
		"{\"name\":\"Bone\",\"children\":[2]}," +
		"{\"name\":\"Bone.001\",\"translation\":[0,1,0]}," +
		"{\"name\":\"Body\",\"mesh\":0,\"skin\":0}," +
		"{\"name\":\"Crate\",\"mesh\":1,\"translation\":[3,0,0]}," +
		"{\"name\":\"_ref_crate_crate2\",\"translation\":[0,0,4]}," +
		"{\"name\":\"lamp\",\"extensions\":{\"KHR_lights_punctual\":{\"light\":0}},\"scale\":[5,1,1]}," +
		"{\"name\":\"bright\",\"extensions\":{\"KHR_lights_punctual\":{\"light\":1}}}]," +
		"\"skins\":[{\"joints\":[1,2],\"inverseBindMatrices\":" + aIbm + "}]," +
		"\"meshes\":[" +
		"{\"name\":\"BodyMesh\",\"primitives\":[{\"attributes\":{\"POSITION\":" + aPos + ",\"TEXCOORD_0\":" + aUv +
		",\"JOINTS_0\":" + aJoints + ",\"WEIGHTS_0\":" + aWeights + "},\"material\":1}]}," +
		"{\"name\":\"CrateMesh\",\"primitives\":[" +
		"{\"attributes\":{\"POSITION\":" + aPos + ",\"NORMAL\":" + aNorm + ",\"TEXCOORD_0\":" + aUv + "},\"material\":0}," +
		"{\"attributes\":{\"POSITION\":" + aPos + ",\"NORMAL\":" + aNorm + ",\"TEXCOORD_0\":" + aUv + "},\"material\":1}]}]," +
		"\"materials\":[{\"name\":\"Wood\",\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0}}},{\"name\":\"metal_plate\"}]," +
		"\"textures\":[{\"source\":0}],\"images\":[{\"uri\":\"textures/wood%20planks.png\"}]," +
		"\"animations\":[{\"name\":\"Wave\",\"channels\":[{\"sampler\":0,\"target\":{\"node\":2,\"path\":\"rotation\"}}," +
		"{\"sampler\":0,\"target\":{\"node\":4,\"path\":\"rotation\"}}]," +
		"\"samplers\":[{\"input\":" + aTimes + ",\"output\":" + aRot + "}]}]," +
		"\"accessors\":[" + Join(vAccessors) + "],\"bufferViews\":[" + Join(vViews) + "]," +
		"\"buffers\":[{\"uri\":\"scene.bin\",\"byteLength\":" + std::to_string(vBin.size()) + "}]}";
	const fs::path gltfFile = aDir / "blender" / "scene.gltf";
	Write(gltfFile, sJson);

	cMeshLoaderCollada loader(nullptr);
	cColladaData data;
	Check(loader.LoadData(gltfFile.string(), data), "read Blender-style glTF");

	Check(NodeNamed(data, "Armature") == nullptr, "armature node left out");
	cColladaNode *pBone = NodeNamed(data, "Bone");
	Check(pBone && pBone->pParent == &data.mScene.mRoot && pBone->msType == "JOINT" &&
			Near(pBone->m_mtxTransform.GetTranslation().y, 1),
		"root bone is a root, with the armature's transform");
	cColladaNode *pBody = NodeNamed(data, "Body");
	Check(pBody && pBody->pParent == &data.mScene.mRoot && Near(pBody->m_mtxWorldTransform.GetTranslation().y, 1),
		"skinned mesh moved out of the armature");

	Check(NodeNamed(data, "Crate") && NodeNamed(data, "Crate_1") && NodeNamed(data, "Crate_1")->pParent == &data.mScene.mRoot &&
			Near(NodeNamed(data, "Crate_1")->m_mtxTransform.GetTranslation().x, 3),
		"second material is a node next to the first");
	cColladaGeometry *pCrate = ModelFindGeometry(data, NodeNamed(data, "Crate")->msSource);
	cColladaGeometry *pCrate1 = ModelFindGeometry(data, NodeNamed(data, "Crate_1")->msSource);
	Check(pCrate && pCrate1 && pCrate->msName == "CrateMesh" && pCrate1->msName == "CrateMesh_1", "geometry names from the mesh");
	Check(loader.GetGeometryMaterialFile(data, *pCrate) == "wood planks.png", "material from the texture file name");
	Check(loader.GetGeometryMaterialFile(data, *pCrate1) == "metal_plate", "material from the material name");
	Check(pCrate->mvTangents.size() == pCrate->mvVertexVec.size() * 4, "tangents made");

	cColladaNode *pRef = NodeNamed(data, "_ref_crate_crate2");
	Check(pRef && ModelSourceKind(data, pRef) == "empty geometry", "special empty node gets an empty geometry");

	Check(data.mvLights.size() == 2, "lights");
	cColladaNode *pLamp = NodeNamed(data, "lamp");
	Check(pLamp && pLamp->mvScale.x == 5 && pLamp->m_mtxTransform.m[0][0] == 1, "light keeps its scale (radius) out of its transform");
	Check(Near(data.mvLights[0].mDiffuseColor.r, 54.35f) && data.mvLights[0].msType == "point", "light colour times intensity");
	Check(Near(data.mvLights[1].mDiffuseColor.r, 1), "intensity in physical units left out");

	cColladaGeometry *pBodyGeom = ModelFindGeometry(data, data.mvControllers.empty() ? "" : data.mvControllers[0].msTarget);
	Check(pBodyGeom && Near(pBodyGeom->mvVertexVec[0].norm.z, 1), "missing normals made");
	Check(data.mvControllers[0].mvJoints == tStringVec({"Bone", "Bone.001"}) && data.mvControllers[0].mvPairs[1].size() == 2,
		"skin joints and weights");

	cSkeleton *pSkeleton = nullptr;
	cAnimation *pAnimation = loader.CreateAnimation(data, true, &pSkeleton);
	//With a skeleton only bones are animated (as with Collada): Bone.001 and
	//Bone's rest pose. The crate's keys are still read, for both its nodes.
	Check(pSkeleton && pSkeleton->GetBoneNum() == 2 && pAnimation && pAnimation->GetTrackNum() == 2 && Near(pAnimation->GetLength(), 2),
		"skeleton and animation");
	int lCrateAnims = 0;
	for(const cColladaAnimation &anim : data.mvAnimations)
		if((anim.msTargetNode == "Crate" || anim.msTargetNode == "Crate_1") && anim.mvBakedKeys.size() == 2) ++lCrateAnims;
	Check(lCrateAnims == 2, "a mesh's second material moves with it");
	cAnimationTrack *pTrack = pAnimation->GetTrackByName("Bone.001");
	Check(pTrack && pTrack->GetKeyFrameNum() == 2, "animated bone keys");
	cQuaternion qEnd = pTrack->GetKeyFrame(1)->rotation;
	Check(Near(std::fabs(qEnd.v.z), 0.70710677f) && Near(pTrack->GetKeyFrame(1)->trans.y, 0), "bone keys relative to the bind pose");
	hplDelete(pAnimation);
	hplDelete(pSkeleton);
}

void RunModelRegressions(const fs::path &aDir)
{
	TestFileSearcher(aDir);
	TestAddons(aDir);
	TestAddonCaches(aDir);
	TestGltfRoundTrip(aDir);
	TestBlenderStyleGltf(aDir);
}
