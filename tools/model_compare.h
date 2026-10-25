/*
 * Copyright (C) 2026 - OpenHPL1 contributors
 *
 * This file is part of OpenHPL1.
 *
 * OpenHPL1 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenHPL1 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenHPL1.  If not, see <http://www.gnu.org/licenses/>.
 */

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
