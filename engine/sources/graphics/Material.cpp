// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Material.h"
#include "graphics/Renderer3D.h"
#include "system/LowLevelSystem.h"
#include "resources/TextureManager.h"
#include "resources/ImageManager.h"
#include "resources/GpuProgramManager.h"
#include "graphics/GPUProgram.h"
#include "scene/Camera.h"
#include "system/String.h"

namespace hpl {

	iMaterial::iMaterial(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
		cImageManager* apImageManager, cTextureManager *apTextureManager,
		cGpuProgramManager* apProgramManager,
		eMaterialPicture aPicture, cRenderer3D *apRenderer3D)
		: iResourceBase(asName, 0)
	{
		if(aPicture==eMaterialPicture_Image){
			mvImage.resize(eMaterialTexture_LastEnum);
			mvImage.assign(mvImage.size(), nullptr);
		}
		else {
			mvTexture.resize(eMaterialTexture_LastEnum);
			mvTexture.assign(mvTexture.size(), nullptr);
		}

		mPicture = aPicture;

		mpLowLevelGraphics = apLowLevelGraphics;
		mpImageManager = apImageManager;
		mpTextureManager = apTextureManager;
		mpRenderer3D = apRenderer3D;
		// Resource fixtures may construct a material without a renderer.
		mpRenderSettings = apRenderer3D ? apRenderer3D->GetRenderSettings() : nullptr;
		mpProgramManager = apProgramManager;

		mbUsesLights = false;
		mbIsTransparent = false;
		mbIsGlowing = false;
		mbHasAlpha = false;
		mbDepthTest = true;
		mfValue = 1;

		for(auto& program : mpProgram) program = nullptr;


		mlId = -1;
	}

	iMaterial::~iMaterial()
	{
		int i;

		for(i=0;i<(int)mvImage.size();i++){
			if(mvImage[i])
				mpImageManager->Destroy(mvImage[i]);
		}
		for(i=0;i<(int)mvTexture.size();i++){
			if(mvTexture[i])
				mpTextureManager->Destroy(mvTexture[i]);
		}

		for(auto* program : mpProgram)
			if(program) mpProgramManager->Destroy(program);
	}

	iTexture* iMaterial::GetTexture(eMaterialTexture aType)
	{
		if(mPicture==eMaterialPicture_Image){
			if(mvImage[aType]==nullptr){
				Log("2: %d\n", aType);return nullptr;
			}
			return mvImage[aType]->GetTexture();
		}
		else
		{
			return mvTexture[aType];
		}
	}

	cRect2f iMaterial::GetTextureOffset(eMaterialTexture aType)
	{
		cRect2f SizeRect;

		if(mPicture==eMaterialPicture_Image)
		{
			tVertexVec VtxVec = mvImage[aType]->GetVertexVecCopy(0,0);

			SizeRect.x = VtxVec[0].tex.x;
			SizeRect.y = VtxVec[0].tex.y;
			SizeRect.w = VtxVec[2].tex.x - VtxVec[0].tex.x;
			SizeRect.h = VtxVec[2].tex.y - VtxVec[0].tex.y;
		}
		else
		{
			SizeRect.x=0;SizeRect.y=0;
			SizeRect.w = 1;//(float) mvTexture[aType]->GetWidth();
			SizeRect.h = 1;//(float) mvTexture[aType]->GetHeight();
		}

		return SizeRect;
	}
}
