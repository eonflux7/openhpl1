// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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
