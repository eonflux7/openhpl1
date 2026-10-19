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
#ifndef GAME_NOTE_BOOK_H
#define GAME_NOTE_BOOK_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;
class cNotebook_GlobalSave;

//---------------------------

#define eNotebookState_Front		0
#define eNotebookState_TaskList		1
#define eNotebookState_NoteList		2
#define eNotebookState_Note			3

//---------------------------

enum eNotebookType
{
	eNotebookType_Front,
	eNotebookType_Open,
};

//---------------------------

class cNotebook_Note
{
public:
	cNotebook_Note() : mbRead(false), mfAlpha(0), mfReveal(0){}

	tWString msName;
	tWString msText;
	bool mbRead;
	float mfAlpha;
	float mfReveal; // How much of the pencil underline is drawn, 0..1
	cRect2f mRect;

	tString msTextCat;
	tString msTextEntry;
};

typedef std::list<cNotebook_Note*> tNotebook_NoteList;
typedef tNotebook_NoteList::iterator tNotebook_NoteListIt;

typedef cSTLIterator<cNotebook_Note*,tNotebook_NoteList,tNotebook_NoteListIt> tNotebook_NoteList_Iterator;

//--------------------------------------------------------

class cNotebook;

class iNotebookState : public iAIState
{
public:
	iNotebookState(cInit *apInit, cNotebook *apNotebook) : iAIState(){mpInit = apInit;mpNotebook = apNotebook;}
	
	virtual void OnMouseDown(eMButton aButton){}
	
	virtual void OnDraw(){}

	virtual void OnExit(){}

protected:
	cInit *mpInit;
	cNotebook *mpNotebook;
};

//---------------------------------------------------

class cNoteBook_Option
{
public:
	cNoteBook_Option() : mfAlpha(0), mfReveal(0){}

	tWString msText;
	cVector3f mvPos;
	cRect2f mRect;
	float mfAlpha;
	float mfReveal; // How much of the pencil underline is drawn, 0..1
};

class cNotebookState_Front : public iNotebookState
{
public:
	cNotebookState_Front(cInit *apInit, cNotebook *apNotebook);
	
	void OnEnterState(int alLastState) override;
	void OnLeaveState(int alNextState) override;

	void OnUpdate(float afTime) override;

	void OnDraw() override;

	void OnMouseDown(eMButton aButton) override;

	void OnExit() override;

	cGraphicsDrawer *mpDrawer;
	cGfxObject *mpTextBack;
	cGfxObject *mpUnderline = nullptr; // The remastered look's, else null

	iFontData *mpFrontFont = nullptr;
	std::vector<cNoteBook_Option> mvOptions;
	int mlSelected;
	cVector2f mvFontSize;
};

//-----------------------------------------------------

class cNotebook_TaskPage
{
public:
	int mlStartTask;
	int mlEndTask;
};

//-----------------------------------------------------

class cNotebookState_TaskList : public iNotebookState
{
public:
	cNotebookState_TaskList(cInit *apInit, cNotebook *apNotebook);

	void OnEnterState(int alLastState) override;
	void OnLeaveState(int alNextState) override;

	void OnUpdate(float afTime) override;

	void OnDraw() override;

	void OnMouseDown(eMButton aButton) override;

	void OnExit() override;

	iFontData *mpTextFont;

	float mfFontSize;

	int mlCurrentPage;
	std::vector<cNotebook_TaskPage> mvTaskPages; 

	int mlSelected;
	std::vector<cNoteBook_Option> mvOptions;
	cGraphicsDrawer *mpDrawer;
	cGfxObject *mpOptionsImage[2];
};

//--------------------------------------------------------

class cNotebookState_NoteList : public iNotebookState
{
public:
	cNotebookState_NoteList(cInit *apInit, cNotebook *apNotebook);

	void OnEnterState(int alLastState) override;
	void OnLeaveState(int alNextState) override;

	void OnUpdate(float afTime) override;
	void OnDraw() override;
	void OnMouseDown(eMButton aButton) override;
	void OnExit() override;

	cGraphicsDrawer *mpDrawer;
	cGfxObject *mpTextBack;
	cGfxObject *mpTab;
	// The remastered look's pencil and ink marks (null with the original HUD)
	cGfxObject *mpUnderline[3] = {};
	cGfxObject *mpRule = nullptr;
	cGfxObject *mpMargin = nullptr;
	cGfxObject *mpUnread = nullptr;

	int mlMaxNotesPerPage;
	int mlFirstNote;
	
