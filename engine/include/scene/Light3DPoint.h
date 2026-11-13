// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LIGHT3D_POINT_H
#define HPL_LIGHT3D_POINT_H

#include "scene/Light3D.h"

namespace hpl {

	kSaveData_ChildClass(iLight3D,cLight3DPoint)
	{
		kSaveData_ClassInit(cLight3DPoint)
	public:

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class cLight3DPoint : public iLight3D
	{
	#ifdef __GNUC__
		typedef iLight3D __super;
	#endif
	public:
		cLight3DPoint(tString asName, cResources *apResources);

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	private:
		cSectorVisibilityContainer* CreateSectorVisibility() override;
		void UpdateBoundingVolume() override;
		bool CreateClipRect(cRect2l &aCliprect,cRenderSettings *apRenderSettings,iLowLevelGraphics *apLowLevelGraphics) override;

	};

};
#endif // HPL_LIGHT3D_POINT_H
