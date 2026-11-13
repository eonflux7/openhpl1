// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_HAPTIC_SHAPE_H
#define HPL_HAPTIC_SHAPE_H

#include "math/MathTypes.h"
#include "haptic/HapticTypes.h"
#include"graphics/GraphicsTypes.h"

namespace hpl {

	class iHapticSurface;
	class iLowLevelGraphics;
	class iPhysicsBody;
	class cSubMeshEntity;

	class iHapticShape
	{
	public:
		iHapticShape(const tString& asName ,eHapticShapeType aType) :
									mpSurface(nullptr), mType(aType), msName(asName),
									mpBody(nullptr),mpSubMeshEntity(nullptr), mlTransformCount(-1) {}
		virtual ~iHapticShape(){}

		eHapticShapeType GetType(){ return mType;}
		const tString& GetName(){ return msName;}

		virtual void SetEnabled(bool abX)=0;
		virtual bool GetEnabled()=0;

		virtual void SetTransform(const cMatrixf &a_mtxTransform)=0;
		virtual cMatrixf GetTransform()=0;

		virtual cVector3f GetAppliedForce()=0;

		virtual void SetSurface(iHapticSurface *apSurface)=0;

		virtual void RenderDebug(iLowLevelGraphics *apLowLevel, const cColor &aColor)=0;

		iHapticSurface* GetSurface()const{ return mpSurface; }

		cVector3f GetSize() const{ return mvSize;}

		iPhysicsBody* GetBody() const{ return mpBody; }
		void SetBody(iPhysicsBody* apBody){ mpBody = apBody; }

		cSubMeshEntity* GetSubMeshEntity() const { return mpSubMeshEntity;}
		void SetSubMeshEntity(cSubMeshEntity* apSubMeshEntity) { mpSubMeshEntity = apSubMeshEntity;}

		int GetTransformCount() const{ return mlTransformCount;}
		void SetTransformCount(int alX){ mlTransformCount = alX;}

	protected:
		tString msName;
		iHapticSurface* mpSurface;
		eHapticShapeType mType;
		cVector3f mvSize;

		iPhysicsBody *mpBody;
		cSubMeshEntity *mpSubMeshEntity;
		int mlTransformCount;

	};

};
#endif // HPL_HAPTIC_SHAPE_H