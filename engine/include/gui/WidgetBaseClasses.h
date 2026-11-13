// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_BASE_CLASSES_H
#define HPL_WIDGET_BASE_CLASSES_H

#include "gui/GuiTypes.h"

namespace hpl {

	class iWidgetItemContainer
	{
	public:
		void AddItem(const tWString &asItem);
		void RemoveItem(int alX);
		void RemoveItem(const tWString &asItem);

		const tWString& GetItem(int alX) const;
		void SetItem(int alX, const tWString& asText);
		int GetItemNum() const;
		bool HasItem(const tWString &asItem);

	protected:
		virtual void UpdateProperties()=0;

		tWStringVec mvItems;
	};
};
#endif // HPL_WIDGET_BASE_CLASSES_H
