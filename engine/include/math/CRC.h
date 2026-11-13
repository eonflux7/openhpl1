// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_CRC_H
#define HPL_CRC_H

namespace hpl {

	typedef unsigned long tCRCKey;

	class cCRCTable
	{
	public:
		cCRCTable () : mKey (0) {}

		void Init (tCRCKey key);

		tCRCKey operator [] (unsigned i){return mTable [i];}

	private:
		tCRCKey mTable [256];
		tCRCKey mKey;
	};

	class cCRC
	{
	public:
		cCRC (tCRCKey key) : mKey (key), mRegister(0)
		{
			mTable.Init (key);
		}

		void PutByte (unsigned aByte);

		tCRCKey Done ()
		{
			tCRCKey temp = mRegister;
			mRegister = 0;
			return temp;
		}

	private:
		static cCRCTable mTable;
		tCRCKey mKey;
		tCRCKey mRegister;
	};
}

#endif // HPL_CRC_H
