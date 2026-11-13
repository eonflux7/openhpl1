// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/ParticleEmitter.h"
#include "system/LowLevelSystem.h"
#include "resources/Resources.h"
#include "graphics/Graphics.h"
#include "graphics/MaterialHandler.h"
#include "resources/ImageManager.h"
#include "resources/MaterialManager.h"


namespace hpl {

	// DATA LOADER

	iParticleEmitterData::iParticleEmitterData(const tString &asName,cResources* apResources,
											cGraphics *apGraphics)
	{
		msName = asName;
		mpResources = apResources;
		mpGraphics = apGraphics;

		mfWarmUpTime =0;
		mfWarmUpStepsPerSec = 20;
	}

	iParticleEmitterData::~iParticleEmitterData()
	{
		for(int i=0;i<(int)mvMaterials.size();i++)
		{
			if(mvMaterials[i]) mpResources->GetMaterialManager()->Destroy(mvMaterials[i]);
		}
	}

	void iParticleEmitterData::AddMaterial(iMaterial *apMaterial)
	{
		mvMaterials.push_back(apMaterial);
	}

	iParticleEmitter::iParticleEmitter(tMaterialVec *avMaterials,
									unsigned int alMaxParticles, cVector3f avSize,
									cGraphics *apGraphics, cResources *apResources)
	{
		mpGraphics = apGraphics;
		mpResources = apResources;

		mvParticles.resize(alMaxParticles);
		for(int i=0;i<(int)alMaxParticles;i++)
		{
			mvParticles[i] = new cParticle();
		}
		mlMaxParticles = alMaxParticles;
		mlNumOfParticles =0;

		mvMaterials = avMaterials;

		//Update vars:
		mbDying = false;
		mfTime =0;
		mfFrame =0;

		mbUpdateGfx = true;
		mbUpdateBV = true;
	}

	iParticleEmitter::~iParticleEmitter()
	{
		for(int i=0;i<(int)mvParticles.size();i++){
			delete mvParticles[i];
		}
	}

	void iParticleEmitter::Update(float afTimeStep)
	{
		UpdateMotion(afTimeStep);

		mfTime++;
		mbUpdateGfx = true;
		mbUpdateBV = true;
	}

	void iParticleEmitter::KillInstantly()
	{
		mlMaxParticles = 0;
		mlNumOfParticles = 0;
		mbDying = true;
	}

	cParticle* iParticleEmitter::CreateParticle()
	{
		if(mlNumOfParticles == mlMaxParticles) return nullptr;
		++mlNumOfParticles;
		return mvParticles[mlNumOfParticles-1];
	}

	void iParticleEmitter::SwapRemove(unsigned int alIndex)
	{
		if(alIndex < mlNumOfParticles-1)
		{
			cParticle* pTemp = mvParticles[alIndex];
			mvParticles[alIndex] = mvParticles[mlNumOfParticles-1];
			mvParticles[mlNumOfParticles-1] = pTemp;
		}
		mlNumOfParticles--;
	}
}
