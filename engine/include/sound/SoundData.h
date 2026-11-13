// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SOUND_DATA_H
#define HPL_SOUND_DATA_H

#include "system/SystemTypes.h"
#include "resources/ResourceBase.h"

namespace hpl {

	class cSoundManager;
	class iSoundChannel;

	class iSoundData : public iResourceBase
	{
	public:
		iSoundData(tString asName, bool abStream) : iResourceBase(asName, 0),
		mpSoundManger(nullptr), mbStream(abStream){}

		~iSoundData() override{}

		virtual bool CreateFromFile(const tString &asFile)=0;

		virtual iSoundChannel* CreateChannel(int alPriority)=0;


		virtual bool IsStereo()=0;

		bool IsStream(){ return mbStream;}
		void SetLoopStream(bool abX){mbLoopStream = abX;}
		bool GetLoopStream(){ return mbLoopStream;}

		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}

		void SetSoundManager(cSoundManager* apSoundManager){
			mpSoundManger = apSoundManager;
		}

	protected:
		bool mbStream;
		bool mbLoopStream;
		cSoundManager* mpSoundManger;
	};
};
#endif // HPL_SOUND_DATA_H
