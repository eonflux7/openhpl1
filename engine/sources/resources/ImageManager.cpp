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
#include "resources/ImageManager.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "resources/ResourceImage.h"
#include "resources/FrameBitmap.h"
#include "resources/FrameTexture.h"
#include "graphics/LowLevelGraphics.h"
#include "resources/LowLevelResources.h"


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cImageManager::cImageManager(cFileSearcher *apFileSearcher,iLowLevelGraphics *apLowLevelGraphics,
								iLowLevelResources *apLowLevelResources,iLowLevelSystem *apLowLevelSystem)
	: iResourceManager(apFileSearcher, apLowLevelResources,apLowLevelSystem)
	{
	   mpLowLevelGraphics = apLowLevelGraphics;

	   mpLowLevelResources->GetSupportedImageFormats(mlstFileFormats);

	   mvFrameSize = cVector2l(512,512);
	   mlFrameHandle = 0;
	}

	cImageManager::~cImageManager()
	{
		//DeleteAllBitmapFrames();
		DestroyAll();

		Log(" Done with images\n");
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iResourceBase* cImageManager::Create(const tString& asName)
	{
		return CreateInFrame(asName, -1);
	}
	//-----------------------------------------------------------------------

	iResourceBase* cImageManager::CreateInFrame(const tString& asName, int alFrameHandle)
	{
		cResourceImage *pImage = nullptr;
		tString sPath;

		BeginLoad(asName);

		pImage = FindImage(asName, sPath);
		if(!pImage)
		{
			if(sPath != "")
			{
				pImage = LoadHighRes(sPath, sPath);
				if(pImage==nullptr)
				{
					iBitmap2D *pBmp;
					pBmp = mpLowLevelResources->LoadBitmap2D(sPath);
					if(pBmp==nullptr){
						Error("Imagemanager Couldn't load bitmap '%s'\n", sPath.c_str());
						EndLoad();
						return nullptr;
					}

					pImage = AddToFrame(pBmp, alFrameHandle);

					hplDelete(pBmp);

					if(pImage==nullptr){
						Error("Imagemanager couldn't create image '%s'\n", asName.c_str());
					}
				}

				if(pImage) AddResource(pImage);
			}
			else
			{
				//Not one of the game's images: it may be a new one in the
				//high-resolution directory
				pImage = LoadHighRes(asName, "");
				if(pImage) AddResource(pImage);
			}
		}
		else
		{
			//Log("Found '%s' in stock!\n",asName.c_str());
		}

		if(pImage)pImage->IncUserCount();
		else Error("Couldn't load image '%s'\n",asName.c_str());

		//Log("Loaded image %s, it has %d users!\n", pImage->GetName().c_str(),pImage->GetUserCount());
		//Log(" frame has %d pics\n", pImage->GetFrameTexture()->GetPicCount());

		EndLoad();
		return pImage;
	}

	//-----------------------------------------------------------------------

	cResourceImage* cImageManager::CreateImage(const tString& asName, int alFrameHandle)
	{
		iResourceBase* pRes = CreateInFrame(asName, alFrameHandle);
		cResourceImage* pImage = static_cast<cResourceImage*>(pRes);

		return pImage;
	}

	//-----------------------------------------------------------------------

	cResourceImage* cImageManager::CreateFromBitmap(const tString &asName,iBitmap2D* apBmp, int alFrameHandle)
	{
		if(apBmp==nullptr)return nullptr;

		cResourceImage *pImage = AddToFrame(apBmp, alFrameHandle);

		if(pImage){
			AddResource(pImage, false);
			pImage->IncUserCount();
		}

		return pImage;
	}

	//-----------------------------------------------------------------------

	void cImageManager::Unload(iResourceBase* apResource)
	{

	}

	void cImageManager::Destroy(iResourceBase* apResource)
	{
		//Lower the user num for the the resource. If it is 0 then lower the
		//user num for the TextureFrame and delete the resource. If the Texture
		//frame reaches 0 it is deleted as well.
		cResourceImage *pImage = static_cast<cResourceImage*>(apResource);
		cFrameTexture *pFrame = pImage->GetFrameTexture();
		cFrameBitmap *pBmpFrame = pImage->GetFrameBitmap();

		//pImage->GetFrameBitmap()->FlushToTexture(); Not needed?


		//Log("Users Before: %d\n",pImage->GetUserCount());
		//Log("Framepics Before: %d\n",pFrame->GetPicCount());

		pImage->DecUserCount();//dec frame count as well.. is that ok?

		//Log("---\n");
		//Log("Destroyed Image: '%s' Users: %d\n",pImage->GetName().c_str(),pImage->GetUserCount());
		//Log("Frame %d has left Pics: %d\n",pFrame,pFrame->GetPicCount());

		if(pImage->HasUsers()==false)
		{
			pFrame->DecPicCount(); // Doing it here now instead.
			pBmpFrame->DecPicCount();
			RemoveResource(apResource);
			hplDelete(apResource);

			//Log("deleting image and dec fram to %d images!\n",pFrame->GetPicCount());
		}


		if(pFrame->IsEmpty())
		{
			//Log(" Deleting frame...");

			//Delete the bitmap frame that has this this frame.
			for(tFrameBitmapListIt it=mlstBitmapFrames.begin();it!=mlstBitmapFrames.end();++it)
			{
				cFrameBitmap *pBmpFrame = *it;
				if(pBmpFrame->GetFrameTexture() == pFrame)
				{
					//Log("and bitmap...");
					hplDelete(pBmpFrame);
					mlstBitmapFrames.erase(it);
					break;
				}
			}

			//delete from list
			m_mapTextureFrames.erase(pFrame->GetHandle());
			hplDelete(pFrame);
			//Log(" Deleted frame!\n");
		}
		//Log("---\n");

	}

	//-----------------------------------------------------------------------

	void cImageManager::DeleteAllBitmapFrames()
	{
		FlushAll();
		for(tFrameBitmapListIt it=mlstBitmapFrames.begin();it!=mlstBitmapFrames.end();)
		{
			hplDelete(*it);
			it = mlstBitmapFrames.erase(it);
		}
	}

	//-----------------------------------------------------------------------

	int cImageManager::FlushAll()
	{
		//Log("Flushing...");
		int lNum =0;
		for(tFrameBitmapListIt it=mlstBitmapFrames.begin();it!=mlstBitmapFrames.end();)
		{
			if((*it)->FlushToTexture()) lNum++;

			if((*it)->IsFull()){
				//Do not delete all, probably this struct needs to be here for easy access :S
				//hplDelete(*it);
				//it = mlstBitmapFrames.erase(it);
				it++;
			}
			else
			{
				it++;
			}
		}

		//Log("Done!\n");

		return lNum;
	}

	//-----------------------------------------------------------------------

	int cImageManager::CreateFrame(cVector2l avSize)
	{
		cFrameBitmap *pBFrame = CreateBitmapFrame(avSize);

		if(pBFrame==nullptr) return -1;

		return pBFrame->GetHandle();
	}

	//-----------------------------------------------------------------------

	void cImageManager::SetFrameLocked(int alHandle, bool abLocked)
	{
		tFrameBitmapListIt it = mlstBitmapFrames.begin();
		while(it != mlstBitmapFrames.end())
		{
			if((*it)->GetHandle() == alHandle){
				(*it)->SetLocked(abLocked);
				break;
			}
			it++;
		}
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cResourceImage *cImageManager::FindImage(const tString &asName, tString &asFilePath)
	{
		cResourceImage *pImage=nullptr;

		if(cString::GetFileExt(asName)=="")
		{
			for(tStringListIt it = mlstFileFormats.begin();it!=mlstFileFormats.end();++it)
			{
				tString sNewName = cString::SetFileExt(asName,*it);
				pImage = static_cast<cResourceImage*> (FindLoadedResource(sNewName, asFilePath));

				if((pImage==nullptr && asFilePath!="") || pImage!=nullptr)break;
			}
		}
		else
		{
			pImage = static_cast<cResourceImage*> (FindLoadedResource(asName, asFilePath));
		}

		return pImage;
	}

	//-----------------------------------------------------------------------

	cResourceImage *cImageManager::LoadHighRes(const tString &asName, const tString &asOriginalPath)
	{
		if(msHighResDir=="") return nullptr;

		tString sBase = cString::SetFileExt(cString::GetFileName(asName),"");
		const int vScales[] = {8,4,3,2};
		for(int lScale : vScales)
		{
			tString sFile = msHighResDir + sBase + "@" + cString::ToString(lScale) + "x.png";
			if(FileExists(cString::To16Char(sFile))==false) continue;

			//The original's size is the size the game lays the image out at. An
			//image the game does not have is laid out at 1/lScale of its size.
			cVector2l vLogicalSize(-1,-1);
			if(asOriginalPath != "")
			{
				iBitmap2D *pOriginal = mpLowLevelResources->LoadBitmap2D(asOriginalPath);
				if(pOriginal==nullptr) return nullptr;
				vLogicalSize = cVector2l(pOriginal->GetWidth(), pOriginal->GetHeight());
				hplDelete(pOriginal);
			}

			iBitmap2D *pBmp = mpLowLevelResources->LoadBitmap2D(sFile);
			if(pBmp==nullptr){
				Error("Couldn't load high-resolution image '%s'\n", sFile.c_str());
				return nullptr;
			}

			//The replacement is lScale times the original plus, optionally, an equal
			//margin (in the original's pixels) on opposite sides.
			cVector2l vMargin(0,0);
			bool bFits = pBmp->GetWidth() % lScale == 0 && pBmp->GetHeight() % lScale == 0;
			if(vLogicalSize.x < 0) vLogicalSize = cVector2l(pBmp->GetWidth()/lScale, pBmp->GetHeight()/lScale);
			if(bFits)
			{
				cVector2l vExtra(pBmp->GetWidth()/lScale - vLogicalSize.x, pBmp->GetHeight()/lScale - vLogicalSize.y);
				vMargin = cVector2l(vExtra.x/2, vExtra.y/2);
				bFits = vExtra.x >= 0 && vExtra.y >= 0 && vExtra.x % 2 == 0 && vExtra.y % 2 == 0;
			}
			if(bFits==false)
			{
				Warning("'%s' is %dx%d, not %d times %dx%d plus an even margin. Not using it.\n", sFile.c_str(),
						pBmp->GetWidth(), pBmp->GetHeight(), lScale, vLogicalSize.x, vLogicalSize.y);
				hplDelete(pBmp);
				return nullptr;
			}

			//The replacement gets its own mipmapped frame so that it is filtered down
			//cleanly and nothing bleeds in from neighbours. Keeping the original path
			//keeps the resource name the game asks for; a new image is named
			//"name.png".
			pBmp->SetPath(asOriginalPath != "" ? asOriginalPath : msHighResDir + sBase + ".png");
			cFrameBitmap *pFrame = CreateBitmapFrame(cVector2l(pBmp->GetWidth()+2, pBmp->GetHeight()+2), true);
			pFrame->SetLocked(true);
			cResourceImage *pImage = pFrame->AddBitmap(pBmp);
			hplDelete(pBmp);

			if(pImage) pImage->SetLogicalSize(vLogicalSize, vMargin);
			return pImage;
		}

		return nullptr;
	}

	//-----------------------------------------------------------------------

	cResourceImage *cImageManager::AddToFrame(iBitmap2D *apBmp,int alFrameHandle)
	{
		bool bFound = false;
		cResourceImage *pImage=nullptr;

		if(mlstBitmapFrames.size()==0){
			CreateBitmapFrame(mvFrameSize);
		}

		if(alFrameHandle<0)
		{
			//Search the frames til one is find that fits the bitmap
			for(tFrameBitmapListIt it=mlstBitmapFrames.begin();it!=mlstBitmapFrames.end();it++)
			{
				if(!(*it)->IsFull() && !(*it)->IsLocked()){
					pImage = (*it)->AddBitmap(apBmp);
					if(pImage!=nullptr)
					{
						bFound = true;
						break;
					}
				}
			}

			//If it fitted in none of the frames, create a new and put it in that.
			if(!bFound)
			{
				//Log("No fit!\n");
				//not 100% it fits in this one...if so maybe the bitmap size of the frame
				//should be changed?

				//pImage = CreateBitmapFrame(mvFrameSize)->AddBitmap(apBmp);
				cFrameBitmap * pFrame = CreateBitmapFrame(mvFrameSize);
				if(pFrame)
				{
					pImage = pFrame->AddBitmap(apBmp);
					if(pImage==nullptr)
					{
						Log("No fit in new frame!\n");
					}
				}
			}
		}
		else
		{
			tFrameBitmapListIt it = mlstBitmapFrames.begin();
			while(it != mlstBitmapFrames.end())
			{
				if((*it)->GetHandle() == alFrameHandle)
				{
					pImage = (*it)->AddBitmap(apBmp);
					break;
				}
				it++;
			}
			if(pImage==nullptr)
				Error("Image didn't fit frame %d!\n", alFrameHandle);
		}

		return pImage;
	}

	//-----------------------------------------------------------------------

	cFrameBitmap *cImageManager::CreateBitmapFrame(cVector2l avSize, bool abMipMaps)
	{
		iTexture *pTex = mpLowLevelGraphics->CreateTexture(abMipMaps,eTextureType_Normal,eTextureTarget_2D);
		cFrameTexture *pTFrame = hplNew( cFrameTexture, (pTex,mlFrameHandle) );
		iBitmap2D *pBmp = mpLowLevelGraphics->CreateBitmap2D(avSize, 32);
		cFrameBitmap *pBFrame = hplNew(  cFrameBitmap, (pBmp,pTFrame,mlFrameHandle) );

		mlstBitmapFrames.push_back(pBFrame);

		std::pair<tFrameTextureMap::iterator, bool> ret = m_mapTextureFrames.insert(tFrameTextureMap::value_type(mlFrameHandle, pTFrame));
		if(ret.second == false)
		{
			Error("Could not add texture frame %d with handle %d! Handle already exist!\n",pTFrame, mlFrameHandle);
		}
		else
		{
			//Log("Added texture frame: %d\n",pTFrame);
		}

		mlFrameHandle++;
		return pBFrame;
	}


	//-----------------------------------------------------------------------
}
