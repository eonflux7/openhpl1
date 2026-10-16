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
#ifndef HPL_MATERIAL_DIFFUSE_ALPHA2D_H
#define HPL_MATERIAL_DIFFUSE_ALPHA2D_H

#include "graphics/Material.h"


namespace hpl {

	class cMaterial_DiffuseAlpha2D : public iMaterial
	{
	public:
		cMaterial_DiffuseAlpha2D(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~cMaterial_DiffuseAlpha2D() override;

		void Compile() override;
		bool StartRendering(eMaterialRenderType mType,iCamera* apCam,iLight *pLight) override;
		void EndRendering(eMaterialRenderType mType) override;
		tVtxBatchFlag GetBatchFlags(eMaterialRenderType mType) override;
		bool NextPass(eMaterialRenderType mType) override;
		bool HasMultiplePasses(eMaterialRenderType mType) override;

		eMaterialType GetType(eMaterialRenderType mType) override;
		void EditVertexes(eMaterialRenderType mType, iCamera* apCam, iLight *pLight,
			tVertexVec *apVtxVec,cVector3f *apTransform,unsigned int alIndexAdd) override;

	private:
	};

	class cMaterialType_DiffuseAlpha2D : public iMaterialType
	{
	public:
		bool IsCorrect(tString asName) override{
			return cString::ToLowerCase(asName)=="diffalpha2d";
		}

		iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D) override
		{
			return hplNew( cMaterial_DiffuseAlpha2D, (asName,apLowLevelGraphics,
				apImageManager,apTextureManager,
				apProgramManager,aPicture,apRenderer3D) );
		}
	};

};
#endif // HPL_MATERIAL_DIFFUSE_ALPHA2D_H
