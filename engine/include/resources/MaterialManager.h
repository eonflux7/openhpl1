// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_MANAGER_H
#define HPL_MATERIAL_MANAGER_H

#include "resources/ResourceManager.h"
#include "graphics/Texture.h"
#include "graphics/Material.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class iMaterial;

	class cMaterialManager : public iResourceManager
	{
	public:
		cMaterialManager(cGraphics* apGraphics,cResources *apResources);
		~cMaterialManager() override;

		iResourceBase* Create(const tString& asName) override;
		iMaterial* CreateMaterial(const tString& asName);

		void Update(float afTimeStep) override;

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

		void SetTextureSizeLevel(unsigned int alLevel){ mlTextureSizeLevel = alLevel;}
		int GetTextureSizeLevel(){ return mlTextureSizeLevel;}

		void SetTextureFilter(eTextureFilter aFilter);
		eTextureFilter GetTextureFilter(){ return mTextureFilter;}

		void SetTextureAnisotropy(float afX);
		float GetTextureAnisotropy(){ return mfTextureAnisotropy;}

		tString GetPhysicsMaterialName(const tString& asName);

	private:
		iMaterial* LoadFromFile(const tString& asName,const tString& asPath);

		eTextureTarget GetTarget(const tString& asType);
		tString GetTextureString(eMaterialTexture aType);
		eTextureWrap GetWrap(const tString& asType);
		eTextureAnimMode GetAnimMode(const tString& asType);

		unsigned int mlTextureSizeLevel;
		eTextureFilter mTextureFilter;
		float mfTextureAnisotropy;

		tStringList mlstFileFormats;

		tStringVec mvCubeSideSuffixes;

		cGraphics* mpGraphics;
		cResources* mpResources;

		int mlIdCounter;
	};

};
#endif // HPL_MATERIAL_MANAGER_H
