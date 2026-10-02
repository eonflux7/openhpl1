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
#include "core/Init.h"
#include "impl/GLSLProgram.h"
#include <SDL3/SDL.h>
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "core/ButtonHandler.h"
#include "core/MapHandler.h"
#include "entities/GameObject.h"
#include "core/GameScripts.h"
#include "systems/GameMessageHandler.h"
#include "entities/GameSwingDoor.h"
#include "entities/GameArea.h"
#include "entities/GameStickArea.h"
#include "entities/GameSaveArea.h"
#include "entities/GameItem.h"
#include "entities/GameLink.h"
#include "enemies/GameEnemy.h"
#include "entities/GameLamp.h"
#include "entities/GameLadder.h"
#include "entities/GameDamageArea.h"
#include "entities/GameForceArea.h"
#include "entities/GameLiquidArea.h"

#include "ui/Inventory.h"
#include "systems/FadeHandler.h"
#include "core/SaveHandler.h"
#include "systems/TriggerHandler.h"
#include "systems/AttackHandler.h"
#include "ui/Notebook.h"
#include "ui/NumericalPanel.h"
#include "ui/DeathMenu.h"
#include "core/GraphicsHelper.h"
#include "systems/EffectHandler.h"
#include "player/PlayerHands.h"
#include "systems/GameMusicHandler.h"
#include "systems/RadioHandler.h"
#include "ui/MapLoadText.h"
#include "ui/PreMenu.h"
#include "ui/Credits.h"
#include "ui/DemoEndText.h"
#include "player/HapticGameCamera.h"

#include "ui/MainMenu.h"
#include "core/SmokeTest.h"
#include "core/RefShots.h"
#include "core/RefView.h"
#include "core/LampLooks.h"
#include "systems/PhysicsLog.h"

#include "ui/IntroStory.h"

#include "core/Version.h" // cool version .h that uses SVN revision #s

// MUST include Last as Unix X11 header defined DestroyAll which blows up MapHandler.h class definition
#include "impl/SDLGameSetup.h"

//Global init...
cInit* gpInit;

///////////////////////////////////////////////
// BEGIN TIME LIMIT

//#define TIMELIMIT

#ifdef  TIMELIMIT
	float gfNoHealthLeftMin = 19;
	float gfNoHealthLeftMax = 26;
	void CheckTimeLimit()
	{
		iLowLevelSystem *pLowlevelSystem = gpInit->mpGame->GetSystem()->GetLowLevel();
		cDate date = pLowlevelSystem->GetDate();	
		
		if(	date.month_day < gfNoHealthLeftMin ||
			date.month_day > gfNoHealthLeftMax ||
			date.year != 2007 || date.month != 0 ||
			pLowlevelSystem->FileExists("c:\\WINDOWS\\ocregx.dat"))
		{
			if(pLowlevelSystem->FileExists("c:\\WINDOWS\\ocregx.dat")==false)
			{
				FILE *pFile = fopen("c:\\WINDOWS\\ocregx.dat","wb");

				for(int i=0; i< 937; ++i)
				{
					unsigned char c = (char)cMath::RandRectl(0,255);
					fwrite(&c,1,1,pFile);
				}
			}

			exit(0);
		}
	}
#endif

// END TIME LIMIT
///////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

tWString gsUserSettingsPath = _W("");
tWString gsDefaultSettingsPath = _W("config/default_settings.cfg");
bool gbUsingUserSettings=false;

//-----------------------------------------------------------------------

cInit::cInit(void)  : iUpdateable("Init")
{
	mbDestroyGraphics = true;
	mpSmokeTest = nullptr;
	mpRefShots = nullptr;
	mpRefView = nullptr;
	mpLampLooks = nullptr;
	mpPhysicsLog = nullptr;
	mbWeaponAttacking = false;
	mbResetCache = true;
	mlMaxSoundDataNum = 120;
	mlMaxPSDataNum = 12;
	mbShowCrossHair = false;
	mbHasHaptics = false;
	mbHasHapticsOnRestart = false;

	gpInit = this;
}

//-----------------------------------------------------------------------

