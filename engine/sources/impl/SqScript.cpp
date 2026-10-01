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
#include "impl/SqScript.h"
#include "system/LowLevelSystem.h"
#include "system/String.h"
#include "math/Math.h"
#include "impl/Platform.h"
#include <stdio.h>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cSqScript::cSqScript(const tString& asName,asIScriptEngine *apScriptEngine,
							cScriptOutput *apScriptOutput, int alHandle)
		: iScript(asName)
	{
		mpScriptEngine = apScriptEngine;
		mpScriptOutput = apScriptOutput;
		mlHandle = alHandle;

		mpContext = mpScriptEngine->CreateContext();
		mpModule = NULL;

		//Create a unique module name
		msModuleName = "Module_"+cString::ToString(cMath::RandRectl(0,1000000))+
						"_"+cString::ToString(mlHandle);

	}

	cSqScript::~cSqScript()
	{
		std::map<tString, asIScriptFunction*>::iterator it = m_mapCompiledLines.begin();
		for(; it != m_mapCompiledLines.end(); ++it)
		{
			if(it->second) it->second->Release();
		}

		mpContext->Release();
		if(mpModule) mpModule->Discard();
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cSqScript::CreateFromFile(const tString& asFileName)
	{
		int lLength;
		char *pCharBuffer = LoadCharBuffer(asFileName,lLength);
		if(pCharBuffer==NULL){
			Error("Couldn't load script '%s'!\n",asFileName.c_str());
			return false;
		}

		mpModule = mpScriptEngine->GetModule(msModuleName.c_str(), asGM_ALWAYS_CREATE);

		if(mpModule->AddScriptSection("main", pCharBuffer, lLength)<0)
		{
			Error("Couldn't add script '%s'!\n",asFileName.c_str());
			hplDeleteArray(pCharBuffer);
			return false;
		}

		if(mpModule->Build()<0)
		{
			Error("Couldn't build script '%s'!\n",asFileName.c_str());
			Log("------- SCRIPT OUTPUT BEGIN --------------------------\n");
			mpScriptOutput->Display();
			mpScriptOutput->Clear();
			Log("------- SCRIPT OUTPUT END ----------------------------\n");



			hplDeleteArray(pCharBuffer);
			return false;
		}
		mpScriptOutput->Clear();

		hplDeleteArray(pCharBuffer);
		return true;
	}

	//-----------------------------------------------------------------------

	int cSqScript::GetFuncHandle(const tString& asFunc)
	{
		if(mpModule==NULL) return -1;

		asIScriptFunction *pFunc = mpModule->GetFunctionByName(asFunc.c_str());
		return pFunc ? pFunc->GetId() : -1;
	}

	//-----------------------------------------------------------------------

	void cSqScript::AddArg(const tString& asArg)
	{

	}

	//-----------------------------------------------------------------------

	bool cSqScript::Run(const tString& asFuncLine)
	{
		if(mpModule==NULL) return true;

		asIScriptFunction *pFunc = NULL;
		std::map<tString, asIScriptFunction*>::iterator it = m_mapCompiledLines.find(asFuncLine);
		if(it != m_mapCompiledLines.end())
		{
			pFunc = it->second;
		}
		else
		{
			// Same as AngelScript 2.7's ExecuteString: wrap the line in a
			// function compiled in the scope of this script's module.
			tString sCode = "void ExecuteString() {\n" + asFuncLine + ";\n}";
			if(mpModule->CompileFunction("ExecuteString", sCode.c_str(), -1, 0, &pFunc) < 0)
				pFunc = NULL;

			mpScriptOutput->Clear();
			m_mapCompiledLines.insert(std::map<tString, asIScriptFunction*>::value_type(asFuncLine, pFunc));
		}

		if(pFunc==NULL) return true;

		if(mpContext->Prepare(pFunc) >= 0)
			mpContext->Execute();

		return true;
	}

	//-----------------------------------------------------------------------

	bool cSqScript::Run(int alHandle)
	{
		asIScriptFunction *pFunc = mpScriptEngine->GetFunctionById(alHandle);
		if(pFunc==NULL) return false;

		mpContext->Prepare(pFunc);

		/* Set all the args here */

		mpContext->Execute();

		return true;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	char* cSqScript::LoadCharBuffer(const tString& asFileName, int& alLength)
	{
		FILE *pFile = fopen(asFileName.c_str(), "rb");
		if(pFile==NULL){
			return NULL;
		}

		int lLength = (int)Platform::FileLength(pFile);
		alLength = lLength;

		char *pBuffer = hplNewArray(char,lLength);
		fread(pBuffer, lLength, 1, pFile);

		fclose(pFile);

		return pBuffer;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// STATIC PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

}
