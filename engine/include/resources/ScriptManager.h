// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SCRIPT_MANAGER_H
#define HPL_SCRIPT_MANAGER_H

#include "resources/ResourceManager.h"

namespace hpl {

	class cSystem;
	class cResources;
	class iScript;

	class cScriptManager : public iResourceManager
	{
	public:
		cScriptManager(cSystem* apSystem,cResources *apResources);
		~cScriptManager() override;

		iResourceBase* Create(const tString& asName) override;
		/**
		 * Create a new script.
		 * \param asName name of the script.
		 * \return
		 */
		iScript* CreateScript(const tString& asName);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

	private:
		cSystem* mpSystem;
		cResources *mpResources;
	};

};
#endif // HPL_SCRIPT_MANAGER_H
