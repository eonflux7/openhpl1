// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIALHANDLER_H
#define HPL_MATERIALHANDLER_H

#include "graphics/Material.h"

namespace hpl {

	class cGraphics;
	class cResources;

	class cMaterialHandler
	{
	public:
		cMaterialHandler(cGraphics* apGraphics, cResources* apResources);
		~cMaterialHandler();

		/**
		 * Add a new type of material
		 * \param apTypedata
		 */
		void Add(iMaterialType* apTypedata);
		/**
		 * Create a new material
		 * \param asMatName
		 * \param mPicType
		 * \return
		 */
		iMaterial* Create(tString asMatName, eMaterialPicture mPicType);

		iMaterial* Create(const tString& asName,tString asMatName, eMaterialPicture mPicType);

	private:
		tMaterialTypeList mlstMatTypes;
		cResources* mpResources;
		cGraphics* mpGraphics;
	};

};
#endif // HPL_MATERIALHANDLER_H
