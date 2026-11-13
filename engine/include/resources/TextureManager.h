// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_TEXTURE_MANAGER_H
#define HPL_TEXTURE_MANAGER_H

#include "resources/ResourceManager.h"
#include "graphics/Texture.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class iTexture;

	class cTextureManager : public iResourceManager
	{
	public:
		cTextureManager(cGraphics* apGraphics,cResources *apResources);
		~cTextureManager() override;

		iResourceBase* Create(const tString& asName) override;
		iTexture* Create1D(const tString& asName,bool abUseMipMaps, bool abCompress=false, eTextureType aType=eTextureType_Normal,
							unsigned int alTextureSizeLevel=0);

		iTexture* Create2D(const tString& asName,bool abUseMipMaps, bool abCompress=false, eTextureType aType=eTextureType_Normal,
							unsigned int alTextureSizeLevel=0, eTextureTarget aTarget= eTextureTarget_2D);

		/**
		 * Creates an animated texture. The name must be [name].[ext]. And then the textures in the animtion must
		 * be named [name]01.[ext], [name]02.[ext], etc
		 * \param asName
		 * \param abUseMipMaps
		 * \param abCompress
		 * \param aType
		 * \return
		 */
		iTexture* CreateAnim2D(const tString& asName,bool abUseMipMaps, bool abCompress=false, eTextureType aType=eTextureType_Normal,
								unsigned int alTextureSizeLevel=0);

		iTexture* CreateCubeMap(const tString& asName,bool abUseMipMaps, bool abCompress=false, eTextureType aType=eTextureType_Normal,
								unsigned int alTextureSizeLevel=0);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;

		void Update(float afTimeStep) override;

	private:
		iTexture* CreateFlatTexture(const tString& asName,bool abUseMipMaps,
									bool abCompress, eTextureType aType, eTextureTarget aTarget,
									unsigned int alTextureSizeLevel);

		iTexture* FindTexture2D(const tString &asName, tString &asFilePath);

		tStringList mlstFileFormats;

		tStringVec mvCubeSideSuffixes;

		cGraphics* mpGraphics;
		cResources* mpResources;
	};

};
#endif // HPL_TEXTURE_MANAGER_H
