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
#include "impl/GLStreamDrawing.h"
#include "impl/GpuProfiler.h"
#include "math/Math.h"
#include <cassert>
namespace hpl
{
	// Copies data to the stream buffer bound to aTarget and returns its
	// offset. A full buffer is replaced by a new one, so that what was
	// drawn from it stays as it was.
	static size_t StreamToBuffer(GLenum aTarget, size_t &alSize, size_t &alOffset,
		const void *apData, size_t alBytes)
	{
		if(alOffset + alBytes > alSize)
		{
			while(alBytes > alSize) alSize *= 2;
			glBufferData(aTarget, alSize, nullptr, GL_STREAM_DRAW);
			alOffset = 0;
		}

		glBufferSubData(aTarget, alOffset, alBytes, apData);
		size_t lStart = alOffset;
		alOffset += alBytes;
		return lStart;
	}

	//-----------------------------------------------------------------------

	void cGLStreamDrawing::Init()
	{
		glGenVertexArrays(1, &mlStreamVertexArray);
		glGenBuffers(1, &mlStreamVertexBuffer);
		glGenBuffers(1, &mlStreamIndexBuffer);
		mState.BindVertexArray(mlStreamVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mlStreamVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, mlStreamVertexSize, nullptr, GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexSize, nullptr, GL_STREAM_DRAW);
		cGpuProfiler::Label(GL_VERTEX_ARRAY, mlStreamVertexArray, "Stream");
		cGpuProfiler::Label(GL_BUFFER, mlStreamVertexBuffer, "Stream vertices");
		cGpuProfiler::Label(GL_BUFFER, mlStreamIndexBuffer, "Stream indices");
	}
	void cGLStreamDrawing::Reset()
	{
		if(mlSheetIndexBuffer) glDeleteBuffers(1, &mlSheetIndexBuffer);
		mlSheetIndexBuffer = 0;
		if(mlStreamVertexArray) glDeleteVertexArrays(1, &mlStreamVertexArray);
		if(mlStreamVertexBuffer) glDeleteBuffers(1, &mlStreamVertexBuffer);
		if(mlStreamIndexBuffer) glDeleteBuffers(1, &mlStreamIndexBuffer);
		mlStreamVertexArray = mlStreamVertexBuffer = mlStreamIndexBuffer = 0;
	}

	void cGLStreamDrawing::DrawTriangles(const tVertexVec &avVertices, const cGLDrawTargetState &aTarget)
	{
		assert(avVertices.size() % 3 == 0);
		ImmediateBegin(1);
		for(const cVertex &vertex : avVertices) ImmediateVertex(vertex.pos, vertex.col, &vertex.tex);
		ImmediateDraw(GL_TRIANGLES, aTarget);
	}

	void cGLStreamDrawing::DrawLine(const cVector3f &avBegin, const cVector3f &avEnd, cColor aCol, const cGLDrawTargetState &aTarget)
	{
		mState.SetTexture(0, nullptr);
		ImmediateBegin(0);
		ImmediateVertex(avBegin, aCol);
		ImmediateVertex(avEnd, aCol);
		ImmediateDraw(GL_LINES, aTarget);
	}

	void cGLStreamDrawing::DrawBoxMaxMin(const cVector3f &avMax, const cVector3f &avMin, cColor aCol, const cGLDrawTargetState &aTarget)
	{
		mState.SetTexture(0, nullptr);
		mState.SetBlendActive(false);

		// Corner i: bit 0 picks x, bit 1 y, bit 2 z from the minimum
		static const int vEdges[12][2] = {
			{0, 1}, {0, 2}, {1, 3}, {3, 2}, //Pos Z Quad
			{4, 5}, {4, 6}, {5, 7}, {7, 6}, //Neg Z Quad
			{0, 4}, {1, 5}, {3, 7}, {2, 6} //Lines between
		};

		ImmediateBegin(0);
		for(int i = 0; i < 12; ++i)
		{
			for(int j = 0; j < 2; ++j)
			{
				int lCorner = vEdges[i][j];
				ImmediateVertex(cVector3f(lCorner & 1 ? avMin.x : avMax.x,
									lCorner & 2 ? avMin.y : avMax.y,
									lCorner & 4 ? avMin.z : avMax.z),
					aCol);
			}
		}
		ImmediateDraw(GL_LINES, aTarget);
	}

