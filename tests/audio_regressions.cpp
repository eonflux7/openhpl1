// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
//
// Runs on OpenAL Soft's null backend (ALSOFT_DRIVERS=null), which mixes in
// real time without an audio device.
#include "impl/LowLevelSoundOpenAL.h"
#include "impl/OpenALDevice.h"
#include "impl/OpenALSoundChannel.h"
#include "impl/OpenALSoundEnvironment.h"
#include "sound/MusicHandler.h"
#include "sound/SoundHandler.h"

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
		device.SetReverb(0, cOpenALReverb());
		device.SetReverb(1, cOpenALReverb());
		device.SetReverbWeight(0, 0.7f);
		device.SetReverbWeight(1, 0.7f);
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

	// A sample that ended by itself still holds its voice until its channel
	// is stopped. A new sound takes that voice before stealing a playing one.
	if(!device.Open(Params(2, false))) { Check(false, "device reopens for reclaim test"); return; }
	std::unique_ptr<cOpenALSample> pShort = MakeSample(1, 2205);
	int lLow = device.PlaySample(pMono.get(), 10, 1, false);
	device.SetLoop(lLow, true);
	int lEnded = device.PlaySample(pShort.get(), 50, 1, false);
	Wait(device, 300);
	Check(device.IsPlaying(lLow) && !device.IsPlaying(lEnded), "short sample ended, loop plays");
	Check(device.PlaySample(pMono.get(), 100, 1, false) >= 0, "new sample gets the ended sample's voice");
	Check(device.IsPlaying(lLow), "playing low-priority loop is not stolen");
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

	// Pausing a sample that already ended (the menu pauses every sound)
	// must not play it again on unpause.
	device.SetPaused(lPaused, true);
	device.SetPaused(lPaused, false);
	Wait(device, 30);
	Check(!device.IsPlaying(lPaused), "unpausing an ended sample does not replay it");

	// Channels start paused and play on unpause. A source whose last sound
	// ended by itself is stopped, not initial, and must still start.
	for(int i = 0; i < 2; ++i)
	{
		int lReused = device.PlaySample(pSample.get(), 10, 1, true);
		device.SetPaused(lReused, false);
		Wait(device, 30);
		Check(device.IsPlaying(lReused), "a sample started paused on a reused source plays");
		Wait(device, 300);
		Check(!device.IsPlaying(lReused), "the reused source's sample finishes");
	}

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

static bool FilterIs(cOpenALDevice &aDevice, int alHandle, bool abOn, float afGainHF)
{
	bool bDirect = !abOn, bSend = !abOn;
	float fGainHF = -1;
	if(!aDevice.GetFilterState(alHandle, bDirect, bSend, fGainHF)) return false;
	return bDirect == abOn && bSend == abOn && std::fabs(fGainHF - afGainHF) < 1e-4f;
}

// Occlusion: the handler's block fade drives the channel's low-pass, which
// covers the direct path and the reverb send and is detached when clear.
static void TestOcclusionFilter()
{
	cOpenALDevice device;
	if(!device.Open(Params(2, false))) { Check(false, "device opens for occlusion test"); return; }
	if(!device.IsEFXActive()) { std::printf("SKIP: occlusion filter (no EFX)\n"); device.Close(); return; }
	std::unique_ptr<cOpenALSample> pSample = MakeSample(1);

	int lHandle = device.PlaySample(pSample.get(), 10, 1, true);
	auto *pChannel = new cOpenALSoundChannel(nullptr, &device, lHandle, nullptr);
	cSoundEntry entry;
	entry.mpSound = pChannel;
	entry.mfBlockMul = 1;

	cSoundHandler::UpdateBlockFilter(&entry, 0.25f);
	Check(FilterIs(device, lHandle, false, 1), "a clear sound is not filtered");

	entry.mfBlockMul = 0;
	cSoundHandler::UpdateBlockFilter(&entry, 0.25f);
	Check(FilterIs(device, lHandle, true, 0.25f), "a blocked sound gets the blocked GainHF on direct and send");

	entry.mfBlockMul = 0.5f;
	cSoundHandler::UpdateBlockFilter(&entry, 0.25f);
	Check(FilterIs(device, lHandle, true, 0.625f), "GainHF follows the block fade");

	entry.mfBlockMul = 1;
	cSoundHandler::UpdateBlockFilter(&entry, 0.25f);
	Check(FilterIs(device, lHandle, false, 0.625f), "the filter is detached once the sound is clear");

	entry.mfBlockMul = 0;
	cSoundHandler::UpdateBlockFilter(&entry, 1.0f);
	Check(FilterIs(device, lHandle, false, 0.625f), "GainHF 1 turns occlusion filtering off");
	Check(alGetError() == AL_NO_ERROR, "filter changes raise no OpenAL error");

	cSoundHandler::UpdateBlockFilter(&entry, 0.1f);
	delete pChannel;
	int lNext = device.PlaySample(pSample.get(), 10, 1, true);
	Check(FilterIs(device, lNext, false, 1), "a reused source starts unfiltered");

	device.Close();
}

