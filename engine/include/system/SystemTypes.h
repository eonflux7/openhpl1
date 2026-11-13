// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SYSTEM_TYPES_H
#define HPL_SYSTEM_TYPES_H

#include <stdio.h>
#include <string>
#include <list>
#include <set>
#include <vector>

#include "system/Container.h"

namespace hpl {

	// TYPES

	#define eFlagBit_None	(0x00000000)
	#define eFlagBit_All	(0xFFFFFFFF)

	#define eFlagBit_0		(0x00000001)
	#define eFlagBit_1		(0x00000002)
	#define eFlagBit_2		(0x00000004)
	#define eFlagBit_3		(0x00000008)
	#define eFlagBit_4		(0x00000010)
	#define eFlagBit_5		(0x00000020)
	#define eFlagBit_6		(0x00000040)
	#define eFlagBit_7		(0x00000080)
	#define eFlagBit_8		(0x00000100)
	#define eFlagBit_9		(0x00000200)
	#define eFlagBit_10		(0x00000400)
	#define eFlagBit_11		(0x00000800)
	#define eFlagBit_12		(0x00001000)
	#define eFlagBit_13		(0x00002000)
	#define eFlagBit_14		(0x00004000)
	#define eFlagBit_15		(0x00008000)

	#define _W(t) L ## t

	enum eSystemPath
	{
		eSystemPath_Personal,
		eSystemPath_LastEnum
	};

	typedef unsigned int tFlag;

	typedef std::string tString;

	typedef std::list<tString> tStringList;
	typedef tStringList::iterator tStringListIt;

	typedef std::vector<tString> tStringVec;

	typedef std::set<tString> tStringSet;
	typedef tStringSet::iterator tStringSetIt;

	typedef std::wstring tWString;
	typedef std::list<tWString> tWStringList;
	typedef tWStringList::iterator tWStringListIt;

	typedef std::vector<tWString> tWStringVec;
	typedef tWStringVec::iterator tWStringVecIt;



	typedef std::vector<unsigned int> tUIntVec;

	typedef std::vector<int> tIntVec;

	typedef std::vector<int> tIntList;

	typedef std::vector<float> tFloatVec;
	typedef tFloatVec::iterator tFloatVecIt;



	typedef std::list<unsigned int> tUIntList;
	typedef tUIntList::iterator tUIntListIt;


	typedef enum
	{
		eMsgBoxType_Info,
		eMsgBoxType_Error,
		eMsgBoxType_Warning,
		eMsgBoxType_Default
	} eMsgBoxType;


	// DEFINES

