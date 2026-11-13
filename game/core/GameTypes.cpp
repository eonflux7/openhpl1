// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/GameTypes.h"

#include "entities/GameEntity.h"

cGameCollideScript::cGameCollideScript()
{
	msFuncName[0]=""; msFuncName[1]=""; msFuncName[2]="";
	mbCollides = false;
	mbDeleteMe = false;
}

// SERIALIZE

kBeginSerializeBase(cGameTimer)
kSerializeVar(msName,eSerializeType_String)
kSerializeVar(msCallback,eSerializeType_String)

kSerializeVar(mbGlobal,eSerializeType_Bool)

kSerializeVar(mfTime,eSerializeType_Float32)

kSerializeVar(mbDeleteMe,eSerializeType_Bool)
kSerializeVar(mbPaused,eSerializeType_Bool)
kEndSerialize()

kBeginSerializeBase(cSaveGame_cGameCollideScript)
kSerializeVarArray(msFuncName,eSerializeType_String,3)
kSerializeVar(msEntity, eSerializeType_String)
kSerializeVar(mbCollides, eSerializeType_Bool)
kEndSerialize()

void cSaveGame_cGameCollideScript::LoadFrom(cGameCollideScript *apScript)
{
	msEntity = apScript->mpEntity->GetName();
	mbCollides = apScript->mbCollides;
	for(int i=0;i<3;++i) msFuncName[i] = apScript->msFuncName[i];
}

void cSaveGame_cGameCollideScript::SaveTo(cGameCollideScript *apScript)
{
	apScript->mbCollides = mbCollides;
	for(int i=0;i<3;++i)  apScript->msFuncName[i] = msFuncName[i];
}