//-----------------------------------------------------------------------

static void TestBusRules()
{
	const char *kVoice = "/games/Penumbra Overture/sounds/voices/eng/intro_01_01.ogg";
	Check(cSoundHandler::BusForSound(kVoice, false) == eSoundBus_Voice, "sounds/voices is the voice bus");
	Check(cSoundHandler::BusForSound("sounds\\Voices\\radio_02.ogg", true) == eSoundBus_Voice,
		  "voice paths match with backslashes and any case, even looping");
	Check(cSoundHandler::BusForSound("music/penumbra_music_E1_I.ogg", false) == eSoundBus_Music, "music/ is the music bus");
	Check(cSoundHandler::BusForSound("sounds/iron_mine/machine_loop.ogg", true) == eSoundBus_Ambience,
		  "other looping sounds are ambience");
	Check(cSoundHandler::BusForSound("sounds/break/glass.ogg", false) == eSoundBus_Effects, "one-shots are effects");
	Check(cSoundHandler::BusForSound("", false) == eSoundBus_Effects, "an unknown path is an effect");
	Check(cSoundHandler::BusForSound("sounds/musicbox.ogg", false) == eSoundBus_Effects,
		  "only a music folder counts, not a file name");
}

// The handler multiplies each entry's volume by its bus gain at every update.
static void TestBusRouting()
{
	cLowLevelSoundOpenAL lowLevel; // uninitialized: only its listener matrix is used
	cOpenALDevice device;
	if(!device.Open(Params(4, false))) { Check(false, "device opens for bus test"); return; }
	std::unique_ptr<cOpenALSample> pSample = MakeSample(1);
	{
		cSoundHandler handler(&lowLevel, nullptr); // deletes its channels before the device closes
		const eSoundBus vBuses[] = { eSoundBus_Effects, eSoundBus_Voice, eSoundBus_Ambience };
		iSoundChannel *vChannels[3];
		for(int i = 0; i < 3; ++i)
		{
			int lHandle = device.PlaySample(pSample.get(), 10, 1, true);
			vChannels[i] = new cOpenALSoundChannel(nullptr, &device, lHandle, nullptr);
			vChannels[i]->SetPositionRelative(true);
			cSoundEntry entry;
			entry.mpSound = vChannels[i];
			entry.mfNormalVolume = 0.8f;
			entry.mfNormalSpeed = 1;
			entry.mfBlockMul = entry.mfBlockFadeDest = 1;
			entry.mfBlockFadeSpeed = 1;
			entry.mbFirstTime = true;
			entry.mEffectType = eSoundDest_Gui;
			entry.mBus = vBuses[i];
			handler.GetGuiEntryList()->push_back(entry);
		}

		handler.Update(1.0f / 60);
		bool bUnity = true;
		for(iSoundChannel *pChannel : vChannels) bUnity = bUnity && std::fabs(pChannel->GetVolume() - 0.8f) < 1e-5f;
		Check(bUnity, "buses at 1 leave volumes unchanged");

		handler.SetBusVolume(eSoundBus_Voice, 0.5f);
		handler.SetBusVolume(eSoundBus_Ambience, 0);
		handler.Update(1.0f / 60);
		Check(std::fabs(vChannels[0]->GetVolume() - 0.8f) < 1e-5f, "other buses leave effects alone");
		Check(std::fabs(vChannels[1]->GetVolume() - 0.4f) < 1e-5f, "the voice bus scales voice entries");
		Check(vChannels[2]->GetVolume() == 0, "a muted bus silences its entries");
		Check(handler.GetGuiEntryList()->size() == 3, "a muted entry keeps playing");

		handler.SetBusVolume(eSoundBus_Voice, 3);
		Check(handler.GetBusVolume(eSoundBus_Voice) == 1, "bus gains are clamped to 1");
		handler.SetBusVolume((eSoundBus)99, 0.5f);
		Check(handler.GetBusVolume((eSoundBus)99) == 1, "an unknown bus is ignored");

		cMusicHandler music(&lowLevel, nullptr);
		music.SetBusVolume(-1);
		Check(music.GetBusVolume() == 0, "the music bus is clamped to 0");
	}
	device.Close();
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

}

