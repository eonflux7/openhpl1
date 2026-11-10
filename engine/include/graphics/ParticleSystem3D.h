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
#ifndef HPL_PARTICLE_SYSTEM_3D_H
#define HPL_PARTICLE_SYSTEM_3D_H

#include "scene/Entity3D.h"
#include "graphics/RenderList.h"
#include "resources/ResourceBase.h"

namespace hpl {

	class iParticleEmitter3D;
	class iParticleEmitterData;
	class cParticleManager;

	//----------------------------------------------------

	class cParticleSystem3D;

	class cParticleSystemData3D : public iResourceBase
	{
	public:
		cParticleSystemData3D(const tString &asName,cResources* apResources,cGraphics *apGraphics);
		~cParticleSystemData3D() override;

		/// abOriginal: the emitters of GetOriginal() where there is one
		cParticleSystem3D* Create(tString asName, cVector3f avSize,const cMatrixf& a_mtxTransform,
									bool abOriginal=false);

		/// abOriginal: the game's own materials where an addon replaces them
		bool LoadFromFile(const tString &asFile, bool abOriginal=false);

		void AddEmitterData(iParticleEmitterData *apData);

		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}

		iParticleEmitterData* GetEmitterData(int alIdx) const { return mvEmitterData[alIdx]; }
		int GetEmitterDataNum() const { return (int)mvEmitterData.size(); }

		/// Adds new emitters of this data to apPS
		void CreateEmitters(cParticleSystem3D *apPS, const tString &asName, const cVector3f &avSize);

		/// An emitter uses a material that an addon replaces
		bool UsesReplacedMaterial() const;

		/**
		 * The system as the game has it, where an addon replaces the .ps
		 * file or a material it uses; nullptr if nothing is replaced. This
		 * data owns it.
		 */
		cParticleSystemData3D* GetOriginal() const { return mpOriginal; }
		void SetOriginal(cParticleSystemData3D *apData);

	private:
		cResources* mpResources;
		cGraphics *mpGraphics;

		std::vector<iParticleEmitterData*> mvEmitterData;
		cParticleSystemData3D *mpOriginal;
	};

	//----------------------------------------------------

	class cSaveData_ParticleEmitter3D : public iSerializable
	{
		kSerializableClassInit(cSaveData_ParticleEmitter3D)
	public:
		bool mbActive;
		bool mbDying;
	};

	kSaveData_ChildClass(iEntity3D,cParticleSystem3D)
	{
		kSaveData_ClassInit(cParticleSystem3D)
	public:

		tString msDataName;
		cVector3f mvDataSize;

		cContainerVec<cSaveData_ParticleEmitter3D> mvEmitters;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	//----------------------------------------------------


	class cParticleSystem3D : public iEntity3D
	{
	#ifdef __GNUC__
		typedef iEntity3D __super;
	#endif
	public:
		cParticleSystem3D(	const tString asName,cParticleSystemData3D *apData,
							cResources *apResources, cGraphics *apGraphics);
		~cParticleSystem3D() override;

		void SetVisible(bool abVisible) override;

		void UpdateLogic(float afTimeStep) override;

		bool IsDead();
		bool IsDying();

		void Kill();
		void KillInstantly();

		void AddEmitter(iParticleEmitter3D* apEmitter);
		iParticleEmitter3D* GetEmitter(int alIdx);
		int GetEmitterNum();

		tString GetEntityType() override{ return "ParticleSystem3D"; }

		void SetDataName(const tString &asName) {msDataName = asName;}
		void SetDataSize(const cVector3f &avSize) {mvDataSize = avSize;}

		const tString& GetDataName() { return msDataName;}
		const cVector3f& GetDataSize() { return mvDataSize;}

		void SetParticleManager(cParticleManager *apParticleManager){
			mpParticleManager = apParticleManager;
		}

		cParticleSystemData3D *GetData(){ return mpData;}

		/// An addon replaces the system or its materials: it can be shown as the game has it
		bool HasOriginal();
		/// The emitters are those of the game's own files (see cParticleSystemData3D::GetOriginal())
		bool IsOriginal(){ return mbOriginal;}
		/**
		 * The data the emitters were made from. Used by cWorld3D::SetParticleSystemOriginal(),
		 * which replaces the emitters.
		 */
		cParticleSystemData3D *GetEmitterData();
		void SetOriginal(bool abX){ mbOriginal = abX; }
		/// Removes the emitters from the system and deletes them
		void DeleteEmitters();
		/// Warm up again on the next update, for new emitters
		void RestartWarmUp(){ mbFirstUpdate = true; }

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	private:
		cResources *mpResources;
		cGraphics *mpGraphics;
		cParticleManager *mpParticleManager;
		cParticleSystemData3D *mpData;

		std::vector<iParticleEmitter3D*> mvEmitters;

		tString msDataName;
		cVector3f mvDataSize;

		bool mbFirstUpdate;
		bool mbOriginal;
	};

};
#endif // HPL_PARTICLE_SYSTEM_3D_H
