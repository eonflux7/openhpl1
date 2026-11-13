// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/GameTypes.h"

#include "system/LowLevelSystem.h"

namespace hpl {

	// CONSTRCTORS

	cScriptVar::cScriptVar()
	{

	}

	// SERIALIZE

	kBeginSerializeBase(cScriptVar)
	kSerializeVar(msName, eSerializeType_String)
	kSerializeVar(mlVal, eSerializeType_Int32)
	kEndSerialize()
}