	void cGLStreamDrawing::DrawSphere(const cVector3f &avPos, float afRadius, cColor aCol, const cGLDrawTargetState &aTarget)
	{
		int alSegments = 32;
		float afAngleStep = k2Pif / (float)alSegments;

		mState.SetTexture(0, nullptr);
		mState.SetBlendActive(false);

		ImmediateBegin(0);

		//X Circle:
		for(float a = 0; a < k2Pif; a += afAngleStep)
		{
			ImmediateVertex(cVector3f(avPos.x, avPos.y + sin(a) * afRadius,
								avPos.z + cos(a) * afRadius),
				aCol);

			ImmediateVertex(cVector3f(avPos.x, avPos.y + sin(a + afAngleStep) * afRadius,
								avPos.z + cos(a + afAngleStep) * afRadius),
				aCol);
		}

		//Y Circle:
		for(float a = 0; a < k2Pif; a += afAngleStep)
		{
			ImmediateVertex(cVector3f(avPos.x + cos(a) * afRadius, avPos.y,
								avPos.z + sin(a) * afRadius),
				aCol);

			ImmediateVertex(cVector3f(avPos.x + cos(a + afAngleStep) * afRadius, avPos.y,
								avPos.z + sin(a + afAngleStep) * afRadius),
				aCol);
		}

		//Z Circle:
		for(float a = 0; a < k2Pif; a += afAngleStep)
		{
			ImmediateVertex(cVector3f(avPos.x + cos(a) * afRadius, avPos.y + sin(a) * afRadius, avPos.z), aCol);

			ImmediateVertex(cVector3f(avPos.x + cos(a + afAngleStep) * afRadius,
								avPos.y + sin(a + afAngleStep) * afRadius,
								avPos.z),
				aCol);
		}

		ImmediateDraw(GL_LINES, aTarget);
	}

	void cGLStreamDrawing::DrawLine2D(const cVector2f &avBegin, const cVector2f &avEnd, float afZ, cColor aCol, const cGLDrawTargetState &aTarget)
	{
		mState.SetTexture(0, nullptr);
		mState.SetBlendActive(false);
		ImmediateBegin(0);
		ImmediateVertex(cVector3f(avBegin.x, avBegin.y, afZ), aCol);
		ImmediateVertex(cVector3f(avEnd.x, avEnd.y, afZ), aCol);
		ImmediateDraw(GL_LINES, aTarget);
	}