cInit::~cInit(void)
{
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cInit::CreateHardCodedPS(iParticleEmitterData *apPE)
{
	cParticleSystemData3D *pPS = hplNew( cParticleSystemData3D, (apPE->GetName(),mpGame->GetResources(),mpGame->GetGraphics()) );
	pPS->AddEmitterData(apPE);
	mpGame->GetResources()->GetParticleManager()->AddData3D(pPS);
}

//-----------------------------------------------------------------------

bool cInit::Init(tString asCommandLine)
{
	/*if(asCommandLine != "")
	{
		Log("CommandLine: %s\n",asCommandLine.c_str());
		if(cSerialChecker::Validate(asCommandLine)<=0)
		{
			FatalError("Invalid serial number!\n");
		}
	}*/

	//iResourceBase::SetLogCreateAndDelete(true);
	SetWindowCaption("Penumbra Loading...");

	// PERSONAL DIR /////////////////////
	tWString sPersonalDir = GetSystemSpecialPath(eSystemPath_Personal);
	if(	cString::GetLastCharW(sPersonalDir) != _W("/") && 
		cString::GetLastCharW(sPersonalDir) != _W("\\"))
	{
		sPersonalDir += _W("/");
	}

	// CREATE NEEDED DIRS /////////////////////
	gsUserSettingsPath = sPersonalDir + PERSONAL_RELATIVEROOT PERSONAL_RELATIVEGAME _W("settings.cfg");
	#ifndef WIN32
	// For Mac OS X and Linux move the OLD Episode 1 folder to Penumbra/Overture and symlink to the old path
	if (FolderExists(sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra Overture/Episode1"))
			&& !IsFileLink(sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra Overture/Episode1"))) {
		// Create the new folder
		if(!FolderExists(sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra")))
			CreateFolder(sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra"));
		// Move the Older Episode 1 to the new Overture
		RenameFile(sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra Overture/Episode1"),
				sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra/Overture"));
		// Link back the old one to the new one
		LinkFile(sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra/Overture"),
			sPersonalDir + PERSONAL_RELATIVEROOT _W("Penumbra Overture/Episode1"));
	}
	#endif

	tWString vDirs[] = { PERSONAL_RELATIVEPIECES //auto includes ,
			PERSONAL_RELATIVEROOT PERSONAL_RELATIVEGAME_PARENT,
			PERSONAL_RELATIVEROOT PERSONAL_RELATIVEGAME};
	int lDirNum = PERSONAL_RELATIVEPIECES_COUNT + 2;

	//Check if directories exist and if not create
	for(int i=0; i<lDirNum; ++i)
	{
		tWString sDir = sPersonalDir + vDirs[i];
		if(FolderExists(sDir)) continue;

		CreateFolder(sDir);
	}

	// LOG FILE SETUP /////////////////////
	SetLogFile(sPersonalDir+PERSONAL_RELATIVEROOT PERSONAL_RELATIVEGAME _W("hpl.log"));
	SetUpdateLogFile(sPersonalDir+PERSONAL_RELATIVEROOT PERSONAL_RELATIVEGAME _W("hpl_update.log"));

	
	// MAIN INIT /////////////////////
	
	//Check for what settings file to use.
	if(FileExists(gsUserSettingsPath))
	{
		mpConfig = hplNew( cConfigFile, (gsUserSettingsPath) );
		gbUsingUserSettings = true;
	}
	else
	{
		mpConfig = hplNew( cConfigFile, (gsDefaultSettingsPath) );
		gbUsingUserSettings = false;
	}
	
	//Load config file
	mpConfig->Load();

#ifdef LAST_INIT_OK
	//If last init was not okay, reset all settings.
	if ( mpConfig->GetBool("Game","LastInitOK",true) == false )
	{
		hplDelete( mpConfig );
		mpConfig = hplNew( cConfigFile, (gsDefaultSettingsPath) );
		gbUsingUserSettings = false;
		CreateMessageBoxW(
			_W("Info"), 
			_W("Game did not end properly last run, resetting configuration"));
	}
#endif

	//Init is not done, so we do not know if it is okay.
	if (gbUsingUserSettings) {
		mpConfig->SetBool("Game","LastInitOK",false);
		mpConfig->Save();
	}

    mpGameConfig = hplNew( cConfigFile, (_W("config/game.cfg")) );
	mpGameConfig->Load();

	mvScreenSize.x = mpConfig->GetInt("Screen","Width",800);
	mvScreenSize.y = mpConfig->GetInt("Screen","Height",600);
	mbFullScreen = mpConfig->GetBool("Screen", "FullScreen", true);
	mbFullScreenSetting = mbFullScreen;
	mlDisplay = mpConfig->GetInt("Screen","Display",-1);
	mbVsync = mpConfig->GetBool("Screen", "Vsync", false);
	mbLogResources = mpConfig->GetBool("Debug", "LogResources", false);
	mbDebugInteraction = mpConfig->GetBool("Debug", "DebugInteraction", false);

	msWebPageOnExit = mpConfig->GetString("Demo","WebPageOnExit","http://www.Penumbra-Overture.com");
	
	mbSubtitles  = mpConfig->GetBool("Game","Subtitles",true); 
	mbSimpleWeaponSwing  = mpConfig->GetBool("Game","SimpleWeaponSwing",false); 
	mbDisablePersonalNotes  = mpConfig->GetBool("Game","DisablePersonalNotes",false); 
	mbAllowQuickSave  = mpConfig->GetBool("Game","AllowQuickSave",false); 
	mbFlashItems  = mpConfig->GetBool("Game","FlashItems",true);
	mbShowCrossHair  = mpConfig->GetBool("Game","ShowCrossHair",false);
	
	mbHapticsAvailable = mpConfig->GetBool("Haptics","Available",false);
	if(mbHapticsAvailable)
	{
		mbHasHaptics = mpConfig->GetBool("Haptics","Active",false);
		cHaptic::SetIsUsed(mbHasHaptics);
		mbHasHapticsOnRestart = mbHasHaptics;
	}
	else
	{
		mbHasHaptics = false;
	}
	mfHapticForceMul = mpConfig->GetFloat("Haptics","ForceMul",1.0f);
	mfHapticMoveScreenSpeedMul = mpConfig->GetFloat("Haptics","MoveScreenSpeedMul",1.0f);
	mfHapticScale = mpConfig->GetFloat("Haptics","Scale",0.04f);
	mfHapticProxyRadius = mpConfig->GetFloat("Haptics","ProxyRadius",0.019f);
	mfHapticOffsetZ = mpConfig->GetFloat("Haptics","OffsetZ",1.9f);
	mfHapticMaxInteractDist = mpConfig->GetFloat("Haptics","HapticMaxInteractDist",2);
	

	mbSimpleSwingInOptions = mpConfig->GetBool("Game","SimpleSwingInOptions",mbHapticsAvailable ? true:false); 
	
	msGlobalScriptFile = mpConfig->GetString("Map","GlobalScript","global_script.hps");
	msLanguageFile = mpConfig->GetString("Game","LanguageFile","english.lang");
	msCurrentUser = mpConfig->GetString("Game","CurrentUser","default");
	mDifficulty = (eGameDifficulty)mpConfig->GetInt("Game","Difficulty",1);

	msStartMap = mpConfig->GetString("Map","File","level00_01_boat_cabin.dae"); 
	msStartLink = mpConfig->GetString("Map","StartPos","link01");

	const char *pDirectMap = SDL_getenv("OVERTURE_MAP");
	if(pDirectMap && pDirectMap[0])
	{
		tString sDirectMap = pDirectMap;
		size_t lColon = sDirectMap.find(':');
		msDirectMap = cString::SetFileExt(sDirectMap.substr(0, lColon), "dae");
		msDirectLink = lColon == tString::npos ? "" : sDirectMap.substr(lColon + 1);
	}

	mlFSAA = mpConfig->GetInt("Graphics","FSAA",0);
	mbPostEffects = mpConfig->GetBool("Graphics","PostEffects",true);
	
	mPhysicsAccuracy = (ePhysicsAccuracy)mpConfig->GetInt("Physics","Accuracy",ePhysicsAccuracy_High);
	mfPhysicsUpdatesPerSec = mpConfig->GetFloat("Physics","UpdatesPerSec",60.0f);

	mlMaxSoundChannels = mpConfig->GetInt("Sound","MaxSoundChannels",32);
	mbUseSoundHardware = mpConfig->GetBool("Sound","UseSoundHardware",false);
	//mbForceGenericSoundDevice = mpConfig->GetBool("Sound", "ForceGeneric", false);
	mlStreamUpdateFreq = mpConfig->GetInt("Sound","StreamUpdateFreq",10);
	mbUseSoundThreading = mpConfig->GetBool("Sound","UseThreading",true);
	//mbUseVoiceManagement = mpConfig->GetBool("Sound","UseVoiceManagement", true);
	mlMaxMonoChannelsHint = mpConfig->GetInt("Sound","MaxMonoChannelsHint",0);
	mlMaxStereoChannelsHint = mpConfig->GetInt("Sound","MaxStereoChannelsHint",0);
	//mlStreamBufferSize = mpConfig->GetInt("Sound", "StreamBufferSize", 64);
	//mlStreamBufferCount = mpConfig->GetInt("Sound", "StreamBufferCount", 4);
	msDeviceName = mpConfig->GetString("Sound","DeviceName","NULL");

	iGpuProgram::SetLogDebugInformation(true);
	iResourceBase::SetLogCreateAndDelete(mbLogResources);

	cSetupVarContainer Vars;
	Vars.AddInt("ScreenWidth",mvScreenSize.x);
	Vars.AddInt("ScreenHeight",mvScreenSize.y);
	Vars.AddInt("ScreenBpp",32);
	Vars.AddBool("Fullscreen",mbFullScreen);
	Vars.AddInt("Display",mlDisplay);
	Vars.AddInt("Multisampling",mlFSAA);
	Vars.AddInt("LogicUpdateRate",60);
	Vars.AddBool("UseSoundHardware",mbUseSoundHardware);
	Vars.AddBool("ForceGeneric", mpConfig->GetBool("Sound", "ForceGeneric", false));
	Vars.AddInt("MaxSoundChannels",mlMaxSoundChannels);
	Vars.AddInt("StreamUpdateFreq",mlStreamUpdateFreq);
    Vars.AddBool("UseSoundThreading", mbUseSoundThreading);
	Vars.AddBool("UseVoiceManagement", mpConfig->GetBool("Sound","UseVoiceManagement",true));
	Vars.AddInt("MaxMonoChannelsHint",mlMaxMonoChannelsHint);
	Vars.AddInt("MaxStereoChannelsHint",mlMaxStereoChannelsHint);
	Vars.AddInt("StreamBufferSize",mpConfig->GetInt("Sound", "StreamBufferSize", 64));
	Vars.AddInt("StreamBufferCount",mpConfig->GetInt("Sound", "StreamBufferCount", 4));
	Vars.AddString("DeviceName",mpConfig->GetString("Sound", "DeviceName", "NULL"));
	Vars.AddString("WindowCaption", "Penumbra Loading...");

	Vars.AddBool("LowLevelSoundLogging", mpConfig->GetBool("Sound","LowLevelLogging", false));

	// GLSL shaders live next to the executable unless overridden.
	const char *pShaderDir = SDL_getenv("OVERTURE_SHADER_DIR");
	if(pShaderDir && pShaderDir[0])
		cGLSLProgram::SetShaderDirectory(tString(pShaderDir) + "/");
	else
		cGLSLProgram::SetShaderDirectory(tString(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "shaders/");

	iLowLevelGameSetup *pSetUp = nullptr;

	pSetUp = hplNew( cSDLGameSetup, () );
	mpGame = hplNew( cGame, ( pSetUp,Vars) );
    
#ifdef  TIMELIMIT
	CheckTimeLimit();
#endif

	//Make sure there really is haptic support!
	if(mbHasHaptics && cHaptic::GetIsUsed()==false)
	{
		CreateMessageBoxW(_W("Error!"),_W("No haptic support found. Mouse will be used instead!\n"));
		mbHasHaptics = false;
	}


	//Make sure hardware is really used.
	mbUseSoundHardware = mpGame->GetSound()->GetLowLevel()->IsHardwareAccelerated();

	mpGame->GetGraphics()->GetLowLevel()->SetVsyncActive(mbVsync);

	mbShowPreMenu = mpConfig->GetBool("Game","ShowPreMenu",true);
	mbShowMenu = mpConfig->GetBool("Game","ShowMenu",true);
	mbShowIntro = mpConfig->GetBool("Game","ShowIntro",true);

	mfMaxPhysicsTimeStep = 1.0f / mfPhysicsUpdatesPerSec;
	
	cMath::Randomize();

	// RESOURCE INIT /////////////////////
	mpGame->GetResources()->LoadResourceDirsFile("resources.cfg");

	// The fonts (fonts/ in the source tree) are next to the executable. They
	// replace the glyphs of the game's bitmap fonts.
	const char *pBitmapFonts = SDL_getenv("OVERTURE_BITMAP_FONTS");
	if(pBitmapFonts==nullptr || pBitmapFonts[0]==0 || pBitmapFonts[0]=='0')
	{
		mpGame->GetResources()->GetFontManager()->LoadVectorFonts(
			tString(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "fonts/");
	}

	// The debug test map (testmap/ in the source tree) is built next to the executable.
	mpGame->GetResources()->AddResourceDir(tString(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "testmap");

	// LANGUAGE ////////////////////////////////
	mpGame->GetResources()->SetLanguageFile(msLanguageFile);

	Log("Initializing " PRODUCT_NAME "\n  Version\t" PRODUCT_VERSION "\n  Date\t" PRODUCT_DATE "\n");

	//Add loaders
	mpGame->GetResources()->AddEntity3DLoader(hplNew( cEntityLoader_GameObject,("Object",this)) );
	mpGame->GetResources()->AddEntity3DLoader(hplNew( cEntityLoader_GameItem,("Item",this)) );
	mpGame->GetResources()->AddEntity3DLoader(hplNew( cEntityLoader_GameSwingDoor,("SwingDoor",this)) );
	mpGame->GetResources()->AddEntity3DLoader(hplNew( cEntityLoader_GameLamp,("Lamp",this)) );

	mpGame->GetResources()->AddEntity3DLoader(hplNew( cEntityLoader_GameEnemy,("Enemy",this)) );

    mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameArea,("script",this)) );
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameLink,("link",this)) );
	
#ifndef DEMO_VERSION
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameSaveArea,("save",this)) );
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameLadder,("ladder",this)) );
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameDamageArea,("damage",this)) );
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameForceArea,("force",this)) );
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameLiquidArea,("liquid",this)) );
	mpGame->GetResources()->AddArea3DLoader(hplNew( cAreaLoader_GameStickArea,("stick",this)) );
