/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "core/GraphicsHelper.h"

#include "core/Init.h"
#include <SDL3/SDL.h>

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGraphicsHelper::cGraphicsHelper(cInit *apInit)
{
	mpInit = apInit;

	mpLowLevelGfx = mpInit->mpGame->GetGraphics()->GetLowLevel();
	mpTexManager = mpInit->mpGame->GetResources()->GetTextureManager();

	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();

	mpFont =mpInit->mpGame->GetResources()->GetFontManager()->CreateFontData("font_menu_small.fnt");

	mvVtx.resize(4);

	// Keys named in tips and the inventory are drawn on a key, from
	// hud/src/hud_keycap.svg (16x16, ends of 6). Plain names with the
	// game's own images.
	mpKeyCap = nullptr;
	if(mpInit->mbHighResHud)
		mpKeyCap = mpDrawer->CreateGfxObject("hud_keycap","diffalpha2d");
	if(mpKeyCap)
		iFontData::SetKeyCapGfx(mpKeyCap, 6.0f/16.0f, 1.0f);
}

cGraphicsHelper::~cGraphicsHelper()
{
	if(mpKeyCap)
	{
		iFontData::SetKeyCapGfx(nullptr, 0, 0);
		mpDrawer->DestroyGfxObject(mpKeyCap);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGraphicsHelper::ClearScreen(const cColor &aColor)
{
	mpLowLevelGfx->SetClearColor(aColor);
	mpLowLevelGfx->ClearScreen();
}

//-----------------------------------------------------------------------

void cGraphicsHelper::DrawTexture(iTexture *apTex, const cVector3f& avPos, const cVector3f& avSize,
								  const cColor &aColor)
{
	DrawTexture(apTex, avPos, avSize, aColor, eBlendFunc_One, eBlendFunc_Zero);
}

void cGraphicsHelper::DrawTexture(iTexture *apTex, const cVector3f& avPos, const cVector3f& avSize,
								  const cColor &aColor, eBlendFunc aSrcBlend, eBlendFunc aDestBlend)
{
	DrawTextureRect(apTex, avPos, avSize, cVector2f(0.01f,0.01f), cVector2f(0.99f,0.99f), aColor,
					aSrcBlend, aDestBlend);
}

void cGraphicsHelper::DrawTextureRect(iTexture *apTex, const cVector3f& avPos, const cVector3f& avSize,
									  const cVector2f& avUVMin, const cVector2f& avUVMax, const cColor &aColor,
									  eBlendFunc aSrcBlend, eBlendFunc aDestBlend)
{
	bool bBlend = aSrcBlend != eBlendFunc_One || aDestBlend != eBlendFunc_Zero;

	mpLowLevelGfx->SetDepthTestActive(false);
	const cDrawMatrices previousMatrices = mpLowLevelGfx->GetDrawMatrices();
	mpLowLevelGfx->SetDrawMatrices(cMatrixf::Identity, cMatrixf::Identity,
		mpLowLevelGfx->CreateVirtualOrthoProjection(-1000, 1000));

	mvVtx[0] = cVertex(avPos + cVector3f(0,0,0),	cVector2f(avUVMin.x,avUVMin.y),aColor );
	mvVtx[1] = cVertex(avPos + cVector3f(avSize.x,0,0),	cVector2f(avUVMax.x,avUVMin.y),aColor);
	mvVtx[2] = cVertex(avPos + cVector3f(avSize.x,avSize.y,0),cVector2f(avUVMax.x,avUVMax.y),aColor);
	mvVtx[3] = cVertex(avPos +  cVector3f(0,avSize.y,0),	cVector2f(avUVMin.x,avUVMax.y),aColor);

	
	for(int i=0;i<6;++i) mpLowLevelGfx->SetTexture(i,nullptr);

	cSpriteBatch batch;
	batch.Add(apTex, cSpriteBlend(aSrcBlend,aDestBlend,bBlend), mvVtx);
	batch.Flush(mpLowLevelGfx);

	mpLowLevelGfx->SetTexture(0,nullptr);

	if(bBlend) mpLowLevelGfx->SetBlendActive(false);

	mpLowLevelGfx->SetDrawMatrices(previousMatrices);
}

//-----------------------------------------------------------------------

cScreenImage cGraphicsHelper::LoadScreenImage(const tString &asFile)
{
	cScreenImage image;

	if(mpInit->mbHighResHud && asFile != "")
	{
		// Black Plague's (and Requiem's) images whose names Overture uses for
		// another image are in their own directories, as in hud/
		tString sDir = tString(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "hud/screens/";
		tStringVec vDirs;
		if(cInit::GetGameId() == eGameId_Requiem) vDirs.push_back(sDir + "requiem/");
		if(cInit::IsOverture()==false) vDirs.push_back(sDir + "blackplague/");
		vDirs.push_back(sDir);

		tString sName = cString::SetFileExt(cString::GetFileName(asFile),"jpg");
		for(const tString &sPath : vDirs)
		{
			tString sFile = sPath + sName;
			if(FileExists(cString::To16Char(sFile))==false) continue;

			iBitmap2D *pBmp = mpInit->mpGame->GetResources()->GetLowLevel()->LoadBitmap2D(sFile);
			iTexture *pTex = pBmp ? mpLowLevelGfx->CreateTexture(true,eTextureType_Normal,eTextureTarget_2D) : nullptr;
			if(pTex && pTex->CreateFromBitmap(pBmp)==false)
			{
				hplDelete(pTex);
				pTex = nullptr;
			}
			if(pBmp)
			{
				hplDelete(pBmp);
			}
			if(pTex==nullptr)
			{
				Error("Couldn't load widescreen image '%s'\n", sFile.c_str());
				break;
			}

			// Drawn edge to edge
			pTex->SetWrapS(eTextureWrap_ClampToEdge);
			pTex->SetWrapT(eTextureWrap_ClampToEdge);
			image.mpTexture = pTex;
			image.mbWide = true;
			return image;
		}
	}

	if(asFile != "") image.mpTexture = mpTexManager->Create2D(asFile,false);
	return image;
}

void cGraphicsHelper::DestroyScreenImage(cScreenImage &aImage)
{
	if(aImage.mbWide)
	{
		hplDelete(aImage.mpTexture);
	}
	else if(aImage.mpTexture)
	{
		mpTexManager->Destroy(aImage.mpTexture);
	}
	aImage = cScreenImage();
}

void cGraphicsHelper::DrawScreenImage(const cScreenImage &aImage, const cVector3f& avPos, const cVector2f& avSize)
{
	iTexture *pTex = aImage.mpTexture;
	if(pTex==nullptr) return;

	if(aImage.mbWide==false)
	{
		DrawTexture(pTex,avPos,cVector3f(avSize.x,avSize.y,0),cColor(1,1));
		return;
	}

	float fWidth = avSize.y * (float)pTex->GetWidth() / (float)pTex->GetHeight();
	cVector3f vPos(avPos.x + (avSize.x - fWidth)*0.5f, avPos.y, avPos.z);
	DrawTextureRect(pTex, vPos, cVector3f(fWidth,avSize.y,0), cVector2f(0,0), cVector2f(1,1), cColor(1,1),
					eBlendFunc_One, eBlendFunc_Zero);
}

void cGraphicsHelper::DrawFullScreenImage(const cScreenImage &aImage, float afZ)
{
	if(aImage.mbWide) DrawScreenImage(aImage,cVector3f(0,0,afZ),cVector2f(800,600));
	else if(aImage.mpTexture) DrawTexture(aImage.mpTexture,GetScreenPos(afZ),GetScreenSize(),cColor(1,1));
}

//-----------------------------------------------------------------------

void cGraphicsHelper::DrawLoadingScreen(const tString &asFile, bool abDrawLoadingText)
{
	cScreenImage image = LoadScreenImage(asFile);
	
	if(image.mpTexture || asFile == "")
	{
		ClearScreen(cColor(0,0));
		
		DrawFullScreenImage(image);
		
		if(abDrawLoadingText)
			mpFont->Draw(cVector3f(400,300,50),22,cColor(1,1),eFontAlign_Center,
						_W("%ls"),kTranslate("LoadTexts", "Loading").c_str());

		mpDrawer->DrawAll();
		
		SwapBuffers();
	}
	
	DestroyScreenImage(image);
}

//-----------------------------------------------------------------------

cVector3f cGraphicsHelper::GetScreenPos(float afZ)
{
	cVector2f vOffset = mpLowLevelGfx->GetVirtualOffset();
	return cVector3f(-vOffset.x, -vOffset.y, afZ);
}

cVector2f cGraphicsHelper::GetScreenSize()
{
	return mpLowLevelGfx->GetVirtualSize() + mpLowLevelGfx->GetVirtualOffset()*2;
}

cVector2f cGraphicsHelper::GetScreenOffset()
{
	return mpLowLevelGfx->GetVirtualOffset();
}

void cGraphicsHelper::ClampToScreen(cVector2f &avPos)
{
	cVector2f vOffset = mpLowLevelGfx->GetVirtualOffset();
	cVector2f vSize = mpLowLevelGfx->GetVirtualSize();

	if(avPos.x < -vOffset.x) avPos.x = -vOffset.x;
	if(avPos.x > vSize.x + vOffset.x) avPos.x = vSize.x + vOffset.x;
	if(avPos.y < -vOffset.y) avPos.y = -vOffset.y;
	if(avPos.y > vSize.y + vOffset.y) avPos.y = vSize.y + vOffset.y;
}

//-----------------------------------------------------------------------

void cGraphicsHelper::SwapBuffers()
{
	mpLowLevelGfx->SwapBuffers();
}


//-----------------------------------------------------------------------
