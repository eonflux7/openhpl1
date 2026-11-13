// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_MUSIC_HANDLER_H
#define GAME_GAME_MUSIC_HANDLER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;
class cMapHandler;
class cGameMusicHandler_GlobalSave;
class iGameEnemy;

class cGameMusic
{
friend class cGameMusicHandler;
public:
	cGameMusic();
	
	void Reset();

private:
	tString msFile;
	
	bool mbLoop;
	float mfVolume;
};

typedef std::set<iGameEnemy*> tAttackerSet;
typedef tAttackerSet::iterator tAttackerSetIt;

class cGameMusicHandler : public iUpdateable
{
public:
	cGameMusicHandler(cInit *apInit);
	~cGameMusicHandler() override;
	
	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;

	void OnWorldLoad();
	void OnWorldExit();

	void LoadFromGlobal(cGameMusicHandler_GlobalSave *apSave);
	void SaveToGlobal(cGameMusicHandler_GlobalSave *apSave);

	void Play(const tString &asFile, bool abLoop,float afVolume, float afFade, int alPrio);
	void Stop(float afFade, int alPrio);

	void AddAttacker(iGameEnemy *apEntity);
	void RemoveAttacker(iGameEnemy *apEntity);
	bool AttackerExist(iGameEnemy *apEntity);

private:
	void PlayHighestPriority();

	cInit *mpInit;
	cMusicHandler *mpMusicHandler;

	std::vector<cGameMusic> mvGameMusic;
	int mlMaxPrio;

	int mlCurrentMaxPrio;

	tAttackerSet m_setAttackers;

	bool mbAttackPlaying;
	float mfAttackStartElapsedSeconds;
	float mfAttackStopElapsedSeconds;

	bool mbEnemyClosePlaying;
	float mfEnemyCloseElapsedSeconds;
	float mfEnemyGoneElapsedSeconds;
};



#endif // GAME_GAME_MUSIC_HANDLER_H