#endif

		
	/// FIRST LOADING SCREEN ////////////////////////////////////
	mpGraphicsHelper = hplNew( cGraphicsHelper, (this) );
	mpGraphicsHelper->DrawLoadingScreen("");

	// SOUND ////////////////////////////////
	mpGame->GetSound()->GetLowLevel()->SetVolume(mpConfig->GetFloat("Sound","Volume",1));
		
	// PHYSICS INIT /////////////////////
	mpGame->GetPhysics()->LoadSurfaceData("materials.cfg", mpGame->GetHaptic());
	
	// EARLY GAME INIT /////////////////////
	mpEffectHandler = hplNew( cEffectHandler, (this) );
	
	// GRAPHICS INIT ////////////////////
	mpGame->GetGraphics()->GetRendererPostEffects()->SetActive(mbPostEffects);
	mpGame->GetGraphics()->GetRendererPostEffects()->SetBloomActive(mpConfig->GetBool("Graphics", "Bloom", true));
	mpGame->GetGraphics()->GetRendererPostEffects()->SetBloomSpread(6);
	{
		cRendererPostEffects *pPostEffects = mpGame->GetGraphics()->GetRendererPostEffects();
		int lType = mpConfig->GetInt("Graphics","BloomType",eBloomType_Original);
		if(lType < 0 || lType >= eBloomType_LastEnum) lType = eBloomType_Original;
		pPostEffects->SetBloomType((eBloomType)lType);

		// The defaults are the tuning's own
		cBloomTuning tuning;
		tuning.mfThreshold = mpConfig->GetFloat("Graphics","BloomThreshold",tuning.mfThreshold);
		tuning.mfIntensity = mpConfig->GetFloat("Graphics","BloomIntensity",tuning.mfIntensity);
		tuning.mfScatter = mpConfig->GetFloat("Graphics","BloomScatter",tuning.mfScatter);
		pPostEffects->SetBloomTuning(tuning);

		// Ambient occlusion: the defaults are the settings' own
		pPostEffects->SetAmbientOcclusionActive(mpConfig->GetBool("Graphics","AmbientOcclusion",false));
		cAmbientOcclusionSettings occlusion;
		occlusion.mlSlices = mpConfig->GetInt("Graphics","AOSlices",occlusion.mlSlices);
		occlusion.mlSteps = mpConfig->GetInt("Graphics","AOSteps",occlusion.mlSteps);
		occlusion.mbHalfRate = mpConfig->GetBool("Graphics","AOHalfRate",occlusion.mbHalfRate);
		occlusion.mfRadius = mpConfig->GetFloat("Graphics","AORadius",occlusion.mfRadius);
		occlusion.mfScreenRadius = mpConfig->GetFloat("Graphics","AOScreenRadius",occlusion.mfScreenRadius);
		occlusion.mfStrength = mpConfig->GetFloat("Graphics","AOStrength",occlusion.mfStrength);
		occlusion.mfFadeDistance = mpConfig->GetFloat("Graphics","AOFadeDistance",occlusion.mfFadeDistance);
		occlusion.mfLightFade = mpConfig->GetFloat("Graphics","AOLightFade",occlusion.mfLightFade);
		occlusion.mlFilter = mpConfig->GetInt("Graphics","AOFilter",occlusion.mlFilter);
		// Read only, for captures of the occlusion alone
		occlusion.mbShowOnly = mpConfig->GetBool("Graphics","AOShowOnly",false);
		pPostEffects->SetAmbientOcclusionSettings(occlusion);
	}

	mpGame->GetGraphics()->GetRendererPostEffects()->SetMotionBlurActive(mpConfig->GetBool("Graphics", "MotionBlur", false));
	mpGame->GetGraphics()->GetRendererPostEffects()->SetMotionBlurAmount(mpConfig->GetFloat("Graphics", "MotionBlurAmount", 0.3f));
	{
		cRendererPostEffects *pPostEffects = mpGame->GetGraphics()->GetRendererPostEffects();
		int lType = mpConfig->GetInt("Graphics","MotionBlurType",eMotionBlurType_Original);
		if(lType < 0 || lType >= eMotionBlurType_LastEnum) lType = eMotionBlurType_Original;
		pPostEffects->SetMotionBlurType((eMotionBlurType)lType);

		cMotionBlurSettings motion;
		motion.mfShutterTime = mpConfig->GetFloat("Graphics","MotionBlurShutter",motion.mfShutterTime);
		motion.mfMaxLength = mpConfig->GetFloat("Graphics","MotionBlurMaxLength",motion.mfMaxLength);
		motion.mlSamples = mpConfig->GetInt("Graphics","MotionBlurSamples",motion.mlSamples);
		motion.mfNearLimit = mpConfig->GetFloat("Graphics","MotionBlurNearLimit",motion.mfNearLimit);
		pPostEffects->SetMotionBlurSettings(motion);

		lType = mpConfig->GetInt("Graphics","DepthOfFieldType",eDepthOfFieldType_Original);
		if(lType < 0 || lType >= eDepthOfFieldType_LastEnum) lType = eDepthOfFieldType_Original;
		pPostEffects->SetDepthOfFieldType((eDepthOfFieldType)lType);

		cDepthOfFieldSettings dof;
		dof.mfRadius = mpConfig->GetFloat("Graphics","DepthOfFieldRadius",dof.mfRadius);
		dof.mlRings = mpConfig->GetInt("Graphics","DepthOfFieldRings",dof.mlRings);
		// For captures; not saved
		dof.mbTest = mpConfig->GetBool("Graphics","DepthOfFieldTest",false);
		dof.mfTestFocus = mpConfig->GetFloat("Graphics","DepthOfFieldTestFocus",dof.mfTestFocus);
		dof.mfTestRange = mpConfig->GetFloat("Graphics","DepthOfFieldTestRange",dof.mfTestRange);
		pPostEffects->SetDepthOfFieldSettings(dof);

		// Which grain, if any, is the noise filter's to say (cPlayerNoiseFilter)
		iLowLevelGraphics *pLowLevel = mpGame->GetGraphics()->GetLowLevel();
		cFilmGrainSettings grain = pLowLevel->GetFilmGrain();
		grain.mfIntensity = mpConfig->GetFloat("Graphics","GrainIntensity",grain.mfIntensity);
		grain.mfSize = mpConfig->GetFloat("Graphics","GrainSize",grain.mfSize);
		grain.mfColor = mpConfig->GetFloat("Graphics","GrainColor",grain.mfColor);
		grain.mfShadows = mpConfig->GetFloat("Graphics","GrainShadows",grain.mfShadows);
		grain.mfHighlights = mpConfig->GetFloat("Graphics","GrainHighlights",grain.mfHighlights);
		grain.mfRate = mpConfig->GetFloat("Graphics","GrainRate",grain.mfRate);
		grain.mfDispersion = mpConfig->GetFloat("Graphics","GrainDispersion",grain.mfDispersion);
		pLowLevel->SetFilmGrain(grain);
	}

	mpGame->GetGraphics()->GetRenderer3D()->SetRefractionUsed(mpConfig->GetBool("Graphics", "Refractions", false));

	mpEffectHandler->GetDepthOfField()->SetDisabled(!mpConfig->GetBool("Graphics", "DepthOfField", true));

	mpGame->GetResources()->GetMaterialManager()->SetTextureSizeLevel(mpConfig->GetInt("Graphics","TextureSizeLevel",0));
	{
		// Trilinear with as much anisotropic filtering as there is (up to
		// 16x), unless the settings say otherwise
		int lFilter = mpConfig->GetInt("Graphics","TextureFilter",eTextureFilter_Trilinear);
		if(lFilter < 0 || lFilter >= eTextureFilter_LastEnum) lFilter = eTextureFilter_Trilinear;
		mpGame->GetResources()->GetMaterialManager()->SetTextureFilter((eTextureFilter)lFilter);
		mpGame->GetResources()->GetMaterialManager()->SetTextureAnisotropy(mpConfig->GetFloat("Graphics","TextureAnisotropy",16.0f));
	}
	{
		iLowLevelGraphics *pLowLevel = mpGame->GetGraphics()->GetLowLevel();
		// What the graphics card made of the samples asked for
		mlFSAA = pLowLevel->GetMultisampling();

		cSmaaSettings smaa;
		smaa.mbActive = mpConfig->GetBool("Graphics","SMAA",smaa.mbActive);
		int lEdges = mpConfig->GetInt("Graphics","SMAAEdges",smaa.mEdges);
		if(lEdges >= 0 && lEdges < eSmaaEdges_LastEnum) smaa.mEdges = (eSmaaEdges)lEdges;
		smaa.mfThreshold = mpConfig->GetFloat("Graphics","SMAAThreshold",smaa.mfThreshold);
		smaa.mlSearchSteps = mpConfig->GetInt("Graphics","SMAASearchSteps",smaa.mlSearchSteps);
		smaa.mlDiagonalSteps = mpConfig->GetInt("Graphics","SMAADiagonalSteps",smaa.mlDiagonalSteps);
		smaa.mlCornerRounding = mpConfig->GetInt("Graphics","SMAACornerRounding",smaa.mlCornerRounding);
		smaa.mbDepthHelp = mpConfig->GetBool("Graphics","SMAADepthHelp",smaa.mbDepthHelp);
		smaa.mfDepthThreshold = mpConfig->GetFloat("Graphics","SMAADepthThreshold",smaa.mfDepthThreshold);
		smaa.mfDepthScale = mpConfig->GetFloat("Graphics","SMAADepthScale",smaa.mfDepthScale);
		smaa.mfDepthStrength = mpConfig->GetFloat("Graphics","SMAADepthStrength",smaa.mfDepthStrength);
		// Read only, for captures of the edges (1) or the weights (2)
		smaa.mlShow = mpConfig->GetInt("Graphics","SMAAShow",0);
		pLowLevel->SetSmaa(smaa);
	}

	mpGame->GetGraphics()->GetLowLevel()->SetGammaCorrection(mpConfig->GetFloat("Graphics","Gamma",1.0f));
	mpGame->GetGraphics()->SetRenderScale(mpConfig->GetFloat("Graphics","RenderScale",1.0f));
	mpGame->GetGraphics()->SetLinearLighting(mpConfig->GetBool("Graphics","LinearLighting",false));
	{
		// The defaults are the tuning's own
		cLinearLightingTuning tuning;
		tuning.mfLightExp = mpConfig->GetFloat("Graphics","LinearLightExp",tuning.mfLightExp);
		tuning.mfExposure = mpConfig->GetFloat("Graphics","LinearExposure",tuning.mfExposure);
		tuning.mfGlow = mpConfig->GetFloat("Graphics","LinearGlow",tuning.mfGlow);
		tuning.mfBlendMatch = mpConfig->GetFloat("Graphics","LinearBlendMatch",tuning.mfBlendMatch);
		tuning.mfShoulder = mpConfig->GetFloat("Graphics","LinearShoulder",tuning.mfShoulder);
		tuning.mfWhite = mpConfig->GetFloat("Graphics","LinearWhite",tuning.mfWhite);
		mpGame->GetGraphics()->GetLowLevel()->SetLinearLightingTuning(tuning);
	}

	{
		// The defaults are the settings' own
		cShadingSettings shading;
		shading.mbImproved = mpConfig->GetBool("Graphics","ShadingImproved",shading.mbImproved);
		shading.mfSpecular = mpConfig->GetFloat("Graphics","ShadingSpecular",shading.mfSpecular);
		shading.mfReflectance = mpConfig->GetFloat("Graphics","ShadingReflectance",shading.mfReflectance);
		shading.mfRoughDull = mpConfig->GetFloat("Graphics","ShadingRoughDull",shading.mfRoughDull);
		shading.mfRoughShiny = mpConfig->GetFloat("Graphics","ShadingRoughShiny",shading.mfRoughShiny);
		shading.mfAmbientUp = mpConfig->GetFloat("Graphics","ShadingAmbientUp",shading.mfAmbientUp);
		shading.mfAmbientFacing = mpConfig->GetFloat("Graphics","ShadingAmbientFacing",shading.mfAmbientFacing);
		shading.mfAmbientSheen = mpConfig->GetFloat("Graphics","ShadingAmbientSheen",shading.mfAmbientSheen);
		mpGame->GetGraphics()->GetLowLevel()->SetShading(shading);
	}

	{
		// The defaults are the settings' own
		cEffectSettings effects;
		effects.mbHalos = mpConfig->GetBool("Graphics","HalosImproved",effects.mbHalos);
		effects.mfHaloBrightness = mpConfig->GetFloat("Graphics","HaloBrightness",effects.mfHaloBrightness);
		effects.mfHaloStreaks = mpConfig->GetFloat("Graphics","HaloStreaks",effects.mfHaloStreaks);
		effects.mbShafts = mpConfig->GetBool("Graphics","ShaftsImproved",effects.mbShafts);
		effects.mfShaftBrightness = mpConfig->GetFloat("Graphics","ShaftBrightness",effects.mfShaftBrightness);
		effects.mfShaftDust = mpConfig->GetFloat("Graphics","ShaftDust",effects.mfShaftDust);
		effects.mbParticles = mpConfig->GetBool("Graphics","ParticlesImproved",effects.mbParticles);
		effects.mfParticleSoft = mpConfig->GetFloat("Graphics","ParticleSoft",effects.mfParticleSoft);
		effects.mfParticleLight = mpConfig->GetFloat("Graphics","ParticleLight",effects.mfParticleLight);
		effects.mfParticleDark = mpConfig->GetFloat("Graphics","ParticleDark",effects.mfParticleDark);
		mpGame->GetGraphics()->GetLowLevel()->SetEffects(effects);
	}

	mpGame->GetGraphics()->GetRenderer3D()->SetShowShadows((eRendererShowShadows)mpConfig->GetInt("Graphics","Shadows",0));
	{
		cRenderer3D *pRenderer = mpGame->GetGraphics()->GetRenderer3D();
		int lType = mpConfig->GetInt("Graphics","ShadowType",eShadowType_Volumes);
		if(lType < 0 || lType >= eShadowType_LastEnum) lType = eShadowType_Volumes;
		pRenderer->SetShadowType((eShadowType)lType);

		// The defaults are the settings' own
		cShadowMapSettings shadows;
		int lFilter = mpConfig->GetInt("Graphics","ShadowFilter",shadows.mFilter);
		if(lFilter >= 0 && lFilter < eShadowMapFilter_LastEnum) shadows.mFilter = (eShadowMapFilter)lFilter;
		shadows.mlSpotSize = mpConfig->GetInt("Graphics","ShadowSpotSize",shadows.mlSpotSize);
		shadows.mlPointSize = mpConfig->GetInt("Graphics","ShadowPointSize",shadows.mlPointSize);
		shadows.mfSoftness = mpConfig->GetFloat("Graphics","ShadowSoftness",shadows.mfSoftness);
		shadows.mfLightSize = mpConfig->GetFloat("Graphics","ShadowLightSize",shadows.mfLightSize);
		shadows.mfMaxBlur = mpConfig->GetFloat("Graphics","ShadowMaxBlur",shadows.mfMaxBlur);
		shadows.mlSamples = mpConfig->GetInt("Graphics","ShadowSamples",shadows.mlSamples);
		shadows.mlBlockerSamples = mpConfig->GetInt("Graphics","ShadowBlockerSamples",shadows.mlBlockerSamples);
		shadows.mbNoise = mpConfig->GetBool("Graphics","ShadowNoise",shadows.mbNoise);
		shadows.mfBias = mpConfig->GetFloat("Graphics","ShadowBias",shadows.mfBias);
		shadows.mfSlopeBias = mpConfig->GetFloat("Graphics","ShadowSlopeBias",shadows.mfSlopeBias);
		int lFaces = mpConfig->GetInt("Graphics","ShadowFaces",shadows.mFaces);
		if(lFaces >= 0 && lFaces < eShadowMapFaces_LastEnum) shadows.mFaces = (eShadowMapFaces)lFaces;
		shadows.mbAlphaCasters = mpConfig->GetBool("Graphics","ShadowAlphaCasters",shadows.mbAlphaCasters);
		pRenderer->SetShadowMapSettings(shadows);
	}

	mpGame->SetLimitFPS( mpConfig->GetBool("Graphics","LimitFPS",true));

	// HAPTIC INIT ////////////////////
	if(mbHasHaptics)
	{
		mpGame->GetHaptic()->GetLowLevel()->SetWorldScale(mfHapticScale);
		mpGame->GetHaptic()->GetLowLevel()->SetVirtualMousePosBounds(cVector2f(-60,-60),
											cVector2f(25,25), cVector2f(800, 600));
		mpGame->GetHaptic()->GetLowLevel()->SetProxyRadius(mfHapticProxyRadius);
		
	}
	
	// BASE GAME INIT /////////////////////
	mpMusicHandler = hplNew( cGameMusicHandler,(this) );
	mpPlayerHands = hplNew( cPlayerHands,(this) );
	mpButtonHandler = hplNew( cButtonHandler,(this) );
	mpMapHandler = hplNew( cMapHandler,(this) );
	mpGameMessageHandler = hplNew( cGameMessageHandler,(this) );
	mpRadioHandler = hplNew( cRadioHandler,(this) );
	mpInventory = hplNew( cInventory,(this) );
	mpFadeHandler = hplNew( cFadeHandler,(this) );
	mpSaveHandler = hplNew( cSaveHandler,(this) );
	mpTriggerHandler = hplNew( cTriggerHandler,(this) );
	mpAttackHandler = hplNew( cAttackHandler,(this) );
	mpNotebook = hplNew( cNotebook,(this) );
	mpNumericalPanel = hplNew( cNumericalPanel,(this) );
	mpDeathMenu = hplNew( cDeathMenu,(this) );
	mpPlayer = hplNew( cPlayer,(this) );
	{
		// The defaults are the settings' own
		cFlashlightSettings flashlight;
		flashlight.mbImproved = mpConfig->GetBool("Graphics","FlashlightImproved",flashlight.mbImproved);
		flashlight.mfBrightness = mpConfig->GetFloat("Graphics","FlashlightBrightness",flashlight.mfBrightness);
		// Below what the panel can set: a settings file from when this was
		// saved after the player had been deleted, which wrote 0 (no light)
		if(flashlight.mfBrightness < 0.25f) flashlight.mfBrightness = cFlashlightSettings().mfBrightness;
		flashlight.mfWarmth = mpConfig->GetFloat("Graphics","FlashlightWarmth",flashlight.mfWarmth);
		flashlight.mfReach = mpConfig->GetFloat("Graphics","FlashlightReach",flashlight.mfReach);
		flashlight.mfEvenAt = mpConfig->GetFloat("Graphics","FlashlightEvenAt",flashlight.mfEvenAt);
		flashlight.mfNearBoost = mpConfig->GetFloat("Graphics","FlashlightNearBoost",flashlight.mfNearBoost);
		flashlight.mfHotspot = mpConfig->GetFloat("Graphics","FlashlightHotspot",flashlight.mfHotspot);
		flashlight.mfField = mpConfig->GetFloat("Graphics","FlashlightField",flashlight.mfField);
		flashlight.mfWidth = mpConfig->GetFloat("Graphics","FlashlightWidth",flashlight.mfWidth);
		flashlight.mfBounce = mpConfig->GetFloat("Graphics","FlashlightBounce",flashlight.mfBounce);
		mpPlayer->GetFlashLight()->SetSettings(flashlight);
	}
	mpLampLooks = hplNew( cLampLooks,(this) );
	{
		cLampSettings lamps;
		lamps.mbImproved = mpConfig->GetBool("Graphics","LampsImproved",lamps.mbImproved);
		lamps.mfFlicker = mpConfig->GetFloat("Graphics","LampsFlicker",lamps.mfFlicker);
		lamps.mfSway = mpConfig->GetFloat("Graphics","LampsSway",lamps.mfSway);
		lamps.mfColour = mpConfig->GetFloat("Graphics","LampsColour",lamps.mfColour);
		lamps.mbFalloff = mpConfig->GetBool("Graphics","LampsFalloff",lamps.mbFalloff);
		lamps.mfEvenAt = mpConfig->GetFloat("Graphics","LampsEvenAt",lamps.mfEvenAt);
		lamps.mfNearBoost = mpConfig->GetFloat("Graphics","LampsNearBoost",lamps.mfNearBoost);
		lamps.mfTube = mpConfig->GetFloat("Graphics","LampsTube",lamps.mfTube);
		lamps.mbMapLights = mpConfig->GetBool("Graphics","LampsMapLights",lamps.mbMapLights);
		mpLampLooks->SetSettings(lamps);
	}
	mpMapLoadText = hplNew( cMapLoadText,(this) );
	mpPreMenu = hplNew( cPreMenu,(this) );
	mpCredits = hplNew( cCredits,(this) );
	mpDemoEndText = hplNew( cDemoEndText,(this) );
    
	mpIntroStory = hplNew( cIntroStory,(this) );

	// Reference images: the menu's snow starts in the same places every run
	if(cRefShots::IsRequested()) cMath::Randomize(1);
	mpMainMenu = hplNew( cMainMenu,(this) );

	if(cSmokeTest::IsRequested())
		mpSmokeTest = hplNew( cSmokeTest,(this, cSmokeTest::GetSpec()) );
	if(cPhysicsLog::IsRequested())
		mpPhysicsLog = hplNew( cPhysicsLog,(this) );
	if(mpSmokeTest==nullptr && cRefShots::IsRequested())
		mpRefShots = hplNew( cRefShots,(this, cRefShots::GetSpec()) );
	if(mpSmokeTest==nullptr && mpRefShots==nullptr)
		mpRefView = hplNew( cRefView,(this) );

	// UPDATE STATES INIT /////////////////////
	//Add to the global state
	mpGame->GetUpdater()->AddGlobalUpdate(mpButtonHandler);
	mpGame->GetUpdater()->AddGlobalUpdate(mpSaveHandler);

	//Add to the default state
	mpGame->GetUpdater()->AddUpdate("Default", mpButtonHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpPlayer);
	mpGame->GetUpdater()->AddUpdate("Default", mpPlayerHands);
	mpGame->GetUpdater()->AddUpdate("Default", mpMusicHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpMapHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpLampLooks);
	mpGame->GetUpdater()->AddUpdate("Default", mpGameMessageHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpRadioHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpInventory);
	mpGame->GetUpdater()->AddUpdate("Default", mpFadeHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpEffectHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpTriggerHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpAttackHandler);
	mpGame->GetUpdater()->AddUpdate("Default", mpNotebook);
	mpGame->GetUpdater()->AddUpdate("Default", mpNumericalPanel);
	mpGame->GetUpdater()->AddUpdate("Default", mpDeathMenu);
	mpGame->GetUpdater()->AddUpdate("Default", this); //need this last because of the init script.

	//Add to intro state
	mpGame->GetUpdater()->AddContainer("Intro");
	mpGame->GetUpdater()->AddUpdate("Intro",mpIntroStory);

	//Add to main menu state
	mpGame->GetUpdater()->AddContainer("MainMenu");
	mpGame->GetUpdater()->AddUpdate("MainMenu",mpMainMenu);

	//Add to map load text state
	mpGame->GetUpdater()->AddContainer("MapLoadText");
	mpGame->GetUpdater()->AddUpdate("MapLoadText",mpMapLoadText);

	//Add to pre menu state
	mpGame->GetUpdater()->AddContainer("PreMenu");
	mpGame->GetUpdater()->AddUpdate("PreMenu",mpPreMenu);

	//Add to credits text state
	mpGame->GetUpdater()->AddContainer("Credits");
	mpGame->GetUpdater()->AddUpdate("Credits", mpCredits);

	//Add to demo end text state
	mpGame->GetUpdater()->AddContainer("DemoEndText");
	mpGame->GetUpdater()->AddUpdate("DemoEndText", mpDemoEndText);

	mpGame->GetUpdater()->SetContainer("Default");

	// Last in every container: they move the camera after the player has.
	iUpdateable *pAutoRun = mpSmokeTest ? static_cast<iUpdateable*>(mpSmokeTest) : mpRefShots;
	if(pAutoRun==nullptr) pAutoRun = mpRefView;
	if(pAutoRun)
	{
		const char *vContainers[] = {"Default","Intro","MainMenu","MapLoadText",
									"PreMenu","Credits","DemoEndText"};
		for(const char *sContainer : vContainers)
			mpGame->GetUpdater()->AddUpdate(sContainer, pAutoRun);
	}
	if(mpPhysicsLog)
		mpGame->GetUpdater()->AddUpdate("Default", mpPhysicsLog);
	
	// SCRIPT INIT /////////////////////
	cGameScripts::Init();


	// MISC INIT ///////////////////////
	mpDefaultFont = mpGame->GetResources()->GetFontManager()->CreateFontData("verdana.fnt");
	
	//GLOBAL SCRIPT ////////////////////////////
	mpGlobalScript = mpGame->GetResources()->GetScriptManager()->CreateScript(msGlobalScriptFile);
	if(mpGlobalScript)
	{
		mpGlobalScript->Run("OnInit()");
	}
	
	//mpGraphicsHelper->DrawLoadingScreen("");

	//mpIntroStory->SetActive(true);
	//mpCredits->SetActive(true);
	//mpDemoEndText->SetActive(true);

	//mpGame->SetRenderOnce(true);
	//mpGame->GetGraphics()->GetRenderer3D()->SetDebugFlags(eRendererDebugFlag_LogRendering);
	
	if(mpSmokeTest)
	{
		if(mpSmokeTest->GetFirstMap() != "")
			mpMapHandler->Load(mpSmokeTest->GetFirstMap(),"");
	}
	else if(mpRefShots)
	{
		if(mpRefShots->GetFirstMap() != "")
			mpMapHandler->Load(mpRefShots->GetFirstMap(),"");
	}
	else if(msDirectMap != "")
	{
		mpMapHandler->Load(msDirectMap,msDirectLink);
	}
	else if(mbShowPreMenu)
	{
		mpPreMenu->SetActive(true);
	}
	else if(mbShowMenu)
	{
		mpMainMenu->SetActive(true);
	}
	else
	{
		mpMapHandler->Load(	msStartMap,msStartLink);
	}

	if (gbUsingUserSettings) {
		mpConfig->SetBool("Game","LastInitOK", true);
		mpConfig->Save();
	}

	//////////////////////////////////////////////////////////
	// Create newer settings file, if using default.
	if(gbUsingUserSettings == false)
	{
		if(mpConfig) hplDelete( mpConfig );
		mpConfig = hplNew( cConfigFile, (gsUserSettingsPath) );
		gbUsingUserSettings = true;
	}

	SetWindowCaption("Penumbra");
	
	return true;
}

