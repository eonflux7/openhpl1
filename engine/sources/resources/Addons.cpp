/*
 * Copyright (C) 2026 - OpenHPL1 contributors
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
#include "resources/Addons.h"

#include "resources/ConfigFile.h"
#include "resources/FileSearcher.h"
#include "resources/Resources.h"
#include "system/LowLevelSystem.h"
#include "system/String.h"

#include "miniz.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace hpl
{

	namespace fs = std::filesystem;

	//////////////////////////////////////////////////////////////////////////
	// HELPERS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	static fs::path ToPath(const tString &asPath)
	{
		return fs::u8path(asPath);
	}
	static tString FromPath(const fs::path &aPath)
	{
		return aPath.u8string();
	}

	static bool IsHidden(const fs::path &aPath)
	{
		tString sName = FromPath(aPath.filename());
		return sName.empty() || sName[0] == '.';
	}

	//-----------------------------------------------------------------------

	// A path inside a .zip file that stays inside the folder it is unpacked to
	static bool IsSafeZipPath(const tString &asPath)
	{
		if(asPath.empty() || asPath[0] == '/' || asPath[0] == '\\' || asPath.find(':') != tString::npos) return false;
		tStringVec vParts;
		tString sSeparators = "/\\";
		cString::GetStringVec(asPath, vParts, &sSeparators);
		for(const tString &sPart : vParts)
			if(sPart == "..") return false;
		return true;
	}

	//-----------------------------------------------------------------------

	// Unpacks a .zip file into a folder of asUnpackDir named after the file,
	// its size and its time, unless that is already there. Returns the folder,
	// "" on failure.
	static tString UnpackZip(const fs::path &aZip, const tString &asUnpackDir)
	{
		std::error_code err;
		uint64_t lSize = (uint64_t)fs::file_size(aZip, err);
		uint64_t lTime = (uint64_t)fs::last_write_time(aZip, err).time_since_epoch().count();

		uint64_t lHash = 14695981039346656037ull;
		auto Add = [&](uint64_t alValue)
		{ for(int i=0; i<8; ++i) { lHash ^= (alValue >> (i*8)) & 0xff; lHash *= 1099511628211ull; } };
		Add(lSize);
		Add(lTime);
		for(unsigned char c : FromPath(fs::absolute(aZip, err))) Add(c);

		char sHash[17];
		snprintf(sHash, sizeof(sHash), "%016llx", (unsigned long long)lHash);
		const tString sStem = FromPath(aZip.stem());
		const fs::path unpackDir = ToPath(asUnpackDir);
		const fs::path dest = unpackDir / ToPath(sStem + "-" + sHash);
		if(fs::exists(dest / ".unpacked", err)) return FromPath(dest);

		//Older unpacked versions of the same file
		if(fs::is_directory(unpackDir, err))
		{
			for(fs::directory_iterator it(unpackDir, err), end; it != end; it.increment(err))
			{
				tString sName = FromPath(it->path().filename());
				if(sName.size() == sStem.size() + 17 && sName.compare(0, sStem.size() + 1, sStem + "-") == 0)
					fs::remove_all(it->path(), err);
			}
		}

		const fs::path temp = unpackDir / ToPath(sStem + "-" + sHash + ".part");
		fs::remove_all(temp, err);
		fs::create_directories(temp, err);

		mz_zip_archive zip;
		memset(&zip, 0, sizeof(zip));
		if(!mz_zip_reader_init_file(&zip, FromPath(aZip).c_str(), 0))
		{
			Error("Couldn't open addon '%s'\n", FromPath(aZip).c_str());
			return "";
		}

		bool bOk = true;
		const mz_uint lCount = mz_zip_reader_get_num_files(&zip);
		for(mz_uint i = 0; i < lCount && bOk; ++i)
		{
			mz_zip_archive_file_stat stat;
			if(!mz_zip_reader_file_stat(&zip, i, &stat))
			{
				bOk = false;
				break;
			}
			if(mz_zip_reader_is_file_a_directory(&zip, i)) continue;

			tString sName = stat.m_filename;
			if(IsSafeZipPath(sName) == false)
			{
				Warning("Addon '%s': leaving out '%s'\n", FromPath(aZip).c_str(), sName.c_str());
				continue;
			}
			fs::path out = temp / ToPath(cString::ReplaceCharTo(sName, "\\", "/"));
			fs::create_directories(out.parent_path(), err);
			if(!mz_zip_reader_extract_to_file(&zip, i, FromPath(out).c_str(), 0))
			{
				Error("Couldn't unpack '%s' from addon '%s'\n", sName.c_str(), FromPath(aZip).c_str());
				bOk = false;
			}
		}
		mz_zip_reader_end(&zip);

		if(bOk)
		{
			std::ofstream(temp / ".unpacked").put('\n');
			fs::rename(temp, dest, err);
			if(err) bOk = false;
		}
		if(bOk == false)
		{
			fs::remove_all(temp, err);
			return "";
		}
		Log(" Unpacked addon '%s' to '%s'\n", FromPath(aZip).c_str(), FromPath(dest).c_str());
		return FromPath(dest);
	}

	//-----------------------------------------------------------------------

	// A .zip file with one folder in it: the addon is that folder.
	static fs::path ContentFolder(const fs::path &aPath)
	{
		std::error_code err;
		if(fs::exists(aPath / "addon.cfg", err)) return aPath;
		fs::path single;
		int lCount = 0;
		for(fs::directory_iterator it(aPath, err), end; it != end; it.increment(err))
		{
			if(IsHidden(it->path())) continue;
			++lCount;
			if(it->is_directory(err)) single = it->path();
		}
		return lCount == 1 && single.empty() == false ? single : aPath;
	}

	//-----------------------------------------------------------------------

	static bool ReadAddon(cAddon &aAddon)
	{
		aAddon.msTitle = aAddon.msName;
		aAddon.mlPriority = 0;

		std::error_code err;
		fs::path cfg = ToPath(aAddon.msPath) / "addon.cfg";
		if(fs::exists(cfg, err) == false) return true;

		cConfigFile config(cString::To16Char(FromPath(cfg)));
		if(config.Load() == false)
		{
			Warning("Couldn't read '%s'\n", FromPath(cfg).c_str());
			return true;
		}
		aAddon.msTitle = config.GetString("Addon", "Name", aAddon.msName);
		aAddon.mlPriority = config.GetInt("Addon", "Priority", 0);
		tString sSeparators = " ,;";
		cString::GetStringVec(cString::ToLowerCase(config.GetString("Addon", "Games", "")), aAddon.mvGames, &sSeparators);
		return config.GetBool("Addon", "Enabled", true);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	tAddonVec cAddons::Find(const tStringVec &avRoots, const tString &asGame,
		const tString &asUnpackDir, const tString &asOnly)
	{
		tStringVec vOnly;
		tString sSeparators = ",";
		cString::GetStringVec(cString::ToLowerCase(asOnly), vOnly, &sSeparators);

		tAddonVec vAddons;
		std::error_code err;
		for(const tString &sRoot : avRoots)
		{
			fs::path root = ToPath(sRoot);
			if(fs::is_directory(root, err) == false) continue;
			root = fs::absolute(root, err);

			std::vector<fs::path> vCandidates;
			if(fs::exists(root / "addon.cfg", err))
				vCandidates.push_back(root);
			else
			{
				for(fs::directory_iterator it(root, err), end; it != end; it.increment(err))
				{
					if(IsHidden(it->path())) continue;
					if(it->is_directory(err) ||
						(it->is_regular_file(err) && cString::ToLowerCase(FromPath(it->path().extension())) == ".zip"))
						vCandidates.push_back(it->path());
				}
				std::sort(vCandidates.begin(), vCandidates.end());
			}

			for(const fs::path &candidate : vCandidates)
			{
				cAddon addon;
				addon.msSource = FromPath(candidate);
				const bool bZip = fs::is_directory(candidate, err) == false;
				addon.msName = FromPath(bZip ? candidate.stem() : candidate.filename());

				if(vOnly.empty() == false && std::find(vOnly.begin(), vOnly.end(), cString::ToLowerCase(addon.msName)) == vOnly.end())
					continue;

				fs::path folder = candidate;
				if(bZip)
				{
					tString sUnpacked = UnpackZip(candidate, asUnpackDir);
					if(sUnpacked == "") continue;
					folder = ContentFolder(ToPath(sUnpacked));
				}
				addon.msPath = FromPath(folder);

				if(ReadAddon(addon) == false)
				{
					Log(" Addon '%s' is disabled\n", addon.msName.c_str());
					continue;
				}
				if(addon.mvGames.empty() == false &&
					std::find(addon.mvGames.begin(), addon.mvGames.end(), cString::ToLowerCase(asGame)) == addon.mvGames.end())
				{
					continue;
				}
				vAddons.push_back(addon);
			}
		}

		//Higher priority first, then by name
		std::stable_sort(vAddons.begin(), vAddons.end(), [](const cAddon &a, const cAddon &b)
			{
			if(a.mlPriority != b.mlPriority) return a.mlPriority > b.mlPriority;
			return cString::ToLowerCase(a.msName) < cString::ToLowerCase(b.msName); });
		return vAddons;
	}

	//-----------------------------------------------------------------------

	void cAddons::AddToResources(cResources *apResources, const tAddonVec &avAddons)
	{
		for(const cAddon &addon : avAddons)
		{
			Log(" Addon '%s' (%s) from '%s'\n", addon.msTitle.c_str(), addon.msName.c_str(), addon.msSource.c_str());

			apResources->GetFileSearcher()->AddDirectoryTree(addon.msPath, "*.*");
		}
	}

	//-----------------------------------------------------------------------

	void cAddons::LogReplacedFiles(cResources *apResources, const tAddonVec &avAddons)
	{
		cFileSearcher *pSearcher = apResources->GetFileSearcher();
		for(const cAddon &addon : avAddons)
		{
			int lFiles = 0, lReplaced = 0;
			std::error_code err;
			for(fs::recursive_directory_iterator it(ToPath(addon.msPath), err), end; it != end; it.increment(err))
			{
				if(IsHidden(it->path()))
				{
					if(it->is_directory(err)) it.disable_recursion_pending();
					continue;
				}
				if(it->is_regular_file(err) == false) continue;
				tString sName = FromPath(it->path().filename());
				if(cString::ToLowerCase(sName) == "addon.cfg") continue;
				++lFiles;

				tString sPath = pSearcher->GetExactFilePath(sName);
				if(fs::equivalent(ToPath(sPath), it->path(), err) == false) continue;

				tString sReplaced = pSearcher->GetHiddenFilePath(sName);
				tString sExt = cString::ToLowerCase(cString::GetFileExt(sName));
				if(sReplaced == "" && (sExt == "glb" || sExt == "gltf"))
				{
					//A converted model used in place of the game's Collada file
					tString sDae = cString::SetFileExt(sName, "dae");
					tString sDaePath = pSearcher->GetExactFilePath(sDae);
					if(sDaePath != "" && fs::equivalent(ToPath(pSearcher->GetFilePath(sDae)), it->path(), err))
						sReplaced = sDaePath;
				}
				if(sReplaced != "")
				{
					++lReplaced;
					Log("  '%s' replaces '%s'\n", sName.c_str(), sReplaced.c_str());
				}
			}
			Log(" Addon '%s': %d files, %d replace game files\n", addon.msName.c_str(), lFiles, lReplaced);
		}
	}

	//-----------------------------------------------------------------------
} // namespace hpl
