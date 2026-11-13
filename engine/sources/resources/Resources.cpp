// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "resources/Resources.h"

#include "resources/LowLevelResources.h"
#include "resources/FileSearcher.h"
#include "resources/ImageManager.h"
#include "resources/GpuProgramManager.h"
#include "resources/ParticleManager.h"
#include "resources/SoundManager.h"
#include "resources/FontManager.h"
#include "resources/ScriptManager.h"
#include "resources/TextureManager.h"
#include "resources/MaterialManager.h"
#include "resources/MeshManager.h"
#include "resources/MeshLoaderHandler.h"
#include "resources/SoundEntityManager.h"
#include "resources/AnimationManager.h"
#include "resources/ConfigFile.h"
#include "resources/LanguageFile.h"
#include "system/System.h"

#include "system/LowLevelSystem.h"

#include "tinyxml.h"

namespace hpl {

	cResources::cResources(iLowLevelResources *apLowLevelResources,iLowLevelGraphics *apLowLevelGraphics)
		: iUpdateable("Resources")
	{
		mpLowLevelResources = apLowLevelResources;
		mpLowLevelGraphics = apLowLevelGraphics;

		mpFileSearcher = new cFileSearcher(mpLowLevelResources);

		//Created by Init (deleting nullptr does nothing if Init was not called)
		mpLowLevelSystem = nullptr;
		mpImageManager = nullptr;
		mpGpuProgramManager = nullptr;
		mpParticleManager = nullptr;
		mpSoundManager = nullptr;
		mpFontManager = nullptr;
		mpScriptManager = nullptr;
		mpTextureManager = nullptr;
		mpMaterialManager = nullptr;
		mpSoundEntityManager = nullptr;
		mpAnimationManager = nullptr;
		mpMeshManager = nullptr;
		mpMeshLoaderHandler = nullptr;

		mpDefaultEntity3DLoader = nullptr;
		mpDefaultArea3DLoader = nullptr;

		mpLanguageFile = nullptr;
	}

	cResources::~cResources()
	{
		Log("Exiting Resources Module\n");
		Log("--------------------------------------------------------\n");

		STLMapDeleteAll(m_mEntity3DLoaders);
		STLMapDeleteAll(m_mArea3DLoaders);

		delete mpFontManager;
		delete mpScriptManager;
		delete mpParticleManager;
		delete mpSoundManager;
		delete mpMeshManager;
		delete mpMaterialManager;
		delete mpGpuProgramManager;
		delete mpImageManager;
		delete mpTextureManager;
		delete mpSoundEntityManager;
		delete mpAnimationManager;

		Log(" All resources deleted\n");

		delete mpFileSearcher;
		delete mpMeshLoaderHandler;

		if(mpLanguageFile) delete mpLanguageFile;

		mlstManagers.clear();
		Log("--------------------------------------------------------\n\n");
	}

	void cResources::Init(	cGraphics* apGraphics,cSystem *apSystem, cSound* apSound, cScene *apScene,
							cGui *apGui)
	{
		Log("Initializing Resources Module\n");
		Log("--------------------------------------------------------\n");

		mpLowLevelSystem = apSystem->GetLowLevel();

		Log(" Creating resource managers\n");

		mpImageManager = new cImageManager(mpFileSearcher,mpLowLevelGraphics,mpLowLevelResources,mpLowLevelSystem);
		mlstManagers.push_back(mpImageManager);
		mpGpuProgramManager = new cGpuProgramManager(mpFileSearcher,mpLowLevelGraphics,mpLowLevelResources,mpLowLevelSystem);
		mlstManagers.push_back(mpGpuProgramManager);
		mpParticleManager = new cParticleManager(apGraphics, this);
		mlstManagers.push_back(mpParticleManager);
		mpSoundManager = new cSoundManager(apSound, this);
		mlstManagers.push_back(mpParticleManager);
		mpFontManager = new cFontManager(apGraphics,apGui, this);
		mlstManagers.push_back(mpFontManager);
		mpScriptManager = new cScriptManager(apSystem, this);
		mlstManagers.push_back(mpScriptManager);
		mpTextureManager = new cTextureManager(apGraphics, this);
		mlstManagers.push_back(mpTextureManager);
		mpMaterialManager = new cMaterialManager(apGraphics, this);
		mlstManagers.push_back(mpMaterialManager);
		mpMeshManager = new cMeshManager(apGraphics, this);
		mlstManagers.push_back(mpMeshManager);
		mpSoundEntityManager = new cSoundEntityManager(apSound, this);
		mlstManagers.push_back(mpSoundEntityManager);
		mpAnimationManager = new cAnimationManager(apGraphics, this);
		mlstManagers.push_back(mpAnimationManager);

		Log(" Misc Creation\n");

		mpMeshLoaderHandler = new cMeshLoaderHandler(this, apScene);

		mpLowLevelResources->AddMeshLoaders(mpMeshLoaderHandler);
		//Models converted to glTF (from addons) are used in place of the
		//Collada files that maps, entities and scripts ask for.
		mpFileSearcher->SetAlternativeExtensions("dae", {"glb", "gltf"});

		Log("--------------------------------------------------------\n\n");
	}

