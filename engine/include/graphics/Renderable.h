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
#ifndef HPL_RENDERABLE_H
#define HPL_RENDERABLE_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"
#include "scene/Entity3D.h"

namespace hpl {

	class iMaterial;
	class iVertexBuffer;

	class cBoundingVolume;
	class cFrustum;

	class iLight3D;
	class cCamera3D;

	class cRenderList;

	class iRenderable;
	class iGpuProgram;
	class cRenderSettings;

	/**
	 * How a see-through object draws itself in place of what its material
	 * says, see iRenderable::GetTransDraw(). What is left as it is here is
	 * taken from the material and the object as usual.
	 */
	class cTransDraw
	{
	public:
		iGpuProgram *mpVtxProgram = nullptr;
		iGpuProgram *mpFragProgram = nullptr;
		iVertexBuffer *mpVtxBuffer = nullptr;
		cMatrixf *mpModelMatrix = nullptr;
		bool mbDepthTest = true;
		/// False to draw the faces turned away from the viewer too.
		bool mbCull = true;
	};

	enum eRenderableType
	{
		eRenderableType_Normal,
		eRenderableType_ParticleSystem,
		eRenderableType_Mesh,
		eRenderableType_Light,
		eRenderableType_Dummy,
		eRenderableType_LastEnum
	};

	//------------------------------------------

	kSaveData_ChildClass(iEntity3D, iRenderable)
	{
		kSerializableClassInit(iRenderable)
	public:
		bool mbStatic;
		bool mbRendered;
		float mfZ;
	};

	//------------------------------------------

	class iRenderable : public iEntity3D
	{
	#ifdef __GNUC__
		typedef iEntity3D __super;
	#endif
	public:
		iRenderable(const tString &asName);

		bool IsRendered(){ return mbRendered;}
		virtual void SetRendered(bool abX){ mbRendered = abX;}

		virtual iMaterial *GetMaterial()=0;
		virtual iVertexBuffer* GetVertexBuffer()=0;

		virtual bool IsShadowCaster()=0;

		virtual bool CollidesWithBV(cBoundingVolume *apBV);
		virtual bool CollidesWithFrustum(cFrustum *apFrustum);

		virtual cMatrixf* GetModelMatrix(cCamera3D *apCamera)=0;

		virtual eRenderableType GetRenderType()=0;

		virtual void UpdateGraphics(cCamera3D *apCamera,float afFrameTime, cRenderList *apRenderList){}

		/**
		 * For the pass of what is see-through: true if the object is drawn
		 * with programs of its own, which it puts in aDraw.
		 */
		virtual bool GetTransDraw(cCamera3D *apCamera, cTransDraw &aDraw){ return false; }
		/**
		 * Called before each draw GetTransDraw() set up, with its programs
		 * bound and "worldViewProj" set, for the object to set the rest.
		 */
		virtual void SetupTransDraw(cCamera3D *apCamera, cRenderSettings *apSettings, cTransDraw &aDraw){}

		/**
		 * Should return a different number each time the renderable model matrix is updated. never -1
		 * \return
		 */
		virtual int GetMatrixUpdateCount()=0;

		float GetZ(){ return mfZ;}
		void SetZ(float afZ){ mfZ = afZ;}

		cMatrixf* GetInvModelMatrix();

		const cVector3f& GetCalcScale();

		void SetStatic(bool abX){ mbStatic = abX;}
		bool IsStatic() const { return mbStatic;}

		inline int GetRenderCount() const { return mlRenderCount;}
		inline void SetRenderCount(const int alCount) { mlRenderCount = alCount;}

		void SetForceShadow(bool abX){ mbForceShadow = abX;}
		bool GetForceShadow(){ return mbForceShadow;}

		bool GetIsOneSided(){ return mbIsOneSided;}
		const cVector3f& GetOneSidedNormal(){ return mvOneSidedNormal;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	protected:
		cMatrixf m_mtxInvModel;

		bool mbIsOneSided;
		cVector3f mvOneSidedNormal;

		int mlLastMatrixCount;

		bool mbStatic;

		bool mbForceShadow;

		int mlRenderCount;

		int mlCalcScaleMatrixCount;
		cVector3f mvCalcScale;

		bool mbRendered;
		float mfZ;
	};
};
#endif // HPL_RENDERABLE_H