//-----------------------------------------------------------------------

void cInit::ResetGame(bool abRunInitScript)
{
	mpGame->GetUpdater()->Reset();

	if(abRunInitScript)
	{
		if(mpGlobalScript)
		{
			mpGlobalScript->Run("OnInit()");
		}
	}

	mpGame->GetSound()->GetMusicHandler()->Stop(100);
	mpGame->GetSound()->GetSoundHandler()->StopAll(eSoundDest_All);
}

//-----------------------------------------------------------------------

void cInit::Run()
{
	mpGame->Run();
}

//-----------------------------------------------------------------------

void cInit::OnStart()
{

}

//-----------------------------------------------------------------------

void cInit::Update(float afTimeStep)
{
	if(mpGlobalScript)
	{
		mpGlobalScript->Run("OnUpdate()");
	}
}

//-----------------------------------------------------------------------

void cInit::Reset()
{
	
}

//-----------------------------------------------------------------------

void cInit::Exit()
{
	mpConfig->SetBool("Haptics","Active",mbHasHapticsOnRestart);
	mpConfig->SetBool("Haptics","Available",mbHapticsAvailable);
	mpConfig->SetFloat("Haptics","ForceMul",mfHapticForceMul);
	mpConfig->SetFloat("Haptics","MoveScreenSpeedMul",mfHapticMoveScreenSpeedMul);
	mpConfig->SetFloat("Haptics","Scale",mfHapticScale);
	mpConfig->SetFloat("Haptics","ProxyRadius",mfHapticProxyRadius);
	mpConfig->SetFloat("Haptics","OffsetZ",mfHapticOffsetZ);
	mpConfig->SetFloat("Haptics","HapticMaxInteractDist",mfHapticMaxInteractDist);
	mpConfig->SetFloat("Haptics","ProxyRadius",mfHapticProxyRadius);
	mpConfig->SetFloat("Haptics","OffsetZ",mfHapticOffsetZ);
	if(mbHasHaptics)
	{
		mpConfig->SetFloat("Haptics","InteractModeCameraSpeed",mpPlayer->GetHapticCamera()->GetInteractModeCameraSpeed());
		mpConfig->SetFloat("Haptics","ActionModeCameraSpeed",mpPlayer->GetHapticCamera()->GetActionModeCameraSpeed());
	}

	// The player keeps the flashlight's settings: read before it goes
	{
		const cFlashlightSettings &flashlight = mpPlayer->GetFlashLight()->GetSettings();
		mpConfig->SetBool("Graphics","FlashlightImproved",flashlight.mbImproved);
		mpConfig->SetFloat("Graphics","FlashlightBrightness",flashlight.mfBrightness);
		mpConfig->SetFloat("Graphics","FlashlightWarmth",flashlight.mfWarmth);
		mpConfig->SetFloat("Graphics","FlashlightReach",flashlight.mfReach);
		mpConfig->SetFloat("Graphics","FlashlightEvenAt",flashlight.mfEvenAt);
		mpConfig->SetFloat("Graphics","FlashlightNearBoost",flashlight.mfNearBoost);
		mpConfig->SetFloat("Graphics","FlashlightHotspot",flashlight.mfHotspot);
		mpConfig->SetFloat("Graphics","FlashlightField",flashlight.mfField);
		mpConfig->SetFloat("Graphics","FlashlightWidth",flashlight.mfWidth);
		mpConfig->SetFloat("Graphics","FlashlightBounce",flashlight.mfBounce);
	}

	{
		const cLampSettings &lamps = mpLampLooks->GetSettings();
		mpConfig->SetBool("Graphics","LampsImproved",lamps.mbImproved);
		mpConfig->SetFloat("Graphics","LampsFlicker",lamps.mfFlicker);
		mpConfig->SetFloat("Graphics","LampsSway",lamps.mfSway);
		mpConfig->SetFloat("Graphics","LampsColour",lamps.mfColour);
		mpConfig->SetBool("Graphics","LampsFalloff",lamps.mbFalloff);
		mpConfig->SetFloat("Graphics","LampsEvenAt",lamps.mfEvenAt);
		mpConfig->SetFloat("Graphics","LampsNearBoost",lamps.mfNearBoost);
		mpConfig->SetFloat("Graphics","LampsTube",lamps.mfTube);
		mpConfig->SetBool("Graphics","LampsMapLights",lamps.mbMapLights);
	}

	// PLAYER EXIT /////////////////////
	//Log(" Exit Save Handler\n");
	//hplDelete( mpSaveHandler );

	Log(" Reset maphandler\n");
	mpMapHandler->Reset();
	

	Log(" Exit Player\n");
	// PLAYER EXIT /////////////////////
	hplDelete( mpPlayer );
	hplDelete( mpLampLooks );
	
	// BASE GAME EXIT //////////////////
	Log(" Exit Button Handler\n");
	hplDelete( mpButtonHandler );
	Log(" Exit Map Handler\n");
	hplDelete( mpMapHandler );
	//Log(" Exit Game Scripts\n");
	//hplDelete( mpGameScripts );
	Log(" Exit Game Message Handler\n");
	hplDelete( mpGameMessageHandler );
	Log(" Exit Radio Handler\n");
	hplDelete( mpRadioHandler );
	Log(" Exit Inventory\n");
	hplDelete( mpInventory );
	Log(" Exit Fade Handler\n");
	hplDelete( mpFadeHandler );
	Log(" Exit Save Handler\n");
	hplDelete( mpSaveHandler );
	Log(" Exit Trigger Handler\n");
	hplDelete( mpTriggerHandler );
	Log(" Exit Attack Handler\n");
	hplDelete( mpAttackHandler );
	Log(" Exit Notebook\n");
	hplDelete( mpNotebook );
	Log(" Exit Numerical panel\n");
	hplDelete( mpNumericalPanel );
	Log(" Exit Intro story\n");
	hplDelete( mpIntroStory );
	Log(" Exit Death menu\n");
	hplDelete( mpDeathMenu );
	Log(" Exit Graphics helper\n");
	hplDelete( mpGraphicsHelper );
	Log(" Exit Main menu\n");
	hplDelete( mpMainMenu );
	if(mpSmokeTest) hplDelete( mpSmokeTest );
	if(mpRefShots) hplDelete( mpRefShots );
	if(mpRefView) hplDelete( mpRefView );
	if(mpPhysicsLog) hplDelete( mpPhysicsLog );
	Log(" Exit Player hands\n");
	hplDelete( mpPlayerHands );
	Log(" Exit Music handler\n");
	hplDelete( mpMusicHandler );
	Log(" Exit Map Load Text\n");
	hplDelete( mpMapLoadText );
	Log(" Exit PreMenu\n");
	hplDelete( mpPreMenu );
	Log(" Exit Credits\n");
	hplDelete( mpCredits );
	Log(" Exit Demo end text\n");
	hplDelete( mpDemoEndText );

    Log(" Saving config\n");
	//Save engine stuff.
	mpConfig->SetBool("Graphics", "Bloom",mpGame->GetGraphics()->GetRendererPostEffects()->GetBloomActive());
	{
		cRendererPostEffects *pPostEffects = mpGame->GetGraphics()->GetRendererPostEffects();
		mpConfig->SetInt("Graphics","BloomType",pPostEffects->GetBloomType());
		mpConfig->SetFloat("Graphics","BloomThreshold",pPostEffects->GetBloomTuning().mfThreshold);
		mpConfig->SetFloat("Graphics","BloomIntensity",pPostEffects->GetBloomTuning().mfIntensity);
		mpConfig->SetFloat("Graphics","BloomScatter",pPostEffects->GetBloomTuning().mfScatter);

		const cAmbientOcclusionSettings &occlusion = pPostEffects->GetAmbientOcclusionSettings();
		mpConfig->SetBool("Graphics","AmbientOcclusion",pPostEffects->GetAmbientOcclusionActive());
		mpConfig->SetInt("Graphics","AOSlices",occlusion.mlSlices);
		mpConfig->SetInt("Graphics","AOSteps",occlusion.mlSteps);
		mpConfig->SetBool("Graphics","AOHalfRate",occlusion.mbHalfRate);
		mpConfig->SetFloat("Graphics","AORadius",occlusion.mfRadius);
		mpConfig->SetFloat("Graphics","AOScreenRadius",occlusion.mfScreenRadius);
		mpConfig->SetFloat("Graphics","AOStrength",occlusion.mfStrength);
		mpConfig->SetFloat("Graphics","AOFadeDistance",occlusion.mfFadeDistance);
		mpConfig->SetFloat("Graphics","AOLightFade",occlusion.mfLightFade);
		mpConfig->SetInt("Graphics","AOFilter",occlusion.mlFilter);
	}
	mpConfig->SetBool("Graphics", "MotionBlur",mpGame->GetGraphics()->GetRendererPostEffects()->GetMotionBlurActive());
	mpConfig->SetFloat("Graphics", "MotionBlurAmount",mpGame->GetGraphics()->GetRendererPostEffects()->GetMotionBlurAmount());
	{
		cRendererPostEffects *pPostEffects = mpGame->GetGraphics()->GetRendererPostEffects();
		const cMotionBlurSettings &motion = pPostEffects->GetMotionBlurSettings();
		mpConfig->SetInt("Graphics","MotionBlurType",pPostEffects->GetMotionBlurType());
		mpConfig->SetFloat("Graphics","MotionBlurShutter",motion.mfShutterTime);
		mpConfig->SetFloat("Graphics","MotionBlurMaxLength",motion.mfMaxLength);
		mpConfig->SetInt("Graphics","MotionBlurSamples",motion.mlSamples);
		mpConfig->SetFloat("Graphics","MotionBlurNearLimit",motion.mfNearLimit);

		const cDepthOfFieldSettings &dof = pPostEffects->GetDepthOfFieldSettings();
		mpConfig->SetInt("Graphics","DepthOfFieldType",pPostEffects->GetDepthOfFieldType());
		mpConfig->SetFloat("Graphics","DepthOfFieldRadius",dof.mfRadius);
		mpConfig->SetInt("Graphics","DepthOfFieldRings",dof.mlRings);

		const cFilmGrainSettings &grain = mpGame->GetGraphics()->GetLowLevel()->GetFilmGrain();
		mpConfig->SetFloat("Graphics","GrainIntensity",grain.mfIntensity);
		mpConfig->SetFloat("Graphics","GrainSize",grain.mfSize);
		mpConfig->SetFloat("Graphics","GrainColor",grain.mfColor);
		mpConfig->SetFloat("Graphics","GrainShadows",grain.mfShadows);
		mpConfig->SetFloat("Graphics","GrainHighlights",grain.mfHighlights);
		mpConfig->SetFloat("Graphics","GrainRate",grain.mfRate);
		mpConfig->SetFloat("Graphics","GrainDispersion",grain.mfDispersion);
	}
	mpConfig->SetBool("Graphics", "DepthOfField",!mpEffectHandler->GetDepthOfField()->IsDisabled());
	mpConfig->GetBool("Graphics", "Refractions", mpGame->GetGraphics()->GetRenderer3D()->GetRefractionUsed());
	
	mpConfig->SetFloat("Sound","Volume",mpGame->GetSound()->GetLowLevel()->GetVolume());
	mpConfig->SetBool("Sound","UseSoundHardware", mbUseSoundHardware);
	mpConfig->SetInt("Sound","MaxSoundChannels",mlMaxSoundChannels);
	mpConfig->SetInt("Sound","StreamUpdateFreq",mlStreamUpdateFreq);
	mpConfig->SetBool("Sound","UseThreading",mbUseSoundThreading);
	mpConfig->SetInt("Sound","MaxMonoChannelsHint",mlMaxMonoChannelsHint);
	mpConfig->SetInt("Sound","MaxStereoChannelsHint",mlMaxStereoChannelsHint);
	mpConfig->SetString("Sound","DeviceName",msDeviceName);

	
	mpConfig->SetInt("Graphics","TextureSizeLevel",mpGame->GetResources()->GetMaterialManager()->GetTextureSizeLevel());
	mpConfig->SetInt("Graphics","TextureFilter",mpGame->GetResources()->GetMaterialManager()->GetTextureFilter());
	mpConfig->SetFloat("Graphics","TextureAnisotropy",mpGame->GetResources()->GetMaterialManager()->GetTextureAnisotropy());

	mpConfig->SetFloat("Graphics","Gamma",mpGame->GetGraphics()->GetLowLevel()->GetGammaCorrection());
	mpConfig->SetFloat("Graphics","RenderScale",mpGame->GetGraphics()->GetRenderScale());
	mpConfig->SetBool("Graphics","LinearLighting",mpGame->GetGraphics()->GetLinearLighting());
	{
		const cLinearLightingTuning &tuning = mpGame->GetGraphics()->GetLowLevel()->GetLinearLightingTuning();
		mpConfig->SetFloat("Graphics","LinearLightExp",tuning.mfLightExp);
		mpConfig->SetFloat("Graphics","LinearExposure",tuning.mfExposure);
		mpConfig->SetFloat("Graphics","LinearGlow",tuning.mfGlow);
		mpConfig->SetFloat("Graphics","LinearBlendMatch",tuning.mfBlendMatch);
		mpConfig->SetFloat("Graphics","LinearShoulder",tuning.mfShoulder);
		mpConfig->SetFloat("Graphics","LinearWhite",tuning.mfWhite);
	}

	{
		const cShadingSettings &shading = mpGame->GetGraphics()->GetLowLevel()->GetShading();
		mpConfig->SetBool("Graphics","ShadingImproved",shading.mbImproved);
		mpConfig->SetFloat("Graphics","ShadingSpecular",shading.mfSpecular);
		mpConfig->SetFloat("Graphics","ShadingReflectance",shading.mfReflectance);
		mpConfig->SetFloat("Graphics","ShadingRoughDull",shading.mfRoughDull);
		mpConfig->SetFloat("Graphics","ShadingRoughShiny",shading.mfRoughShiny);
		mpConfig->SetFloat("Graphics","ShadingAmbientUp",shading.mfAmbientUp);
		mpConfig->SetFloat("Graphics","ShadingAmbientFacing",shading.mfAmbientFacing);
		mpConfig->SetFloat("Graphics","ShadingAmbientSheen",shading.mfAmbientSheen);
	}

	{
		const cEffectSettings &effects = mpGame->GetGraphics()->GetLowLevel()->GetEffects();
		mpConfig->SetBool("Graphics","HalosImproved",effects.mbHalos);
		mpConfig->SetFloat("Graphics","HaloBrightness",effects.mfHaloBrightness);
		mpConfig->SetFloat("Graphics","HaloStreaks",effects.mfHaloStreaks);
		mpConfig->SetBool("Graphics","ShaftsImproved",effects.mbShafts);
		mpConfig->SetFloat("Graphics","ShaftBrightness",effects.mfShaftBrightness);
		mpConfig->SetFloat("Graphics","ShaftDust",effects.mfShaftDust);
		mpConfig->SetBool("Graphics","ParticlesImproved",effects.mbParticles);
		mpConfig->SetFloat("Graphics","ParticleSoft",effects.mfParticleSoft);
		mpConfig->SetFloat("Graphics","ParticleLight",effects.mfParticleLight);
		mpConfig->SetFloat("Graphics","ParticleDark",effects.mfParticleDark);
	}

	mpConfig->SetInt("Graphics","FSAA",mlFSAA);
	{
		const cSmaaSettings &smaa = mpGame->GetGraphics()->GetLowLevel()->GetSmaa();
		mpConfig->SetBool("Graphics","SMAA",smaa.mbActive);
		mpConfig->SetInt("Graphics","SMAAEdges",smaa.mEdges);
		mpConfig->SetFloat("Graphics","SMAAThreshold",smaa.mfThreshold);
		mpConfig->SetInt("Graphics","SMAASearchSteps",smaa.mlSearchSteps);
		mpConfig->SetInt("Graphics","SMAADiagonalSteps",smaa.mlDiagonalSteps);
		mpConfig->SetInt("Graphics","SMAACornerRounding",smaa.mlCornerRounding);
		mpConfig->SetBool("Graphics","SMAADepthHelp",smaa.mbDepthHelp);
		mpConfig->SetFloat("Graphics","SMAADepthThreshold",smaa.mfDepthThreshold);
		mpConfig->SetFloat("Graphics","SMAADepthScale",smaa.mfDepthScale);
		mpConfig->SetFloat("Graphics","SMAADepthStrength",smaa.mfDepthStrength);
	}
	mpConfig->SetBool("Graphics","PostEffects",mbPostEffects);
	mpConfig->SetBool("Graphics","LimitFPS",mpGame->GetLimitFPS());

	mpConfig->SetInt("Graphics","Shadows",mpGame->GetGraphics()->GetRenderer3D()->GetShowShadows());
	{
		cRenderer3D *pRenderer = mpGame->GetGraphics()->GetRenderer3D();
		const cShadowMapSettings &shadows = pRenderer->GetShadowMapSettings();
		mpConfig->SetInt("Graphics","ShadowType",pRenderer->GetShadowType());
		mpConfig->SetInt("Graphics","ShadowFilter",shadows.mFilter);
		mpConfig->SetInt("Graphics","ShadowSpotSize",shadows.mlSpotSize);
		mpConfig->SetInt("Graphics","ShadowPointSize",shadows.mlPointSize);
		mpConfig->SetFloat("Graphics","ShadowSoftness",shadows.mfSoftness);
		mpConfig->SetFloat("Graphics","ShadowLightSize",shadows.mfLightSize);
		mpConfig->SetFloat("Graphics","ShadowMaxBlur",shadows.mfMaxBlur);
		mpConfig->SetInt("Graphics","ShadowSamples",shadows.mlSamples);
		mpConfig->SetInt("Graphics","ShadowBlockerSamples",shadows.mlBlockerSamples);
		mpConfig->SetBool("Graphics","ShadowNoise",shadows.mbNoise);
		mpConfig->SetFloat("Graphics","ShadowBias",shadows.mfBias);
		mpConfig->SetFloat("Graphics","ShadowSlopeBias",shadows.mfSlopeBias);
		mpConfig->SetInt("Graphics","ShadowFaces",shadows.mFaces);
		mpConfig->SetBool("Graphics","ShadowAlphaCasters",shadows.mbAlphaCasters);
	}
		
	Log(" Exit Effect Handler\n");
	hplDelete( mpEffectHandler );

	Log(" Exit Game\n");

	// MAIN EXIT /////////////////////
	//Delete the game,  do this after all else is deleted.
	hplDelete( mpGame );
	
	Log(" Saving last config\n");
	//Save the stuff to the config file
	mpConfig->SetInt("Screen","Width",mvScreenSize.x);
	mpConfig->SetInt("Screen","Height",mvScreenSize.y);
	mpConfig->SetBool("Screen", "FullScreen", mbFullScreenSetting);
	mpConfig->SetInt("Screen","Display",mlDisplay);
	mpConfig->SetBool("Screen", "Vsync", mbVsync);

	mpConfig->SetString("Demo","WebPageOnExit",msWebPageOnExit);
	
	mpConfig->SetString("Map","GlobalScript",msGlobalScriptFile);
	
	mpConfig->SetBool("Game","Subtitles",mbSubtitles);
	mpConfig->SetBool("Game","SimpleWeaponSwing",mbSimpleWeaponSwing); 
	mpConfig->SetBool("Game","DisablePersonalNotes",mbDisablePersonalNotes); 
	mpConfig->SetBool("Game","AllowQuickSave",mbAllowQuickSave); 
	mpConfig->SetBool("Game","FlashItems",mbFlashItems); 
	mpConfig->SetBool("Game","ShowCrossHair",mbShowCrossHair);

	mpConfig->SetBool("Game","SimpleSwingInOptions",mbSimpleSwingInOptions); 

	
	mpConfig->SetString("Map","File",msStartMap); 
	mpConfig->SetString("Map","StartPos",msStartLink);
	
	mpConfig->SetInt("Game","Difficulty",mDifficulty);
	mpConfig->SetString("Game","CurrentUser",msCurrentUser);
	mpConfig->SetString("Game","LanguageFile",msLanguageFile);
	mpConfig->SetBool("Game","ShowPreMenu",mbShowPreMenu);
	mpConfig->SetBool("Game","ShowMenu",mbShowMenu);
	mpConfig->SetBool("Game","ShowIntro",mbShowIntro);

	mpConfig->SetInt("Physics","PhysicsAccuracy",mPhysicsAccuracy);
	mpConfig->SetFloat("Physics","UpdatesPerSec",mfPhysicsUpdatesPerSec);


	mpConfig->SetBool("Debug", "LogResources", mbLogResources);

	mpConfig->Save();
	hplDelete( mpConfig );

	hplDelete( mpGameConfig );

	
#ifdef DEMO_VERSION
	if(msWebPageOnExit != "")
		OpenBrowserWindow(cString::To16Char(msWebPageOnExit));
#endif
}