	void cResources::Update(float afTimeStep)
	{
		tResourceManagerListIt it = mlstManagers.begin();
		for(; it != mlstManagers.end(); ++it)
		{
			iResourceManager *pManager = *it;

			pManager->Update(afTimeStep);
		}
	}

	cFileSearcher* cResources::GetFileSearcher()
	{
		return mpFileSearcher;
	}

	/**
	 * \todo File searcher should check so if the dir is allready added and if so return false and not add
	 * \param &asDir
	 * \param &asMask
	 * \return
	 */
	bool cResources::AddResourceDir(const tString &asDir, const tString &asMask)
	{
		mpFileSearcher->AddDirectory(asDir, asMask);
		if(iResourceBase::GetLogCreateAndDelete())
			Log(" Added resource directory '%s'\n",asDir.c_str());
		return true;
	}

	void cResources::ClearResourceDirs()
	{
		mpFileSearcher->ClearDirectories();
	}

	bool cResources::SetLanguageFile(const tString &asFile)
	{
		if(mpLanguageFile){
			delete mpLanguageFile;
			mpLanguageFile = nullptr;
		}

		tString asPath = mpFileSearcher->GetFilePath(asFile);
		if(asPath=="")
		{
			Error("Couldn't load language file '%s'\n",asFile.c_str());
			return false;
		}

		mpLanguageFile = new cLanguageFile(this);

		return mpLanguageFile->LoadFromFile(asPath);
	}

	const tWString& cResources::Translate(const tString& asCat, const tString& asName)
	{
		if(mpLanguageFile)
		{
			return mpLanguageFile->Translate(asCat,asName);
		}
		else
		{
			return mwsEmptyString;
		}
	}

	void cResources::AddEntity3DLoader(iEntity3DLoader* apLoader, bool abSetAsDefault)
	{
		m_mEntity3DLoaders.insert(tEntity3DLoaderMap::value_type(apLoader->GetName(), apLoader));

		if(abSetAsDefault){
			mpDefaultEntity3DLoader = apLoader;
		}
	}

	iEntity3DLoader* cResources::GetEntity3DLoader(const tString& asName)
	{
		tEntity3DLoaderMapIt it = m_mEntity3DLoaders.find(asName);
		if(it == m_mEntity3DLoaders.end()){
			Warning("No loader for type '%s' found!\n",asName.c_str());

			if(mpDefaultEntity3DLoader){
				Log("Using default loader!\n");
				return mpDefaultEntity3DLoader;
			}
			else {
				return nullptr;
			}
		}

		return it->second;
	}

	void iArea3DLoader::SplitScale(cVector3f &avSize, cMatrixf &a_mtxTransform)
	{
		for(int lAxis=0; lAxis<3; ++lAxis)
		{
			float fScale = std::sqrt(a_mtxTransform.m[0][lAxis]*a_mtxTransform.m[0][lAxis] +
									a_mtxTransform.m[1][lAxis]*a_mtxTransform.m[1][lAxis] +
									a_mtxTransform.m[2][lAxis]*a_mtxTransform.m[2][lAxis]);
			if(fScale < kEpsilonf || std::abs(fScale - 1.0f) < 1e-4f) continue;

			for(int lRow=0; lRow<3; ++lRow) a_mtxTransform.m[lRow][lAxis] /= fScale;
			avSize.v[lAxis] *= fScale;
		}
	}

	void cResources::AddArea3DLoader(iArea3DLoader* apLoader, bool abSetAsDefault)
	{
		m_mArea3DLoaders.insert(tArea3DLoaderMap::value_type(apLoader->GetName(), apLoader));

		if(abSetAsDefault){
			mpDefaultArea3DLoader = apLoader;
		}
	}

	iArea3DLoader* cResources::GetArea3DLoader(const tString& asName)
	{
		tArea3DLoaderMapIt it = m_mArea3DLoaders.find(asName);
		if(it == m_mArea3DLoaders.end()){
			Warning("No loader for area type '%s' found!\n",asName.c_str());

			if(mpDefaultArea3DLoader){
				Log("Using default loader!\n");
				return mpDefaultArea3DLoader;
			}
			else {
				return nullptr;
			}
		}

		return it->second;
	}

	bool cResources::LoadResourceDirsFile(const tString &asFile)
	{
		TiXmlDocument* pXmlDoc = new TiXmlDocument(asFile.c_str());
		if(pXmlDoc->LoadFile()==false)
		{
			Error("Couldn't load XML file '%s'!\n",asFile.c_str());
			delete pXmlDoc;
			return false;
		}

		//Get the root.
		TiXmlElement* pRootElem = pXmlDoc->RootElement();

		TiXmlElement* pChildElem = pRootElem->FirstChildElement();
		for(; pChildElem != nullptr; pChildElem = pChildElem->NextSiblingElement())
		{
			tString sPath = cString::ToString(pChildElem->Attribute("Path"),"");
			if(sPath==""){
				continue;
			}

			if(sPath[0]=='/' || sPath[0]=='\\') sPath = sPath.substr(1);

			AddResourceDir(sPath);
		}

		delete pXmlDoc;
		return true;
	}

	iLowLevelResources* cResources::GetLowLevel()
	{
		return mpLowLevelResources;
	}
}
