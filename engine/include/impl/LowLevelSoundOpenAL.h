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
#ifndef HPL_LOWLEVELSOUND_OPENAL_H
#define HPL_LOWLEVELSOUND_OPENAL_H

#include "sound/LowLevelSound.h"

#include <memory>

namespace hpl
{
	class cOpenALDevice;
	enum class eOpenALHRTF;
	class cOpenALSoundEnvironment;

	class cLowLevelSoundOpenAL : public iLowLevelSound
	{
	public:
		cLowLevelSoundOpenAL();
		~cLowLevelSoundOpenAL() override;

		void GetSupportedFormats(tStringList &alstFormats) override;

		iSoundData* LoadSoundData(const tString& asName,const tString& asFilePath,
									const tString& asType, bool abStream,bool abLoopStream) override;

		void UpdateSound(float afTimeStep) override;

		void SetListenerAttributes (const cVector3f &avPos,const cVector3f &avVel,
								const cVector3f &avForward,const cVector3f &avUp) override;
		void SetListenerPosition(const cVector3f &avPos) override;

		void SetRolloffFactor(float afFactor) override;
		void SetListenerAttenuation (bool abEnabled) override;

		void Init ( bool abUseHardware, bool abForceGeneric, bool abUseEnvAudio, int alMaxChannels,
					int alStreamUpdateFreq, bool abUseThreading, bool abUseVoiceManagement,
					int alMaxMonoSourceHint, int alMaxStereoSourceHint,
					int alStreamingBufferSize, int alStreamingBufferCount, bool abEnableLowLevelLog,
					tString asDeviceName
					) override;

		void SetVolume(float afVolume) override;

		void SetEnvVolume( float afEnvVolume ) override;

		iSoundEnvironment* LoadSoundEnvironment (const tString& asFilePath) override;
		void SetSoundEnvironment ( iSoundEnvironment* apSoundEnv ) override;
		void FadeSoundEnvironment( iSoundEnvironment* apSourceSoundEnv, iSoundEnvironment* apDestSoundEnv, float afT ) override;

		tStringVec GetOutputDevices() override;
		tString GetDeviceName() override;
		bool SetOutputDevice(const tString& asDeviceName) override;
		void SetHRTF(eSoundHRTF aMode) override;
		tString GetHRTFStatus() override;

		cOpenALDevice* GetDevice() { return mpDevice.get(); }
		/** The environment loaded in a reverb slot (tests). */
		iSoundEnvironment* GetSlotEnvironment(int alSlot) const;

		/** Stream block size in bytes. Values below 4096 are KiB, as in the
		 * shipped settings (StreamBufferSize=64). */
		static size_t StreamBlockBytes(int alConfigured);

	private:
		void ApplyEnvironment(int alSlot, const cOpenALSoundEnvironment &aEnv);
		int SlotFor(iSoundEnvironment *apEnv, int alAvoidSlot);
		static eOpenALHRTF ToDeviceHRTF(eSoundHRTF aMode);

		std::unique_ptr<cOpenALDevice> mpDevice;
		tString mvFormats[3];
		bool	mbInitialized;
		static constexpr int kReverbSlots = 2;	///< cOpenALDevice::kMaxReverbSlots
		iSoundEnvironment *mpSlotEnv[kReverbSlots] = {nullptr, nullptr};
	};
};
#endif // HPL_LOWLEVELSOUND_OPENAL_H
