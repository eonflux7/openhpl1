// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SCRIPT_FUNCS_H
#define HPL_SCRIPT_FUNCS_H

#include "physics/PhysicsJoint.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class cSystem;
	class cSound;
	class cScene;
	class cInput;
	class cGame;

	class cScriptJointCallback : public iPhysicsJointCallback
	{
	public:
		cScriptJointCallback(cScene *apScene);

		void OnMinLimit(iPhysicsJoint *apJoint) override;
		void OnMaxLimit(iPhysicsJoint *apJoint) override;

		bool IsScript() override{ return true;}

		tString msMaxFunc;
		tString msMinFunc;

		cScene *mpScene;
	};

	class cScriptFuncs
	{
	public:
		static void Init(	cGraphics* apGraphics,
					cResources *apResources,
					cSystem *apSystem,
					cInput *apInput,
					cScene *apScene,
					cSound *apSound,
					cGame *apGame
					);
	};

};
#endif // HPL_SCRIPT_FUNCS_H
