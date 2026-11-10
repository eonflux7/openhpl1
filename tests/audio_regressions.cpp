// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
//
// Runs on OpenAL Soft's null backend (ALSOFT_DRIVERS=null), which mixes in
// real time without an audio device.
#include "impl/LowLevelSoundOpenAL.h"
#include "impl/OpenALDevice.h"
#include "impl/OpenALSoundEnvironment.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace hpl;
namespace fs = std::filesystem;

static int gFailures = 0;

static void Check(bool abResult, const char* asMessage)
{
	if(!abResult)
	{
		std::cerr << "FAIL: " << asMessage << "\n";
		++gFailures;
	}
}

static const std::string kStreamFile = std::string(OPENHPL_TEST_DATA_DIR) + "/tone_stereo_0.5s.ogg";

static std::vector<int16_t> Tone(size_t alFrames, int alChannels)
{
	std::vector<int16_t> vData(alFrames * alChannels);
	for(size_t i = 0; i < alFrames; ++i)
		for(int c = 0; c < alChannels; ++c)
			vData[i * alChannels + c] = (int16_t)(8000 * std::sin(0.06 * (double)i));
	return vData;
}

static std::unique_ptr<cOpenALSample> MakeSample(int alChannels, size_t alFrames = 22050)
{
	std::vector<int16_t> vData = Tone(alFrames, alChannels);
	return cOpenALSample::FromPCM16(vData.data(), alFrames, alChannels, 22050);
}

static cOpenALDeviceParams Params(int alSources, bool abThread)
{
	cOpenALDeviceParams params;
	params.mlSources = alSources;
	params.mbUseThread = abThread;
	params.mbUseEFX = true;
	params.mlStreamUpdateFreq = 100;
	params.mlStreamBlockBytes = 4096;
	params.mlStreamBlockCount = 3;
	return params;
}

