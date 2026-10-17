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
#ifndef GAME_INIT_H
#define GAME_INIT_H

#include "core/StdAfx.h"

using namespace hpl;

class cMapHandler;
class cButtonHandler;
class cPlayer;
class cGameScripts;
class cGameMessageHandler;
class cInventory;
class cFadeHandler;
class cSaveHandler;
class cTriggerHandler;
class cAttackHandler;
class cNotebook;
class cNumericalPanel;
class cDeathMenu;
class cGraphicsHelper;
class cEffectHandler;
class cPlayerHands;
class cGameMusicHandler;
class cRadioHandler;
class cGameGuiHandler;
class cGameSoundEnvHandler;
class cEnding;
class cMapLoadText;
class cPreMenu;
class cCredits;
class cDemoEndText;

class cIntroStory;

class cMainMenu;
class cSmokeTest;
class cRefShots;
class cRefView;
class cLampLooks;
class cPhysicsLog;
class cDogTestMap;

#include "core/GameTypes.h"

#if defined(WIN32)
	#define PERSONAL_RELATIVEROOT _W("")
	#define PERSONAL_RELATIVEPIECES
	#define PERSONAL_RELATIVEPIECES_COUNT 0
	#define PERSONAL_RELATIVEGAME_PARENT _W("Penumbra Overture/")
#elif defined(__linux__)
	#define PERSONAL_RELATIVEROOT _W(".frictionalgames/")
	#define PERSONAL_RELATIVEPIECES _W(".frictionalgames"),
	#define PERSONAL_RELATIVEPIECES_COUNT 1
	#define PERSONAL_RELATIVEGAME_PARENT _W("Penumbra/")
#else
	#define PERSONAL_RELATIVEROOT _W("Library/Preferences/Frictional Games/")
	#define PERSONAL_RELATIVEPIECES _W("Library"), _W("Library/Preferences"), _W("Library/Preferences/Frictional Games"),
	#define PERSONAL_RELATIVEPIECES_COUNT 3
	#define PERSONAL_RELATIVEGAME_PARENT _W("Penumbra/")
#endif
// The folder of the game being played, with the root in front: settings,
// saves and logs. Separate from the original games so theirs are untouched.
#define PERSONAL_RELATIVEGAME (tWString(PERSONAL_RELATIVEROOT) + cInit::GetPersonalGameDir())

//--------------------------------------------------------

// The three games share the engine and most of the game code; which one is
// played is picked at startup (OVERTURE_GAME, or by the data in the working
// directory).
enum eGameId
{
	eGameId_Overture,
	eGameId_BlackPlague,
	eGameId_Requiem,
	eGameId_LastEnum
};

//--------------------------------------------------------

//#define DEMO_VERSION

//--------------------------------------------------------

class cInit : public iUpdateable
{
public:
	cInit();
	~cInit() override;

	bool Init(tString asCommandLine);
	// Optional isolated settings, saves and caches for automated checks.
	static tWString GetPersonalDir();
	void Run();
	void Exit();
	
	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;

	void ResetGame(bool abRunInitScript);
	bool GetImprovedDogAI() const { return mbImprovedDogAI; }
	void SetImprovedDogAI(bool abX) { mbImprovedDogAI = abX; }

	void RunScriptCommand(const tString& asCommand);

	void CreateHardCodedPS(iParticleEmitterData *apPE);

	static eGameId GetGameId(){ return mGameId; }
	static const char* GetGameName(eGameId aId);

	// Changing game: the executable is started again in the other game's
	// folder after a normal exit. The folders are remembered in games.cfg
	// next to the games' personal folders; Steam's are found without it.
	static bool CanChangeGame();
	// The folder holding a game's data, "" if not found
	static tString FindGameFolder(eGameId aId);
	// Saves and exits as Quit does; hplMain then calls RunRequestedGame
	void RequestGameChange(eGameId aId);
	// Does not return when it succeeds
	static void RunRequestedGame();
	static bool IsOverture(){ return mGameId == eGameId_Overture; }
	// Folder of this game under the personal root, ending in a slash
	static tWString GetPersonalGameDir();
	// The files that are named differently in Requiem
	static tString GetStartupConfigFile();
	static tWString GetMapsDir();
	//The music of the main menu and the menu before it
	static tString GetMenuMusic();

	void PreloadSoundEntityData(const tString &asFile);
	void PreloadParticleSystem(const tString &asFile);
	
