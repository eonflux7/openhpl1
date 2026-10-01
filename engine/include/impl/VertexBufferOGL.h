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
#ifndef HPL_VERTEXBUFFER_OGL_H
#define HPL_VERTEXBUFFER_OGL_H

#include "graphics/VertexBuffer.h"

namespace hpl {

	class cVertexBufferOGL : public iVertexBuffer
	{
	public:
		cVertexBufferOGL(iLowLevelGraphics* apLowLevelGraphics,tVertexFlag aFlags,
			eVertexBufferDrawType aDrawType,eVertexBufferUsageType aUsageType,
			int alReserveVtxSize,int alReserveIdxSize);
		~cVertexBufferOGL() override;

		void AddVertex(tVertexFlag aType,const cVector3f& avVtx) override;
		void AddColor(tVertexFlag aType,const cColor& aColor) override;
		void AddIndex(unsigned int alIndex) override;

		bool Compile(tVertexCompileFlag aFlags) override;
		void UpdateData(tVertexFlag aTypes, bool abIndices) override;

		void CreateShadowDouble(bool abUpdateData) override;

		void Transform(const cMatrixf &mtxTransform) override;

		void Draw(eVertexBufferDrawType aDrawType) override;
		void DrawIndices(unsigned int *apIndices, int alCount,
			eVertexBufferDrawType aDrawType = eVertexBufferDrawType_LastEnum) override;


		void Bind() override;
		void UnBind() override;

		iVertexBuffer* CreateCopy(eVertexBufferUsageType aUsageType) override;

		cBoundingVolume CreateBoundingVolume() override;

		float* GetArray(tVertexFlag aType) override;
		unsigned int* GetIndices() override;

		void ResizeArray(tVertexFlag aType, int alSize) override;
		void ResizeIndices(int alSize) override;

		//For debugging purposes
		int GetVertexNum() override;
		int GetIndexNum() override;

		cVector3f GetVector3(tVertexFlag aType, unsigned alIdx) override;
		cVector3f GetVector4(tVertexFlag aType, unsigned alIdx) override;
		cColor GetColor(tVertexFlag aType, unsigned alIdx) override;
		unsigned int GetIndex(tVertexFlag aType, unsigned alIdx) override;


	private:
		void SetVertexStates(tVertexFlag aFlags);

		tFloatVec mvVertexArray[klNumOfVertexFlags];
		tUIntVec mvIndexArray;

		bool mbTangents;

		bool mbHasShadowDouble;
	};

};
#endif // HPL_RENDERER3D_OGL_H
