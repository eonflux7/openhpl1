// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_CONTAINER_H
#define HPL_CONTAINER_H

#include <vector>
#include <list>
#include <map>
#include <utility>


namespace hpl {

	class iContainerIterator
	{
	friend class cSerializeClass;
	public:
		virtual ~iContainerIterator(){}
	protected:
		virtual bool HasNext()=0;

		virtual void* NextPtr()=0;
	};

	// Range-for over an iterator with HasNext() and Next(). As in
	// while(it.HasNext()){ x = it.Next(); ... }, the iterator is moved past
	// each element before the loop body runs, so the body may remove that
	// element from its container.
	struct cHasNextEnd {};

	template<class IT>
	struct cHasNextPos
	{
		IT *mpIt;
		bool operator!=(cHasNextEnd) const { return mpIt->HasNext(); }
		decltype(auto) operator*() const { return mpIt->Next(); }
		void operator++() const {}
	};

	template<class IT, class = decltype(std::declval<IT&>().HasNext())>
	cHasNextPos<IT> begin(IT &aIt){ return {&aIt}; }
	template<class IT, class = decltype(std::declval<IT&>().HasNext())>
	cHasNextEnd end(IT &aIt){ return {}; }

	class iContainer
	{
	friend class cSerializeClass;
	public:
		virtual size_t Size()=0;
		virtual void Clear()=0;
	protected:

		virtual void AddVoidPtr(void **apPtr)=0;
		virtual void AddVoidClass(void *apClass)=0;

		virtual iContainerIterator* CreateIteratorPtr()=0;
	};

	class iContainerKeyPair
	{
	public:
		virtual size_t Size()=0;

		virtual void AddVoidPtr(void *apKey, void **apClass)=0;
		virtual void AddVoidClass(void *apKey, void *apClass)=0;
	};

	template<class T>
	class cContainerVecIterator : public iContainerIterator
	{
		void* NextPtr() override{
			return &Next();
		}

	public:
		cContainerVecIterator(std::vector<T> *apVec)
		{
			mpVec = apVec;
			mIt = apVec->begin();
		}

		bool HasNext() override
		{
			return mIt != mpVec->end();
		}

		T& Next()
		{
			T &val = *mIt;
			mIt++;
			return val;
		}

		T& PeekNext()
		{
			return *mIt;
		}

		void Erase()
		{
			if(mIt != mpVec->end())
				mIt = mpVec->erase(mIt);
		}

	private:
		std::vector<T> *mpVec;
		typename std::vector<T>::iterator mIt;
	};

	template<class T>
	class cContainerVec : public iContainer
	{
	private:
		void AddVoidPtr(void **apPtr) override
		{
			mvVector.push_back(*((T*)apPtr));
		}
		void AddVoidClass(void *apClass) override
		{
			mvVector.push_back(*((T*)apClass));
		}
		iContainerIterator* CreateIteratorPtr() override
		{
			return new cContainerVecIterator<T>(&mvVector);
		}

	public:
		cContainerVec(){}

		size_t Size() override
		{
			return mvVector.size();
		}

		void Clear() override
		{
			mvVector.clear();
		}

		void Reserve(size_t alSize)
		{
			mvVector.reserve(alSize);
		}

		void Resize(size_t alSize)
		{
			mvVector.resize(alSize);
		}

		void Add(T aVal)
		{
			mvVector.push_back(aVal);
		}

		cContainerVecIterator<T> GetIterator()
		{
			return cContainerVecIterator<T>(&mvVector);
		}

		T& operator [](size_t alX)
		{
			return mvVector[alX];
		}

		std::vector<T> mvVector;
	};

	template<class T>
	class cContainerListIterator : public iContainerIterator
	{
		void* NextPtr() override{
			return &Next();
		}

	public:
		cContainerListIterator(std::list<T> *apVec)
		{
			mpVec = apVec;
			mIt = apVec->begin();
		}

		bool HasNext() override
		{
			return mIt != mpVec->end();
		}

		T& Next()
		{
			T &val = *mIt;
			mIt++;
			return val;
		}

		T& PeekNext()
		{
			return *mIt;
		}

		void Erase()
		{
			if(mIt != mpVec->end())
				mIt = mpVec->erase(mIt);
		}

	private:
		std::list<T> *mpVec;
		typename std::list<T>::iterator mIt;
	};

	template<class T>
	class cContainerList : public iContainer
	{
	private:
		void AddVoidPtr(void **apPtr) override
		{
			mvVector.push_back(*((T*)apPtr));
		}
		void AddVoidClass(void *apClass) override
		{
			mvVector.push_back(*((T*)apClass));
		}
		iContainerIterator* CreateIteratorPtr() override
		{
			return new cContainerListIterator<T>(&mvVector);
		}

	public:
		cContainerList(){}

		size_t Size() override
		{
			return mvVector.size();
		}

		void Clear() override
		{
			mvVector.clear();
		}

		void Add(T aVal)
		{
			mvVector.push_back(aVal);
		}

		cContainerListIterator<T> GetIterator()
		{
			return cContainerListIterator<T>(&mvVector);
		}

		std::list<T> mvVector;
	};
};
#endif // HPL_CONTAINER_H
