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
#include "impl/VertexBufferVBO.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "system/LowLevelSystem.h"
#include "math/Math.h"

#include <memory.h>
#include <algorithm>
#include <cstddef>

#include <glad/gl.h>


namespace hpl {

#define BUFFER_OFFSET(i) ((void*)(i*sizeof(float)))

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cVertexBufferVBO::cVertexBufferVBO(iLowLevelGraphics* apLowLevelGraphics,tVertexFlag aFlags,
		eVertexBufferDrawType aDrawType,eVertexBufferUsageType aUsageType,
		int alReserveVtxSize,int alReserveIdxSize) :
	iVertexBuffer(apLowLevelGraphics, aFlags, aDrawType,aUsageType, alReserveVtxSize, alReserveIdxSize)
	{
		if(alReserveVtxSize>0)
		{
			for(int i=0;i< klNumOfVertexFlags; i++)
			{
				if(aFlags & kvVertexFlags[i])
				{
					mvVertexArray[i].reserve(alReserveVtxSize * kvVertexElements[i]);
				}

			}
		}

		if(alReserveIdxSize>0)
			mvIndexArray.reserve(alReserveIdxSize);

		mlElementHandle =0;
		mlVertexHandle = 0;
		mlVertexArray = 0;

		mbTangents = false;

		mbCompiled = false;


		mpLowLevelGraphics = apLowLevelGraphics;
	}

	cVertexBufferVBO::~cVertexBufferVBO()
	{
		glDeleteBuffers(1, &mlVertexHandle);
		glDeleteBuffers(1, &mlElementHandle);
		DeleteVertexArray();
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::AddVertex(tVertexFlag aType,const cVector3f& avVtx)
	{
		int idx = cMath::Log2ToInt((int)aType);

		mvVertexArray[idx].push_back(avVtx.x);
		mvVertexArray[idx].push_back(avVtx.y);
		mvVertexArray[idx].push_back(avVtx.z);
		if(kvVertexElements[idx]==4)
			mvVertexArray[idx].push_back(1);
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::AddColor(tVertexFlag aType,const cColor& aColor)
	{
		int idx = cMath::Log2ToInt((int)aType);

		mvVertexArray[idx].push_back(aColor.r);
		mvVertexArray[idx].push_back(aColor.g);
		mvVertexArray[idx].push_back(aColor.b);
		mvVertexArray[idx].push_back(aColor.a);
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::AddIndex(unsigned int alIndex)
	{
		mvIndexArray.push_back(alIndex);
	}

	//-----------------------------------------------------------------------

	cBoundingVolume cVertexBufferVBO::CreateBoundingVolume()
	{
		cBoundingVolume bv;
		if(mbQuadInstances)
		{
			cVector3f minimum(0), maximum(0);
			for(size_t i=0; i<mvQuadInstances.size(); ++i)
			{
				const cQuadInstance &sheet = mvQuadInstances[i];
				for(int axis=0; axis<3; ++axis)
				{
					const float extent = std::abs(sheet.mvRight.v[axis]) + std::abs(sheet.mvUp.v[axis]);
					const float low = sheet.mvCentre.v[axis] - extent, high = sheet.mvCentre.v[axis] + extent;
					minimum.v[axis] = i == 0 ? low : std::min(minimum.v[axis], low);
					maximum.v[axis] = i == 0 ? high : std::max(maximum.v[axis], high);
				}
			}
			bv.SetLocalMinMax(minimum, maximum);
			return bv;
		}

		int lNum = cMath::Log2ToInt((int)eVertexFlag_Position);

		bv.AddArrayPoints(mvVertexArray[lNum].data(), GetVertexNum());
		bv.CreateFromPoints(kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)]);

		return bv;
	}

	//-----------------------------------------------------------------------

	bool cVertexBufferVBO::Compile(tVertexCompileFlag aFlags)
	{
		if(mbCompiled) return false;
		if(mbQuadInstances)
		{
			glGenBuffers(1, &mlVertexHandle);
			mbCompiled = true;
			return true;
		}

		//Create tangents
		if(aFlags & eVertexCompileFlag_CreateTangents)
		{
			mbTangents = true;

			mVertexFlags |= eVertexFlag_Tangent;

			int idx = cMath::Log2ToInt((int)eVertexFlag_Tangent);

			int lSize = GetVertexNum()*4;
			mvVertexArray[idx].resize(lSize);

			cMath::CreateTriTangentVectors(mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Tangent)].data(),
				mvIndexArray.data(), GetIndexNum(),

				mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Position)].data(),
				kvVertexElements[cMath::Log2ToInt((int)eVertexFlag_Position)],

				mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_TexCoord)].data(),
				mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Normal)].data(),
				GetVertexNum()
				);
		}

		GLenum usageType = GL_STATIC_DRAW;
		if(mUsageType== eVertexBufferUsageType_Dynamic) usageType = GL_DYNAMIC_DRAW;
		else if(mUsageType== eVertexBufferUsageType_Stream) usageType = GL_STREAM_DRAW;

		if(!PackVertices()) return false;
		glGenBuffers(1, &mlVertexHandle);
		UploadVertices();
		mbCompiled = true;

		//Create the VBO index array. Filled through GL_ARRAY_BUFFER: the
		//element array binding belongs to whichever vertex array is bound.
		glGenBuffers(1,(GLuint *)&mlElementHandle);
		glBindBuffer(GL_ARRAY_BUFFER,mlElementHandle);
		glBufferData(GL_ARRAY_BUFFER, GetIndexNum()*sizeof(unsigned int),
														mvIndexArray.data(), usageType);
		glBindBuffer(GL_ARRAY_BUFFER,0);

		//The arrays may have changed (tangents)
		DeleteVertexArray();


		return true;
	}


	//-----------------------------------------------------------------------

	void cVertexBufferVBO::UpdateData(tVertexFlag aTypes, bool abIndices)
	{
		if(mbQuadInstances) { mbQuadInstancesDirty = true; return; }
		GLenum usageType = GL_STATIC_DRAW;
		if(mUsageType== eVertexBufferUsageType_Dynamic) usageType = GL_DYNAMIC_DRAW;
		else if(mUsageType== eVertexBufferUsageType_Stream) usageType = GL_STREAM_DRAW;

		// All enabled fields travel together, including unchanged UVs/tangents.
		// Never invalidate the packed CPU arrays: callers retain pointers to them.
		if((aTypes & mVertexFlags) && PackVertices()) UploadVertices();

		//Create the VBO index array
		if(abIndices)
		{
			glBindBuffer(GL_ARRAY_BUFFER,mlElementHandle);

			glBufferData(GL_ARRAY_BUFFER, GetIndexNum()*sizeof(unsigned int),
				mvIndexArray.data(), usageType);

			glBindBuffer(GL_ARRAY_BUFFER,0);
		}
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::Transform(const cMatrixf &a_mtxTransform)
	{
		float *pPosArray = GetArray(eVertexFlag_Position);
		float *pNormalArray = GetArray(eVertexFlag_Normal);
		float *pTangentArray = nullptr;
		if(mbTangents)pTangentArray = GetArray(eVertexFlag_Tangent);

		int lVtxNum = GetVertexNum();

		cMatrixf mtxRot = a_mtxTransform.GetRotation();

		int lVtxStride = kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)];

		for(int i=0; i<lVtxNum; i++)
		{
			float* pPos = &pPosArray[i*lVtxStride];
			float* pNorm = &pNormalArray[i*3];
			float* pTan = nullptr;
			if(mbTangents)pTan = &pTangentArray[i*4];

			cVector3f vPos = cMath::MatrixMul(a_mtxTransform, cVector3f(pPos[0],pPos[1],pPos[2]));
			pPos[0] = vPos.x; pPos[1] = vPos.y; pPos[2] = vPos.z;

			cVector3f vNorm = cMath::MatrixMul(mtxRot, cVector3f(pNorm[0],pNorm[1],pNorm[2]));
			vNorm.Normalise();
			pNorm[0] = vNorm.x; pNorm[1] = vNorm.y; pNorm[2] = vNorm.z;

			if(mbTangents){
				cVector3f vTan = cMath::MatrixMul(mtxRot, cVector3f(pTan[0],pTan[1],pTan[2]));
				vTan.Normalise();
				pTan[0] = vTan.x; pTan[1] = vTan.y; pTan[2] = vTan.z;
			}
		}

		if(mbCompiled)
		{
			if(mbTangents)
				UpdateData(eVertexFlag_Position | eVertexFlag_Normal | eVertexFlag_Tangent,false);
			else
				UpdateData(eVertexFlag_Position | eVertexFlag_Normal,false);
		}
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::Draw(eVertexBufferDrawType aDrawType)
	{
		if(mbQuadInstances)
		{
			const int count = mlElementNum < 0 ? (int)mvQuadInstances.size() :
				std::min((int)mvQuadInstances.size(), mlElementNum / 6);
			if(count == 0) return;
			auto *gfx = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);
			if(!gfx->PrepareDraw()) return;
			BindVertexArray();
			const GLenum mode = aDrawType == eVertexBufferDrawType_Lines ? GL_LINE_STRIP : GL_TRIANGLES;
			gfx->GetGpuProfiler()->CountDraw(mode, count * 6);
			glDrawElementsInstanced(mode, 6, GL_UNSIGNED_INT, nullptr, count);
			// Array draws leave generic attributes unspecified. Ordinary mesh,
			// stream and fullscreen VAOs disable 3 and must see kind zero.
			glVertexAttrib1f(3, 0);
			return;
		}
		eVertexBufferDrawType drawType = aDrawType == eVertexBufferDrawType_LastEnum ? mDrawType : aDrawType;
		cLowLevelGraphicsSDL *pLowLevel = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);

		int lSize = mlElementNum;
		if(mlElementNum<0) lSize = GetIndexNum();

		///////////////////////////////
		//Get the draw type
		GLenum mode = GL_TRIANGLES;
		unsigned int lElementHandle = mlElementHandle;
		if(drawType==eVertexBufferDrawType_Lines)
		{
			mode = GL_LINE_STRIP;
		}

		//////////////////////////////////
		//Bind and draw the buffer
		if(!pLowLevel->PrepareDraw()) return;
		pLowLevel->GetGpuProfiler()->CountDraw(mode, lSize);

		BindVertexArray();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lElementHandle);
		glDrawElements(mode,lSize,GL_UNSIGNED_INT, (char*) nullptr);
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::DrawIndices(unsigned int *apIndices, int alCount,eVertexBufferDrawType aDrawType)
	{
		eVertexBufferDrawType drawType = aDrawType == eVertexBufferDrawType_LastEnum ? mDrawType : aDrawType;
		cLowLevelGraphicsSDL *pLowLevel = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);

		///////////////////////////////
		//Get the draw type
		GLenum mode = GL_TRIANGLES;
		if(drawType==eVertexBufferDrawType_Lines)
		{
			mode = GL_LINE_STRIP;
		}
		if(alCount <= 0) return;

		//////////////////////////////////
		//Bind and draw the buffer
		if(!pLowLevel->PrepareDraw()) return;
		pLowLevel->GetGpuProfiler()->CountDraw(mode, alCount);

		BindVertexArray();
		const void *pOffset = pLowLevel->StreamIndices(apIndices, alCount);
		glDrawElements(mode, alCount, GL_UNSIGNED_INT, pOffset);
	}


	//-----------------------------------------------------------------------

	void cVertexBufferVBO::Bind()
	{
		BindVertexArray();
	}

	//-----------------------------------------------------------------------

	// Every draw binds the vertex array it needs.
	void cVertexBufferVBO::UnBind()
	{
	}

	//-----------------------------------------------------------------------

	float* cVertexBufferVBO::GetArray(tVertexFlag aType)
	{
		int idx = cMath::Log2ToInt((int)aType);

		return mvVertexArray[idx].data();
	}

	//-----------------------------------------------------------------------

	unsigned int* cVertexBufferVBO::GetIndices()
	{
		return mvIndexArray.data();
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::ResizeArray(tVertexFlag aType, int alSize)
	{
		int idx = cMath::Log2ToInt((int)aType);

		mvVertexArray[idx].resize(alSize);
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::ResizeIndices(int alSize)
	{
		mvIndexArray.resize(alSize);
	}

	//-----------------------------------------------------------------------

	iVertexBuffer* cVertexBufferVBO::CreateCopy(eVertexBufferUsageType aUsageType)
	{
		cVertexBufferVBO *pVtxBuff = hplNew( cVertexBufferVBO, (mpLowLevelGraphics,
															mVertexFlags,mDrawType,aUsageType,
															GetVertexNum(),GetIndexNum()));

		if(mbQuadInstances)
		{
			pVtxBuff->SetQuadInstances(mvQuadInstances);
			pVtxBuff->SetElementNum(mlElementNum);
			pVtxBuff->Compile(0);
			return pVtxBuff;
		}

		//Copy the vertices to the new buffer.
		for(int i=0; i < klNumOfVertexFlags; i++)
		{
			if(kvVertexFlags[i] & mVertexFlags)
			{
				pVtxBuff->ResizeArray(kvVertexFlags[i], (int)mvVertexArray[i].size());

				memcpy(pVtxBuff->GetArray(kvVertexFlags[i]),
						mvVertexArray[i].data(), mvVertexArray[i].size() * sizeof(float));
			}
		}

		//Copy indices to the new buffer
		pVtxBuff->ResizeIndices(GetIndexNum());
		memcpy(pVtxBuff->GetIndices(), GetIndices(), GetIndexNum() * sizeof(unsigned int) );

		pVtxBuff->mbTangents = mbTangents;

		pVtxBuff->Compile(0);

		return pVtxBuff;
	}

	//-----------------------------------------------------------------------

	int cVertexBufferVBO::GetVertexNum()
	{
		if(mbQuadInstances) return (int)mvQuadInstances.size() * 4;
		int idx = cMath::Log2ToInt((int)eVertexFlag_Position);
		return (int)mvVertexArray[idx].size()/kvVertexElements[idx];
	}
	int cVertexBufferVBO::GetIndexNum()
	{
		return mbQuadInstances ? (int)mvQuadInstances.size() * 6 : (int)mvIndexArray.size();
	}

	cVector3f cVertexBufferVBO::GetVector3(tVertexFlag aType, unsigned alIdx)
	{
		// Debug inspection only. Rendering never expands CPU corners.
		if(mbQuadInstances)
		{
			const cQuadInstance &sheet = mvQuadInstances[alIdx / 4];
			const int corner = alIdx % 4;
			if(aType == eVertexFlag_Position)
				return sheet.mvCentre + (sheet.mvRight * (corner == 0 || corner == 3 ? 1.0f : -1.0f) +
					sheet.mvUp * (corner < 2 ? -1.0f : 1.0f));
			if(aType == eVertexFlag_Normal && sheet.mfKind < 2.5f) return cVector3f(0,0,1);
			if(aType == eVertexFlag_Tangent && sheet.mfKind < 2.5f) return cVector3f(1,0,0);
			return cVector3f(0);
		}
		if(!(aType & mVertexFlags)) return cVector3f(0,0,0);

		int idx = cMath::Log2ToInt((int)aType);
		int pos = alIdx * kvVertexElements[idx];

		return cVector3f(mvVertexArray[idx][pos+0],mvVertexArray[idx][pos+1],
			mvVertexArray[idx][pos+2]);
	}
	cVector3f cVertexBufferVBO::GetVector4(tVertexFlag aType, unsigned alIdx)
	{
		if(mbQuadInstances) return GetVector3(aType, alIdx);
		if(!(aType & mVertexFlags)) return cVector3f(0,0,0);

		int idx = cMath::Log2ToInt((int)aType);
		int pos = alIdx * kvVertexElements[idx];

		return cVector3f(mvVertexArray[idx][pos+0],mvVertexArray[idx][pos+1],
			mvVertexArray[idx][pos+2]);
	}
	cColor cVertexBufferVBO::GetColor(tVertexFlag aType, unsigned alIdx)
	{
		if(mbQuadInstances && aType == eVertexFlag_Color0)
			return alIdx % 4 < 2 ? mvQuadInstances[alIdx / 4].mColor : mvQuadInstances[alIdx / 4].mEndColor;
		if(!(aType & mVertexFlags)) return cColor();

		int idx = cMath::Log2ToInt((int)aType);
		int pos = alIdx * kvVertexElements[idx];

		return cColor(mvVertexArray[idx][pos+0],mvVertexArray[idx][pos+1],
			mvVertexArray[idx][pos+2],mvVertexArray[idx][pos+3]);
	}
	unsigned int cVertexBufferVBO::GetIndex(tVertexFlag aType, unsigned alIdx)
	{
		if(mbQuadInstances)
		{
			static const unsigned corners[] = {0,1,2,2,3,0};
			return (alIdx / 6) * 4 + corners[alIdx % 6];
		}
		return mvIndexArray[alIdx];
	}


	//-----------------------------------------------------------------------

	/////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	/////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	int cVertexBufferVBO::GetElementNum(tVertexFlag aFlag)
	{
		int idx = cMath::Log2ToInt((int)aFlag);

		return kvVertexElements[idx];
	}
	//-----------------------------------------------------------------------

	bool cVertexBufferVBO::PackVertices()
	{
		mlStride = 0;
		const int count = GetVertexNum();
		for(int i=0; i<klNumOfVertexFlags; ++i)
		{
			mvOffsets[i] = mlStride;
			if(!(mVertexFlags & kvVertexFlags[i])) continue;
			if(mvVertexArray[i].size() != (size_t)count * kvVertexElements[i])
			{
				Error("Vertex buffer attribute %d has an invalid size\n", i);
				return false;
			}
			mlStride += kvVertexElements[i];
		}
		mvInterleaved.resize((size_t)count * mlStride);
		for(int v=0; v<count; ++v)
			for(int i=0; i<klNumOfVertexFlags; ++i)
				if(mVertexFlags & kvVertexFlags[i])
					memcpy(&mvInterleaved[(size_t)v*mlStride + mvOffsets[i]],
						&mvVertexArray[i][(size_t)v*kvVertexElements[i]], kvVertexElements[i]*sizeof(float));
		return true;
	}

	void cVertexBufferVBO::UploadVertices()
	{
		GLenum usage = mUsageType == eVertexBufferUsageType_Stream ? GL_STREAM_DRAW :
			(mUsageType == eVertexBufferUsageType_Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, mlVertexHandle);
		glBufferData(GL_ARRAY_BUFFER, mvInterleaved.size()*sizeof(float), mvInterleaved.data(), usage);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	//-----------------------------------------------------------------------

	// The vertex array object is made when first drawn with, from the
	// arrays the buffer has then.
	void cVertexBufferVBO::BindVertexArray()
	{
		cLowLevelGraphicsSDL *pLowLevel = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);
		if(mbQuadInstances && mbQuadInstancesDirty)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mlVertexHandle);
			glBufferData(GL_ARRAY_BUFFER, mvQuadInstances.size()*sizeof(cQuadInstance),
				mvQuadInstances.data(), GL_STREAM_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			mbQuadInstancesDirty = false;
		}
		if(mlVertexArray)
		{
			pLowLevel->BindVertexArray(mlVertexArray);
			return;
		}

		glGenVertexArrays(1, &mlVertexArray);
		pLowLevel->BindVertexArray(mlVertexArray);

		if(mbQuadInstances)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mlVertexHandle);
			const int attributes[] = {6,3,7,8,9,10,11};
			const int sizes[] = {3,1,3,3,4,4,4};
			const size_t offsets[] = {offsetof(cQuadInstance,mvCentre), offsetof(cQuadInstance,mfKind),
				offsetof(cQuadInstance,mvRight), offsetof(cQuadInstance,mvUp), offsetof(cQuadInstance,mColor),
				offsetof(cQuadInstance,mEndColor), offsetof(cQuadInstance,mvUV)};
			for(int i=0; i<7; ++i)
			{
				glVertexAttribPointer(attributes[i], sizes[i], GL_FLOAT, GL_FALSE,
					sizeof(cQuadInstance), (const void*)offsets[i]);
				glVertexAttribDivisor(attributes[i], 1);
				glEnableVertexAttribArray(attributes[i]);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			pLowLevel->BindSheetIndices();
			return;
		}

		for(int i=0;i< klNumOfVertexFlags; i++)
		{
			if((mVertexFlags & kvVertexFlags[i])==0) continue;

			int lAttrib = -1;
			int lElements = kvVertexElements[i];
			switch(kvVertexFlags[i])
			{
			case eVertexFlag_Position:	lAttrib = cGLSLProgram::eVertexAttrib_Position; break;
			case eVertexFlag_Normal:	lAttrib = cGLSLProgram::eVertexAttrib_Normal; break;
			case eVertexFlag_Color0:	lAttrib = cGLSLProgram::eVertexAttrib_Color; break;
			case eVertexFlag_TexCoord:	lAttrib = cGLSLProgram::eVertexAttrib_TexCoord; break;
			case eVertexFlag_Tangent: lAttrib = cGLSLProgram::eVertexAttrib_Tangent; break;
			default: break;
			}
			if(lAttrib < 0) continue;

			glBindBuffer(GL_ARRAY_BUFFER, mlVertexHandle);
			glVertexAttribPointer(lAttrib, lElements, GL_FLOAT, GL_FALSE, mlStride*sizeof(float), BUFFER_OFFSET(mvOffsets[i]));
			glEnableVertexAttribArray(lAttrib);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::DeleteVertexArray()
	{
		if(mlVertexArray==0) return;

		// Deleting the bound one unbinds it
		static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics)->BindVertexArray(0);
		glDeleteVertexArrays(1, &mlVertexArray);
		mlVertexArray = 0;
	}

	//-----------------------------------------------------------------------

}
