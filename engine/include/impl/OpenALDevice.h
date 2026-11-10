/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_OPENAL_DEVICE_H
#define HPL_OPENAL_DEVICE_H

#include <AL/al.h>
#include <AL/alc.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// AUDIO DATA
	//////////////////////////////////////////////////////////////////////////

	/** A sound decoded completely into one OpenAL buffer. */
	class cOpenALSample
	{
	public:
		~cOpenALSample();

		/** Loads an Ogg Vorbis or WAV file; the extension selects the decoder. */
		static std::unique_ptr<cOpenALSample> Load(const std::string &asFile);
		/** Creates a sample from 16-bit PCM (used by tests). */
		static std::unique_ptr<cOpenALSample> FromPCM16(const int16_t *apData, size_t alFrames,
														int alChannels, int alFrequency);

		ALuint GetBuffer() const { return mlBuffer; }
		int GetChannels() const { return mlChannels; }
		double GetTotalTime() const { return mfTotalTime; }

	private:
		cOpenALSample() = default;
		bool Upload(const void *apData, size_t alBytes, ALenum aFormat, int alChannels,
					int alFrequency, int alBytesPerFrame);

		ALuint mlBuffer = 0;
		int mlChannels = 0;
		double mfTotalTime = 0;
	};

	/** An Ogg Vorbis file decoded in blocks into a queue of OpenAL buffers. */
	class cOpenALStream
	{
	public:
		~cOpenALStream();

		static std::unique_ptr<cOpenALStream> Open(const std::string &asFile, size_t alBlockBytes,
												   int alBlockCount);

		int GetChannels() const { return mlChannels; }
		double GetTotalTime() const { return mfTotalTime; }
		void SetLoop(bool abX) { mbLoop = abX; }
		bool GetLoop() const { return mbLoop; }

	private:
		friend class cOpenALDevice;
		cOpenALStream() = default;

		/** Decodes the next block into alBuffer. False at the end of the file or on error. */
		bool Fill(ALuint alBuffer);
		void Rewind();
		double BlockTime(size_t alBytes) const;

		struct sDecoder;
		std::unique_ptr<sDecoder> mpDecoder;
		std::vector<ALuint> mvBuffers;
		std::vector<char> mvPCM;
		std::vector<double> mvBufferTimes;
		int mlChannels = 0;
		int mlFrequency = 0;
		ALenum mFormat = 0;
		double mfTotalTime = 0;
		double mfProcessedTime = 0;
		bool mbLoop = false;
		bool mbEOF = false;
		bool mbFailed = false;
		int mlBoundSource = -1;
	};

	//////////////////////////////////////////////////////////////////////////
	// DEVICE
	//////////////////////////////////////////////////////////////////////////

	struct cOpenALDeviceParams
	{
		std::string msDeviceName;	///< Empty or "NULL" selects the default device.
		int mlSources = 32;			///< Size of the source pool; also the voice budget.
		bool mbUseEFX = true;
		bool mbUseThread = true;
		int mlStreamUpdateFreq = 10;
		size_t mlStreamBlockBytes = 64 * 1024;
		int mlStreamBlockCount = 4;
	};

	/** EAX reverb parameters (see efx.h for their ranges). */
	struct cOpenALReverb
	{
		float mfDensity = 1.0f;
		float mfDiffusion = 1.0f;
		float mfGain = 0.32f;
		float mfGainHF = 0.89f;
		float mfGainLF = 0.0f;
		float mfDecayTime = 1.49f;
		float mfDecayHFRatio = 0.83f;
		float mfDecayLFRatio = 1.0f;
		float mfReflectionsGain = 0.05f;
		float mfReflectionsDelay = 0.007f;
		float mfLateReverbGain = 1.26f;
		float mfLateReverbDelay = 0.011f;
		float mfEchoTime = 0.25f;
		float mfEchoDepth = 0.0f;
		float mfModulationTime = 0.25f;
		float mfModulationDepth = 0.0f;
		float mfAirAbsorptionGainHF = 0.994f;
		float mfHFReference = 5000.0f;
		float mfLFReference = 250.0f;
		float mfRoomRolloffFactor = 0.0f;
		bool mbDecayHFLimit = true;
	};

	/**
	 * OpenAL Soft device, context and a fixed pool of sources.
	 *
	 * Sources are addressed by handles that pack a slot index with a
	 * generation. Stopping or stealing a source bumps the generation, so
	 * stale handles are ignored. A sample uses one voice per channel. When the
	 * voice budget is used up, the lowest-priority source below the request is
	 * stolen. Streams have priority 256 and cannot be stolen by samples.
	 *
	 * One mutex guards every source and stream. The stream worker takes the
	 * same mutex, and its stop flag is atomic. Locking never depends on whether
	 * the worker runs.
	 */
	class cOpenALDevice
	{
	public:
		static const int kInvalidHandle = -1;

		cOpenALDevice();
		~cOpenALDevice();

		bool Open(const cOpenALDeviceParams &aParams);
		void Close();
		bool IsOpen() const { return mpDevice != nullptr; }

		static std::vector<std::string> GetOutputDevices();
		const std::string& GetDeviceName() const { return msDeviceName; }
		std::string GetString(ALenum aParam) const;
		int GetNumSources() const { return (int)mvSources.size(); }
		bool IsEFXActive() const { return mbEFX; }
		size_t GetStreamBlockBytes() const { return mParams.mlStreamBlockBytes; }
		int GetStreamBlockCount() const { return mParams.mlStreamBlockCount; }

		/** Refills streams when no worker thread is used. */
		void Update();

		//////////////
		// Listener
		void SetListener(const float *apPos, const float *apVel, const float *apForward, const float *apUp);
		void SetListenerGain(float afGain);

		//////////////
		// Sources
		int PlaySample(cOpenALSample *apSample, int alPriority, float afGain, bool abStartPaused);
		int PlayStream(cOpenALStream *apStream, float afGain, bool abStartPaused);
		void Stop(int alHandle);
		void SetPaused(int alHandle, bool abPaused);
		void SetGain(int alHandle, float afGain);
		void SetPitch(int alHandle, float afPitch);
		void SetLoop(int alHandle, bool abLoop);
		void SetPositionVelocity(int alHandle, const float *apPos, const float *apVel);
		void SetPriority(int alHandle, int alPriority);
		int GetPriority(int alHandle);
		bool IsPlaying(int alHandle);
		double GetElapsedTime(int alHandle);
		double GetTotalTime(int alHandle);

		void SetLowPassFilter(int alHandle, bool abLowPass);
		void SetFiltering(int alHandle, bool abEnabled, int alFlags);
		void SetFilterGain(int alHandle, float afGain);
		void SetFilterGainHF(int alHandle, float afGainHF);
		void SetSendToReverb(int alHandle, bool abSend);

		/** Stops every source using the data so it can be deleted. */
		void ReleaseSample(cOpenALSample *apSample);
		void ReleaseStream(cOpenALStream *apStream);

		//////////////
		// Reverb
		void SetReverb(const cOpenALReverb &aReverb);
		void SetReverbGain(float afGain);

	private:
		struct sSource
		{
			ALuint mlId = 0;
			ALuint mlFilter = 0;
			uint32_t mlGeneration = 0;
			int mlPriority = 255;
			cOpenALSample *mpSample = nullptr;
			cOpenALStream *mpStream = nullptr;
			int mlVoices = 0;
			bool mbPlaying = false;
			bool mbPaused = false;
			bool mbLoop = false;
			bool mbLowPass = false;
			bool mbDirectFilter = false;
			bool mbSendFilter = false;
			bool mbSendToReverb = false;
			float mfFilterGain = 1;
			float mfFilterGainHF = 1;
		};

		sSource* GetSource(int alHandle);
		int MakeHandle(int alIndex) const;
		int AcquireSource(int alPriority, int alVoices);
		bool IsFree(sSource &aSource);
		bool IsStreamDrained(sSource &aSource);
		void StopSource(sSource &aSource);
		void ApplyFilters(sSource &aSource);
		void UpdateStream(sSource &aSource);
		void UpdateStreamsLocked();
		void WorkerLoop();
		bool LoadEFX();

		ALCdevice *mpDevice = nullptr;
		ALCcontext *mpContext = nullptr;
		std::string msDeviceName;
		cOpenALDeviceParams mParams;
		bool mbEFX = false;
		bool mbEAXReverb = false;
		ALuint mlReverbSlot = 0;
		ALuint mlReverbEffect = 0;

		std::vector<sSource> mvSources;
		int mlFreeVoices = 0;

		std::mutex mMutex;
		std::condition_variable mWake;
		std::atomic<bool> mbStopWorker{false};
		std::thread mWorker;

		struct sEFX;
		std::unique_ptr<sEFX> mpEFX;
	};

};
#endif // HPL_OPENAL_DEVICE_H