static void Wait(cOpenALDevice &aDevice, int alMilliseconds)
{
	auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(alMilliseconds);
	while(std::chrono::steady_clock::now() < end)
	{
		aDevice.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

static void WriteFile(const fs::path &aPath, const std::string &asContents)
{
	std::ofstream file(aPath, std::ios::binary);
	file << asContents;
}

static void WriteWav(const fs::path &aPath, int alChannels, int alBits, bool abFloat, int alFrames)
{
	std::string sData;
	for(int i = 0; i < alFrames * alChannels; ++i)
	{
		double fValue = 0.25 * std::sin(0.06 * (double)(i / alChannels));
		if(abFloat)
		{
			float f = (float)fValue;
			sData.append(reinterpret_cast<const char*>(&f), 4);
		}
		else
		{
			int16_t v = (int16_t)(fValue * 32767);
			sData.append(reinterpret_cast<const char*>(&v), 2);
		}
	}
	auto u32 = [](uint32_t v) { return std::string(reinterpret_cast<const char*>(&v), 4); };
	auto u16 = [](uint16_t v) { return std::string(reinterpret_cast<const char*>(&v), 2); };
	int lBlock = alChannels * alBits / 8;
	std::string sWav = "RIFF" + u32(36 + (uint32_t)sData.size()) + "WAVE"
		+ "fmt " + u32(16) + u16(abFloat ? 3 : 1) + u16((uint16_t)alChannels) + u32(22050)
		+ u32(22050 * lBlock) + u16((uint16_t)lBlock) + u16((uint16_t)alBits)
		+ "data" + u32((uint32_t)sData.size()) + sData;
	WriteFile(aPath, sWav);
}

//-----------------------------------------------------------------------

// Opening and closing with streams and samples playing, alternating the
// stream worker. Closing joins the worker before sources are destroyed.
static void TestLifecycle()
{
	for(int i = 0; i < 12; ++i)
	{
		bool bThread = (i % 2) == 0;
		cOpenALDevice device;
		Check(device.Open(Params(8, bThread)), "null device opens");
		Check(device.IsEFXActive(), "null device supports EFX");

		std::unique_ptr<cOpenALStream> pStream = cOpenALStream::Open(kStreamFile, 4096, 3);
		Check(pStream != nullptr, "test stream opens");
		std::unique_ptr<cOpenALSample> pSample = MakeSample(1);
		if(pStream)
		{
			pStream->SetLoop(true);
			int lStream = device.PlayStream(pStream.get(), 1, false);
			Check(lStream >= 0, "stream gets a source");
			device.SetSendToReverb(lStream, true);
		}
		int lSample = device.PlaySample(pSample.get(), 10, 1, false);
		device.SetLoop(lSample, true);
		device.SetFiltering(lSample, true, 0x3);
		device.SetReverb(cOpenALReverb());
		Wait(device, 15);

		// Release while playing, as the sound manager does when unloading.
		if(pStream) device.ReleaseStream(pStream.get());
		pStream.reset();
		device.ReleaseSample(pSample.get());
		pSample.reset();
		device.Close();
	}

	// Close while the worker is mid-update with streams still bound.
	for(int i = 0; i < 6; ++i)
	{
		cOpenALDevice device;
		Check(device.Open(Params(8, true)), "threaded device opens");
		std::unique_ptr<cOpenALStream> pStream = cOpenALStream::Open(kStreamFile, 4096, 3);
		if(pStream) device.PlayStream(pStream.get(), 1, false);
		std::this_thread::sleep_for(std::chrono::milliseconds(3 * i));
		device.Close();
	}
}

//-----------------------------------------------------------------------

static void TestVoiceStealing()
{
	cOpenALDevice device;
	if(!device.Open(Params(4, false))) { Check(false, "device opens for voice test"); return; }

	std::unique_ptr<cOpenALSample> pMono = MakeSample(1);
	std::unique_ptr<cOpenALSample> pStereo = MakeSample(2);

	int vHandles[4];
	for(int i = 0; i < 4; ++i)
	{
		vHandles[i] = device.PlaySample(pMono.get(), 10 + i, 1, false);
		device.SetLoop(vHandles[i], true);
		Check(vHandles[i] >= 0, "mono samples fill the pool");
	}

	Check(device.PlaySample(pMono.get(), 10, 1, false) == cOpenALDevice::kInvalidHandle,
		  "equal priority cannot steal");

	int lHigh = device.PlaySample(pMono.get(), 50, 1, false);
	Check(lHigh >= 0, "higher priority steals");
	Check(!device.IsPlaying(vHandles[0]), "lowest priority source was stolen");
	Check(device.GetPriority(vHandles[0]) == 0, "stolen handle is stale");
	Check(device.IsPlaying(vHandles[1]), "other sources keep playing");

	// A stereo sample needs two voices, so it steals the two lowest.
	int lStereo = device.PlaySample(pStereo.get(), 100, 1, false);
	Check(lStereo >= 0, "stereo sample steals two voices");
	Check(!device.IsPlaying(vHandles[1]) && !device.IsPlaying(vHandles[2]), "two lowest stolen for stereo");
	Check(device.IsPlaying(vHandles[3]) && device.IsPlaying(lHigh), "higher sources survive");

	// Streams have priority 256 and samples cannot take them.
	std::unique_ptr<cOpenALStream> pStream = cOpenALStream::Open(kStreamFile, 4096, 3);
	int lStream = pStream ? device.PlayStream(pStream.get(), 1, false) : -1;
	Check(lStream >= 0, "stream steals from samples");
	Check(device.GetPriority(lStream) == 256, "stream priority is 256");
	device.SetPriority(lStream, 0);
	Check(device.GetPriority(lStream) == 256, "stream priority cannot be lowered");
	Check(device.PlaySample(pMono.get(), 255, 1, false) != lStream, "sample cannot steal a stream");
	Check(device.IsPlaying(lStream), "stream still plays");

	device.ReleaseStream(pStream.get());
	device.Close();
}

//-----------------------------------------------------------------------

static void TestHandlesAndPause()
{
	cOpenALDevice device;
	if(!device.Open(Params(2, false))) { Check(false, "device opens for handle test"); return; }
	std::unique_ptr<cOpenALSample> pSample = MakeSample(1, 2205);

	int lPaused = device.PlaySample(pSample.get(), 10, 1, true);
	Wait(device, 150);
	Check(device.IsPlaying(lPaused), "a sample started paused counts as playing");
	device.SetPaused(lPaused, false);
	Wait(device, 30);
	Check(device.IsPlaying(lPaused), "unpaused sample plays");
	Wait(device, 300);
	Check(!device.IsPlaying(lPaused), "0.1 s sample finishes");

	int lFirst = device.PlaySample(pSample.get(), 10, 1, false);
	device.Stop(lFirst);
	Check(!device.IsPlaying(lFirst), "stopped handle is not playing");
	int lSecond = device.PlaySample(pSample.get(), 20, 1, false);
	device.SetLoop(lSecond, true);
	Check(lSecond != lFirst, "rebinding a source gives a new handle");
	device.Stop(lFirst);
	device.SetPriority(lFirst, 200);
	Check(device.IsPlaying(lSecond) && device.GetPriority(lSecond) == 20, "stale handle cannot touch the new sound");

	device.Close();
	Check(!device.IsPlaying(lSecond), "closed device has no playing handles");
}

//-----------------------------------------------------------------------

static void TestStreaming(bool abThread)
{
	cOpenALDevice device;
	if(!device.Open(Params(4, abThread))) { Check(false, "device opens for stream test"); return; }

	std::unique_ptr<cOpenALStream> pStream = cOpenALStream::Open(kStreamFile, 4096, 3);
	if(!pStream) { Check(false, "stream opens"); return; }
	Check(std::fabs(pStream->GetTotalTime() - 0.5) < 0.01, "stream length is 0.5 s");
	Check(pStream->GetChannels() == 2, "stream is stereo");

	// 0.5 s at 22.05 kHz is about 11 blocks, so this refills repeatedly.
	int lHandle = device.PlayStream(pStream.get(), 1, false);
	Check(device.PlayStream(pStream.get(), 1, false) == cOpenALDevice::kInvalidHandle,
		  "a stream plays on one source at a time");
	Wait(device, 250);
	Check(device.IsPlaying(lHandle), "stream plays past its queued blocks");
	Wait(device, 600);
	Check(!device.IsPlaying(lHandle), "non-looping stream ends");
	Check(device.PlayStream(pStream.get(), 1, false) == cOpenALDevice::kInvalidHandle,
		  "a finished stream stays bound until stopped");
	// The sound handler deletes the finished channel, which stops it.
	device.Stop(lHandle);

	lHandle = device.PlayStream(pStream.get(), 1, false);
	device.SetLoop(lHandle, true);
	Wait(device, 1200);
	Check(device.IsPlaying(lHandle), "looping stream keeps playing past its length");
	double fElapsed = device.GetElapsedTime(lHandle);
	Check(fElapsed >= 0 && fElapsed <= 0.5, "looping stream time wraps");

	device.Stop(lHandle);
	Check(!device.IsPlaying(lHandle), "stopped stream is not playing");
	Check(device.PlayStream(pStream.get(), 1, false) >= 0, "stopped stream can play again");
	device.ReleaseStream(pStream.get());
	device.Close();
}

//-----------------------------------------------------------------------

static void TestLoaders(const fs::path &aDir)
{
	cOpenALDevice device;
	if(!device.Open(Params(4, false))) { Check(false, "device opens for loader test"); return; }

	std::unique_ptr<cOpenALSample> pOgg = cOpenALSample::Load(kStreamFile);
	Check(pOgg && pOgg->GetChannels() == 2 && std::fabs(pOgg->GetTotalTime() - 0.5) < 0.01,
		  "Ogg sample decodes completely");

	WriteWav(aDir / "pcm16.wav", 2, 16, false, 2205);
	std::unique_ptr<cOpenALSample> pWav = cOpenALSample::Load((aDir / "pcm16.wav").string());
	Check(pWav && pWav->GetChannels() == 2 && std::fabs(pWav->GetTotalTime() - 0.1) < 0.001,
		  "16-bit WAV loads");

	WriteWav(aDir / "float.wav", 1, 32, true, 2205);
	std::unique_ptr<cOpenALSample> pFloat = cOpenALSample::Load((aDir / "float.wav").string());
	Check(pFloat && pFloat->GetChannels() == 1 && std::fabs(pFloat->GetTotalTime() - 0.1) < 0.001,
		  "float WAV converts to 16-bit");

	WriteFile(aDir / "broken.wav", "RIFF\x04\x00\x00\x00WAVE");
	Check(cOpenALSample::Load((aDir / "broken.wav").string()) == nullptr, "truncated WAV is rejected");
	WriteFile(aDir / "broken.ogg", "OggS but not really");
	Check(cOpenALSample::Load((aDir / "broken.ogg").string()) == nullptr, "invalid Ogg is rejected");
	Check(cOpenALStream::Open((aDir / "broken.ogg").string(), 4096, 3) == nullptr, "invalid Ogg stream is rejected");
	Check(cOpenALSample::Load((aDir / "missing.ogg").string()) == nullptr, "missing file is rejected");

	device.Close();
}

//-----------------------------------------------------------------------

static void TestEnvironmentFiles(const fs::path &aDir)
{
	struct sCase { const char *mpName; const char *mpXml; bool mbValid; };
	const sCase vCases[] = {
		{ "empty.snt", "", false },
		{ "noroot.snt", "<Other/>", false },
		{ "nomain.snt", "<SoundEnvironment><Parameters Density=\"1\"/></SoundEnvironment>", false },
		{ "notype.snt", "<SoundEnvironment><Main Name=\"x\"/><Parameters Density=\"1\"/></SoundEnvironment>", false },
		{ "noname.snt", "<SoundEnvironment><Main Type=\"OpenAL\"/><Parameters Density=\"0.5\"/></SoundEnvironment>", true },
		{ "eax.snt", "<SoundEnvironment><Main Type=\"EAX\" Name=\"x\"/><Parameters/></SoundEnvironment>", false },
		{ "valid.snt", "<SoundEnvironment><Main Type=\"OpenAL\" Name=\"Hall\"/>"
			"<Parameters Density=\"1\" DecayTime=\"2.5\" DecayHFRatio=\"0.5\" DecayHFLimit=\"1\"/></SoundEnvironment>", true },
	};
	for(const sCase &c : vCases)
	{
		WriteFile(aDir / c.mpName, c.mpXml);
		cOpenALSoundEnvironment env;
		bool bLoaded = env.CreateFromFile((aDir / c.mpName).string());
		Check(bLoaded == c.mbValid, c.mpName);
	}

	cOpenALSoundEnvironment hall;
	hall.CreateFromFile((aDir / "valid.snt").string());
	Check(hall.GetName() == "Hall" && hall.mfDecayTime == 2.5f && hall.mbDecayHFLimit == 1, "parameters are read");

	// Fading from nothing interpolates every parameter, including DecayHFRatio.
	cOpenALSoundEnvironment zero, mid;
	mid.LerpFrom(zero, hall, 0.5f);
	Check(mid.mfDecayTime == 1.25f && mid.mfDecayHFRatio == 0.25f && mid.mfDensity == 0.5f, "fade interpolates");
}

//-----------------------------------------------------------------------

static void TestSettings()
{
	Check(cLowLevelSoundOpenAL::StreamBlockBytes(64) == 64 * 1024, "StreamBufferSize=64 means 64 KiB");
	Check(cLowLevelSoundOpenAL::StreamBlockBytes(16384) == 16384, "large values are bytes");
	Check(cLowLevelSoundOpenAL::StreamBlockBytes(0) == 64 * 1024, "zero uses the default");
	Check(cLowLevelSoundOpenAL::StreamBlockBytes(4) == 16 * 1024, "small blocks are raised");

	// The shipped settings name the default device "NULL".
	cOpenALDeviceParams params = Params(4, false);
	params.msDeviceName = "NULL";
	cOpenALDevice device;
	Check(device.Open(params), "device name NULL opens the default device");
	params.msDeviceName = "No Such Device";
	cOpenALDevice missing;
	Check(!missing.Open(params), "unknown device fails to open");
}

//-----------------------------------------------------------------------

static void TestHRTFSetting()
{
	Check(SoundHRTFFromString("on") == eSoundHRTF_On && SoundHRTFFromString("Off") == eSoundHRTF_Off
		  && SoundHRTFFromString("auto") == eSoundHRTF_Auto && SoundHRTFFromString("") == eSoundHRTF_Auto
		  && SoundHRTFFromString("1") == eSoundHRTF_On && SoundHRTFFromString("false") == eSoundHRTF_Off,
		  "HRTF setting values are read");
	for(int i = 0; i < eSoundHRTF_LastEnum; ++i)
		Check(SoundHRTFFromString(SoundHRTFToString((eSoundHRTF)i)) == i, "HRTF setting round-trips");
}

static void TestOutput()
{
	// HRTF requested at open.
	{
		cOpenALDeviceParams params = Params(4, false);
		params.mHRTF = eOpenALHRTF::On;
		cOpenALDevice device;
		Check(device.Open(params), "device opens with HRTF on");
		Check(device.GetHRTFStatus() != -1, "ALC_SOFT_HRTF is available");
		Check(device.IsHRTFEnabled(), "HRTF on at open is enabled");
		Check(device.GetHRTFStatus() == ALC_HRTF_ENABLED_SOFT, "HRTF status reads enabled");
		Check(!device.GetHRTFName().empty(), "HRTF data set is named");
	}

	// HRTF changed at runtime, with a stream and a looping sample playing.
	cOpenALDevice device;
	if(!device.Open(Params(8, true))) { Check(false, "device opens for output test"); return; }
	Check(device.CanReopen(), "ALC_SOFT_reopen_device is available");
	Check(!device.IsHRTFEnabled(), "auto HRTF stays off on the null backend");

	auto pStream = cOpenALStream::Open(kStreamFile, 4096, 3);
	auto pLoop = MakeSample(1, 2205);
	auto pStereo = MakeSample(2, 2205);
	if(!pStream || !pLoop || !pStereo) { Check(false, "output test data loads"); return; }
	pStream->SetLoop(true);
	int lStream = device.PlayStream(pStream.get(), 1, false);
	int lLoop = device.PlaySample(pLoop.get(), 10, 1, true);
	device.SetLoop(lLoop, true);
	device.SetPaused(lLoop, false);
	int lStereo = device.PlaySample(pStereo.get(), 10, 1, false);
	Check(lStream >= 0 && lLoop >= 0 && lStereo >= 0, "output test sounds play");

	Check(device.SetHRTF(eOpenALHRTF::On), "HRTF can be turned on at runtime");
	Check(device.IsHRTFEnabled() && device.GetHRTFStatus() == ALC_HRTF_ENABLED_SOFT, "runtime HRTF on is enabled");
	Wait(device, 150);
	Check(device.IsPlaying(lStream) && device.IsPlaying(lLoop), "sounds survive an HRTF reset");
	Check(device.SetHRTF(eOpenALHRTF::Off), "HRTF can be turned off at runtime");
	Check(!device.IsHRTFEnabled() && device.GetHRTFStatus() == ALC_HRTF_DISABLED_SOFT, "runtime HRTF off is disabled");
	Check(device.SetHRTF(eOpenALHRTF::Auto) && !device.IsHRTFEnabled(), "HRTF back to auto");

	// Reopen keeps the context, sources and buffers, so handles stay valid.
	std::string sName = device.GetDeviceName();
	Check(device.Reopen(""), "device reopens on the default device");
	Check(device.IsConnected(), "reopened device is connected");
	Check(device.GetDeviceName() == sName, "reopened device name is read again");
	Check(device.IsPlaying(lStream) && device.IsPlaying(lLoop), "sounds survive a reopen");
	Check(device.GetPriority(lLoop) == 10, "handles stay valid across a reopen");
	Wait(device, 700);
	Check(device.IsPlaying(lStream), "looping stream keeps refilling after a reopen");
	Check(device.IsPlaying(lLoop), "looping sample keeps playing after a reopen");
	Check(!device.IsPlaying(lStereo), "one-shot stereo sample still finishes");
	Check(!device.Reopen("No Such Device"), "reopening an unknown device fails");
	Check(device.IsPlaying(lStream) && device.GetDeviceName() == sName, "a failed reopen keeps the device");

	device.Close();
	Check(!device.Reopen(""), "a closed device cannot reopen");
}

//-----------------------------------------------------------------------

int hplMain(const hpl::tString&)
{
	fs::path dir = fs::temp_directory_path() / ("openhpl_audio_" + std::to_string(
		std::chrono::steady_clock::now().time_since_epoch().count()));
	fs::create_directories(dir);

	TestSettings();
	TestHRTFSetting();
	TestOutput();
	TestLifecycle();
	TestVoiceStealing();
	TestHandlesAndPause();
	TestStreaming(false);
	TestStreaming(true);
	TestLoaders(dir);
	TestEnvironmentFiles(dir);

	std::error_code ec;
	fs::remove_all(dir, ec);

	if(gFailures)
	{
		std::cerr << gFailures << " audio check(s) failed\n";
		return 1;
	}
	std::cout << "audio regressions passed\n";
	return 0;
}
