// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_VIDEO_MANAGER_H
#define HPL_VIDEO_MANAGER_H

#include "resources/ResourceManager.h"

namespace hpl {

	class cResources;
	class cGraphics;
	class iVideoStream;
	class iVideoStreamLoader;

	typedef std::list<iVideoStreamLoader*> tVideoStreamLoaderList;
	typedef tVideoStreamLoaderList::iterator tVideoStreamLoaderListIt;

	typedef std::list<iVideoStream*> tVideoStreamList;
	typedef tVideoStreamList::iterator tVideoStreamListIt;

	class cVideoManager : public iResourceManager
	{
	public:
		cVideoManager(cGraphics* apGraphics,cResources *apResources);
		~cVideoManager() override;

		iResourceBase* Create(const tString& asName) override;

		iVideoStream* CreateVideo(const tString& asName);

		void AddVideoLoader(iVideoStreamLoader *apLoader);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

		void Update(float afTimeStep) override;
	private:
		iVideoStreamLoader *GetLoader(const tString& asFileName);

		cGraphics* mpGraphics;
		cResources *mpResources;

		tVideoStreamLoaderList mlstVideoLoaders;
	};

};
#endif // HPL_VIDEO_MANAGER_H