	void cGLStreamDrawing::BindSheetIndices()
	{
		if(!mlSheetIndexBuffer)
		{
			static const unsigned int corners[] = {0,1,2,2,3,0};
			glGenBuffers(1, &mlSheetIndexBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, mlSheetIndexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			cGpuProfiler::Label(GL_BUFFER, mlSheetIndexBuffer, "Effect sheet triangles");
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mlSheetIndexBuffer);
	}

	const void *cGLStreamDrawing::StreamIndices(const unsigned int *apIndices, int alCount)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexBuffer);
		return (const void *)StreamToBuffer(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexSize, mlStreamIndexOffset,
			apIndices, alCount * sizeof(unsigned int));
	}

	void cGLStreamDrawing::StreamArray(unsigned int &alArrays, int alAttrib, int alElements,
		size_t alStride, size_t alOffset)
	{
		glVertexAttribPointer(alAttrib, alElements, GL_FLOAT, GL_FALSE, (GLsizei)alStride, (const void *)alOffset);
		alArrays |= 1u << alAttrib;
	}

	void cGLStreamDrawing::SetStreamArrays(unsigned int alArrays)
	{
		unsigned int lChanged = alArrays ^ mlStreamArrays;
		for(int i = 0; lChanged && i < cGLSLProgram::eVertexAttrib_LastEnum; ++i)
		{
			if((lChanged & (1u << i)) == 0) continue;
			if(alArrays & (1u << i))
				glEnableVertexAttribArray(i);
			else
				glDisableVertexAttribArray(i);
		}
		mlStreamArrays = alArrays;
	}

	void cGLStreamDrawing::ImmediateBegin(int alTexUnits)
	{
		mvImmediate.clear();
		mlImmediateTexUnits = alTexUnits;
	}

	void cGLStreamDrawing::ImmediateVertex(const cVector3f &avPos, const cColor &aCol, const cVector3f *apTex)
	{
		mvImmediate.insert(mvImmediate.end(), {avPos.x, avPos.y, avPos.z, aCol.r, aCol.g, aCol.b, aCol.a});
		for(int i = 0; i < mlImmediateTexUnits; ++i)
			mvImmediate.insert(mvImmediate.end(), {apTex[i].x, apTex[i].y, apTex[i].z});
	}

	void cGLStreamDrawing::DrawSpriteBatch(const cSpriteVertex *apVertices, size_t alCount,
		iTexture *apTexture, const cSpriteBlend &aBlend, const cGLDrawTargetState &aTarget)
	{
		if(alCount == 0) return;
		assert(alCount % 3 == 0);
		mState.SetTexture(0, apTexture);
		mState.SetBlendActive(aBlend.mbActive);
		mState.SetBlendFunc(aBlend.mSrc, aBlend.mDest);
		if(!mState.PrepareDraw(aTarget)) return;
		mGpuProfiler.CountDraw(GL_TRIANGLES, (int)alCount);
		mState.BindVertexArray(mlStreamVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mlStreamVertexBuffer);
		const size_t offset = StreamToBuffer(GL_ARRAY_BUFFER, mlStreamVertexSize, mlStreamVertexOffset,
			apVertices, alCount * sizeof(cSpriteVertex));
		unsigned int arrays = 0;
		StreamArray(arrays, cGLSLProgram::eVertexAttrib_Position, 3, sizeof(cSpriteVertex), offset + offsetof(cSpriteVertex, pos));
		StreamArray(arrays, cGLSLProgram::eVertexAttrib_Color, 4, sizeof(cSpriteVertex), offset + offsetof(cSpriteVertex, col));
		StreamArray(arrays, cGLSLProgram::eVertexAttrib_TexCoord, 3, sizeof(cSpriteVertex), offset + offsetof(cSpriteVertex, tex));
		SetStreamArrays(arrays);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)alCount);
	}

	void cGLStreamDrawing::ImmediateDraw(GLenum aMode, const cGLDrawTargetState &aTarget)
	{
		const size_t lFloats = 7 + 3 * mlImmediateTexUnits;
		const size_t lStride = lFloats * sizeof(float);
		const int lVertices = (int)(mvImmediate.size() / lFloats);
		if(lVertices == 0) return;

		if(!mState.PrepareDraw(aTarget)) return;
		mGpuProfiler.CountDraw(aMode, lVertices);

		mState.BindVertexArray(mlStreamVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mlStreamVertexBuffer);
		size_t lOffset = StreamToBuffer(GL_ARRAY_BUFFER, mlStreamVertexSize, mlStreamVertexOffset,
			mvImmediate.data(), mvImmediate.size() * sizeof(float));

		unsigned int lArrays = 0;
		StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Position, 3, lStride, lOffset);
		StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Color, 4, lStride, lOffset + 3 * sizeof(float));
		if(mlImmediateTexUnits)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_TexCoord, 3, lStride, lOffset + 7 * sizeof(float));
		SetStreamArrays(lArrays);

		glDrawArrays(aMode, 0, lVertices);

		// The last vertex's values stay as the current ones, as with glColor
		// and glTexCoord: draws without those arrays get them.
		const float *pLast = &mvImmediate[mvImmediate.size() - lFloats];
		mState.SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, pLast[3], pLast[4], pLast[5], pLast[6]);
		if(mlImmediateTexUnits)
			mState.SetCurrentAttrib(cGLSLProgram::eVertexAttrib_TexCoord, pLast[7], pLast[8], pLast[9], 1);
	}
} // namespace hpl
