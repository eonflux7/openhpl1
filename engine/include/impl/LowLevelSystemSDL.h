// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELSYSTEM_SDL_H
#define HPL_LOWLEVELSYSTEM_SDL_H

#include "system/LowLevelSystem.h"
#include <angelscript.h>
#include <stdio.h>

namespace hpl {

	class cLogWriter
	{
	public:
		cLogWriter(const tWString& asDefaultFile);
		~cLogWriter();

		void Write(const tString& asMessage);
		void Clear();

		void SetFileName(const tWString& asFile);

	private:
		void ReopenFile();

		FILE *mpFile;
		tWString msFileName;
	};

	class cScriptOutput// : public  asIOutputStream
	{
	public:
		cScriptOutput() : msMessage("") {}
		~cScriptOutput(){}

		void AddMessage(const asSMessageInfo *msg);
		void Display();
		void Clear();

	private:
		tString msMessage;
	};

	class cLowLevelSystemSDL : public iLowLevelSystem
	{
	public:
		cLowLevelSystemSDL();
		~cLowLevelSystemSDL() override;

		void SetWindowCaption(const tString &asName);

		unsigned long GetTime() override;
		cDate GetDate() override;

		iScript* CreateScript(const tString& asName) override;

		bool AddScriptFunc(const tString& asFuncDecl, void* pFunc, int callConv) override;
		bool AddScriptVar(const tString& asVarDecl, void *pVar) override;

		void Sleep ( const unsigned int alMillisecs ) override;
		void SleepPrecise ( double afMillisecs ) override;

	private:
		asIScriptEngine *mpScriptEngine;
		cScriptOutput *mpScriptOutput;
		int mlHandleCount;
	};
};
#endif // HPL_LOWLEVELSYSTEM_SDL_H
