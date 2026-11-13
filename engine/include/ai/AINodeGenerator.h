// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_AI_NODE_GENERATOR_H
#define HPL_AI_NODE_GENERATOR_H

#include "system/SystemTypes.h"
#include "game/GameTypes.h"

#include "physics/PhysicsWorld.h"
#include "scene/World3D.h"

namespace hpl {

	class cWorld3D;

	class cAINodeGeneratorParams
	{
	public:
		cAINodeGeneratorParams();

		tString msNodeType;

		float mfHeightFromGround;
		float mfMinWallDist;

		cVector3f mvMinPos;
		cVector3f mvMaxPos;

		float mfGridSize;
	};

	class cAINodeGenerator : public iPhysicsRayCallback
	{
	public:
		cAINodeGenerator();
		~cAINodeGenerator() override;

		void Generate(cWorld3D* apWorld,cAINodeGeneratorParams *apParams);

	private:
		bool OnIntersect(iPhysicsBody *pBody,cPhysicsRayParams *apParams) override;

		void SaveToFile();
		void LoadFromFile();

		cAINodeGeneratorParams *mpParams;
		cWorld3D* mpWorld;
		tTempAiNodeList *mpNodeList;
	};

};
#endif // HPL_AI_NODE_GENERATOR_H