//-----------------------------------------------------------------------

void cInit::RunScriptCommand(const tString& asCommand)
{
	if(asCommand[0]=='@')
	{
		if(mpGlobalScript && mpGlobalScript->Run(cString::Sub(asCommand,1))==false)
		{
			Warning("Couldn't run command '%s' from global script\n",asCommand.c_str());
		}
	}
	else
	{
		cWorld3D *pWorld = mpGame->GetScene()->GetWorld3D();
		if(pWorld->GetScript()->Run(asCommand)==false)
		{
			Warning("Couldn't run command '%s' in map script file\n",asCommand.c_str());
		}
	}
}

//-----------------------------------------------------------------------
void cInit::PreloadSoundEntityData(const tString &asFile)
{
	if(asFile=="") return;
	
	mpGame->GetResources()->GetSoundEntityManager()->Preload(asFile);
		
	/*iSoundData *pSound = mpGame->GetResources()->GetSoundManager()->CreateSoundData(asFile,false);
	if(pSound){
		Warning("Couldn't preload sound '%s'\n",asFile.c_str());
	}*/
	
}

//-----------------------------------------------------------------------

void cInit::PreloadParticleSystem(const tString &asFile)
{
	if(asFile=="") return;
	cParticleSystem3D *pPS  = mpGame->GetResources()->GetParticleManager()->CreatePS3D(
																"",asFile,1,cMatrixf::Identity);
	hplDelete( pPS );
}

//-----------------------------------------------------------------------
