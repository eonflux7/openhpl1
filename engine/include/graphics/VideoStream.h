// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_VIDEO_STREAM_H
#define HPL_VIDEO_STREAM_H

#include "graphics/GraphicsTypes.h"
#include "resources/ResourceBase.h"

namespace hpl {

	class iTexture;

	class iVideoStream;

	class iVideoStreamLoader
	{
	public:
		virtual ~iVideoStreamLoader(){}

		virtual iVideoStream* Create(const tString& asName)=0;

		tStringVec& GetExtensions(){ return mvExtensions;}
	protected:
		tStringVec mvExtensions;
	};

	class iVideoStream : public iResourceBase
	{
	public:
		iVideoStream(tString asName) : iResourceBase(asName,0){}
		~iVideoStream() override{}

		virtual bool LoadFromFile(tString asFilePath)=0;

		virtual void Update(float afTimeStep)=0;

		virtual void Play()=0;
		virtual void Stop()=0;

		virtual void Pause(bool abX)=0;
		virtual bool IsPaused()=0;

		virtual void SetLoop(bool abX)=0;
		virtual bool IsLooping()=0;

		virtual void CopyToTexture(iTexture *apTexture)=0;


		const tString& GetFileName(){return msFilePath;}
		const cVector2l& GetSize(){return mvSize;}

		// ResourceBase implementation
		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}


	protected:
		tString msFilePath;
		cVector2l mvSize;
	};
};
#endif // HPL_VIDEO_STREAM_H
