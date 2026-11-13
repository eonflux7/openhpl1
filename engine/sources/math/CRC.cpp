// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "math/CRC.h"

namespace hpl {

	cCRCTable cCRC::mTable;

	void cCRCTable::Init (tCRCKey aKey)
	{
		if (aKey == mKey) return;

		mKey = aKey;

		// for all possible byte values
		for (unsigned i = 0; i < 256; ++i)
		{
			tCRCKey reg = i << 24;
			// for all bits in a byte
			for (int j = 0; j < 8; ++j)
			{
				bool topBit = (reg & 0x80000000) != 0;
				reg <<= 1;
				if (topBit)
					reg ^= mKey;
			}
			mTable [i] = reg;
		}
	}

	void cCRC::PutByte (unsigned aByte)
	{
		unsigned top = mRegister >> 24;
		top ^= aByte;
		mRegister = (mRegister << 8) ^ mTable [top];
	}
}