	//Main objects
	cConfigFile* mpConfig;
	cConfigFile* mpGameConfig;
	cGame *mpGame;
	iScript *mpGlobalScript;

	iFontData *mpDefaultFont;
	
	//Main properties
	// The size, fullscreen and display settings are saved to the config and
	// used at the next start; mbFullScreen is how this run was started.
	cVector2l mvScreenSize;
	bool mbFullScreen;
	bool mbFullScreenSetting;
	int mlDisplay;
	bool mbVsync;
	tString msGlobalScriptFile;
	tString msLanguageFile;
	bool mbLogResources;
	bool mbShowPreMenu;
	bool mbShowMenu;
	bool mbShowIntro;
	int mlFSAA;
	eGameDifficulty mDifficulty;
	
	bool mbHasHaptics;
	bool mbHasHapticsOnRestart;
	bool mbHapticsAvailable;
	float mfHapticForceMul;
	float mfHapticMoveScreenSpeedMul;
	float mfHapticScale;
	float mfHapticProxyRadius;
	float mfHapticOffsetZ;
	float mfHapticMaxInteractDist;
		
	int mlMaxSoundChannels;
    int mlStreamUpdateFreq;
	bool mbUseSoundHardware;
	bool mbUseEnvAudio;
	// Black Plague: New Game asks once whether to play the tutorial first
	bool mbAskTutorial;
	bool mbUseSoundThreading;
	int mlMaxMonoChannelsHint;
	int mlMaxStereoChannelsHint;
	tString msDeviceName;

	ePhysicsAccuracy mPhysicsAccuracy;
	float mfPhysicsUpdatesPerSec;

	tString msStartMap;
	tString msStartLink;
	// OVERTURE_MAP=map[:link] loads this map at startup, skipping the menus
	tString msDirectMap;
	tString msDirectLink;
	
	bool mbSubtitles;
	bool mbSimpleWeaponSwing;
	bool mbDisablePersonalNotes;
	bool mbAllowQuickSave;
	bool mbFlashItems;
	bool mbSimpleSwingInOptions;
	bool mbShowCrossHair;

	tWString msErrorMessage;

	tString msWebPageOnExit;

	bool mbDebugInteraction;
	float mfMaxPhysicsTimeStep;
	bool mbDestroyGraphics; //If graphics should be destroyed by the entities. Set to false at map change.
	bool mbResetCache;
	bool mbPostEffects;
	int mlMaxSoundDataNum;
	int mlMaxPSDataNum;

	bool mbWeaponAttacking; //To see if it is a weapon that is dealing the damage.

	static eGameId mGameId;
	static eGameId mRequestedGameId;

	tString msCurrentUser;

	//Game objects
	cMapHandler *mpMapHandler;
	cButtonHandler *mpButtonHandler;
	cPlayer *mpPlayer;
	//cGameScripts *mpGameScripts;
	cGameMessageHandler *mpGameMessageHandler;
	cInventory* mpInventory;
	cFadeHandler* mpFadeHandler;
	cSaveHandler* mpSaveHandler;
	cTriggerHandler* mpTriggerHandler;
	cAttackHandler* mpAttackHandler;
	cNotebook* mpNotebook;
	cNumericalPanel* mpNumericalPanel;
	cDeathMenu* mpDeathMenu;
	cGraphicsHelper *mpGraphicsHelper;
	cEffectHandler *mpEffectHandler;
	cPlayerHands *mpPlayerHands;
	cGameMusicHandler *mpMusicHandler;
	cRadioHandler *mpRadioHandler;
	cGameGuiHandler *mpGameGuiHandler;
	cGameSoundEnvHandler *mpSoundEnvHandler;
	cEnding *mpEnding;
	cMapLoadText *mpMapLoadText;
	cPreMenu *mpPreMenu;
	cCredits *mpCredits;
	cDemoEndText *mpDemoEndText;
	
	cMainMenu *mpMainMenu;

	cSmokeTest *mpSmokeTest; // NULL unless OVERTURE_SMOKE is set
	cRefShots *mpRefShots; // NULL unless OVERTURE_REFSHOTS is set
	cRefView *mpRefView; // NULL during the smoke test and the reference images
	cDogTestMap *mpDogTestMap;
	bool mbImprovedDogAI = true;
	cLampLooks *mpLampLooks;
	cPhysicsLog *mpPhysicsLog; // NULL unless OVERTURE_PHYSLOG is set

	cIntroStory* mpIntroStory;	

};


#endif // GAME_INIT_H