	int mlSelected;

	iFontData *mpTextFont;
	iFontData *mpHeadingFont = nullptr;
	tWString msUnread;
	cVector2f mvSize;
	cNotebook_Note *mpSelectedNote;

	std::vector<cNoteBook_Option> mvOptions;
	cGfxObject *mpOptionsImage[2];
};

//--------------------------------------------------------

class cNotePage
{
public:
	int mlFirstRow;
	// Exclusive; a [new_page] marker is not part of the drawn range.
	int mlEndRow;
};

class cNotebookState_Note : public iNotebookState
{
public:
	cNotebookState_Note(cInit *apInit, cNotebook *apNotebook);

	void OnEnterState(int alLastState) override;
	void OnLeaveState(int alNextState) override;

	void OnUpdate(float afTime) override;
	void OnDraw() override;
	void OnMouseDown(eMButton aButton) override;
	void OnExit() override;

	iFontData *mpTextFont;
	iFontData *mpNavigateFont;
	tWStringVec mvRows;
	
	std::vector<cNotePage> mvPages;

	int mlCurrentPage;
	int mlMaxRows;
	cVector2f mvSize;
	
	int mlSelected;
	std::vector<cNoteBook_Option> mvOptions;
	cGraphicsDrawer *mpDrawer;
	cGfxObject *mpOptionsImage[2];

};

//--------------------------------------------------------

class cNotebook_BookType
{
public:
	cGfxObject *mpGfxImage;
	float mfAlpha;
	eNotebookType mType;
};

//--------------------------------------------------------

class cNotebook_BookTask
{
public:
	tString msName;
	tWString msText;
};

typedef std::list<cNotebook_BookTask*> tNotebook_BookTaskList;
typedef tNotebook_BookTaskList::iterator tNotebook_BookTaskListIt;

typedef cSTLIterator<cNotebook_BookTask*,tNotebook_BookTaskList,tNotebook_BookTaskListIt> tNotebook_BookTaskList_Iterator;

//--------------------------------------------------------

class cNotebook : public iUpdateable
{
public:
	cNotebook(cInit *apInit);
	~cNotebook() override;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void OnDraw() override;
	void Reset() override;

	void SetMousePos(const cVector2f &avPos);
	void AddMousePos(const cVector2f &avRel);
	cVector2f GetMousePos(){ return mvMousePos;}

	void OnMouseDown(eMButton aButton);
	void OnMouseUp(eMButton aButton);

	void OnExit() override;
	
	void SetActive(bool abActive);
	bool IsActive(){ return mbActive;}

	// Black Plague: script function called, once, when the notebook is closed
	void SetCloseCallback(const tString& asFunc){ msCloseCallback = asFunc;}

	float GetAlpha(){ return mfAlpha;}

	void SetBookType(eNotebookType aType);
	eNotebookType GetBookType(){ return mBookType;}

	tNotebook_NoteList_Iterator GetNoteIterator();
	cNotebook_Note* AddNote(const tWString &asName, const tString &asTextCat, const tString &asTextEntry);
	bool NoteExists(const tString &asTextCat, const tString &asTextEntry);
	void RemoveNote(const tString &asName);
	int GetNoteNum(){ return (int)mlstNotes.size();}

	tNotebook_BookTaskList_Iterator GetTaskIterator();
	void AddTask(const tString &asName, const tWString &asText);
	void RemoveTask(const tString &asName);
	int GetTaskNum(){ return (int)mlstTasks.size();}
	
	cNotebook_Note* GetSelectedNote(){ return mpSelectedNote;}
	void SetSelectedNote(cNotebook_Note* apNote){ mpSelectedNote = apNote;}

	cStateMachine* GetStateMachine(){return &mStateMachine;}

	/////////////////////////////////////////
	//Saving
	void SaveToGlobal(cNotebook_GlobalSave *apSave);
	void LoadFromGlobal(cNotebook_GlobalSave *apSave);

private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpGfxBackground;
		
	cStateMachine mStateMachine;
	
	bool mbActive;
	float mfAlpha;

	tString msCloseCallback;

	std::vector<cNotebook_BookType> mvBookTypes;

	eNotebookType mBookType;

	eCrossHairState mLastCrossHairState;

	bool mbInventoryWasActive;

	tNotebook_BookTaskList mlstTasks;
	tNotebook_NoteList mlstNotes;

	cNotebook_Note *mpSelectedNote;

	cVector2f mvMousePos;
};



#endif // GAME_NOTE_BOOK_H
