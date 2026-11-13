// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SOUND_ENTITY_MANAGER_H
#define HPL_SOUND_ENTITY_MANAGER_H

#include "resources/ResourceManager.h"

namespace hpl {

	class cSound;
	class cResources;
	class cSoundEntityData;

	class cSoundEntityManager : public iResourceManager
	{
	public:
		cSoundEntityManager(cSound* apSound,cResources *apResources);
		~cSoundEntityManager() override;

		void Preload(const tString& asFile);

		iResourceBase* Create(const tString& asName) override;
		cSoundEntityData* CreateSoundEntity(const tString& asName);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

	private:
		cSound* mpSound;
		cResources* mpResources;
	};

};
#endif // HPL_SOUND_ENTITY_MANAGER_H
