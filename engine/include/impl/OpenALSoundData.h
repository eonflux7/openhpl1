// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_OPENAL_SOUND_DATA_H
#define HPL_OPENAL_SOUND_DATA_H

#include "sound/SoundData.h"

#include <memory>

namespace hpl {

	class cOpenALDevice;
	class cOpenALSample;
	class cOpenALStream;

	class cOpenALSoundData : public iSoundData
	{
	public:
		cOpenALSoundData(tString asName, bool abStream, cOpenALDevice* apDevice);
		~cOpenALSoundData() override;

		bool CreateFromFile(const tString &asFile) override;

		iSoundChannel* CreateChannel(int alPriority) override;

		bool IsStereo() override;

	private:
		cOpenALDevice* mpDevice;
		std::unique_ptr<cOpenALSample> mpSample;
		std::unique_ptr<cOpenALStream> mpStream;
	};
};
#endif // HPL_OPENAL_SOUND_DATA_H
