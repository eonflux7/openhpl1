/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "impl/OpenALDevice.h"

#include "system/LowLevelSystem.h"

#include <AL/alext.h>
#include <AL/efx.h>

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_endian.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>

#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// HELPERS
	//////////////////////////////////////////////////////////////////////////

	namespace {

		const int kHandleIndexBits = 12;
		const int kHandleIndexMask = (1 << kHandleIndexBits) - 1;
		const uint32_t kGenerationMask = 0x7FFFF;
		const int kStreamPriority = 256;
		const int kMaxPriority = 255;

		float Clamp(float afX, float afMin, float afMax)
		{
			return afX < afMin ? afMin : (afX > afMax ? afMax : afX);
		}

		struct sPan { ALfloat v[3]; };

		/** EAX reverb pans are vectors no longer than 1; a non-finite value
		 * counts as no pan. */
		sPan ClampPan(const float *apPan)
		{
			sPan pan = {{apPan[0], apPan[1], apPan[2]}};
			float fLength = std::sqrt(pan.v[0]*pan.v[0] + pan.v[1]*pan.v[1] + pan.v[2]*pan.v[2]);
			if(!std::isfinite(fLength)) return sPan{{0, 0, 0}};
			if(fLength > 1)
				for(ALfloat &f : pan.v) f /= fLength;
			return pan;
		}

		std::string LowerExtension(const std::string &asFile)
		{
			size_t lDot = asFile.find_last_of('.');
			if(lDot == std::string::npos) return "";
			std::string sExt = asFile.substr(lDot + 1);
			for(char &c : sExt) c = (char)std::tolower((unsigned char)c);
			return sExt;
		}

		bool ReadFile(const std::string &asFile, std::vector<unsigned char> &avData)
		{
			FILE *pFile = fopen(asFile.c_str(), "rb");
			if(pFile == nullptr) return false;
			unsigned char vChunk[65536];
			size_t lRead;
			while((lRead = fread(vChunk, 1, sizeof(vChunk), pFile)) > 0)
				avData.insert(avData.end(), vChunk, vChunk + lRead);
			bool bOk = ferror(pFile) == 0;
			fclose(pFile);
			return bOk;
		}

		//////////////////////
		// Vorbis file callbacks: the decoder never uses another runtime's FILE*.

		size_t VorbisRead(void *apDest, size_t alSize, size_t alCount, void *apFile)
		{
			return fread(apDest, alSize, alCount, static_cast<FILE*>(apFile));
		}

		int VorbisSeek(void *apFile, ogg_int64_t alOffset, int alWhence)
		{
#ifdef _WIN32
			return _fseeki64(static_cast<FILE*>(apFile), alOffset, alWhence);
#else
			return fseeko(static_cast<FILE*>(apFile), (off_t)alOffset, alWhence);
#endif
		}

		int VorbisClose(void *apFile)
		{
			return fclose(static_cast<FILE*>(apFile));
		}

		long VorbisTell(void *apFile)
		{
			return ftell(static_cast<FILE*>(apFile));
		}

		const ov_callbacks kVorbisFileCallbacks = { VorbisRead, VorbisSeek, VorbisClose, VorbisTell };

		const int kVorbisBigEndian = SDL_BYTEORDER == SDL_BIG_ENDIAN ? 1 : 0;

		bool OpenVorbis(const std::string &asFile, OggVorbis_File *apVorbis)
		{
			FILE *pFile = fopen(asFile.c_str(), "rb");
			if(pFile == nullptr) return false;
			if(ov_open_callbacks(pFile, apVorbis, nullptr, 0, kVorbisFileCallbacks) < 0)
			{
				fclose(pFile);
				return false;
			}
			return true;
		}

		ALenum Format16(int alChannels)
		{
			if(alChannels == 1) return AL_FORMAT_MONO16;
			if(alChannels == 2) return AL_FORMAT_STEREO16;
			return 0;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// EFX ENTRY POINTS
	//////////////////////////////////////////////////////////////////////////

	struct cOpenALDevice::sEFX
	{
		LPALGENEFFECTS alGenEffects = nullptr;
		LPALDELETEEFFECTS alDeleteEffects = nullptr;
		LPALEFFECTI alEffecti = nullptr;
		LPALEFFECTF alEffectf = nullptr;
		LPALEFFECTFV alEffectfv = nullptr;
		LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
		LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
		LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
		LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = nullptr;
		LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf = nullptr;
		LPALGENFILTERS alGenFilters = nullptr;
		LPALDELETEFILTERS alDeleteFilters = nullptr;
		LPALFILTERI alFilteri = nullptr;
		LPALFILTERF alFilterf = nullptr;
	};

	bool cOpenALDevice::LoadEFX()
	{
		mpEFX.reset(new sEFX);
		sEFX &e = *mpEFX;
#define HPL_LOAD_EFX(name) e.name = reinterpret_cast<decltype(e.name)>(alGetProcAddress(#name)); if(e.name == nullptr) return false;
		HPL_LOAD_EFX(alGenEffects)
		HPL_LOAD_EFX(alDeleteEffects)
		HPL_LOAD_EFX(alEffecti)
		HPL_LOAD_EFX(alEffectf)
		HPL_LOAD_EFX(alEffectfv)
		HPL_LOAD_EFX(alGenAuxiliaryEffectSlots)
		HPL_LOAD_EFX(alDeleteAuxiliaryEffectSlots)
		HPL_LOAD_EFX(alAuxiliaryEffectSloti)
		HPL_LOAD_EFX(alAuxiliaryEffectSlotf)
		HPL_LOAD_EFX(alGetAuxiliaryEffectSlotf)
		HPL_LOAD_EFX(alGenFilters)
		HPL_LOAD_EFX(alDeleteFilters)
		HPL_LOAD_EFX(alFilteri)
		HPL_LOAD_EFX(alFilterf)
#undef HPL_LOAD_EFX
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// SAMPLE
	//////////////////////////////////////////////////////////////////////////

	cOpenALSample::~cOpenALSample()
	{
		if(mlBuffer) alDeleteBuffers(1, &mlBuffer);
	}

	bool cOpenALSample::Upload(const void *apData, size_t alBytes, ALenum aFormat, int alChannels,
								int alFrequency, int alBytesPerFrame)
	{
		if(aFormat == 0 || alBytes == 0 || alFrequency <= 0) return false;
		alGetError();
		alGenBuffers(1, &mlBuffer);
		if(alGetError() != AL_NO_ERROR)
		{
			mlBuffer = 0;
			return false;
		}
		alBufferData(mlBuffer, aFormat, apData, (ALsizei)alBytes, alFrequency);
		if(alGetError() != AL_NO_ERROR) return false;
		mlChannels = alChannels;
		mfTotalTime = (double)(alBytes / alBytesPerFrame) / (double)alFrequency;
		return true;
	}

	std::unique_ptr<cOpenALSample> cOpenALSample::FromPCM16(const int16_t *apData, size_t alFrames,
															int alChannels, int alFrequency)
	{
		std::unique_ptr<cOpenALSample> pSample(new cOpenALSample);
		if(!pSample->Upload(apData, alFrames * alChannels * 2, Format16(alChannels), alChannels,
							alFrequency, alChannels * 2))
			return nullptr;
		return pSample;
	}

	std::unique_ptr<cOpenALSample> cOpenALSample::Load(const std::string &asFile)
	{
		std::string sExt = LowerExtension(asFile);
		std::unique_ptr<cOpenALSample> pSample(new cOpenALSample);

		if(sExt == "ogg" || sExt == "oga")
		{
			OggVorbis_File vorbis;
			if(!OpenVorbis(asFile, &vorbis)) return nullptr;

			vorbis_info *pInfo = ov_info(&vorbis, -1);
			int lChannels = pInfo ? pInfo->channels : 0;
			int lFrequency = pInfo ? (int)pInfo->rate : 0;
			ogg_int64_t lTotal = ov_pcm_total(&vorbis, -1);

			std::vector<char> vPCM;
			if(lTotal > 0) vPCM.reserve((size_t)lTotal * lChannels * 2);
			char vChunk[16384];
			int lSection = 0;
			bool bOk = Format16(lChannels) != 0;
			while(bOk)
			{
				long lRead = ov_read(&vorbis, vChunk, sizeof(vChunk), kVorbisBigEndian, 2, 1, &lSection);
				if(lRead == 0) break;
				if(lRead == OV_HOLE) continue;
				if(lRead < 0) { bOk = false; break; }
				vPCM.insert(vPCM.end(), vChunk, vChunk + lRead);
			}
			ov_clear(&vorbis);

			if(!bOk || !pSample->Upload(vPCM.data(), vPCM.size(), Format16(lChannels), lChannels,
										lFrequency, lChannels * 2))
				return nullptr;
			return pSample;
		}

		if(sExt == "wav")
		{
			std::vector<unsigned char> vFile;
			if(!ReadFile(asFile, vFile) || vFile.empty()) return nullptr;

			SDL_IOStream *pIO = SDL_IOFromConstMem(vFile.data(), vFile.size());
			SDL_AudioSpec spec;
			Uint8 *pData = nullptr;
			Uint32 lLength = 0;
			if(pIO == nullptr || !SDL_LoadWAV_IO(pIO, true, &spec, &pData, &lLength))
			{
				Error("Couldn't decode WAV '%s': %s\n", asFile.c_str(), SDL_GetError());
				return nullptr;
			}

			// OpenAL takes 8-bit unsigned and 16-bit native PCM. Convert anything
			// else, and fold more than two channels to stereo.
			bool bOk;
			if((spec.format == SDL_AUDIO_U8 || spec.format == SDL_AUDIO_S16) && spec.channels <= 2)
			{
				int lBytes = SDL_AUDIO_BYTESIZE(spec.format);
				ALenum format = lBytes == 1
					? (spec.channels == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8)
					: Format16(spec.channels);
				bOk = pSample->Upload(pData, lLength, format, spec.channels, spec.freq, lBytes * spec.channels);
			}
			else
			{
				SDL_AudioSpec dest = spec;
				dest.format = SDL_AUDIO_S16;
				dest.channels = spec.channels > 2 ? 2 : spec.channels;
				Uint8 *pConverted = nullptr;
				int lConverted = 0;
				bOk = SDL_ConvertAudioSamples(&spec, pData, (int)lLength, &dest, &pConverted, &lConverted)
					&& pSample->Upload(pConverted, (size_t)lConverted, Format16(dest.channels),
										dest.channels, dest.freq, 2 * dest.channels);
				SDL_free(pConverted);
			}
			SDL_free(pData);
			return bOk ? std::move(pSample) : nullptr;
		}

		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// STREAM
	//////////////////////////////////////////////////////////////////////////

	struct cOpenALStream::sDecoder
	{
		OggVorbis_File mVorbis;
	};

	cOpenALStream::~cOpenALStream()
	{
		if(!mvBuffers.empty()) alDeleteBuffers((ALsizei)mvBuffers.size(), mvBuffers.data());
		if(mpDecoder) ov_clear(&mpDecoder->mVorbis);
	}

	std::unique_ptr<cOpenALStream> cOpenALStream::Open(const std::string &asFile, size_t alBlockBytes,
													   int alBlockCount)
	{
		std::string sExt = LowerExtension(asFile);
		if(sExt != "ogg" && sExt != "oga") return nullptr;

		std::unique_ptr<cOpenALStream> pStream(new cOpenALStream);
		std::unique_ptr<sDecoder> pDecoder(new sDecoder);
		if(!OpenVorbis(asFile, &pDecoder->mVorbis)) return nullptr;
		pStream->mpDecoder = std::move(pDecoder);

		OggVorbis_File *pVorbis = &pStream->mpDecoder->mVorbis;
		vorbis_info *pInfo = ov_info(pVorbis, -1);
		pStream->mlChannels = pInfo ? pInfo->channels : 0;
		pStream->mlFrequency = pInfo ? (int)pInfo->rate : 0;
		pStream->mFormat = Format16(pStream->mlChannels);
		pStream->mfTotalTime = ov_time_total(pVorbis, -1);
		if(pStream->mFormat == 0 || pStream->mlFrequency <= 0) return nullptr;

		// Whole frames per block, so a block never splits a sample.
		size_t lFrameBytes = (size_t)pStream->mlChannels * 2;
		alBlockBytes -= alBlockBytes % lFrameBytes;
		pStream->mvPCM.resize(alBlockBytes);
		pStream->mvBuffers.resize(alBlockCount);
		pStream->mvBufferTimes.assign(alBlockCount, 0.0);

		alGetError();
		alGenBuffers(alBlockCount, pStream->mvBuffers.data());
		if(alGetError() != AL_NO_ERROR)
		{
			pStream->mvBuffers.clear();
			return nullptr;
		}
		return pStream;
	}

	double cOpenALStream::BlockTime(size_t alBytes) const
	{
		return (double)alBytes / ((double)mlChannels * 2.0 * (double)mlFrequency);
	}

	bool cOpenALStream::Fill(ALuint alBuffer)
	{
		if(mbFailed) return false;

		size_t lBytes = 0;
		int lSection = 0;
		while(lBytes < mvPCM.size())
		{
			long lRead = ov_read(&mpDecoder->mVorbis, mvPCM.data() + lBytes, (int)(mvPCM.size() - lBytes),
								 kVorbisBigEndian, 2, 1, &lSection);
			if(lRead == 0) break;
			if(lRead == OV_HOLE) continue;
			if(lRead < 0)
			{
				mbFailed = true;
				break;
			}
			lBytes += (size_t)lRead;
		}

		if(lBytes == 0)
		{
			mbEOF = true;
			return false;
		}

		alBufferData(alBuffer, mFormat, mvPCM.data(), (ALsizei)lBytes, mlFrequency);
		for(size_t i = 0; i < mvBuffers.size(); ++i)
		{
			if(mvBuffers[i] == alBuffer) mvBufferTimes[i] = BlockTime(lBytes);
		}
		return true;
	}

	void cOpenALStream::Rewind()
	{
		ov_pcm_seek(&mpDecoder->mVorbis, 0);
		mbEOF = false;
	}

	//////////////////////////////////////////////////////////////////////////
	// DEVICE
	//////////////////////////////////////////////////////////////////////////

	cOpenALDevice::cOpenALDevice()
	{
	}

	cOpenALDevice::~cOpenALDevice()
	{
		Close();
	}

	//-----------------------------------------------------------------------

	std::vector<std::string> cOpenALDevice::GetOutputDevices()
	{
		std::vector<std::string> vDevices;
		const ALCchar *pList = nullptr;
		if(alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") == ALC_TRUE)
			pList = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
		else if(alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") == ALC_TRUE)
			pList = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);

		while(pList && *pList)
		{
			vDevices.push_back(pList);
			pList += strlen(pList) + 1;
		}
		return vDevices;
	}

	std::string cOpenALDevice::GetString(ALenum aParam) const
	{
		const ALchar *pString = mpContext ? alGetString(aParam) : nullptr;
		return pString ? pString : "";
	}

	//-----------------------------------------------------------------------

	bool cOpenALDevice::Open(const cOpenALDeviceParams &aParams)
	{
		Close();
		mParams = aParams;

		std::string sName = aParams.msDeviceName;
		if(sName == "NULL") sName.clear();
		mpDevice = alcOpenDevice(sName.empty() ? nullptr : sName.c_str());
		if(mpDevice == nullptr) return false;

		bool bEFX = aParams.mbUseEFX && alcIsExtensionPresent(mpDevice, "ALC_EXT_EFX") == ALC_TRUE;
		mbEFXRequested = bEFX;
		mbHRTFExt = alcIsExtensionPresent(mpDevice, "ALC_SOFT_HRTF") == ALC_TRUE;
		mbOutputModeExt = alcIsExtensionPresent(mpDevice, "ALC_SOFT_output_mode") == ALC_TRUE;
		mbDisconnectExt = alcIsExtensionPresent(mpDevice, "ALC_EXT_disconnect") == ALC_TRUE;
		mpResetDevice = mbHRTFExt
			? reinterpret_cast<LPALCRESETDEVICESOFT>(alcGetProcAddress(mpDevice, "alcResetDeviceSOFT")) : nullptr;
		mpReopenDevice = alcIsExtensionPresent(mpDevice, "ALC_SOFT_reopen_device") == ALC_TRUE
			? reinterpret_cast<LPALCREOPENDEVICESOFT>(alcGetProcAddress(mpDevice, "alcReopenDeviceSOFT")) : nullptr;

		int lSources = std::max(1, std::min(aParams.mlSources, kHandleIndexMask));
		std::vector<ALCint> vAttributes = Attributes();
		mpContext = alcCreateContext(mpDevice, vAttributes.data());
		if(mpContext == nullptr || alcMakeContextCurrent(mpContext) == ALC_FALSE)
		{
			if(mpContext) alcDestroyContext(mpContext);
			mpContext = nullptr;
			alcCloseDevice(mpDevice);
			mpDevice = nullptr;
			return false;
		}

		ReadDeviceName();
		mbConnected = true;
		mbLossReported = false;

		alDistanceModel(AL_NONE);

		// Stereo data plays straight to the matching output channels, so HRTF
		// or a surround layout doesn't turn music and UI sounds into virtual
		// speakers. OpenAL doesn't position stereo sources either way.
		mlDirectChannels = 0;
		if(alIsExtensionPresent("AL_SOFT_direct_channels_remix") == AL_TRUE)
			mlDirectChannels = AL_REMIX_UNMATCHED_SOFT;
		else if(alIsExtensionPresent("AL_SOFT_direct_channels") == AL_TRUE)
			mlDirectChannels = AL_TRUE;

		//////////////
		// Reverb slots and effects: one per send, up to two, so environments
		// can crossfade. Slots start silent (weight 0).
		mbEFX = false;
		mlReverbSlots = 0;
		if(bEFX && LoadEFX())
		{
			ALCint lSends = 0;
			alcGetIntegerv(mpDevice, ALC_MAX_AUXILIARY_SENDS, 1, &lSends);
			int lSlots = std::max(0, std::min((int)lSends, kMaxReverbSlots));
			alGetError();
			for(int i = 0; i < lSlots; ++i)
			{
				mpEFX->alGenAuxiliaryEffectSlots(1, &mlReverbSlot[i]);
				mpEFX->alGenEffects(1, &mlReverbEffect[i]);
				if(alGetError() != AL_NO_ERROR) break;
				if(i == 0)
				{
					mpEFX->alEffecti(mlReverbEffect[i], AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
					mbEAXReverb = alGetError() == AL_NO_ERROR;
				}
				if(i > 0 || !mbEAXReverb)
					mpEFX->alEffecti(mlReverbEffect[i], AL_EFFECT_TYPE, mbEAXReverb ? AL_EFFECT_EAXREVERB : AL_EFFECT_REVERB);
				mpEFX->alAuxiliaryEffectSlotf(mlReverbSlot[i], AL_EFFECTSLOT_GAIN, 0.0f);
				mpEFX->alAuxiliaryEffectSloti(mlReverbSlot[i], AL_EFFECTSLOT_EFFECT, (ALint)mlReverbEffect[i]);
				mpEFX->alAuxiliaryEffectSloti(mlReverbSlot[i], AL_EFFECTSLOT_AUXILIARY_SEND_AUTO, AL_FALSE);
				if(alGetError() != AL_NO_ERROR) break;
				mfReverbWeight[i] = 0;
				mlReverbSlots = i + 1;
			}
			mbEFX = mlReverbSlots > 0;
			if(!mbEFX) DeleteReverb();
		}

		//////////////
		// Source pool
		alGetError();
		mvSources.reserve(lSources);
		for(int i = 0; i < lSources; ++i)
		{
			sSource source;
			alGenSources(1, &source.mlId);
			if(alGetError() != AL_NO_ERROR) break;
			if(mbEFX) mpEFX->alGenFilters(1, &source.mlFilter);
			mvSources.push_back(source);
		}
		mlFreeVoices = (int)mvSources.size();
		if(mvSources.empty())
		{
			Close();
			return false;
		}
		for(sSource &source : mvSources) StopSource(source);

		mbStopWorker = false;
		if(aParams.mbUseThread) mWorker = std::thread(&cOpenALDevice::WorkerLoop, this);

		return true;
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::Close()
	{
		if(mWorker.joinable())
		{
			mbStopWorker = true;
			mWake.notify_all();
			mWorker.join();
		}

		std::lock_guard<std::mutex> lock(mMutex);
		for(sSource &source : mvSources)
		{
			StopSource(source);
			alDeleteSources(1, &source.mlId);
			if(source.mlFilter) mpEFX->alDeleteFilters(1, &source.mlFilter);
		}
		mvSources.clear();
		mlFreeVoices = 0;

		DeleteReverb();
		mbEFX = false;

		if(mpContext)
		{
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(mpContext);
			mpContext = nullptr;
		}
		if(mpDevice)
		{
			alcCloseDevice(mpDevice);
			mpDevice = nullptr;
		}
		mpReopenDevice = nullptr;
		mpResetDevice = nullptr;
		mbConnected = true;
	}

	//-----------------------------------------------------------------------

	std::vector<ALCint> cOpenALDevice::Attributes() const
	{
		int lSources = std::max(1, std::min(mParams.mlSources, kHandleIndexMask));
		// No ALC_FREQUENCY: OpenAL Soft mixes at the device's own rate.
		std::vector<ALCint> vAttributes = {
			ALC_MONO_SOURCES, lSources,
			ALC_STEREO_SOURCES, lSources,
			ALC_MAX_AUXILIARY_SENDS, mbEFXRequested ? 2 : 0,
		};
		if(mbHRTFExt && mParams.mHRTF != eOpenALHRTF::Auto)
		{
			vAttributes.push_back(ALC_HRTF_SOFT);
			vAttributes.push_back(mParams.mHRTF == eOpenALHRTF::On ? ALC_TRUE : ALC_FALSE);
		}
		// HRTF needs stereo output, so ask for it even when the device reports
		// a surround layout. Otherwise the device's own layout is used.
		if(mbOutputModeExt && mParams.mHRTF == eOpenALHRTF::On)
		{
			vAttributes.push_back(ALC_OUTPUT_MODE_SOFT);
			vAttributes.push_back(ALC_STEREO_HRTF_SOFT);
		}
		vAttributes.push_back(0);
		return vAttributes;
	}

	void cOpenALDevice::ReadDeviceName()
	{
		const ALCchar *pName = nullptr;
		if(alcIsExtensionPresent(mpDevice, "ALC_ENUMERATE_ALL_EXT") == ALC_TRUE)
			pName = alcGetString(mpDevice, ALC_ALL_DEVICES_SPECIFIER);
		if(pName == nullptr) pName = alcGetString(mpDevice, ALC_DEVICE_SPECIFIER);
		msDeviceName = pName ? pName : "";
	}

	//-----------------------------------------------------------------------

	bool cOpenALDevice::SetHRTF(eOpenALHRTF aMode)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mParams.mHRTF = aMode;
		if(mpDevice == nullptr) return true;
		if(mpResetDevice == nullptr) return false;
		std::vector<ALCint> vAttributes = Attributes();
		return mpResetDevice(mpDevice, vAttributes.data()) == ALC_TRUE;
	}

	int cOpenALDevice::GetHRTFStatus() const
	{
		if(mpDevice == nullptr || !mbHRTFExt) return -1;
		ALCint lStatus = 0;
		alcGetIntegerv(mpDevice, ALC_HRTF_STATUS_SOFT, 1, &lStatus);
		return lStatus;
	}

	bool cOpenALDevice::IsHRTFEnabled() const
	{
		if(mpDevice == nullptr || !mbHRTFExt) return false;
		ALCint lEnabled = 0;
		alcGetIntegerv(mpDevice, ALC_HRTF_SOFT, 1, &lEnabled);
		return lEnabled == ALC_TRUE;
	}

	std::string cOpenALDevice::GetHRTFName() const
	{
		if(!IsHRTFEnabled()) return "";
		const ALCchar *pName = alcGetString(mpDevice, ALC_HRTF_SPECIFIER_SOFT);
		return pName ? pName : "";
	}

	//-----------------------------------------------------------------------

	bool cOpenALDevice::Reopen(const std::string &asDeviceName)
	{
		if(mpDevice == nullptr || mpReopenDevice == nullptr) return false;
		std::string sName = asDeviceName == "NULL" ? "" : asDeviceName;

		std::lock_guard<std::mutex> lock(mMutex);
		std::vector<ALCint> vAttributes = Attributes();
		if(mpReopenDevice(mpDevice, sName.empty() ? nullptr : sName.c_str(), vAttributes.data()) != ALC_TRUE)
			return false;
		ReadDeviceName();
		mbConnected = true;
		mbLossReported = false;
		ResumeAfterReopen();
		return true;
	}

	bool cOpenALDevice::CheckConnected()
	{
		if(mpDevice == nullptr || !mbDisconnectExt) return true;
		ALCint lConnected = ALC_TRUE;
		alcGetIntegerv(mpDevice, ALC_CONNECTED, 1, &lConnected);
		mbConnected = lConnected != ALC_FALSE;
		return mbConnected;
	}

	void cOpenALDevice::ResumeAfterReopen()
	{
		// A lost device stops every source. Looping samples start again,
		// streams restart through the underrun check, and one-shot samples
		// that were cut off end normally.
		for(sSource &source : mvSources)
		{
			if(!source.mbPlaying || source.mbPaused || source.mpSample == nullptr || !source.mbLoop) continue;
			ALint lState = 0;
			alGetSourcei(source.mlId, AL_SOURCE_STATE, &lState);
			if(lState == AL_STOPPED) alSourcePlay(source.mlId);
		}
	}

	void cOpenALDevice::ApplyDirectChannels(sSource &aSource, int alChannels)
	{
		if(mlDirectChannels == 0) return;
		alSourcei(aSource.mlId, AL_DIRECT_CHANNELS_SOFT, alChannels > 1 ? mlDirectChannels : AL_FALSE);
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::WorkerLoop()
	{
		int lFreq = mParams.mlStreamUpdateFreq > 0 ? mParams.mlStreamUpdateFreq : 10;
		auto wait = std::chrono::milliseconds(std::max(5, std::min(50, 1000 / lFreq)));

		std::unique_lock<std::mutex> lock(mMutex);
		while(!mbStopWorker)
		{
			UpdateStreamsLocked();
			mWake.wait_for(lock, wait, [this] { return mbStopWorker.load(); });
		}
	}

	void cOpenALDevice::Update()
	{
		if(mpDevice == nullptr) return;

		bool bReopen = false;
		{
			std::lock_guard<std::mutex> lock(mMutex);
			if(CheckConnected())
			{
				if(!mWorker.joinable()) UpdateStreamsLocked();
				return;
			}
			auto now = std::chrono::steady_clock::now();
			if(!mbLossReported)
			{
				Warning("Audio device '%s' was disconnected.\n", msDeviceName.c_str());
				mbLossReported = true;
				mNextReconnect = now;
			}
			if(mpReopenDevice && now >= mNextReconnect)
			{
				mNextReconnect = now + std::chrono::seconds(1);
				bReopen = true;
			}
		}

		// Retried once a second until a device is available.
		if(bReopen && Reopen(""))
			Log("Audio moved to the default device: %s\n", msDeviceName.c_str());
	}

	void cOpenALDevice::UpdateStreamsLocked()
	{
		// A lost device stops its sources at once; refilling them would race
		// through the streams.
		if(!CheckConnected()) return;
		for(sSource &source : mvSources)
		{
			if(source.mpStream && !IsFree(source)) UpdateStream(source);
		}
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::UpdateStream(sSource &aSource)
	{
		cOpenALStream *pStream = aSource.mpStream;

		ALint lProcessed = 0;
		alGetSourcei(aSource.mlId, AL_BUFFERS_PROCESSED, &lProcessed);
		while(lProcessed-- > 0)
		{
			ALuint lBuffer = 0;
			alSourceUnqueueBuffers(aSource.mlId, 1, &lBuffer);
			for(size_t i = 0; i < pStream->mvBuffers.size(); ++i)
			{
				if(pStream->mvBuffers[i] == lBuffer) pStream->mfProcessedTime += pStream->mvBufferTimes[i];
			}
			if(pStream->mfTotalTime > 0 && pStream->mfProcessedTime > pStream->mfTotalTime)
				pStream->mfProcessedTime -= pStream->mfTotalTime;

			bool bFilled = pStream->Fill(lBuffer);
			if(!bFilled && pStream->mbEOF && (pStream->mbLoop || aSource.mbLoop))
			{
				pStream->Rewind();
				bFilled = pStream->Fill(lBuffer);
			}
			if(bFilled) alSourceQueueBuffers(aSource.mlId, 1, &lBuffer);
		}

		// Restart after an underrun: the source stopped with data still queued.
		if(aSource.mbPlaying && !aSource.mbPaused)
		{
			ALint lState = 0, lQueued = 0;
			alGetSourcei(aSource.mlId, AL_SOURCE_STATE, &lState);
			alGetSourcei(aSource.mlId, AL_BUFFERS_QUEUED, &lQueued);
			if((lState == AL_STOPPED || lState == AL_INITIAL) && lQueued > 0)
				alSourcePlay(aSource.mlId);
		}
	}

	//-----------------------------------------------------------------------

	bool cOpenALDevice::IsStreamDrained(sSource &aSource)
	{
		cOpenALStream *pStream = aSource.mpStream;
		if(pStream->mbFailed) return true;
		if(!pStream->mbEOF) return false;
		ALint lQueued = 0, lProcessed = 0;
		alGetSourcei(aSource.mlId, AL_BUFFERS_QUEUED, &lQueued);
		alGetSourcei(aSource.mlId, AL_BUFFERS_PROCESSED, &lProcessed);
		return lQueued == lProcessed;
	}

	bool cOpenALDevice::IsFree(sSource &aSource)
	{
		if(aSource.mpSample == nullptr && aSource.mpStream == nullptr) return true;

		ALint lState = 0;
		alGetSourcei(aSource.mlId, AL_SOURCE_STATE, &lState);
		if(lState != AL_STOPPED && lState != AL_INITIAL) return false;
		if(aSource.mbPaused) return false;
		// A looping sample only ends through Stop. It stops by itself only
		// when the device is lost, and plays again after the reopen.
		if(aSource.mbPlaying && aSource.mpSample && aSource.mbLoop) return false;
		if(aSource.mbPlaying && aSource.mpStream && !IsStreamDrained(aSource)) return false;
		return true;
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::StopSource(sSource &aSource)
	{
		alSourceStop(aSource.mlId);
		alSourcei(aSource.mlId, AL_LOOPING, AL_FALSE);
		alSourcef(aSource.mlId, AL_GAIN, 0.00001f);
		alSourcef(aSource.mlId, AL_PITCH, 1.0f);
		alSourcei(aSource.mlId, AL_BUFFER, 0);

		aSource.mlPriority = kMaxPriority;
		aSource.mbPlaying = false;
		aSource.mbPaused = false;
		aSource.mbLoop = false;
		aSource.mbLowPass = false;
		aSource.mbDirectFilter = false;
		aSource.mbSendFilter = false;
		aSource.mbSendToReverb = false;
		aSource.mfFilterGain = 1;
		aSource.mfFilterGainHF = 1;
		ApplyFilters(aSource);

		if(aSource.mpStream) aSource.mpStream->mlBoundSource = -1;
		aSource.mpStream = nullptr;
		aSource.mpSample = nullptr;
		mlFreeVoices += aSource.mlVoices;
		aSource.mlVoices = 0;
	}

	//-----------------------------------------------------------------------

	int cOpenALDevice::MakeHandle(int alIndex) const
	{
		return (int)((mvSources[alIndex].mlGeneration & kGenerationMask) << kHandleIndexBits) | alIndex;
	}

	cOpenALDevice::sSource* cOpenALDevice::GetSource(int alHandle)
	{
		if(alHandle < 0) return nullptr;
		int lIndex = alHandle & kHandleIndexMask;
		uint32_t lGeneration = ((uint32_t)alHandle >> kHandleIndexBits) & kGenerationMask;
		if(lIndex >= (int)mvSources.size()) return nullptr;
		sSource &source = mvSources[lIndex];
		if((source.mlGeneration & kGenerationMask) != lGeneration) return nullptr;
		return &source;
	}

	int cOpenALDevice::AcquireSource(int alPriority, int alVoices)
	{
		// Sources that ended by themselves keep their voices until the
		// engine stops their channels. Reclaim those before stealing a sound
		// that is still playing.
		if(mlFreeVoices < alVoices)
		{
			for(sSource &source : mvSources)
			{
				if(source.mlVoices > 0 && IsFree(source))
				{
					StopSource(source);
					++source.mlGeneration;
				}
			}
		}

		// Steal the lowest-priority sources below the request until enough
		// voices are free.
		while(mlFreeVoices < alVoices)
		{
			int lLowest = -1;
			int lLowestPriority = alPriority;
			for(int i = 0; i < (int)mvSources.size(); ++i)
			{
				if(mvSources[i].mlPriority < lLowestPriority)
				{
					lLowestPriority = mvSources[i].mlPriority;
					lLowest = i;
				}
			}
			if(lLowest == -1) break;
			StopSource(mvSources[lLowest]);
			++mvSources[lLowest].mlGeneration;
		}
		if(mlFreeVoices < alVoices) return -1;

		for(int i = 0; i < (int)mvSources.size(); ++i)
		{
			if(IsFree(mvSources[i])) return i;
		}
		return -1;
	}

	//-----------------------------------------------------------------------

	int cOpenALDevice::PlaySample(cOpenALSample *apSample, int alPriority, float afGain, bool abStartPaused)
	{
		if(apSample == nullptr || mpContext == nullptr) return kInvalidHandle;
		std::lock_guard<std::mutex> lock(mMutex);

		int lPriority = std::max(0, std::min(kMaxPriority, alPriority));
		int lIndex = AcquireSource(lPriority, apSample->GetChannels());
		if(lIndex < 0) return kInvalidHandle;

		sSource &source = mvSources[lIndex];
		StopSource(source);
		++source.mlGeneration;
		alSourcei(source.mlId, AL_BUFFER, (ALint)apSample->GetBuffer());
		ApplyDirectChannels(source, apSample->GetChannels());
		source.mpSample = apSample;
		source.mlVoices = apSample->GetChannels();
		mlFreeVoices -= source.mlVoices;
		source.mlPriority = lPriority;
		alSourcef(source.mlId, AL_GAIN, Clamp(afGain, 0.00001f, 1.0f));
		source.mbPaused = abStartPaused;
		source.mbPlaying = true;
		if(!abStartPaused) alSourcePlay(source.mlId);

		return MakeHandle(lIndex);
	}

	int cOpenALDevice::PlayStream(cOpenALStream *apStream, float afGain, bool abStartPaused)
	{
		if(apStream == nullptr || mpContext == nullptr) return kInvalidHandle;
		std::lock_guard<std::mutex> lock(mMutex);
		if(apStream->mlBoundSource != -1) return kInvalidHandle;

		int lIndex = AcquireSource(kStreamPriority, apStream->GetChannels());
		if(lIndex < 0) return kInvalidHandle;

		sSource &source = mvSources[lIndex];
		StopSource(source);
		++source.mlGeneration;
		source.mpStream = apStream;
		ApplyDirectChannels(source, apStream->GetChannels());
		source.mlVoices = apStream->GetChannels();
		mlFreeVoices -= source.mlVoices;
		source.mlPriority = kStreamPriority;
		apStream->mlBoundSource = lIndex;

		apStream->Rewind();
		apStream->mbFailed = false;
		apStream->mfProcessedTime = 0;
		for(ALuint lBuffer : apStream->mvBuffers)
		{
			if(!apStream->Fill(lBuffer)) break;
			alSourceQueueBuffers(source.mlId, 1, &lBuffer);
		}

		alSourcef(source.mlId, AL_GAIN, Clamp(afGain, 0.00001f, 1.0f));
		source.mbPaused = abStartPaused;
		source.mbPlaying = true;
		if(!abStartPaused) alSourcePlay(source.mlId);

		return MakeHandle(lIndex);
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::Stop(int alHandle)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource && (pSource->mpSample || pSource->mpStream)) StopSource(*pSource);
	}

	void cOpenALDevice::SetPaused(int alHandle, bool abPaused)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr) return;
		if(abPaused && !pSource->mbPaused)
		{
			alSourcePause(pSource->mlId);
		}
		else if(!abPaused && pSource->mbPaused && (pSource->mpSample || pSource->mpStream))
		{
			// A one-shot sample that ended before the pause stays ended;
			// playing a stopped source would start it again from the top.
			// Looping samples stopped by a device loss resume, and streams
			// restart through the underrun check.
			ALint lState = 0;
			alGetSourcei(pSource->mlId, AL_SOURCE_STATE, &lState);
			if(lState == AL_PAUSED || lState == AL_INITIAL ||
			   (lState == AL_STOPPED && pSource->mpSample && pSource->mbLoop))
				alSourcePlay(pSource->mlId);
		}
		pSource->mbPaused = abPaused;
	}

	void cOpenALDevice::SetGain(int alHandle, float afGain)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource) alSourcef(pSource->mlId, AL_GAIN, Clamp(afGain, 0.00001f, 1.0f));
	}

	void cOpenALDevice::SetPitch(int alHandle, float afPitch)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource) alSourcef(pSource->mlId, AL_PITCH, Clamp(afPitch, 0.0001f, 10.0f));
	}

	void cOpenALDevice::SetLoop(int alHandle, bool abLoop)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr) return;
		pSource->mbLoop = abLoop;
		// Streams loop by rewinding the decoder, not with AL_LOOPING.
		if(pSource->mpSample) alSourcei(pSource->mlId, AL_LOOPING, abLoop ? AL_TRUE : AL_FALSE);
	}

	void cOpenALDevice::SetPositionVelocity(int alHandle, const float *apPos, const float *apVel)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr) return;
		alSourcefv(pSource->mlId, AL_POSITION, apPos);
		alSourcefv(pSource->mlId, AL_VELOCITY, apVel);
	}

	void cOpenALDevice::SetPriority(int alHandle, int alPriority)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr) return;
		pSource->mlPriority = pSource->mpStream ? kStreamPriority
								: std::max(0, std::min(kMaxPriority, alPriority));
	}

	int cOpenALDevice::GetPriority(int alHandle)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		return pSource ? pSource->mlPriority : 0;
	}

	bool cOpenALDevice::IsPlaying(int alHandle)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		return pSource && !IsFree(*pSource);
	}

	double cOpenALDevice::GetElapsedTime(int alHandle)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr || (pSource->mpSample == nullptr && pSource->mpStream == nullptr)) return 0;
		ALfloat fOffset = 0;
		alGetSourcef(pSource->mlId, AL_SEC_OFFSET, &fOffset);
		if(pSource->mpSample) return fOffset;
		double fElapsed = pSource->mpStream->mfProcessedTime + fOffset;
		double fTotal = pSource->mpStream->mfTotalTime;
		if(fTotal > 0 && fElapsed > fTotal) fElapsed -= fTotal;
		return fElapsed;
	}

	double cOpenALDevice::GetTotalTime(int alHandle)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr) return 0;
		if(pSource->mpSample) return pSource->mpSample->GetTotalTime();
		if(pSource->mpStream) return pSource->mpStream->GetTotalTime();
		return 0;
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::ApplyFilters(sSource &aSource)
	{
		if(!mbEFX || aSource.mlFilter == 0) return;

		ALuint lFilter = AL_FILTER_NULL;
		if(aSource.mbLowPass)
		{
			mpEFX->alFilteri(aSource.mlFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			mpEFX->alFilterf(aSource.mlFilter, AL_LOWPASS_GAIN, aSource.mfFilterGain);
			mpEFX->alFilterf(aSource.mlFilter, AL_LOWPASS_GAINHF, aSource.mfFilterGainHF);
			lFilter = aSource.mlFilter;
		}
		// Filter parameters are copied when attached, so reattach after changes.
		alSourcei(aSource.mlId, AL_DIRECT_FILTER, (ALint)(aSource.mbDirectFilter ? lFilter : AL_FILTER_NULL));
		for(int i = 0; i < mlReverbSlots; ++i)
		{
			alSource3i(aSource.mlId, AL_AUXILIARY_SEND_FILTER,
					   (ALint)(aSource.mbSendToReverb ? mlReverbSlot[i] : AL_EFFECTSLOT_NULL), (ALint)i,
					   (ALint)(aSource.mbSendFilter ? lFilter : AL_FILTER_NULL));
		}
	}

	void cOpenALDevice::SetLowPassFilter(int alHandle, bool abLowPass)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr || !mbEFX) return;
		pSource->mbLowPass = abLowPass;
		ApplyFilters(*pSource);
	}

	void cOpenALDevice::SetFiltering(int alHandle, bool abEnabled, int alFlags)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr || !mbEFX) return;
		if(alFlags & 0x1) pSource->mbDirectFilter = abEnabled;
		if(alFlags & 0x2) pSource->mbSendFilter = abEnabled;
		ApplyFilters(*pSource);
	}

	void cOpenALDevice::SetFilterGain(int alHandle, float afGain)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr || !mbEFX) return;
		pSource->mfFilterGain = Clamp(afGain, 0, 1);
		ApplyFilters(*pSource);
	}

	void cOpenALDevice::SetFilterGainHF(int alHandle, float afGainHF)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr || !mbEFX) return;
		pSource->mfFilterGainHF = Clamp(afGainHF, 0, 1);
		ApplyFilters(*pSource);
	}

	bool cOpenALDevice::GetFilterState(int alHandle, bool &abDirect, bool &abSend, float &afGainHF)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr) return false;
		abDirect = pSource->mbLowPass && pSource->mbDirectFilter;
		abSend = pSource->mbLowPass && pSource->mbSendFilter;
		afGainHF = pSource->mfFilterGainHF;
		return true;
	}

	void cOpenALDevice::SetSendToReverb(int alHandle, bool abSend)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		sSource *pSource = GetSource(alHandle);
		if(pSource == nullptr || !mbEFX) return;
		pSource->mbSendToReverb = abSend;
		ApplyFilters(*pSource);
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::ReleaseSample(cOpenALSample *apSample)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		for(sSource &source : mvSources)
		{
			if(source.mpSample == apSample)
			{
				StopSource(source);
				++source.mlGeneration;
			}
		}
	}

	void cOpenALDevice::ReleaseStream(cOpenALStream *apStream)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		for(sSource &source : mvSources)
		{
			if(source.mpStream == apStream)
			{
				StopSource(source);
				++source.mlGeneration;
			}
		}
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::SetListener(const float *apPos, const float *apVel, const float *apForward,
									const float *apUp)
	{
		if(mpContext == nullptr) return;
		alListenerfv(AL_POSITION, apPos);
		alListenerfv(AL_VELOCITY, apVel);
		ALfloat vOrientation[6] = { apForward[0], apForward[1], apForward[2], apUp[0], apUp[1], apUp[2] };
		alListenerfv(AL_ORIENTATION, vOrientation);
	}

	void cOpenALDevice::SetListenerGain(float afGain)
	{
		if(mpContext) alListenerf(AL_GAIN, Clamp(afGain, 0, 1));
	}

	//-----------------------------------------------------------------------

	void cOpenALDevice::DeleteReverb()
	{
		for(int i = 0; i < kMaxReverbSlots; ++i)
		{
			if(mpEFX && mlReverbSlot[i]) mpEFX->alDeleteAuxiliaryEffectSlots(1, &mlReverbSlot[i]);
			if(mpEFX && mlReverbEffect[i]) mpEFX->alDeleteEffects(1, &mlReverbEffect[i]);
			mlReverbSlot[i] = mlReverbEffect[i] = 0;
			mfReverbWeight[i] = 0;
		}
		mlReverbSlots = 0;
	}

	void cOpenALDevice::ApplyReverbGains()
	{
		if(!mbEFX) return;
		for(int i = 0; i < mlReverbSlots; ++i)
			mpEFX->alAuxiliaryEffectSlotf(mlReverbSlot[i], AL_EFFECTSLOT_GAIN, mfReverbGain * mfReverbWeight[i]);
	}

	void cOpenALDevice::SetReverbGain(float afGain)
	{
		mfReverbGain = Clamp(afGain, 0, 1);
		ApplyReverbGains();
	}

	void cOpenALDevice::SetReverbWeight(int alSlot, float afWeight)
	{
		if(!mbEFX || alSlot < 0 || alSlot >= mlReverbSlots) return;
		float fWeight = Clamp(afWeight, 0, 1);
		if(fWeight == mfReverbWeight[alSlot]) return;
		mfReverbWeight[alSlot] = fWeight;
		mpEFX->alAuxiliaryEffectSlotf(mlReverbSlot[alSlot], AL_EFFECTSLOT_GAIN, mfReverbGain * fWeight);
	}

	float cOpenALDevice::GetReverbSlotGain(int alSlot)
	{
		if(!mbEFX || alSlot < 0 || alSlot >= mlReverbSlots) return 0;
		ALfloat fGain = 0;
		mpEFX->alGetAuxiliaryEffectSlotf(mlReverbSlot[alSlot], AL_EFFECTSLOT_GAIN, &fGain);
		return fGain;
	}

	void cOpenALDevice::SetReverb(int alSlot, const cOpenALReverb &r)
	{
		if(!mbEFX || alSlot < 0 || alSlot >= mlReverbSlots) return;
		sEFX &e = *mpEFX;
		ALuint id = mlReverbEffect[alSlot];

		if(mbEAXReverb)
		{
#define HPL_EAX(param, value) e.alEffectf(id, AL_EAXREVERB_##param, Clamp(value, AL_EAXREVERB_MIN_##param, AL_EAXREVERB_MAX_##param))
			HPL_EAX(DENSITY, r.mfDensity);
			HPL_EAX(DIFFUSION, r.mfDiffusion);
			HPL_EAX(GAIN, r.mfGain);
			HPL_EAX(GAINHF, r.mfGainHF);
			HPL_EAX(GAINLF, r.mfGainLF);
			HPL_EAX(DECAY_TIME, r.mfDecayTime);
			HPL_EAX(DECAY_HFRATIO, r.mfDecayHFRatio);
			HPL_EAX(DECAY_LFRATIO, r.mfDecayLFRatio);
			HPL_EAX(REFLECTIONS_GAIN, r.mfReflectionsGain);
			HPL_EAX(REFLECTIONS_DELAY, r.mfReflectionsDelay);
			HPL_EAX(LATE_REVERB_GAIN, r.mfLateReverbGain);
			HPL_EAX(LATE_REVERB_DELAY, r.mfLateReverbDelay);
			HPL_EAX(ECHO_TIME, r.mfEchoTime);
			HPL_EAX(ECHO_DEPTH, r.mfEchoDepth);
			HPL_EAX(MODULATION_TIME, r.mfModulationTime);
			HPL_EAX(MODULATION_DEPTH, r.mfModulationDepth);
			HPL_EAX(AIR_ABSORPTION_GAINHF, r.mfAirAbsorptionGainHF);
			HPL_EAX(HFREFERENCE, r.mfHFReference);
			HPL_EAX(LFREFERENCE, r.mfLFReference);
			HPL_EAX(ROOM_ROLLOFF_FACTOR, r.mfRoomRolloffFactor);
#undef HPL_EAX
			e.alEffecti(id, AL_EAXREVERB_DECAY_HFLIMIT, r.mbDecayHFLimit ? AL_TRUE : AL_FALSE);
			e.alEffectfv(id, AL_EAXREVERB_REFLECTIONS_PAN, ClampPan(r.mvReflectionsPan).v);
			e.alEffectfv(id, AL_EAXREVERB_LATE_REVERB_PAN, ClampPan(r.mvLateReverbPan).v);
		}
		else
		{
#define HPL_REVERB(param, value) e.alEffectf(id, AL_REVERB_##param, Clamp(value, AL_REVERB_MIN_##param, AL_REVERB_MAX_##param))
			HPL_REVERB(DENSITY, r.mfDensity);
			HPL_REVERB(DIFFUSION, r.mfDiffusion);
			HPL_REVERB(GAIN, r.mfGain);
			HPL_REVERB(GAINHF, r.mfGainHF);
			HPL_REVERB(DECAY_TIME, r.mfDecayTime);
			HPL_REVERB(DECAY_HFRATIO, r.mfDecayHFRatio);
			HPL_REVERB(REFLECTIONS_GAIN, r.mfReflectionsGain);
			HPL_REVERB(REFLECTIONS_DELAY, r.mfReflectionsDelay);
			HPL_REVERB(LATE_REVERB_GAIN, r.mfLateReverbGain);
			HPL_REVERB(LATE_REVERB_DELAY, r.mfLateReverbDelay);
			HPL_REVERB(AIR_ABSORPTION_GAINHF, r.mfAirAbsorptionGainHF);
			HPL_REVERB(ROOM_ROLLOFF_FACTOR, r.mfRoomRolloffFactor);
#undef HPL_REVERB
			e.alEffecti(id, AL_REVERB_DECAY_HFLIMIT, r.mbDecayHFLimit ? AL_TRUE : AL_FALSE);
		}

		// The slot copies the effect's parameters when the effect is attached.
		e.alAuxiliaryEffectSloti(mlReverbSlot[alSlot], AL_EFFECTSLOT_EFFECT, (ALint)id);
	}

}
