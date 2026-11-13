// Copyright (C) 2026 - OpenHPL1 contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Compares what the engine's model loader reads from two files (e.g. a
// Collada file and the glTF file converted from it). Used by
// tools/model_tool.cpp and tests/engine_regressions.cpp.

#ifndef OPENHPL_MODEL_COMPARE_H
#define OPENHPL_MODEL_COMPARE_H

#include "impl/MeshLoaderCollada.h"

// Differences between what the engine reads from two files
class cModelDiff
{
public:
	hpl::tStringVec mvErrors;
	hpl::tStringVec mvNotes;

	void Error(const hpl::tString &asText) { mvErrors.push_back(asText); }
	void Note(const hpl::tString &asText) { mvNotes.push_back(asText); }
};

// Nodes are matched by id. Geometries are compared as the engine uses them
// (with skin bind shapes applied), animations as the engine's keys.
void CompareModelData(hpl::cMeshLoaderCollada &aLoader, hpl::cColladaData &aA, hpl::cColladaData &aB, cModelDiff &aDiff);
// Helpers: the scene's nodes in order, a geometry by id, and what a node's
// source is to the loader ("none", "geometry", "empty geometry", "skin",
// "light", "file", "missing").
std::vector<hpl::cColladaNode *> ModelSceneNodes(hpl::cColladaScene &aScene);
hpl::cColladaGeometry *ModelFindGeometry(hpl::cColladaData &aData, const hpl::tString &asId);
hpl::tString ModelSourceKind(hpl::cColladaData &aData, hpl::cColladaNode *apNode);

// Bit for bit instead of within tolerances
void SetModelCompareExact(bool abX);

bool CompareModelFiles(hpl::cMeshLoaderCollada &aLoader, const hpl::tString &asA, const hpl::tString &asB, cModelDiff &aDiff);

#endif
