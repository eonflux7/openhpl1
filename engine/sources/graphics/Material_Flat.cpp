// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material_Flat.h"
#include "scene/Light.h"
#include "scene/Camera.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "graphics/GPUProgram.h"
#include "math/Math.h"
#include "system/LowLevelSystem.h"


namespace hpl {

	cMaterial_Flat::cMaterial_Flat(	const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iMaterial(asName,apLowLevelGraphics,apImageManager,apTextureManager,apProgramManager,
					aPicture,apRenderer3D)
	{
		mbIsTransparent = false;
		mbIsGlowing= false;
		mbUsesLights = false;

		//Load the Z pass vertex program
		iGpuProgram *pVtxProg = mpProgramManager->CreateProgram("Diffuse_Color");
		SetProgram(pVtxProg,0);
	}

	cMaterial_Flat::~cMaterial_Flat()
	{
	}

	iGpuProgram* cMaterial_Flat::GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return nullptr;//return mpProgram[0];
	}

	bool cMaterial_Flat::ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}

	bool cMaterial_Flat::ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		return false;
	}



	eMaterialAlphaMode cMaterial_Flat::GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z && mbHasAlpha) return eMaterialAlphaMode_Trans;

		return eMaterialAlphaMode_Solid;
	}

	eMaterialBlendMode cMaterial_Flat::GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z) return eMaterialBlendMode_None;

		if(alPass ==1)	return eMaterialBlendMode_Add;
		else			return eMaterialBlendMode_Replace;
	}

	eMaterialChannelMode cMaterial_Flat::GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z) return eMaterialChannelMode_Z;
		return eMaterialChannelMode_RGBA;
	}

	iTexture* cMaterial_Flat::GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight)
	{
		if(aType == eMaterialRenderType_Z)
		{
				if(alUnit==0 && mbHasAlpha) return mvTexture[eMaterialTexture_Diffuse];
				return nullptr;
		}

		if(alUnit == 0)
		{
			if(alPass ==0)	return mvTexture[eMaterialTexture_Diffuse];
			else			return mvTexture[eMaterialTexture_Illumination];
		}

		return nullptr;
	}

	bool cMaterial_Flat::UsesType(eMaterialRenderType aType)
	{
		if(aType == eMaterialRenderType_Diffuse || aType == eMaterialRenderType_Z) return true;
		return false;
	}

	int cMaterial_Flat::GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight)
	{
		if(mvTexture[eMaterialTexture_Illumination]) return 2;
		return 1;
	}

	tTextureTypeList cMaterial_Flat::GetTextureTypes()
	{
		tTextureTypeList vTypes;
		vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
		vTypes.push_back(cTextureType("",eMaterialTexture_Illumination));

		return vTypes;
	}
}
