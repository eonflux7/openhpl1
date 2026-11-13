/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef GAME_ADDON_WATCHER_H
#define GAME_ADDON_WATCHER_H

#include "core/StdAfx.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace hpl;

// Watches the files in addon folders (not .zip addons) from a thread of
// its own while it is active, so that the map can be reloaded when an
// edited model or map is saved into one (developer mode, docs/modding.md).
class cAddonWatcher
{
public:
	cAddonWatcher(const tStringVec &avFolders);
	~cAddonWatcher();

	/**
	 * Watches while active; the files are looked at twice a second.
	 */
	void SetActive(bool abActive);

	/**
	 * True once after files were added, changed or removed, when they have
	 * stayed the same for one more look (so a file being written is done).
	 */
	bool HasChanged();

	/**
	 * The files as they are now are not a change: for files the game wrote
	 * itself (the level editor), after which it reloads the map anyway.
	 */
	void Accept();

private:
	void Run();
	uint64_t Signature();

	tStringVec mvFolders;
	std::thread mThread;
	std::mutex mMutex;
	std::condition_variable mCondition;
	bool mbActive;
	bool mbStop;
	bool mbAccept;
	std::atomic<bool> mbChanged;
};

#endif // GAME_ADDON_WATCHER_H
