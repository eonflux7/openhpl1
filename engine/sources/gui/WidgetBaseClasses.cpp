// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "gui/WidgetBaseClasses.h"

#include "system/LowLevelSystem.h"

#include "math/Math.h"

#include "gui/Widget.h"
#include "gui/GuiSkin.h"
#include "gui/GuiSet.h"
#include "gui/GuiGfxElement.h"

namespace hpl {

	// ITEM CONTAINER

	void iWidgetItemContainer::AddItem(const tWString &asItem)
	{
		mvItems.push_back(asItem);

		UpdateProperties();
	}

	void iWidgetItemContainer::RemoveItem(int alX)
	{
		int lCount =0;
		tWStringVecIt it = mvItems.begin();
		for(; it != mvItems.end(); ++it,++lCount)
		{
			if(lCount == alX)
			{
				mvItems.erase(it);
				break;
			}
		}

		UpdateProperties();
	}

	void iWidgetItemContainer::RemoveItem(const tWString &asItem)
	{
		tWStringVecIt it = mvItems.begin();
		for(; it != mvItems.end(); ++it)
		{
			if(*it == asItem)
			{
				mvItems.erase(it);
				break;
			}
		}
	}

	const tWString& iWidgetItemContainer::GetItem(int alX) const
	{
		return mvItems[alX];
	}
	void iWidgetItemContainer::SetItem(int alX, const tWString& asText)
	{
		mvItems[alX] = asText;
	}

	int iWidgetItemContainer::GetItemNum() const
	{
		return (int)mvItems.size();
	}
	bool iWidgetItemContainer::HasItem(const tWString &asItem)
	{
		tWStringVecIt it = mvItems.begin();
		for(; it != mvItems.end(); ++it)
		{
			if(*it == asItem) return true;
		}
		return false;
	}
}
