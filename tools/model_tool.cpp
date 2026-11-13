// Copyright (C) 2026 - OpenHPL1 contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// openhpl1-model: converts the games' Collada models and maps to glTF 2.0
// and checks that the engine reads the same from both. See docs/modding.md.
//
//   openhpl1-model convert <in.dae> <out.glb|out.gltf>
//   openhpl1-model verify <in.dae> [<out.glb>]
//   openhpl1-model compare <a> <b>
//   openhpl1-model convert-dir <game dir> <out dir> [--verify] [--textures]
//   openhpl1-model info <file>
// Options: --log <file> (engine log; default: none), --quiet

#include "hpl.h"
#include "impl/MeshLoaderCollada.h"
#include "model_compare.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <map>
#include <set>

using namespace hpl;
namespace fs = std::filesystem;

static bool gbQuiet = false;

static bool PrintDiff(const tString &asName, const cModelDiff &aDiff, size_t alMax)
{
	if(aDiff.mvErrors.empty())
	{
		if(gbQuiet == false) printf("OK      %s%s\n", asName.c_str(),
			aDiff.mvNotes.empty() ? "" : (" (" + cString::ToString((int)aDiff.mvNotes.size()) + " notes)").c_str());
		return true;
	}
	printf("DIFFER  %s (%d)\n", asName.c_str(), (int)aDiff.mvErrors.size());
	for(size_t i = 0; i < aDiff.mvErrors.size() && i < alMax; ++i) printf("        %s\n", aDiff.mvErrors[i].c_str());
	if(aDiff.mvErrors.size() > alMax) printf("        ...\n");
	return false;
}

static bool Convert(cMeshLoaderCollada &aLoader, const tString &asIn, const tString &asOut, tStringVec &avWarnings)
{
	cColladaData data;
	if(aLoader.LoadData(asIn, data) == false)
	{
		avWarnings.push_back("could not be read");
		return false;
	}
	return aLoader.SaveGltf(data, asOut, cString::GetFileName(asIn), &avWarnings);
}

static void PrintWarnings(const tString &asName, const tStringVec &avWarnings)
{
	for(const tString &sWarning : avWarnings) printf("WARNING %s: %s\n", asName.c_str(), sWarning.c_str());
}

static int Info(cMeshLoaderCollada &aLoader, const tString &asFile)
{
	cColladaData data;
	if(aLoader.LoadData(asFile, data) == false)
	{
		printf("Could not read '%s'\n", asFile.c_str());
		return 1;
	}
	std::vector<cColladaNode *> vNodes = ModelSceneNodes(data.mScene);
	printf("%s: %d nodes, %d geometries, %d skins, %d lights, %d animated nodes, length %g\n", asFile.c_str(),
		(int)vNodes.size(), (int)data.mvGeometries.size(), (int)data.mvControllers.size(), (int)data.mvLights.size(),
		(int)data.mvAnimations.size(), data.mScene.mfDeltaTime);
	for(cColladaNode *pNode : vNodes)
	{
		int lDepth = 0;
		for(cColladaNode *p = pNode->pParent; p && p != &data.mScene.mRoot; p = p->pParent) ++lDepth;
		tString sSource = ModelSourceKind(data, pNode);
		if(sSource == "geometry")
		{
			cColladaGeometry *pGeom = ModelFindGeometry(data, pNode->msSource);
			sSource += " '" + pGeom->msName + "' " + cString::ToString((int)pGeom->mvIndexVec.size() / 3) + " triangles, material '" +
				aLoader.GetGeometryMaterialFile(data, *pGeom) + "'";
		}
		printf("%*s%s%s [%s] scale %s\n", lDepth * 2, "", pNode->msName.c_str(), pNode->msType == "JOINT" ? " (joint)" : "",
			sSource.c_str(), pNode->mvScale.ToString().c_str());
	}
	return 0;
}

static bool IsImage(const fs::path &aPath)
{
	tString sExt = cString::ToLowerCase(aPath.extension().string());
	return sExt == ".jpg" || sExt == ".jpeg" || sExt == ".tga" || sExt == ".png" || sExt == ".bmp" || sExt == ".dds";
}

