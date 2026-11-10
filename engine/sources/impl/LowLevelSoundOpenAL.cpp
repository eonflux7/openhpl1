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

#include "impl/LowLevelSoundOpenAL.h"
#include "system/String.h"
#include "impl/OpenALDevice.h"
#include "impl/OpenALSoundData.h"
#include "impl/OpenALSoundEnvironment.h"

#include "math/Math.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelSoundOpenAL::cLowLevelSoundOpenAL()
		: mpDevice(new cOpenALDevice)
	{
		mvFormats[0] = "WAV";
		mvFormats[1] = "OGG";
		mvFormats[2] = "";
		mbInitialized = false;
		mbEnvAudioEnabled = false;
	}

	//-----------------------------------------------------------------------

	cLowLevelSoundOpenAL::~cLowLevelSoundOpenAL()
	{
		mpDevice->Close();
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHOD
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::GetSupportedFormats(tStringList &alstFormats)
	{
		for(int i = 0; mvFormats[i] != ""; ++i)
			alstFormats.push_back(mvFormats[i]);
	}
	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::UpdateSound(float afTimeStep)
	{
		mpDevice->Update();
	}

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::SetListenerPosition(const cVector3f &avPos)
	{
		mvListenerPosition = avPos;

		cVector3f vForward = mvListenerForward*(-1);
		mpDevice->SetListener(avPos.v, mvListenerVelocity.v, vForward.v, mvListenerUp.v);
	}

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::SetListenerAttenuation (bool abEnabled)
	{
		mbListenerAttenuation = abEnabled;
	}

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::SetRolloffFactor(float afFactor)
	{
		// OpenAL distance attenuation is off; the sound handler computes gains.
	}

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::SetVolume(float afVolume)
	{
		mfVolume = afVolume;

		mpDevice->SetListenerGain(afVolume);
	}

	//-----------------------------------------------------------------------

	size_t cLowLevelSoundOpenAL::StreamBlockBytes(int alConfigured)
	{
		size_t lBytes = alConfigured <= 0 ? 64 * 1024
						: (alConfigured < 4096 ? (size_t)alConfigured * 1024 : (size_t)alConfigured);
		const size_t lMin = 16 * 1024;
		const size_t lMax = 1024 * 1024;
		return lBytes < lMin ? lMin : (lBytes > lMax ? lMax : lBytes);
	}

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	tStringVec cLowLevelSoundOpenAL::GetOutputDevices()
	{
		return cOpenALDevice::GetOutputDevices();
	}

	tString cLowLevelSoundOpenAL::GetDeviceName()
	{
		return mpDevice->GetDeviceName();
	}

	//-----------------------------------------------------------------------

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

	//-----------------------------------------------------------------------

	iSoundEnvironment* cLowLevelSoundOpenAL::LoadSoundEnvironment(const tString &asFilePath)
	{
		if (!mbEnvAudioEnabled)
			return nullptr;

		/////////////////////////////////////////////
		///Check if sound already exist
		iSoundEnvironment *pSE = GetSoundEnvironmentFromFileName(asFilePath);
		if(pSE) return pSE;

		/////////////////////////////////////////////
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

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::ApplyEnvironment(const cOpenALSoundEnvironment &e)
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
		mpDevice->SetReverb(r);
	}

	//-----------------------------------------------------------------------

	void cLowLevelSoundOpenAL::SetSoundEnvironment ( iSoundEnvironment* apSoundEnv )
	{
		if (!mbEnvAudioEnabled)
			return;

		// No environment: every parameter at zero (clamped to its minimum),
		// which silences the reverb.
		if (apSoundEnv == nullptr)
			ApplyEnvironment(cOpenALSoundEnvironment());
		else
			ApplyEnvironment(*static_cast<cOpenALSoundEnvironment*>(apSoundEnv));
	}

	//-----------------------------------------------------------------------

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

		// A missing environment fades from or to zero, as in the original.
		const cOpenALSoundEnvironment zero;
		const cOpenALSoundEnvironment &src = apSourceSoundEnv ? *static_cast<cOpenALSoundEnvironment*>(apSourceSoundEnv) : zero;
		const cOpenALSoundEnvironment &dst = apDestSoundEnv ? *static_cast<cOpenALSoundEnvironment*>(apDestSoundEnv) : zero;

		cOpenALSoundEnvironment env;
		env.LerpFrom(src, dst, afT);
		env.mbDecayHFLimit = apDestSoundEnv ? dst.mbDecayHFLimit : src.mbDecayHFLimit;

		ApplyEnvironment(env);
	}

}
