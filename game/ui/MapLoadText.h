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
#ifndef GAME_MAP_LOAD_TEXT_H
#define GAME_MAP_LOAD_TEXT_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

//-------------------------------------------------

class cMapLoadText : public iUpdateable
{
public:
	cMapLoadText(cInit *apInit);
	~cMapLoadText() override;

	void Reset() override;

	void OnPostSceneDraw() override;
	
	void OnDraw() override;
    
	void Update(float afTimeStep) override;
	
	void OnMouseDown(eMButton aButton);
	void OnMouseUp(eMButton aButton);

	void SetText(tString asCat, tString asEntry){
		msTextCat = asCat;
		msTextEntry =asEntry;
	}

	// Black Plague: the picture behind the text (Overture's is fixed)
	void SetBackground(const tString &asFile){ msBackground = asFile; }

	void SetActive(bool abX);
	bool IsActive(){ return mbActive;}

	void OnExit() override;
private:
	void DrawText(bool abLoading);

	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	iFontData *mpFont;
	iFontData *mpTextFont;

	tWStringVec mvRows;

	iTexture *mpBack;

	tString msTextCat;
	tString msTextEntry;
	tString msBackground;

	float mfAlpha;
	float mfAlphaAdd;

	bool mbActive;
};

//---------------------------------------------


#endif // GAME_MAP_LOAD_TEXT_H
