// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ANIMATION_MANAGER_H
#define HPL_ANIMATION_MANAGER_H

#include "resources/ResourceManager.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class cAnimation;

	class cAnimationManager : public iResourceManager
	{
	public:
		cAnimationManager(cGraphics* apGraphics,cResources *apResources);
		~cAnimationManager() override;

		iResourceBase* Create(const tString& asName) override;
		cAnimation* CreateAnimation(const tString& asName);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

	private:
		cGraphics* mpGraphics;
		cResources* mpResources;
	};

};
#endif // HPL_ANIMATION_MANAGER_H
