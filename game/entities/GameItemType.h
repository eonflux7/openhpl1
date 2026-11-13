// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_ITEM_TYPE_H
#define GAME_GAME_ITEM_TYPE_H

#include "core/StdAfx.h"

using namespace hpl;

#include "core/GameTypes.h"
#include "player/PlayerHelper.h"

class cInit;
class cInventoryItem;
class iGameEntity;
class cGameItem;

class cGameItemType
{
public:
	cGameItemType(cInit *apInit);
	virtual ~cGameItemType();

	tWStringVec* GetActions(bool abCanBeDropped){
			if(abCanBeDropped)
				return &mvActions;
			else
				return &mvNonDropActions;
		}

	virtual bool OnPickUp(cInventoryItem *apItem, bool abInGame){ return true;}

	virtual bool OnAction(cInventoryItem *apItem, int alActionIndex){return true;}

	virtual void OnUse(cInventoryItem *apItem, iGameEntity *apEntity){}

	/**
	 * Black Plague: apItem dropped on apWithItem or the other way round.
	 * \return true if the type did something with them
	 */
	virtual bool OnCombine(cInventoryItem *apItem, cInventoryItem *apWithItem, int alSlotIndex){ return false;}

	virtual tString GetShortCutAction(cInventoryItem *apItem){return "";}

	virtual tWString GetString(cInventoryItem *apItem){return _W("");}

protected:
	cInit *mpInit;
	
	tWStringVec mvActions;
	tWStringVec mvNonDropActions;
};

// NORMAL

class cGameItemType_Normal : public cGameItemType
{
public:
	cGameItemType_Normal(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;

	void OnUse(cInventoryItem *apItem, iGameEntity *apEntity) override;
};

// NOTEBOOK

class cGameItemType_Notebook : public cGameItemType
{
public:
	cGameItemType_Notebook(cInit *apInit);
	
	bool OnPickUp(cInventoryItem *apItem, bool abInGame) override;
	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;

	tString GetShortCutAction(cInventoryItem *apItem) override{return "NoteBook";}
};

// NOTE

class cGameItemType_Note : public cGameItemType
{
public:
	cGameItemType_Note(cInit *apInit);

	bool OnPickUp(cInventoryItem *apItem, bool abInGame) override;

};

// BATTERY

class cGameItemType_Battery : public cGameItemType
{
public:
	cGameItemType_Battery(cInit *apInit);

	bool OnPickUp(cInventoryItem *apItem, bool abInGame) override;
	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;
	bool OnCombine(cInventoryItem *apItem, cInventoryItem *apWithItem, int alSlotIndex) override;

};

// FLASHLIGHT

class cGameItemType_Flashlight : public cGameItemType
{
public:
	cGameItemType_Flashlight(cInit *apInit);

	bool OnPickUp(cInventoryItem *apItem, bool abInGame) override;
	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;

	tWString GetString(cInventoryItem *apItem) override;

	tString GetShortCutAction(cInventoryItem *apItem) override{return "Flashlight";}
};

// WEAPON MELEE

class cGameItemType_WeaponMelee : public cGameItemType
{
public:
	cGameItemType_WeaponMelee(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;
};

// WEAPON MELEE HAPTX

class cGameItemType_WeaponMeleeHaptX : public cGameItemType
{
public:
	cGameItemType_WeaponMeleeHaptX(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;
};

// THROW

class cGameItemType_Throw : public cGameItemType
{
public:
	cGameItemType_Throw(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;
};

// GAS MASK (Black Plague)

class cGameItemType_GasMask : public cGameItemType
{
public:
	cGameItemType_GasMask(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;

	tWString GetString(cInventoryItem *apItem) override;
};

// COLLECTABLE (Black Plague): the artefacts, counted when picked up

class cGameItemType_Collectable : public cGameItemType
{
public:
	cGameItemType_Collectable(cInit *apInit);

	bool OnPickUp(cInventoryItem *apItem, bool abInGame) override;
	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;
};

// GLOW STICK

class cGameItemType_GlowStick : public cGameItemType
{
public:
	cGameItemType_GlowStick(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;

	tWString GetString(cInventoryItem *apItem) override;

	tString GetShortCutAction(cInventoryItem *apItem) override{return "GlowStick";}
};

// FLARE

class cGameItemType_Flare : public cGameItemType
{
public:
	cGameItemType_Flare(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;
};

// PAINKILLERS

class cGameItemType_Painkillers : public cGameItemType
{
public:
	cGameItemType_Painkillers(cInit *apInit);

	bool OnAction(cInventoryItem *apItem, int alActionIndex) override;

};

#endif // GAME_GAME_ITEM_TYPE_H
