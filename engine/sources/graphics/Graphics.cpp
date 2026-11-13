// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Graphics.h"

#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "resources/LowLevelResources.h"
#include "graphics/GraphicsDrawer.h"
#include "graphics/Renderer3D.h"
#include "graphics/RendererPostEffects.h"
#include "graphics/RenderList.h"
#include "graphics/MaterialHandler.h"
#include "graphics/MeshCreator.h"
#include "game/Updateable.h"
#include "resources/Resources.h"

//2D Materials
#include "graphics/Material_DiffuseAdditive2D.h"
#include "graphics/Material_DiffuseAlpha2D.h"
#include "graphics/Material_Smoke2D.h"
#include "graphics/Material_FontNormal.h"

//3D Materials
#include "graphics/Material_Diffuse.h"
#include "graphics/Material_DiffuseSpec.h"
#include "graphics/Material_Bump.h"
#include "graphics/Material_BumpSpec.h"
#include "graphics/Material_BumpColorSpec.h"
#include "graphics/Material_Flat.h"
#include "graphics/Material_Translucent.h"
#include "graphics/Material_EnvMap_Reflect.h"
#include "graphics/Material_Water.h"

namespace hpl {

	cGraphics::cGraphics(iLowLevelGraphics *apLowLevelGraphics, iLowLevelResources *apLowLevelResources)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		mpLowLevelResources = apLowLevelResources;

		mpDrawer = nullptr;
		mpMeshCreator = nullptr;
		mpMaterialHandler = nullptr;
		mpRenderer3D = nullptr;
		mpRendererPostEffects = nullptr;
		mpRenderList = nullptr;
	}

	cGraphics::~cGraphics()
	{
		Log("Exiting Graphics Module\n");
		Log("--------------------------------------------------------\n");

		delete mpRenderer3D;
		delete mpRendererPostEffects;
		delete mpDrawer;
		delete mpMeshCreator;
		delete mpMaterialHandler;
		delete mpRenderList;

		Log("--------------------------------------------------------\n\n");
	}

	bool cGraphics::Init(	int alWidth, int alHeight, int alBpp, int abFullscreen,
							int alMultisampling,const tString &asWindowCaption,
							cResources* apResources, int alDisplay)
	{
		Log("Initializing Graphics Module\n");
		Log("--------------------------------------------------------\n");

		//Setup the graphic directories:
		apResources->AddResourceDir("core/programs");
		apResources->AddResourceDir("core/textures");

		Log(" Init low level graphics\n");
		mpLowLevelGraphics->Init(alWidth,alHeight,alBpp,abFullscreen,alMultisampling,asWindowCaption,alDisplay);

		Log(" Creating graphic systems\n");
		mpMaterialHandler = new cMaterialHandler(this, apResources);
		mpDrawer = new cGraphicsDrawer(mpLowLevelGraphics,mpMaterialHandler,apResources);
		mpRenderList = new cRenderList(this);
		mpMeshCreator = new cMeshCreator(mpLowLevelGraphics, apResources);
		mpRenderer3D = new cRenderer3D(mpLowLevelGraphics,apResources,mpMeshCreator,mpRenderList);
		mpRendererPostEffects = new cRendererPostEffects(mpLowLevelGraphics,apResources,mpRenderList,
														mpRenderer3D);
		mpRenderer3D->SetPostEffects(mpRendererPostEffects);


		//Add all the materials.
		//2D
		Log(" Adding engine materials\n");
		mpMaterialHandler->Add(new cMaterialType_DiffuseAdditive2D() );
		mpMaterialHandler->Add(new cMaterialType_DiffuseAlpha2D() );
		mpMaterialHandler->Add(new cMaterialType_Smoke2D() );
		mpMaterialHandler->Add(new cMaterialType_FontNormal() );

		//3D
		mpMaterialHandler->Add(new cMaterialType_Diffuse() );
		mpMaterialHandler->Add(new cMaterialType_Bump() );
		mpMaterialHandler->Add(new cMaterialType_DiffuseSpec() );
		mpMaterialHandler->Add(new cMaterialType_BumpSpec() );
		mpMaterialHandler->Add(new cMaterialType_BumpColorSpec() );

		mpMaterialHandler->Add(new cMaterialType_Translucent({"additive", eMaterialBlendMode_Add, "", false}) );
		mpMaterialHandler->Add(new cMaterialType_Translucent({"alpha", eMaterialBlendMode_Alpha, "", true}) );
		mpMaterialHandler->Add(new cMaterialType_Flat() );
		mpMaterialHandler->Add(new cMaterialType_Translucent({"modulative", eMaterialBlendMode_Mul, "MOD", false}) );
		mpMaterialHandler->Add(new cMaterialType_Translucent({"modulativex2", eMaterialBlendMode_MulX2, "MODX2", false}) );

		mpMaterialHandler->Add(new cMaterialType_EnvMap_Reflect() );

		mpMaterialHandler->Add(new cMaterialType_Water() );

		Log("--------------------------------------------------------\n\n");

		return true;
	}

	void cGraphics::SetRenderScale(float afScale)
	{
		cVector2f vOldSize = mpLowLevelGraphics->GetRenderSize();
		mpLowLevelGraphics->SetRenderScale(afScale);
		if(mpLowLevelGraphics->GetRenderSize() != vOldSize)
			mpRendererPostEffects->CreateScreenBuffers();
	}

	bool cGraphics::ApplyWindowSize()
	{
		if(mpLowLevelGraphics->ApplyWindowSize() == false) return false;
		mpRendererPostEffects->CreateScreenBuffers();
		return true;
	}

	float cGraphics::GetRenderScale()
	{
		return mpLowLevelGraphics->GetRenderScale();
	}

	iLowLevelGraphics* cGraphics::GetLowLevel()
	{
		return mpLowLevelGraphics;
	}

	cGraphicsDrawer* cGraphics::GetDrawer()
	{
		return mpDrawer;
	}
}
