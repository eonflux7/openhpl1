// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SOUND_MANAGER_H
#define HPL_SOUND_MANAGER_H

#include "resources/ResourceManager.h"

namespace hpl {

	class cSound;
	class cResources;
	class iSoundData;

	class cSoundManager : public iResourceManager
	{
	public:
		cSoundManager(cSound* apSound,cResources *apResources);
		~cSoundManager() override;

		iResourceBase* Create(const tString& asName) override;
		iSoundData* CreateSoundData(const tString& asName, bool abStream, bool abLoopStream=false);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

		void DestroyAll() override;

	private:
		cSound* mpSound;
		cResources *mpResources;

		tStringList mlstFileFormats;

		iSoundData *FindData(const tString &asName, tString &asFilePath);
	};

};
#endif // HPL_SOUND_MANAGER_H
