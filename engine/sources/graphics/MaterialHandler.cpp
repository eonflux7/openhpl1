// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/MaterialHandler.h"
#include "resources/ResourceImage.h"
#include "graphics/Graphics.h"
#include "resources/Resources.h"

namespace hpl {

	cMaterialHandler::cMaterialHandler(cGraphics* apGraphics, cResources* apResources)
	{
		mpGraphics = apGraphics;
		mpResources = apResources;
	}

	cMaterialHandler::~cMaterialHandler()
	{
		tMaterialTypeListIt it = mlstMatTypes.begin();
		for(;it!=mlstMatTypes.end();it++)
		{
			delete *it;
		}

		mlstMatTypes.clear();
	}

	void cMaterialHandler::Add(iMaterialType* apTypedata)
	{
		mlstMatTypes.push_back(apTypedata);
	}

	iMaterial* cMaterialHandler::Create(tString asMatName, eMaterialPicture mPicType)
	{
		return Create("",asMatName,mPicType);
	}

	iMaterial* cMaterialHandler::Create(const tString& asName,tString asMatName, eMaterialPicture mPicType)
	{
		iMaterial* pMat=nullptr;
		cResourceImage* pImage=nullptr;
		unsigned int lIdCount = 1;

		for(tMaterialTypeListIt it = mlstMatTypes.begin(); it!=mlstMatTypes.end();it++)
		{
			if((*it)->IsCorrect(asMatName))
			{
				pMat = (*it)->Create(asName,mpGraphics->GetLowLevel(),mpResources->GetImageManager(),
									mpResources->GetTextureManager(),
									mpResources->GetGpuProgramManager(),
									mPicType,mpGraphics->GetRenderer3D());

				//Set an id to the material for easier rendering later on.
				pMat->SetId(lIdCount);

				break;
			}

			lIdCount++;
		}

		return pMat;
	}
}