//-----------------------------------------------------------------------

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// Environments sit in their own reverb slots; fades move only slot gains,
// equal-power, and reuse environments already loaded.
static void TestReverbCrossfade(const fs::path &aDir)
{
	cLowLevelSoundOpenAL lowLevel;
	lowLevel.Init(false, false, true, 8, 10, false, false, 0, 0, 64, 4, false, "");
	cOpenALDevice *pDevice = lowLevel.GetDevice();
	if(!lowLevel.IsEnvAudioAvailable()) { Check(false, "EFX for the reverb test"); return; }
	Check(pDevice->GetReverbSlotCount() == 2, "two reverb slots");
	auto Gain = [&](int alSlot) { return pDevice->GetReverbSlotGain(alSlot); };
	Check(Gain(0) == 0 && Gain(1) == 0, "reverb is silent before an environment is set");

	iSoundEnvironment *pHall = lowLevel.LoadSoundEnvironment((aDir / "valid.snt").string());
	iSoundEnvironment *pRoom = lowLevel.LoadSoundEnvironment((aDir / "noname.snt").string());
	if(pHall == nullptr || pRoom == nullptr) { Check(false, "reverb test environments load"); return; }

	// From nothing: only the destination's slot rises, along sin.
	lowLevel.FadeSoundEnvironment(nullptr, pHall, 0.5f);
	int lHall = lowLevel.GetSlotEnvironment(0) == pHall ? 0 : 1;
	Check(lowLevel.GetSlotEnvironment(lHall) == pHall, "the faded-in environment gets a slot");
	Check(Near(Gain(lHall), std::sin(0.25f * 3.14159265f)) && Gain(1 - lHall) == 0, "fade from nothing is a gain ramp");

	lowLevel.SetSoundEnvironment(pHall);
	Check(Near(Gain(lHall), 1) && Gain(1 - lHall) == 0, "an environment plays at full gain");

	// Hall to room: the hall stays in its slot, the room takes the other.
	bool bPower = true, bMonotonic = true;
	float fLastHall = 2;
	for(int i = 0; i <= 8; ++i)
	{
		float fT = i / 8.0f;
		lowLevel.FadeSoundEnvironment(pHall, pRoom, fT);
		float fH = Gain(lHall), fR = Gain(1 - lHall);
		bPower = bPower && Near(fH * fH + fR * fR, 1);
		bMonotonic = bMonotonic && fH <= fLastHall;
		fLastHall = fH;
	}
	Check(lowLevel.GetSlotEnvironment(lHall) == pHall && lowLevel.GetSlotEnvironment(1 - lHall) == pRoom,
		  "a crossfade keeps the source's slot and loads the destination into the other");
	Check(bPower, "crossfade gains are equal-power");
	Check(bMonotonic && Near(Gain(lHall), 0) && Near(Gain(1 - lHall), 1), "crossfade ends on the destination");

	// And back again: both are loaded, so nothing moves between slots.
	lowLevel.FadeSoundEnvironment(pRoom, pHall, 0.25f);
	Check(lowLevel.GetSlotEnvironment(lHall) == pHall && lowLevel.GetSlotEnvironment(1 - lHall) == pRoom,
		  "fading back reuses both slots");
	Check(Near(Gain(1 - lHall), std::cos(0.125f * 3.14159265f)), "fading back starts from the room");

	// To nothing: the source fades out.
	lowLevel.SetSoundEnvironment(pRoom);
	lowLevel.FadeSoundEnvironment(pRoom, nullptr, 0.5f);
	Check(Near(Gain(1 - lHall), std::cos(0.25f * 3.14159265f)) && Gain(lHall) == 0, "fade to nothing");

	// The environment volume scales every slot.
	lowLevel.SetSoundEnvironment(pRoom);
	lowLevel.SetEnvVolume(0.5f);
	Check(Near(Gain(1 - lHall), 0.5f) && Gain(lHall) == 0, "env volume scales the slot gain");
	lowLevel.FadeSoundEnvironment(pRoom, pHall, 0.5f);
	Check(Near(Gain(lHall), 0.5f * std::sin(0.25f * 3.14159265f)), "env volume scales crossfades");

	lowLevel.SetSoundEnvironment(nullptr);
	Check(Gain(0) == 0 && Gain(1) == 0, "no environment silences both slots");

	// A world sound sends to both slots and plays through a crossfade.
	std::unique_ptr<cOpenALSample> pSample = MakeSample(1);
	int lHandle = pDevice->PlaySample(pSample.get(), 10, 1, false);
	pDevice->SetSendToReverb(lHandle, true);
	pDevice->SetFiltering(lHandle, true, 0x3);
	lowLevel.FadeSoundEnvironment(pHall, pRoom, 0.3f);
	Check(alGetError() == AL_NO_ERROR, "two sends and two slots raise no AL error");
	pDevice->ReleaseSample(pSample.get());
}

