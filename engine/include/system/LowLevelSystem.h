// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LOWLEVELSYSTEM_H
#define HPL_LOWLEVELSYSTEM_H

#include "system/MemoryManager.h"
#include "system/SystemTypes.h"


namespace hpl {

#ifdef UPDATE_TIMING_ENABLED
	#define START_TIMING_EX(x,y)	LogUpdate("Updating %s in file %s at line %d\n",x,__FILE__,__LINE__); \
									unsigned int y##_lTime = GetApplicationTime();
	#define START_TIMING(x)	LogUpdate("Updating %s in file %s at line %d\n",#x,__FILE__,__LINE__); \
								unsigned int x##_lTime = GetApplicationTime();
	#define STOP_TIMING(x)	LogUpdate(" Time spent: %d ms\n",GetApplicationTime() - x##_lTime);
	#define START_TIMING_TAB(x)	LogUpdate("\tUpdating %s in file %s at line %d\n",#x,__FILE__,__LINE__); \
							unsigned int x##_lTime = GetApplicationTime();
	#define STOP_TIMING_TAB(x)	LogUpdate("\t Time spent: %d ms\n",GetApplicationTime() - x##_lTime);
#else
	#define START_TIMING_EX(x,y)
	#define START_TIMING(x)
	#define STOP_TIMING(x)
	#define START_TIMING_TAB(x)
	#define STOP_TIMING_TAB(x)
#endif

	class iScript;

	extern void SetLogFile(const tWString &asFile);
	// The program's arguments as main() got them (hplMain gets them joined)
	extern const tStringVec &GetCommandLineArguments();
	extern void FatalError(const char* fmt,... );
	extern void Error(const char* fmt, ...);
	extern void Warning(const char* fmt, ...);
	extern void Log(const char* fmt, ...);

	extern void SetUpdateLogFile(const tWString &asFile);
	extern void ClearUpdateLogFile();
	extern void SetUpdateLogActive(bool abX);
	extern void LogUpdate(const char* fmt, ...);

//	extern void CreateMessageBox ( const char* asCaption, const char *fmt, ...);
//	extern void CreateMessageBox ( eMsgBoxType eType, const char* asCaption, const char *fmt, ...);

	extern void CreateMessageBoxW( const wchar_t* asCaption, const wchar_t* fmt, ...);
	extern void CreateMessageBoxW( eMsgBoxType eType, const wchar_t* asCaption, const wchar_t* fmt, ...);

	extern void OpenBrowserWindow ( const tWString& asURL );

	extern void CopyTextToClipboard(const tWString &asText);
	extern tWString LoadTextFromClipboard();

	extern tWString GetSystemSpecialPath(eSystemPath aPathType);

	extern bool FileExists(const tWString& asFileName);
	extern void RemoveFile(const tWString& asFileName);
	extern bool CloneFile(const tWString& asSrcFileName,const tWString& asDestFileName,
					bool abFailIfExists);
	extern bool CreateFolder(const tWString& asPath);
	extern bool FolderExists(const tWString& asPath);
	extern bool IsFileLink(const tWString& asPath);
	extern bool LinkFile(const tWString& asPointsTo, const tWString& asLink);
	extern bool RenameFile(const tWString& asFrom, const tWString& asTo);
	extern cDate FileModifiedDate(const tWString& asFilePath);
	extern cDate FileCreationDate(const tWString& asFilePath);

	extern void SetWindowCaption(const tString &asName);

	extern bool HasWindowFocus(const tWString &asWindowCaption);

	extern unsigned long GetApplicationTime();
	/**
	 * The same clock as GetApplicationTime(), in milliseconds with fractions.
	 */
	extern double GetApplicationTimeExact();

	class iLowLevelSystem
	{
	public:
		virtual ~iLowLevelSystem(){}

		/**
		 * Remark: Usually not finer then 10ms accuracy.
		 * \return Number of millisecs since start of app.
		 */
		virtual unsigned long GetTime()=0;

		/**
		* Gets the current date.
		*/
		virtual cDate GetDate()=0;

		/**
		 * Creates a ne script
		 * \param asName name of the script.
		 * \return
		 */
		virtual iScript* CreateScript(const tString& asName)=0;

		/**
		 * Add a function to the script vm. Example: "void test(float x)"
		 * \param asFuncDecl the declaration.
		 * \return
		 */
		virtual bool AddScriptFunc(const tString& asFuncDecl, void* pFunc, int callConv)=0;

		/**
		 * Add a variable to the script vm. Example: "int MyVar"
		 * \param asVarDecl the declartion
		 * \param *pVar the variable
		 * \return
		 */
		virtual bool AddScriptVar(const tString& asVarDecl, void *pVar)=0;

		/**
		* Sets the main thread to rest for a number of milliseconds.
		* \param alMillisecs
		*/
		virtual void Sleep ( const unsigned int alMillisecs )=0 ;

		/**
		* Rests until this much time has passed, to well under a millisecond:
		* sleeps for most of it and waits out the rest.
		*/
		virtual void SleepPrecise ( double afMillisecs )=0 ;

	};
};
#endif // HPL_LOWLEVELSYSTEM_H
