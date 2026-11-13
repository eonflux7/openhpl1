// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GLOBAL_INIT_H
#define GAME_GLOBAL_INIT_H

#include "core/Init.h"
//This is necessary because of some bad design...
//cInit should be global and be using Singleton or something.

extern cInit* gpInit;

#endif // GAME_GLOBAL_INIT_H
