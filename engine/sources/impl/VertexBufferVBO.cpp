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

				mvArrayHandle[i] = 0;
			}
		}

		if(alReserveIdxSize>0)
			mvIndexArray.reserve(alReserveIdxSize);

		mlElementHandle =0;
		mlQuadElementHandle = 0;
		mbQuadIndicesChanged = true;
		mlVertexArray = 0;

		mbTangents = false;

		mbCompiled = false;

		mbHasShadowDouble = false;

		mpLowLevelGraphics = apLowLevelGraphics;
	}

	cVertexBufferVBO::~cVertexBufferVBO()
	{
		for(int i=0;i< klNumOfVertexFlags; i++)
		{
			mvVertexArray[i].clear();
			if(mVertexFlags & kvVertexFlags[i])
			{
				glDeleteBuffers(1,(GLuint *)&mvArrayHandle[i]);
			}
		}

		mvIndexArray.clear();

		glDeleteBuffers(1,(GLuint *)&mlElementHandle);
		if(mlQuadElementHandle) glDeleteBuffers(1, &mlQuadElementHandle);
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

		int lNum = cMath::Log2ToInt((int)eVertexFlag_Position);

		bv.AddArrayPoints(mvVertexArray[lNum].data(), GetVertexNum());
		bv.CreateFromPoints(kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)]);

		return bv;
	}

	//-----------------------------------------------------------------------

	bool cVertexBufferVBO::Compile(tVertexCompileFlag aFlags)
	{
		if(mbCompiled) return false;
		mbCompiled = true;

		//Create tangents
		if(aFlags & eVertexCompileFlag_CreateTangents)
		{
			mbTangents = true;

			mVertexFlags |= eVertexFlag_Texture1;

			int idx = cMath::Log2ToInt((int)eVertexFlag_Texture1);

			int lSize = GetVertexNum()*4;
			mvVertexArray[idx].resize(lSize);

			cMath::CreateTriTangentVectors(mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Texture1)].data(),
				mvIndexArray.data(), GetIndexNum(),

				mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Position)].data(),
				kvVertexElements[cMath::Log2ToInt((int)eVertexFlag_Position)],

				mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Texture0)].data(),
				mvVertexArray[cMath::Log2ToInt((int)eVertexFlag_Normal)].data(),
				GetVertexNum()
				);
		}

		GLenum usageType = GL_STATIC_DRAW;
		if(mUsageType== eVertexBufferUsageType_Dynamic) usageType = GL_DYNAMIC_DRAW;
		else if(mUsageType== eVertexBufferUsageType_Stream) usageType = GL_STREAM_DRAW;

		//Create the VBO vertex arrays
		for(int i=0;i< klNumOfVertexFlags; i++)
		{
			if(mVertexFlags & kvVertexFlags[i])
			{
				glGenBuffers(1,(GLuint *)&mvArrayHandle[i]);
				glBindBuffer(GL_ARRAY_BUFFER, mvArrayHandle[i]);

				glBufferData(GL_ARRAY_BUFFER, mvVertexArray[i].size()*sizeof(float),
						mvVertexArray[i].data(), usageType);

				glBindBuffer(GL_ARRAY_BUFFER, 0);

			}
		}

		//Create the VBO index array. Filled through GL_ARRAY_BUFFER: the
		//element array binding belongs to whichever vertex array is bound.
		glGenBuffers(1,(GLuint *)&mlElementHandle);
		glBindBuffer(GL_ARRAY_BUFFER,mlElementHandle);
		glBufferData(GL_ARRAY_BUFFER, GetIndexNum()*sizeof(unsigned int),
														mvIndexArray.data(), usageType);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		mbQuadIndicesChanged = true;

		//The arrays may have changed (tangents)
		DeleteVertexArray();


		return true;
	}


	//-----------------------------------------------------------------------

	void cVertexBufferVBO::UpdateData(tVertexFlag aTypes, bool abIndices)
	{
		GLenum usageType = GL_STATIC_DRAW;
		if(mUsageType== eVertexBufferUsageType_Dynamic) usageType = GL_DYNAMIC_DRAW;
		else if(mUsageType== eVertexBufferUsageType_Stream) usageType = GL_STREAM_DRAW;

		//Create the VBO vertex arrays
		for(int i=0;i< klNumOfVertexFlags; i++)
		{
			if((mVertexFlags & kvVertexFlags[i]) && (aTypes & kvVertexFlags[i]))
			{
				glBindBuffer(GL_ARRAY_BUFFER, mvArrayHandle[i]);

				//This was apparently VERY slow.
				glBufferData(GL_ARRAY_BUFFER, mvVertexArray[i].size()*sizeof(float),
					nullptr, usageType);//Clear memory

				glBufferData(GL_ARRAY_BUFFER, mvVertexArray[i].size()*sizeof(float),
					mvVertexArray[i].data(), usageType);
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//Create the VBO index array
		if(abIndices)
		{
			glBindBuffer(GL_ARRAY_BUFFER,mlElementHandle);

			glBufferData(GL_ARRAY_BUFFER, GetIndexNum()*sizeof(unsigned int),
				mvIndexArray.data(), usageType);

			glBindBuffer(GL_ARRAY_BUFFER,0);
			mbQuadIndicesChanged = true;
		}
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::CreateShadowDouble(bool abUpdateData)
	{
		int lIdx = cMath::Log2ToInt(eVertexFlag_Position);

		//Set to new size.
		int lSize = (int)mvVertexArray[lIdx].size();
		mvVertexArray[lIdx].reserve(lSize*2);

		int lCount = lSize /4;
		for(int i=0; i< lCount; i++)
		{
			mvVertexArray[lIdx].push_back(mvVertexArray[lIdx][i*4+0]);
			mvVertexArray[lIdx].push_back(mvVertexArray[lIdx][i*4+1]);
			mvVertexArray[lIdx].push_back(mvVertexArray[lIdx][i*4+2]);
			mvVertexArray[lIdx].push_back(0);//0);
		}

		mbHasShadowDouble = true;

		if(abUpdateData)
		{
			UpdateData(eVertexFlag_Position, false);
		}
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::Transform(const cMatrixf &a_mtxTransform)
	{
		float *pPosArray = GetArray(eVertexFlag_Position);
		float *pNormalArray = GetArray(eVertexFlag_Normal);
		float *pTangentArray = nullptr;
		if(mbTangents)pTangentArray = GetArray(eVertexFlag_Texture1);

		int lVtxNum = GetVertexNum();

		cMatrixf mtxRot = a_mtxTransform.GetRotation();

		int lVtxStride = kvVertexElements[cMath::Log2ToInt(eVertexFlag_Position)];

		int lOffset = GetVertexNum()*4;

		for(int i=0; i<lVtxNum; i++)
		{
			float* pPos = &pPosArray[i*lVtxStride];
			float* pNorm = &pNormalArray[i*3];
			float* pTan = nullptr;
			if(mbTangents)pTan = &pTangentArray[i*4];

			cVector3f vPos = cMath::MatrixMul(a_mtxTransform, cVector3f(pPos[0],pPos[1],pPos[2]));
			pPos[0] = vPos.x; pPos[1] = vPos.y; pPos[2] = vPos.z;

			if(mbHasShadowDouble){
				float* pExtraPos = &pPosArray[i*lVtxStride + lOffset];
				pExtraPos[0] = vPos.x; pExtraPos[1] = vPos.y; pExtraPos[2] = vPos.z;
			}

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
				UpdateData(eVertexFlag_Position | eVertexFlag_Normal | eVertexFlag_Texture1,false);
			else
				UpdateData(eVertexFlag_Position | eVertexFlag_Normal,false);
		}
	}

	//-----------------------------------------------------------------------

	void cVertexBufferVBO::Draw(eVertexBufferDrawType aDrawType)
	{
		eVertexBufferDrawType drawType = aDrawType == eVertexBufferDrawType_LastEnum ? mDrawType : aDrawType;
		cLowLevelGraphicsSDL *pLowLevel = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);

		int lSize = mlElementNum;
		if(mlElementNum<0) lSize = GetIndexNum();

		///////////////////////////////
		//Get the draw type
		GLenum mode = GL_TRIANGLES;
		unsigned int lElementHandle = mlElementHandle;
		if(drawType==eVertexBufferDrawType_Quad)
		{
			//Two triangles per quad, from a second index buffer
			if(mbQuadIndicesChanged)
			{
				tUIntVec vTriangles;
				QuadsToTriangles(mvIndexArray.data(), GetIndexNum(), vTriangles);

				if(mlQuadElementHandle==0) glGenBuffers(1, &mlQuadElementHandle);
				glBindBuffer(GL_ARRAY_BUFFER, mlQuadElementHandle);
				glBufferData(GL_ARRAY_BUFFER, vTriangles.size()*sizeof(unsigned int), vTriangles.data(),
							GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				mbQuadIndicesChanged = false;
			}
			lElementHandle = mlQuadElementHandle;
			lSize = lSize / 4 * 6;
		}
		else if(drawType==eVertexBufferDrawType_Lines)
		{
			mode = GL_LINE_STRIP;
		}

		//////////////////////////////////
		//Bind and draw the buffer
		pLowLevel->PrepareDraw();
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
		tUIntVec vTriangles;
		if(drawType==eVertexBufferDrawType_Quad)
		{
			QuadsToTriangles(apIndices, alCount, vTriangles);
			apIndices = vTriangles.data();
			alCount = (int)vTriangles.size();
		}
		else if(drawType==eVertexBufferDrawType_Lines)
		{
			mode = GL_LINE_STRIP;
		}
		if(alCount <= 0) return;

		//////////////////////////////////
		//Bind and draw the buffer
		pLowLevel->PrepareDraw();
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

		//Copy the vertices to the new buffer.
		for(int i=0; i < klNumOfVertexFlags; i++)
		{
			if(kvVertexFlags[i] & mVertexFlags)
			{
				int lElements = kvVertexElements[i];
				if(mbTangents && kvVertexFlags[i] == eVertexFlag_Texture1)
					lElements=4;

				pVtxBuff->ResizeArray(kvVertexFlags[i], (int)mvVertexArray[i].size());

				memcpy(pVtxBuff->GetArray(kvVertexFlags[i]),
						mvVertexArray[i].data(), mvVertexArray[i].size() * sizeof(float));
			}
		}

		//Copy indices to the new buffer
		pVtxBuff->ResizeIndices(GetIndexNum());
		memcpy(pVtxBuff->GetIndices(), GetIndices(), GetIndexNum() * sizeof(unsigned int) );

		pVtxBuff->mbTangents = mbTangents;
		pVtxBuff->mbHasShadowDouble = mbHasShadowDouble;

		pVtxBuff->Compile(0);

		return pVtxBuff;
	}

	//-----------------------------------------------------------------------

	int cVertexBufferVBO::GetVertexNum()
	{
		int idx = cMath::Log2ToInt((int)eVertexFlag_Position);
		int lSize = (int)mvVertexArray[idx].size()/kvVertexElements[idx];

		//If there is a shadow double, just return the length of the first half.
		if(mbHasShadowDouble) return lSize / 2;
		else return lSize;
	}
	int cVertexBufferVBO::GetIndexNum()
	{
		return (int)mvIndexArray.size();
	}

	cVector3f cVertexBufferVBO::GetVector3(tVertexFlag aType, unsigned alIdx)
	{
		if(!(aType & mVertexFlags)) return cVector3f(0,0,0);

		int idx = cMath::Log2ToInt((int)aType);
		int pos = alIdx * kvVertexElements[idx];

		return cVector3f(mvVertexArray[idx][pos+0],mvVertexArray[idx][pos+1],
			mvVertexArray[idx][pos+2]);
	}
	cVector3f cVertexBufferVBO::GetVector4(tVertexFlag aType, unsigned alIdx)
	{
		if(!(aType & mVertexFlags)) return cVector3f(0,0,0);

		int idx = cMath::Log2ToInt((int)aType);
		int pos = alIdx * 4;//kvVertexElements[idx];

		return cVector3f(mvVertexArray[idx][pos+0],mvVertexArray[idx][pos+1],
			mvVertexArray[idx][pos+2]);
	}
	cColor cVertexBufferVBO::GetColor(tVertexFlag aType, unsigned alIdx)
	{
		if(!(aType & mVertexFlags)) return cColor();

		int idx = cMath::Log2ToInt((int)aType);
		int pos = alIdx * kvVertexElements[idx];

		return cColor(mvVertexArray[idx][pos+0],mvVertexArray[idx][pos+1],
			mvVertexArray[idx][pos+2],mvVertexArray[idx][pos+3]);
	}
	unsigned int cVertexBufferVBO::GetIndex(tVertexFlag aType, unsigned alIdx)
	{
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

	// The triangles 0,1,2 and 0,2,3 of each quad.
	void cVertexBufferVBO::QuadsToTriangles(const unsigned int *apIndices, int alCount, tUIntVec &avTriangles)
	{
		avTriangles.clear();
		avTriangles.reserve(alCount / 4 * 6);
		for(int i=0; i+3 < alCount; i+=4)
		{
			const unsigned int *pQuad = &apIndices[i];
			avTriangles.insert(avTriangles.end(), {pQuad[0], pQuad[1], pQuad[2], pQuad[0], pQuad[2], pQuad[3]});
		}
	}

	//-----------------------------------------------------------------------

	// The vertex array object is made when first drawn with, from the
	// arrays the buffer has then.
	void cVertexBufferVBO::BindVertexArray()
	{
		cLowLevelGraphicsSDL *pLowLevel = static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics);
		if(mlVertexArray)
		{
			pLowLevel->BindVertexArray(mlVertexArray);
			return;
		}

		glGenVertexArrays(1, &mlVertexArray);
		pLowLevel->BindVertexArray(mlVertexArray);

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
			case eVertexFlag_Texture0:	lAttrib = cGLSLProgram::eVertexAttrib_TexCoord0; break;
			case eVertexFlag_Texture1:	lAttrib = cGLSLProgram::eVertexAttrib_TexCoord0 + 1;
										if(mbTangents) lElements = 4;
										break;
			case eVertexFlag_Texture2:	lAttrib = cGLSLProgram::eVertexAttrib_TexCoord0 + 2; break;
			case eVertexFlag_Texture3:	lAttrib = cGLSLProgram::eVertexAttrib_TexCoord0 + 3; break;
			case eVertexFlag_Texture4:	lAttrib = cGLSLProgram::eVertexAttrib_TexCoord0 + 4; break;
			default: break; //Color1 is not drawn with
			}
			if(lAttrib < 0) continue;

			glBindBuffer(GL_ARRAY_BUFFER, mvArrayHandle[i]);
			glVertexAttribPointer(lAttrib, lElements, GL_FLOAT, GL_FALSE, 0, nullptr);
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