// Music fades: equal-power from and to silence, linear otherwise, no jumps.
static void TestMusicFades()
{
	cMusicEntry in, out;
	in.mfCurveRef = 1;
	out.mfVolume = 1;
	cMusicHandler::BeginFadeOut(&out);
	Check(out.mfCurveRef == 1 && cMusicHandler::FadeGain(&out) == 1, "a fade out starts at the current gain");

	bool bPower = true;
	for(int i = 0; i <= 10; ++i)
	{
		in.mfVolume = i / 10.0f;
		out.mfVolume = 1 - i / 10.0f;
		float a = cMusicHandler::FadeGain(&in), b = cMusicHandler::FadeGain(&out);
		bPower = bPower && Near(a * a + b * b, 1);
	}
	Check(bPower, "a symmetric music crossfade keeps constant power");

	// A new target halfway through a fade in carries on from the gain heard.
	cMusicEntry song;
	song.mfCurveRef = 0.8f;
	song.mfVolume = 0.4f;
	float fHeard = cMusicHandler::FadeGain(&song);
	cMusicHandler::EndCurve(&song);
	Check(Near(cMusicHandler::FadeGain(&song), fHeard) && song.mfCurveRef == 0, "leaving the curve doesn't jump");
	Check(Near(fHeard, 0.8f * std::sin(0.25f * 3.14159265f)), "fade in follows ref*sin");

	// Fading out halfway through a fade in.
	song.mfCurveRef = 0.8f;
	song.mfVolume = 0.2f;
	fHeard = cMusicHandler::FadeGain(&song);
	cMusicHandler::BeginFadeOut(&song);
	Check(Near(cMusicHandler::FadeGain(&song), fHeard), "a fade out mid fade-in doesn't jump");

	cMusicEntry steady;
	steady.mfVolume = 0.6f;
	Check(cMusicHandler::FadeGain(&steady) == 0.6f, "without a curve the gain is the volume");
	steady.mfCurveRef = 0.6f;
	Check(Near(cMusicHandler::FadeGain(&steady), 0.6f), "a finished fade in plays at its volume");
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
	TestOcclusionFilter();
	TestBusRules();
	TestBusRouting();
	TestStreaming(false);
	TestStreaming(true);
	TestLoaders(dir);
	TestEnvironmentFiles(dir);
	TestReverbCrossfade(dir);
	TestMusicFades();

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
