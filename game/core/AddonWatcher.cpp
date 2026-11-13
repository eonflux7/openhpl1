/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/AddonWatcher.h"

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

cAddonWatcher::cAddonWatcher(const tStringVec &avFolders)
{
	mvFolders = avFolders;
	mbActive = false;
	mbStop = false;
	mbAccept = false;
	mbChanged = false;
	mThread = std::thread(&cAddonWatcher::Run, this);
}

cAddonWatcher::~cAddonWatcher()
{
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mbStop = true;
	}
	mCondition.notify_all();
	mThread.join();
}

void cAddonWatcher::SetActive(bool abActive)
{
	{
		std::lock_guard<std::mutex> lock(mMutex);
		if(mbActive == abActive) return;
		mbActive = abActive;
	}
	mCondition.notify_all();
}

bool cAddonWatcher::HasChanged()
{
	return mbChanged.exchange(false);
}

void cAddonWatcher::Accept()
{
	{
		std::lock_guard<std::mutex> lock(mMutex);
		mbAccept = true;
		mbChanged = false;
	}
	mCondition.notify_all();
}

// Path, size and time of every file, hashed (FNV-1a)
uint64_t cAddonWatcher::Signature()
{
	uint64_t lHash = 14695981039346656037ull;
	auto Add = [&lHash](const void *apData, size_t alSize)
	{
		const unsigned char *pData = static_cast<const unsigned char *>(apData);
		for(size_t i = 0; i < alSize; ++i) lHash = (lHash ^ pData[i]) * 1099511628211ull;
	};

	for(const tString &sFolder : mvFolders)
	{
		std::error_code err;
		for(fs::recursive_directory_iterator it(fs::u8path(sFolder), err), end; it != end; it.increment(err))
		{
			if(it->is_regular_file(err) == false) continue;
			tString sPath = it->path().u8string();
			uint64_t lSize = it->file_size(err);
			int64_t lTime = it->last_write_time(err).time_since_epoch().count();
			Add(sPath.data(), sPath.size());
			Add(&lSize, sizeof(lSize));
			Add(&lTime, sizeof(lTime));
		}
	}
	return lHash;
}

void cAddonWatcher::Run()
{
	// The files as last reported, and as at the look before
	uint64_t lReported = 0, lLast = 0;
	bool bWasActive = false;

	std::unique_lock<std::mutex> lock(mMutex);
	while(mbStop == false)
	{
		if(mbActive == false)
		{
			bWasActive = false;
			mCondition.wait(lock);
			continue;
		}
		// Accepted before this look started: what it sees is the new start
		bool bAccept = mbAccept;
		mbAccept = false;
		lock.unlock();
		uint64_t lNow = Signature();
		lock.lock();

		// Changes made while not watching are not reported
		if(bWasActive == false || bAccept)
			lReported = lLast = lNow;
		else
		{
			// Not while an accept waits for the next look
			if(lNow != lReported && lNow == lLast && mbAccept == false)
			{
				lReported = lNow;
				mbChanged = true;
			}
			lLast = lNow;
		}
		bWasActive = true;

		mCondition.wait_for(lock, std::chrono::milliseconds(500));
	}
}
