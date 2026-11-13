// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ANGELSCRIPT_SCRIPT_H
#define HPL_ANGELSCRIPT_SCRIPT_H

#include "system/Script.h"
#include "impl/LowLevelSystemSDL.h"
#include <angelscript.h>
#include <map>


namespace hpl {

	class cAngelScriptScript : public iScript
	{
	public:
		cAngelScriptScript(const tString& asName, asIScriptEngine *apScriptEngine,
					cScriptOutput *apScriptOutput, int alHandle);
		~cAngelScriptScript() override;

		bool CreateFromFile(const tString& asFileName) override;

		int GetFuncHandle(const tString& asFunc) override;
		void AddArg(const tString& asArg) override;

		bool Run(const tString& asFuncLine) override;
		bool Run(int alHandle) override;

		void GetTestCalls(tStringVec &avCalls) override;

	private:
		asIScriptEngine *mpScriptEngine;
		cScriptOutput *mpScriptOutput;

		asIScriptContext *mpContext;
		asIScriptModule *mpModule;

		// Run(asFuncLine) snippets compiled once and reused; NULL if the
		// snippet did not compile.
		std::map<tString, asIScriptFunction*> m_mapCompiledLines;

		int mlHandle;
		tString msModuleName;

		void Execute(asIScriptFunction *apFunc);
		char* LoadCharBuffer(const tString& asFileName, int& alLength);
	};
};
#endif // HPL_SCRIPT_H
