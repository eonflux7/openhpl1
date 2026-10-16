/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HPL_MATERIAL_FONTNORMAL_H
#define HPL_MATERIAL_FONTNORMAL_H

#include "graphics/Material.h"


namespace hpl {

	class cMaterial_FontNormal : public iMaterial
	{
	public:
		cMaterial_FontNormal(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_FontNormal() override;

		void Compile() override;
		bool StartRendering(eMaterialRenderType aType,iCamera* apCam,iLight *pLight) override;
		void EndRendering(eMaterialRenderType aType) override;
		tVtxBatchFlag GetBatchFlags(eMaterialRenderType aType) override;
		bool NextPass(eMaterialRenderType aType) override;
		bool HasMultiplePasses(eMaterialRenderType aType) override;

		eMaterialType GetType(eMaterialRenderType aType) override;
		void EditVertexes(eMaterialRenderType aType, iCamera* apCam, iLight *pLight,
			tVertexVec *apVtxVec,cVector3f *apTransform,unsigned int alIndexAdd) override;

	private:
	};

	class cMaterialType_FontNormal : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="fontnormal";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return hplNew( cMaterial_FontNormal, (asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D) );
		}
	};

};
#endif // HPL_MATERIAL_FONTNORMAL_H
