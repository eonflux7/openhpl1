/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "impl/OpenALSoundData.h"
#include "impl/OpenALDevice.h"
#include "impl/OpenALSoundChannel.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	cOpenALSoundData::cOpenALSoundData(tString asName, bool abStream, cOpenALDevice* apDevice)
		: iSoundData(asName,abStream), mpDevice(apDevice)
	{
	}

	cOpenALSoundData::~cOpenALSoundData()
	{
		// Stop any source still using the data before its buffers go away.
		if(mpSample) mpDevice->ReleaseSample(mpSample.get());
		if(mpStream) mpDevice->ReleaseStream(mpStream.get());
	}

	bool cOpenALSoundData::CreateFromFile(const tString &asFile)
	{
		if(mbStream)
		{
			mpStream = cOpenALStream::Open(asFile, mpDevice->GetStreamBlockBytes(),
										   mpDevice->GetStreamBlockCount());
			if(mpStream == nullptr)
			{
				Error("Couldn't load sound stream '%s'\n", asFile.c_str());
				return false;
			}
			mpStream->SetLoop(mbLoopStream);
		}
		else
		{
			mpSample = cOpenALSample::Load(asFile);
			if(mpSample == nullptr)
			{
				Error("Couldn't load sound data '%s'\n", asFile.c_str());
				return false;
			}
		}

		return true;
	}

	iSoundChannel* cOpenALSoundData::CreateChannel(int alPriority)
	{
		int lHandle;
		if(mpStream)
			lHandle = mpDevice->PlayStream(mpStream.get(), 1.0f, true);
		else if(mpSample)
			lHandle = mpDevice->PlaySample(mpSample.get(), alPriority, 1.0f, true);
		else
			return nullptr;

		if(lHandle == cOpenALDevice::kInvalidHandle) return nullptr;

		iSoundChannel *pSoundChannel = hplNew( cOpenALSoundChannel, (this, mpDevice, lHandle, mpSoundManger) );
		IncUserCount();
		return pSoundChannel;
	}

	bool cOpenALSoundData::IsStereo()
	{
		if(mpStream) return mpStream->GetChannels() == 2;
		if(mpSample) return mpSample->GetChannels() == 2;
		return false;
	}
}
