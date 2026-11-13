// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_TRIGGERS_H
#define GAME_TRIGGERS_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

#include "systems/TriggerHandler.h"

class cGameTrigger_Sound : public cGameTrigger
{
public:
	cSoundEntityData *mpSound;
};

#endif // GAME_TRIGGERS_H
