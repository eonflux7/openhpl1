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
#ifndef HPL_VERTEXBUFFER_H
#define HPL_VERTEXBUFFER_H

#include "graphics/GraphicsTypes.h"
#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "math/BoundingVolume.h"

namespace hpl {

	class cBoundingVolume;

	enum eVertexBufferDrawType
	{
		eVertexBufferDrawType_Tri,
		eVertexBufferDrawType_Lines,
		eVertexBufferDrawType_LastEnum
	};

	enum eVertexBufferUsageType
	{
		eVertexBufferUsageType_Static,
		eVertexBufferUsageType_Dynamic,
		eVertexBufferUsageType_Stream,
		eVertexBufferUsageType_LastEnum
	};

	typedef tFlag tVertexFlag;

	#define eVertexFlag_Normal   (0x01)
	#define eVertexFlag_Position (0x02)
	#define eVertexFlag_Color0   (0x04)
	#define eVertexFlag_TexCoord (0x08)
	#define eVertexFlag_Tangent  (0x10)

	#define klNumOfVertexFlags (5)
	const tVertexFlag kvVertexFlags[] = {eVertexFlag_Normal, eVertexFlag_Position,
		eVertexFlag_Color0, eVertexFlag_TexCoord, eVertexFlag_Tangent};
	// CPU editing arrays are tightly packed per attribute. GPU uploads interleave
	// them; callers (skinning, physics, caches) never use the GPU stride.
	const int kvVertexElements[] = {3, 4, 4, 3, 4};

	typedef tFlag tVertexCompileFlag;

	#define eVertexCompileFlag_CreateTangents	(0x00000001)


	// One sheet, expanded from shared triangle corners in the vertex shader.
	// Extents are half-width/half-height vectors in the object's draw space.
	// UV bounds are left, bottom, right, top; endpoint colours run bottom to top.
	struct cQuadInstance
	{
		cVector3f mvCentre = cVector3f(0);
		float mfKind = 1; // 1: billboard, 2: beam, 3: particle
		cVector3f mvRight = cVector3f(0.5f,0,0);
		cVector3f mvUp = cVector3f(0,0.5f,0);
		cColor mColor = cColor(1,1);
		cColor mEndColor = cColor(1,1);
		float mvUV[4] = {0,0,1,1};
	};
	static_assert(sizeof(cQuadInstance) == 22 * sizeof(float), "Instance GPU layout");

	class iLowLevelGraphics;

	class iVertexBuffer
	{
	public:
		iVertexBuffer(iLowLevelGraphics* apLowLevelGraphics,tVertexFlag aFlags,
			eVertexBufferDrawType aDrawType,eVertexBufferUsageType aUsageType,
			int alReserveVtxSize,int alReserveIdxSize) :
			mVertexFlags(aFlags), mpLowLevelGraphics(apLowLevelGraphics),
			mDrawType(aDrawType), mUsageType(aUsageType), mlElementNum(-1),
			mbTangents(false){}

		virtual ~iVertexBuffer(){}

		// Select compact sheets before Compile instead of editable mesh arrays. No GL work here;
		// uploads happen when bound/drawn. Empty submissions remain instance buffers.
		std::vector<cQuadInstance>& UpdateQuadInstances(size_t alCount)
		{
			mvQuadInstances.resize(alCount);
			mbQuadInstances = true;
			mbQuadInstancesDirty = true;
			return mvQuadInstances;
		}
		void SetQuadInstances(const std::vector<cQuadInstance>& avInstances)
		{
			UpdateQuadInstances(avInstances.size()) = avInstances;
		}
		const std::vector<cQuadInstance>& GetQuadInstances() const { return mvQuadInstances; }

		tVertexFlag GetFlags(){ return mVertexFlags;}

		virtual void AddVertex(tVertexFlag aType,const cVector3f& avVtx)=0;
		virtual void AddColor(tVertexFlag aType,const cColor& aColor)=0;
		virtual void AddIndex(unsigned int alIndex)=0;

		virtual bool Compile(tVertexCompileFlag aFlags)=0;
		virtual void UpdateData(tVertexFlag aTypes, bool abIndices)=0;

		/**
		 * Transform the entire buffer with transform.
		*/
		virtual void Transform(const cMatrixf &mtxTransform)=0;

		virtual void Draw(eVertexBufferDrawType aDrawType = eVertexBufferDrawType_LastEnum)=0;
		virtual void DrawIndices(unsigned int *apIndices, int alCount,
								eVertexBufferDrawType aDrawType = eVertexBufferDrawType_LastEnum)=0;

		virtual void Bind()=0;
		virtual void UnBind()=0;

		virtual iVertexBuffer* CreateCopy(eVertexBufferUsageType aUsageType)=0;

		virtual cBoundingVolume CreateBoundingVolume()=0;

		// Mutable packed CPU data. Call UpdateData after editing a compiled buffer.
		virtual float* GetArray(tVertexFlag aType)=0;
		virtual unsigned int* GetIndices()=0;

		virtual int GetVertexNum()=0;
		virtual int GetIndexNum()=0;

		/**
		 * Resizes an array to a custom size, the size is number of elements and NOT number of vertices.
		 */
		virtual void ResizeArray(tVertexFlag aType, int alSize)=0;
		virtual void ResizeIndices(int alSize)=0;

		//For debugging purposes, quite slow to use.
		virtual cVector3f GetVector3(tVertexFlag aType, unsigned alIdx)=0;
		virtual cVector3f GetVector4(tVertexFlag aType, unsigned alIdx)=0;
		virtual cColor GetColor(tVertexFlag aType, unsigned alIdx)=0;
		virtual unsigned int GetIndex(tVertexFlag aType, unsigned alIdx)=0;

		/**
		 * Set the number of of elements to draw.
		 * \param alNum If < 0, draw all indices.
		 */
		void SetElementNum(int alNum){ mlElementNum = alNum;}
		int GetElementNum(){ return mlElementNum;}

		tVertexFlag GetVertexFlags(){ return mVertexFlags;}

		bool HasTangents(){ return mbTangents;}
		void SetTangents(bool abX){ mbTangents = abX;}

	protected:
		std::vector<cQuadInstance> mvQuadInstances;
		bool mbQuadInstances = false;
		bool mbQuadInstancesDirty = false;
		tVertexFlag mVertexFlags;
		eVertexBufferDrawType mDrawType;
		eVertexBufferUsageType mUsageType;
		iLowLevelGraphics* mpLowLevelGraphics;

		int mlElementNum;

		bool mbTangents;
	};

};
#endif // HPL_RENDERER3D_H