	#define STLCallInAll( ContType, aCont, aFunc )				\
	{															\
		##ContType##::iterator it = ##aCont##.begin();			\
		for(; it != ##aCont##.end(); ++it) (*it)->##aFunc##;	\
	}

	// CLASS

	class cDate
	{
	public:
		int seconds;
		int minutes;
		int hours;
		int month_day;
		int month;
		int year;
		int week_day;
		int year_day;

		tString ToString()
		{
			char buff[256];

			sprintf(buff,"%d/%d-%d %d:%d:%d",month_day,month,1900+year,hours, minutes, seconds);

			return buff;
		}

		bool operator==(const cDate &aDate) const
		{
			if(	seconds == aDate.seconds &&
				minutes == aDate.minutes &&
				hours == aDate.hours &&
				month_day == aDate.month_day &&
				month == aDate.month &&
				year == aDate.year)
				{
					return true;
				}

			return false;
		}

		bool operator!=(const cDate &aDate) const
		{
			return !(*this == aDate);
		}

		bool operator>(const cDate &aDate) const
		{
			if(year > aDate.year) return true;
			else if(year < aDate.year) return false;

			if(month > aDate.month) return true;
			else if(month < aDate.month) return false;

			if(month_day > aDate.month_day) return true;
			else if(month_day < aDate.month_day) return false;

			if(hours > aDate.hours) return true;
			else if(hours < aDate.hours) return false;

			if(minutes > aDate.minutes) return true;
			else if(minutes < aDate.minutes) return false;

			if(seconds > aDate.seconds) return true;
			else if(seconds < aDate.seconds) return false;

			return false;
		}

		bool operator<(const cDate &aDate) const
		{
			if(year < aDate.year) return true;
			else if(year > aDate.year) return false;

			if(month < aDate.month) return true;
			else if(month > aDate.month) return false;

			if(month_day < aDate.month_day) return true;
			else if(month_day > aDate.month_day) return false;

			if(hours < aDate.hours) return true;
			else if(hours > aDate.hours) return false;

			if(minutes < aDate.minutes) return true;
			else if(minutes > aDate.minutes) return false;

			if(seconds < aDate.seconds) return true;
			else if(seconds > aDate.seconds) return false;

			return false;
		}
	};

	template <class T>
	class cMemoryPool
	{
	public:
		cMemoryPool(size_t alSize, T* (*apCreateFunc)())
		{
			mvData.resize(alSize,nullptr);
			mlCurrentData = 0;

			mpCreateFunc = apCreateFunc;

			for(size_t i=0; i< mvData.size(); ++i)
			{
				if(mpCreateFunc)	mvData[i] = mpCreateFunc();
				else				mvData[i] = new T();
			}
		}

		~cMemoryPool()
		{
			for(size_t i=0; i< mvData.size(); ++i) delete mvData[i];
		}

		T* Create()
		{
			T*  pData = mvData[mlCurrentData];

			if(mlCurrentData == mvData.size()-1)
			{
				size_t lNewSize = mvData.size() * 2;
				size_t lStart = mvData.size();
				mvData.resize(lNewSize);

				for(size_t i=lStart; i< mvData.size(); ++i)
				{
					if(mpCreateFunc)	mvData[i] = mpCreateFunc();
					else				mvData[i] = new T();
				}

				++mlCurrentData;
			}
			else
			{
				++mlCurrentData;
			}

			return pData;
		}

		void Release(T *apData)
		{
			if(mlCurrentData==0) return;

			--mlCurrentData;
			mvData[mlCurrentData] = apData;
		}

		void ClearUnused()
		{
			for(size_t i=mlCurrentData+1; i< mvData.size(); ++i)
			{
				delete mvData[i];
			}
			mvData.resize(mlCurrentData+1);
		}

	private:
		std::vector<T*> mvData;

		size_t mlCurrentData;

		T* (*mpCreateFunc)();
	};

	// TEMPLATES

	template <class CONT,class T>
	void STLFindAndRemove(CONT &aCont, T* pObject)
	{
		typename CONT::iterator it = aCont.begin();
		for(;it != aCont.end();it++)
		{
			if(*it == pObject){
				aCont.erase(it);
			}
		}
	}

	template <class CONT,class T>
	void STLFindAndDelete(CONT &aCont, T* pObject)
	{
		typename CONT::iterator it = aCont.begin();
		for(;it != aCont.end();it++)
		{
			if(*it == pObject){
				aCont.erase(it);
				break;
			}
		}
		delete pObject;
	}

	template <class CONT>
	void* STLFindByName(CONT &aCont,const tString& asName )
	{
		typename CONT::iterator it = aCont.begin();
		for(;it != aCont.end();it++)
		{
			if((*it)->GetName() == asName){
				return *it;
			}
		}
		return nullptr;
	}

	template <class T>
	void STLDeleteAll(T &aCont){
		typename T::iterator it = aCont.begin();
		for(;it != aCont.end();it++)
		{
			delete *it;
		}
		aCont.clear();
	}

	template <class T>
	void STLMapDeleteAll(T &aCont){
		typename T::iterator it = aCont.begin();
		for(;it != aCont.end();it++)
		{
			delete it->second;
		}
		aCont.clear();
	}

	template <class T, class CONT, class IT>
	class cSTLIterator : public iContainerIterator
	{
	public:
		cSTLIterator(CONT *apCont)
		{
			mpCont = apCont;
			mIt = mpCont->begin();

		}

		bool HasNext() override
		{
			return mIt != mpCont->end();
		}

		void* NextPtr() override
		{
			if(mIt == mpCont->end())
				return nullptr;
			else
			{
				T &temp = const_cast<T &> (*mIt);
				mIt++;
				return &temp;
			}
		}

		T Next()
		{
			if(mIt == mpCont->end())
				return nullptr;
			else
			{
				T &temp = const_cast<T &> (*mIt);
				mIt++;
				return temp;
			}
		}

		T PeekNext()
		{
			if(mIt == mpCont->end())
				return NULL;
			else{
				return *mIt;
			}
		}

	private:
		IT mIt;
		CONT* mpCont;

	};

	template <class T, class CONT, class IT>
	class cSTLMapIterator : public iContainerIterator
	{
	public:
		cSTLMapIterator(CONT *apCont)
		{
			mpCont = apCont;
			mIt = mpCont->begin();

		}

		bool HasNext() override
		{
			return mIt != mpCont->end();
		}

		void* NextPtr() override
		{
			if(mIt == mpCont->end())
				return nullptr;
			else
			{
				T& temp = mIt->second;
				mIt++;
				return &temp;
			}
		}

		T Next()
		{
			if(mIt == mpCont->end())
				return nullptr;
			else
			{
				T temp = mIt->second;
				mIt++;
				return temp;
			}
		}

		T PeekNext()
		{
			if(mIt == mpCont->end())
				return NULL;
			else{
				return mIt->second;
			}
		}

	private:
		IT mIt;
		CONT* mpCont;

	};

};
#endif // HPL_SYSTEM_TYPES_H
