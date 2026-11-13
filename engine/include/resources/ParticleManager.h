// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PARTICLE_SYSTEM_MANAGER_H
#define HPL_PARTICLE_SYSTEM_MANAGER_H

#include "resources/ResourceManager.h"

#include "math/MathTypes.h"

namespace hpl {

	class cGraphics;
	class cResources;
	class iParticleSystem2D;
	class cParticleSystem3D;
	class cParticleSystemData3D;

	typedef std::map<tString, cParticleSystemData3D*> tParticleSystemData3DMap;
	typedef tParticleSystemData3DMap::iterator tParticleSystemData3DMapIt;

	class cParticleManager : public iResourceManager
	{
	public:
		cParticleManager(cGraphics* apGraphics,cResources *apResources);
		~cParticleManager() override;

		/**
		 * NOT USED! For now that is... there is no need for it since particle systems are not really
		 * resources, yet
		 * \param asName
		 * \return
		 */
		iResourceBase* Create(const tString& asName) override;

		iParticleSystem2D* CreatePS2D(const tString& asName, cVector3f avSize);

		cParticleSystem3D* CreatePS3D(const tString& asName,const tString& asType, cVector3f avSize,
										const cMatrixf& a_mtxTransform);

		void AddData3D(cParticleSystemData3D *apData);

		void Preload(const tString& asFile);

		void Destroy(iResourceBase* apResource) override;
		void Unload(iResourceBase* apResource) override;
	private:
		cGraphics* mpGraphics;
		cResources *mpResources;

		std::list<cParticleSystem3D*> mlstSystems;

	};

};
#endif // HPL_PARTICLE_SYSTEM_MANAGER_H
