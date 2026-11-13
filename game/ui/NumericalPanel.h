// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_NUMERICAL_PANEL_H
#define GAME_NUMERICAL_PANEL_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

class cNumericalPanel;

class cNumericalButton
{
public:
	cNumericalButton(cInit *apInit, cNumericalPanel* apPanel, cVector2f avPos, cVector2f avSize, int alNum);
	~cNumericalButton();

	void OnUpdate(float afTimeStep);

	void OnDraw();

	void OnMouseDown();
	void OnMouseUp();
	void OnMouseOver(bool abOver);
	
	const cRect2f& GetRect(){return mRect;}

private:
	cGfxObject *mpGfxUp;
	cGfxObject *mpGfxDown;

	float mfAlpha;
	bool mbOver;
	
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;
	cNumericalPanel *mpPanel;

	cVector3f mvPosition;
	cRect2f mRect;

	int mlNum;
};

typedef std::list<cNumericalButton*> tNumericalButtonList;
typedef tNumericalButtonList::iterator tNumericalButtonListIt;

class cNumericalPanel : public iUpdateable
{
friend class cNumericalButton;
public:
	cNumericalPanel(cInit *apInit);
	~cNumericalPanel() override;
	
	void Reset() override;
	
	void OnDraw() override;

	void Update(float afTimeStep) override;

	void SetMousePos(const cVector2f &avPos);
	void AddMousePos(const cVector2f &avRel);
	cVector2f GetMousePos(){ return mvMousePos;}

	void OnMouseDown(eMButton aButton);
	void OnMouseUp(eMButton aButton);

	void SetActive(bool abActive);
	bool IsActive(){ return mbActive;}

	void OnExit() override;

	void AddDigit(int alNum);

	void SetUp(const tString& asName, const tString& asCallback);

	void SetCode(tIntVec &avCode);

private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpGfxBackground;
	cGfxObject *mpGfxPanel;

	bool mbMouseIsDown;

	bool mbActive;
	float mfAlpha;

	cVector2f mvMousePos;

	eCrossHairState mLastCrossHairState;

	tNumericalButtonList mlstButtons;

	tString msName;
	tString msCallback;

	tIntVec mvDigits;
	tIntVec mvCode;
};

#endif // GAME_NUMERICAL_PANEL_H
