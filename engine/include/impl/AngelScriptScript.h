/*
 * Copyright (C) 2006-2010 - Frictional Games
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