static int ConvertDir(cMeshLoaderCollada &aLoader, const fs::path &aIn, const fs::path &aOut, bool abVerify, bool abTextures)
{
	std::vector<fs::path> vFiles;
	std::map<tString, fs::path> mapImages;
	std::error_code err;
	for(fs::recursive_directory_iterator it(aIn, err), end; it != end; it.increment(err))
	{
		if(it->is_regular_file() == false) continue;
		if(cString::ToLowerCase(it->path().extension().string()) == ".dae")
			vFiles.push_back(it->path());
		else if(abTextures && IsImage(it->path()))
			mapImages.insert(std::pair<tString, fs::path>(cString::ToLowerCase(it->path().filename().string()), it->path()));
	}
	std::sort(vFiles.begin(), vFiles.end());

	int lFailed = 0, lDiffer = 0, lWarned = 0;
	for(const fs::path &file : vFiles)
	{
		fs::path rel = fs::relative(file, aIn);
		fs::path out = aOut / rel;
		out.replace_extension(".glb");
		fs::create_directories(out.parent_path(), err);

		tStringVec vWarnings;
		tString sName = rel.generic_string();
		if(Convert(aLoader, file.string(), out.string(), vWarnings) == false)
		{
			++lFailed;
			printf("FAILED  %s\n", sName.c_str());
			PrintWarnings(sName, vWarnings);
			continue;
		}
		if(vWarnings.empty() == false)
		{
			++lWarned;
			PrintWarnings(sName, vWarnings);
		}

		if(abTextures)
		{
			cColladaData data;
			aLoader.LoadData(file.string(), data);
			for(cColladaGeometry &geom : data.mvGeometries)
			{
				tString sTex = aLoader.GetGeometryMaterialFile(data, geom);
				if(sTex == "") continue;
				std::map<tString, fs::path>::iterator imgIt = mapImages.find(cString::ToLowerCase(sTex));
				if(imgIt == mapImages.end()) continue;
				fs::path dest = out.parent_path() / imgIt->second.filename();
				if(fs::exists(dest, err) == false) fs::copy_file(imgIt->second, dest, err);
			}
		}

		if(abVerify)
		{
			cModelDiff diff;
			CompareModelFiles(aLoader, file.string(), out.string(), diff);
			if(PrintDiff(sName, diff, 8) == false) ++lDiffer;
		}
		else if(gbQuiet == false)
			printf("WROTE   %s\n", sName.c_str());
	}

	printf("\n%d files, %d failed, %d with warnings%s\n", (int)vFiles.size(), lFailed, lWarned,
		abVerify ? (", " + cString::ToString(lDiffer) + " differ").c_str() : "");
	return lFailed || lDiffer ? 1 : 0;
}

static int Usage()
{
	printf("openhpl1-model: Collada to glTF conversion for OpenHPL1 (see docs/modding.md)\n\n"
		   "  openhpl1-model convert <in.dae> <out.glb|out.gltf>\n"
		   "  openhpl1-model verify <in.dae> [<out.glb>]   convert and compare what the engine reads\n"
		   "  openhpl1-model compare <a> <b>               compare any two models (e.g. after Blender)\n"
		   "  openhpl1-model convert-dir <game dir> <out dir> [--verify] [--textures]\n"
		   "  openhpl1-model info <file>\n\n"
		   "Options: --log <file> (engine log), --quiet (only print problems),\n"
		   "         --exact (compare bit for bit instead of within tolerances)\n");
	return 2;
}

// The engine's main() calls this
int hplMain(const tString &)
{
	const tStringVec &vAll = GetCommandLineArguments();
	tStringVec vArgs;
	tString sLog;
	bool bVerify = false, bTextures = false;
	for(size_t i = 0; i < vAll.size(); ++i)
	{
		const tString &sArg = vAll[i];
		if(sArg == "--log" && i + 1 < vAll.size())
			sLog = vAll[++i];
		else if(sArg == "--quiet")
			gbQuiet = true;
		else if(sArg == "--verify")
			bVerify = true;
		else if(sArg == "--textures")
			bTextures = true;
		else if(sArg == "--exact")
			SetModelCompareExact(true);
		else if(sArg.size() > 2 && sArg.substr(0, 2) == "--")
			return Usage();
		else
			vArgs.push_back(sArg);
	}
	if(vArgs.empty()) return Usage();

#ifdef WIN32
	SetLogFile(cString::To16Char(sLog != "" ? sLog : "NUL"));
#else
	SetLogFile(cString::To16Char(sLog != "" ? sLog : "/dev/null"));
#endif

	cMeshLoaderCollada loader(nullptr);
	const tString &sCommand = vArgs[0];

	if(sCommand == "convert" && vArgs.size() == 3)
	{
		tStringVec vWarnings;
		bool bOk = Convert(loader, vArgs[1], vArgs[2], vWarnings);
		PrintWarnings(vArgs[1], vWarnings);
		if(bOk == false) printf("Could not convert '%s'\n", vArgs[1].c_str());
		return bOk ? 0 : 1;
	}
	if(sCommand == "verify" && (vArgs.size() == 2 || vArgs.size() == 3))
	{
		tString sOut = vArgs.size() == 3 ? vArgs[2] : (fs::temp_directory_path() / (cString::SetFileExt(cString::GetFileName(vArgs[1]), "") + ".verify.glb")).string();
		tStringVec vWarnings;
		if(Convert(loader, vArgs[1], sOut, vWarnings) == false)
		{
			PrintWarnings(vArgs[1], vWarnings);
			return 1;
		}
		PrintWarnings(vArgs[1], vWarnings);
		cModelDiff diff;
		CompareModelFiles(loader, vArgs[1], sOut, diff);
		if(vArgs.size() == 2)
		{
			std::error_code err;
			fs::remove(sOut, err);
		}
		return PrintDiff(vArgs[1], diff, 50) ? 0 : 1;
	}
	if(sCommand == "compare" && vArgs.size() == 3)
	{
		cModelDiff diff;
		CompareModelFiles(loader, vArgs[1], vArgs[2], diff);
		for(const tString &sNote : diff.mvNotes) printf("NOTE    %s\n", sNote.c_str());
		return PrintDiff(vArgs[2], diff, 200) ? 0 : 1;
	}
	if(sCommand == "convert-dir" && vArgs.size() == 3)
		return ConvertDir(loader, fs::u8path(vArgs[1]), fs::u8path(vArgs[2]), bVerify, bTextures);
	if(sCommand == "info" && vArgs.size() == 2)
		return Info(loader, vArgs[1]);

	return Usage();
}
