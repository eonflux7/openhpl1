// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_COLLIDER_ENTITY_H
#define HPL_COLLIDER_ENTITY_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/Renderable.h"

namespace hpl {

	class iPhysicsWorld;
	class iPhysicsBody;

	class cColliderEntity : public iRenderable
	{
	public:
		cColliderEntity(const tString &asName,iPhysicsBody* apBody, iPhysicsWorld *apWorld);
		~cColliderEntity() override;

		iMaterial *GetMaterial() override{ return nullptr;}
		void UpdateGraphics(cCamera3D *apCamera, float afFrameTime, cRenderList *apRenderList) override{}
		iVertexBuffer* GetVertexBuffer() override{ return nullptr;}

		cBoundingVolume* GetBoundingVolume() override;

		cMatrixf* GetModelMatrix(cCamera3D* apCamera) override;
		int GetMatrixUpdateCount() override;

		eRenderableType GetRenderType() override{ return eRenderableType_Dummy;}

		//Entity implementation
		tString GetEntityType() override{ return "Collider";}
		bool IsVisible() override{ return false; }

		bool IsShadowCaster() override{ return false;}

		void UpdateLogic(float afTimeStep) override{}

	private:
		iPhysicsWorld* mpPhysicsWorld;
		iPhysicsBody* mpBody;
	};
};
#endif // HPL_COLLIDER_ENTITY_H
