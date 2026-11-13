// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later

#include "impl/LowLevelSoundOpenAL.h"

#include <cmath>

#include "system/String.h"
#include "impl/OpenALDevice.h"
#include "impl/OpenALSoundData.h"
#include "impl/OpenALSoundEnvironment.h"

#include "math/Math.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	cLowLevelSoundOpenAL::cLowLevelSoundOpenAL()
		: mpDevice(new cOpenALDevice)
	{
		mvFormats[0] = "WAV";
		mvFormats[1] = "OGG";
		mvFormats[2] = "";
		mbInitialized = false;
		mbEnvAudioEnabled = false;
	}

	cLowLevelSoundOpenAL::~cLowLevelSoundOpenAL()
	{
		mpDevice->Close();
	}

	iSoundData* cLowLevelSoundOpenAL::LoadSoundData(const tString& asName, const tString& asFilePath,
												const tString& asType, bool abStream,bool abLoopStream)
	{
		cOpenALSoundData* pSoundData = hplNew( cOpenALSoundData, (asName,abStream,mpDevice.get()) );

		pSoundData->SetLoopStream(abLoopStream);

		if(pSoundData->CreateFromFile(asFilePath)==false)
		{
			hplDelete(pSoundData);
			return nullptr;
		}

		return pSoundData;
	}

	void cLowLevelSoundOpenAL::GetSupportedFormats(tStringList &alstFormats)
	{
		for(int i = 0; mvFormats[i] != ""; ++i)
			alstFormats.push_back(mvFormats[i]);
	}

	void cLowLevelSoundOpenAL::UpdateSound(float afTimeStep)
	{
		mpDevice->Update();
	}

	void cLowLevelSoundOpenAL::SetListenerAttributes(const cVector3f &avPos,const cVector3f &avVel,
							const cVector3f &avForward,const cVector3f &avUp)
	{
		mvListenerPosition = avPos;
		mvListenerVelocity = avVel;
		mvListenerForward = avForward;
		mvListenerUp = avUp;

		mvListenerRight = cMath::Vector3Cross(mvListenerForward,mvListenerUp);

		m_mtxListener = cMatrixf::Identity;
		m_mtxListener.SetRight(mvListenerRight);
		m_mtxListener.SetUp(mvListenerUp);
		m_mtxListener.SetForward(mvListenerForward*-1);
		m_mtxListener = cMath::MatrixInverse(m_mtxListener);
		m_mtxListener.SetTranslation(mvListenerPosition);

		cVector3f vForward = avForward*(-1);
		mpDevice->SetListener(avPos.v, avVel.v, vForward.v, avUp.v);
	}

	void cLowLevelSoundOpenAL::SetListenerPosition(const cVector3f &avPos)
	{
		mvListenerPosition = avPos;

		cVector3f vForward = mvListenerForward*(-1);
		mpDevice->SetListener(avPos.v, mvListenerVelocity.v, vForward.v, mvListenerUp.v);
	}

	void cLowLevelSoundOpenAL::SetListenerAttenuation (bool abEnabled)
	{
		mbListenerAttenuation = abEnabled;
	}

	void cLowLevelSoundOpenAL::SetRolloffFactor(float afFactor)
	{
		// OpenAL distance attenuation is off; the sound handler computes gains.
	}

	void cLowLevelSoundOpenAL::SetVolume(float afVolume)
	{
		mfVolume = afVolume;

		mpDevice->SetListenerGain(afVolume);
	}

	size_t cLowLevelSoundOpenAL::StreamBlockBytes(int alConfigured)
	{
		size_t lBytes = alConfigured <= 0 ? 64 * 1024
						: (alConfigured < 4096 ? (size_t)alConfigured * 1024 : (size_t)alConfigured);
		const size_t lMin = 16 * 1024;
		const size_t lMax = 1024 * 1024;
		return lBytes < lMin ? lMin : (lBytes > lMax ? lMax : lBytes);
	}

	void cLowLevelSoundOpenAL::Init(bool abUseHardware, bool abForceGeneric, bool abUseEnvAudio, int alMaxChannels,
									int alStreamUpdateFreq, bool abUseThreading, bool abUseVoiceManagement,
									int alMaxMonoSourceHint, int alMaxStereoSourceHint,
									int alStreamingBufferSize, int alStreamingBufferCount, bool abEnableLowLevelLog, tString asDeviceName)
	{
		Log(" Initializing OpenAL.\n");

		cOpenALDeviceParams params;
		params.msDeviceName = asDeviceName;
		params.mlSources = alMaxChannels > 0 ? alMaxChannels : 32;
		params.mbUseEFX = abUseEnvAudio;
		params.mbUseThread = abUseThreading;
		params.mlStreamUpdateFreq = alStreamUpdateFreq;
		params.mlStreamBlockBytes = StreamBlockBytes(alStreamingBufferSize);
		params.mlStreamBlockCount = alStreamingBufferCount < 3 ? 3 : (alStreamingBufferCount > 16 ? 16 : alStreamingBufferCount);
		params.mHRTF = ToDeviceHRTF(mHRTF);

		bool bNamed = asDeviceName != "" && asDeviceName != "NULL";
		Log("  Trying to open audio device %s... ", bNamed ? asDeviceName.c_str() : "(default)");

		mbInitialized = mpDevice->Open(params);
		if (!mbInitialized && bNamed)
		{
			// Device names saved by older OpenAL builds (e.g. "PulseAudio
			// Software") no longer exist; fall back to the default device.
			Log("not found, trying default device... ");
			params.msDeviceName = "";
			mbInitialized = mpDevice->Open(params);
		}
		if (!mbInitialized)
		{
			FatalError("Failed! Check your OpenAL installation\n");
			return;
		}

		Log("Success!\n");

		Log("  Device name: %s\n", mpDevice->GetDeviceName().c_str());
		Log("  Number of sources: %d\n", mpDevice->GetNumSources());
		Log("  Vendor name: %s\n", mpDevice->GetString(AL_VENDOR).c_str());
		Log("  Renderer: %s\n", mpDevice->GetString(AL_RENDERER).c_str());
		Log("  OpenAL version: %s\n", mpDevice->GetString(AL_VERSION).c_str());
		Log("  Stream buffers: %d x %d bytes\n", params.mlStreamBlockCount, (int)params.mlStreamBlockBytes);
		Log("  EFX: %d\n", mpDevice->IsEFXActive() ? 1 : 0);
		Log("  HRTF: %s, %s\n", SoundHRTFToString(mHRTF), GetHRTFStatus().c_str());
		Log("  Device reopen: %d\n", mpDevice->CanReopen() ? 1 : 0);
		Log("  Output Devices: \n");
		for (const tString &sDevice : GetOutputDevices())
			Log("    %s\n", sDevice.c_str());

		if (abUseEnvAudio)
		{
			mbEnvAudioEnabled = mpDevice->IsEFXActive();
			if (mbEnvAudioEnabled)
			{
				SetEnvVolume(1.0f);
				Log("  Setting up Environmental Audio...Success.\n");
			}
			else
				Log("  Setting up Environmental Audio...Failed.\n");
		}

		//Default listener settings.
		mvListenerForward = cVector3f(0,0,1);
		mvListenerUp = cVector3f(0,1,0);
		const float vZero[3] = {0,0,0};
		mpDevice->SetListener(vZero, vZero, mvListenerForward.v, mvListenerUp.v);

		//Default volume:
		SetVolume(1.0f);
	}

	tStringVec cLowLevelSoundOpenAL::GetOutputDevices()
	{
		return cOpenALDevice::GetOutputDevices();
	}

	tString cLowLevelSoundOpenAL::GetDeviceName()
	{
		return mpDevice->GetDeviceName();
	}

	bool cLowLevelSoundOpenAL::SetOutputDevice(const tString& asDeviceName)
	{
		if(!mbInitialized) return false;
		if(!mpDevice->Reopen(asDeviceName))
		{
			Warning("Couldn't switch audio to '%s'; keeping '%s'.\n", asDeviceName.c_str(),
					mpDevice->GetDeviceName().c_str());
			return false;
		}
		Log("Audio output moved to: %s (HRTF %s)\n", mpDevice->GetDeviceName().c_str(), GetHRTFStatus().c_str());
		return true;
	}

	eOpenALHRTF cLowLevelSoundOpenAL::ToDeviceHRTF(eSoundHRTF aMode)
	{
		if(aMode == eSoundHRTF_On) return eOpenALHRTF::On;
		if(aMode == eSoundHRTF_Off) return eOpenALHRTF::Off;
		return eOpenALHRTF::Auto;
	}

	void cLowLevelSoundOpenAL::SetHRTF(eSoundHRTF aMode)
	{
		mHRTF = aMode;
		if(!mbInitialized) return;
		if(!mpDevice->SetHRTF(ToDeviceHRTF(aMode)))
			Warning("Couldn't apply HRTF setting '%s'.\n", SoundHRTFToString(aMode));
		Log("HRTF set to %s: %s\n", SoundHRTFToString(aMode), GetHRTFStatus().c_str());
	}

	tString cLowLevelSoundOpenAL::GetHRTFStatus()
	{
		switch(mpDevice->GetHRTFStatus())
		{
		case ALC_HRTF_ENABLED_SOFT:
		case ALC_HRTF_HEADPHONES_DETECTED_SOFT:
		case ALC_HRTF_REQUIRED_SOFT:
		{
			tString sName = mpDevice->GetHRTFName();
			return sName.empty() ? tString("on") : "on (" + sName + ")";
		}
		case ALC_HRTF_DISABLED_SOFT: return "off";
		case ALC_HRTF_DENIED_SOFT: return "off (denied by the OpenAL config)";
		case ALC_HRTF_UNSUPPORTED_FORMAT_SOFT: return "off (output isn't stereo)";
		default: return "unavailable";
		}
	}

	void cLowLevelSoundOpenAL::SetEnvVolume( float afEnvVolume )
	{
		if (!mbEnvAudioEnabled)
			return;

		if (afEnvVolume <0)
			afEnvVolume = 0;
		if (afEnvVolume >1)
			afEnvVolume = 1;
		mfEnvVolume = afEnvVolume;
		mpDevice->SetReverbGain(mfEnvVolume);
	}

	iSoundEnvironment* cLowLevelSoundOpenAL::LoadSoundEnvironment(const tString &asFilePath)
	{
		if (!mbEnvAudioEnabled)
			return nullptr;

		///Check if sound already exist
		iSoundEnvironment *pSE = GetSoundEnvironmentFromFileName(asFilePath);
		if(pSE) return pSE;

		///Create new and load from file
		cOpenALSoundEnvironment* pSoundEnv = hplNew(cOpenALSoundEnvironment,());

		if (pSoundEnv->CreateFromFile(asFilePath)==false)
		{
			hplDelete(pSoundEnv);
			return nullptr;
		}

		pSoundEnv->SetFileName(cString::ToLowerCase(asFilePath));

		mlstSoundEnv.push_back(pSoundEnv);

		return pSoundEnv;
	}

	void cLowLevelSoundOpenAL::ApplyEnvironment(int alSlot, const cOpenALSoundEnvironment &e)
	{
		cOpenALReverb r;
		r.mfDensity = e.mfDensity;
		r.mfDiffusion = e.mfDiffusion;
		r.mfGain = e.mfGain;
		r.mfGainHF = e.mfGainHF;
		r.mfGainLF = e.mfGainLF;
		r.mfDecayTime = e.mfDecayTime;
		r.mfDecayHFRatio = e.mfDecayHFRatio;
		r.mfDecayLFRatio = e.mfDecayLFRatio;
		r.mfReflectionsGain = e.mfReflectionsGain;
		r.mfReflectionsDelay = e.mfReflectionsDelay;
		r.mfLateReverbGain = e.mfLateReverbGain;
		r.mfLateReverbDelay = e.mfLateReverbDelay;
		r.mfEchoTime = e.mfEchoTime;
		r.mfEchoDepth = e.mfEchoDepth;
		r.mfModulationTime = e.mfModulationTime;
		r.mfModulationDepth = e.mfModulationDepth;
		r.mfAirAbsorptionGainHF = e.mfAirAbsorptionGainHF;
		r.mfHFReference = e.mfHFReference;
		r.mfLFReference = e.mfLFReference;
		r.mfRoomRolloffFactor = e.mfRoomRolloffFactor;
		r.mbDecayHFLimit = e.mbDecayHFLimit != 0;
		for(int i = 0; i < 3; ++i)
		{
			r.mvReflectionsPan[i] = e.mfReflectionsPan[i];
			r.mvLateReverbPan[i] = e.mfLateReverbPan[i];
		}
		mpDevice->SetReverb(alSlot, r);
	}

	iSoundEnvironment* cLowLevelSoundOpenAL::GetSlotEnvironment(int alSlot) const
	{
		static_assert(kReverbSlots == cOpenALDevice::kMaxReverbSlots, "one environment per reverb slot");
		if(alSlot < 0 || alSlot >= kReverbSlots) return nullptr;
		return mpSlotEnv[alSlot];
	}

	// The slot holding the environment, other than alAvoidSlot. If none
	// does, the environment is loaded into the first other slot; that slot
	// is silent whenever the game fades, since only the source and the
	// destination are ever audible.
	int cLowLevelSoundOpenAL::SlotFor(iSoundEnvironment *apEnv, int alAvoidSlot)
	{
		int lSlots = mpDevice->GetReverbSlotCount();
		for(int i = 0; i < lSlots; ++i)
			if(i != alAvoidSlot && mpSlotEnv[i] == apEnv) return i;
		for(int i = 0; i < lSlots; ++i)
		{
			if(i == alAvoidSlot) continue;
			mpSlotEnv[i] = apEnv;
			ApplyEnvironment(i, *static_cast<cOpenALSoundEnvironment*>(apEnv));
			return i;
		}
		return -1;
	}

	void cLowLevelSoundOpenAL::SetSoundEnvironment ( iSoundEnvironment* apSoundEnv )
	{
		if (!mbEnvAudioEnabled)
			return;

		// No environment: every slot silent. Loaded environments stay in
		// their slots for the next fade.
		int lSlot = apSoundEnv ? SlotFor(apSoundEnv, -1) : -1;
		for(int i = 0; i < mpDevice->GetReverbSlotCount(); ++i)
			mpDevice->SetReverbWeight(i, i == lSlot ? 1.0f : 0.0f);
	}

	void cLowLevelSoundOpenAL::FadeSoundEnvironment( iSoundEnvironment* apSourceSoundEnv, iSoundEnvironment* apDestSoundEnv, float afT )
	{
		if (!mbEnvAudioEnabled)
			return;

		if ((apSourceSoundEnv == nullptr) && (apDestSoundEnv==nullptr))
			return;

		if (afT<0)
			afT = 0;
		if (afT>1)
			afT = 1;

		if (apSourceSoundEnv == apDestSoundEnv)
		{
			SetSoundEnvironment(apDestSoundEnv);
			return;
		}

		// Each environment keeps its own parameters in its own slot; only the
		// slot gains move. The two reverbs are uncorrelated, so the weights
		// are equal-power: cos^2 + sin^2 = 1 through the fade. A missing
		// environment is silence.
		int lSlots = mpDevice->GetReverbSlotCount();
		float fSrc, fDst;
		if(lSlots >= 2)
		{
			fSrc = std::cos(afT * kPi2f);
			fDst = std::sin(afT * kPi2f);
		}
		else
		{
			// One send: fade the source out, then the destination in.
			fSrc = afT < 0.5f ? 1 - 2 * afT : 0;
			fDst = afT < 0.5f ? 0 : 2 * afT - 1;
		}

		int lDstHeld = -1;
		for(int i = 0; i < lSlots; ++i)
			if(apDestSoundEnv && mpSlotEnv[i] == apDestSoundEnv) lDstHeld = i;

		int lSrc = -1, lDst = -1;
		if(lSlots >= 2)
		{
			if(apSourceSoundEnv) lSrc = SlotFor(apSourceSoundEnv, lDstHeld);
			if(apDestSoundEnv) lDst = SlotFor(apDestSoundEnv, lSrc);
		}
		else if(lSlots == 1)
		{
			// The single slot switches environments at the silent midpoint.
			iSoundEnvironment *pEnv = afT < 0.5f ? apSourceSoundEnv : apDestSoundEnv;
			if(pEnv) (afT < 0.5f ? lSrc : lDst) = SlotFor(pEnv, -1);
		}

		for(int i = 0; i < lSlots; ++i)
		{
			float fWeight = (i == lSrc ? fSrc : 0.0f) + (i == lDst ? fDst : 0.0f);
			mpDevice->SetReverbWeight(i, fWeight);
		}
	}

}
