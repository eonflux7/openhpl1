// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SCRIPT_H
#define HPL_SCRIPT_H

#include "resources/ResourceBase.h"
#include <angelscript.h>
#include <type_traits>
#include <utility>

namespace hpl {

	// The script types a C++ function can take and return: its name in the
	// declaration, and how an argument is read and a return value written
	// through asIScriptGeneric.
	template<class T> struct cScriptType;

	template<> struct cScriptType<void> { static constexpr const char *mpName = "void"; };

	template<> struct cScriptType<std::string>
	{
		static constexpr const char *mpName = "string";
		static std::string Get(asIScriptGeneric *apGen, int alArg){ return *(std::string *)apGen->GetArgObject(alArg); }
		static void Set(asIScriptGeneric *apGen, std::string &asX){ apGen->SetReturnObject(&asX); }
	};
	template<> struct cScriptType<float>
	{
		static constexpr const char *mpName = "float";
		static float Get(asIScriptGeneric *apGen, int alArg){ return apGen->GetArgFloat(alArg); }
		static void Set(asIScriptGeneric *apGen, float afX){ apGen->SetReturnFloat(afX); }
	};
	template<> struct cScriptType<int>
	{
		static constexpr const char *mpName = "int";
		static int Get(asIScriptGeneric *apGen, int alArg){ return (int)apGen->GetArgDWord(alArg); }
		static void Set(asIScriptGeneric *apGen, int alX){ apGen->SetReturnDWord(alX); }
	};
	template<> struct cScriptType<bool>
	{
		static constexpr const char *mpName = "bool";
		static bool Get(asIScriptGeneric *apGen, int alArg){ return apGen->GetArgByte(alArg) != 0; }
		static void Set(asIScriptGeneric *apGen, bool abX){ apGen->SetReturnByte(abX ? 1 : 0); }
	};

	// A generic-calling-convention wrapper of a C++ function and its script
	// declaration, both made from the function's signature. The generic
	// convention works on every platform and does not depend on AngelScript's
	// native call support.
	template<auto Func> struct cScriptGeneric;

	template<class R, class... A, R (*Func)(A...)>
	struct cScriptGeneric<Func>
	{
		static void Call(asIScriptGeneric *apGen){ CallArgs(apGen, std::index_sequence_for<A...>{}); }

		static std::string Declaration(const char *asName)
		{
			std::string sArgs;
			for(const char *pArg : {(const char *)nullptr, cScriptType<std::decay_t<A>>::mpName...})
			{
				if(pArg == nullptr) continue;
				if(!sArgs.empty()) sArgs += ",";
				sArgs += pArg;
			}
			return std::string(cScriptType<R>::mpName) + " " + asName + " (" + sArgs + ")";
		}

	private:
		template<size_t... I>
		static void CallArgs(asIScriptGeneric *apGen, std::index_sequence<I...>)
		{
			if constexpr(std::is_void_v<R>)
			{
				Func(cScriptType<std::decay_t<A>>::Get(apGen, (int)I)...);
			}
			else
			{
				R ret = Func(cScriptType<std::decay_t<A>>::Get(apGen, (int)I)...);
				cScriptType<R>::Set(apGen, ret);
			}
		}
	};
}

// The arguments of iLowLevelSystem::AddScriptFunc for a function, under its
// own name or, for overloads (a function that takes more arguments in a
// later game), another name in the scripts.
#define SCRIPT_REGISTER_FUNC(funcname) SCRIPT_REGISTER_FUNC_AS(funcname, #funcname)
#define SCRIPT_REGISTER_FUNC_AS(funcname, scriptname) \
	hpl::cScriptGeneric<&funcname>::Declaration(scriptname), (void *)&hpl::cScriptGeneric<&funcname>::Call, asCALL_GENERIC


namespace hpl {

	class iScript : public iResourceBase
	{
	public:
		iScript(const tString& asName) : iResourceBase(asName,0){}
		~iScript() override{}

		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}

		virtual bool CreateFromFile(const tString& asFile)=0;

		virtual int GetFuncHandle(const tString& asFunc)=0;

		virtual void AddArg(const tString& asArg)=0;

		/**
		 * Runs a func in the script, for example "test(15)"
		 * \param asFuncLine the line of code
		 * \return true if everything was ok, else false
		 */
		virtual bool Run(const tString& asFuncLine)=0;

		virtual bool Run(int alHandle)=0;

		/**
		 * For tests: a call of each of the script's functions whose arguments
		 * are all strings or numbers, with empty strings and zeros.
		 */
		virtual void GetTestCalls(tStringVec &avCalls){}
	};
};
#endif // HPL_SCRIPT_H
