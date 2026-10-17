// This file is part of Overture Remade, licensed under the GNU GPL v3 or later.
#ifndef HPL_GLSTREAMDRAWING_H
#define HPL_GLSTREAMDRAWING_H
#include <glad/gl.h>
#include "impl/GLGraphicsState.h"
#include "impl/GpuProfiler.h"

namespace hpl
{
	// Owns streamed triangle/sprite/debug geometry; callers supply the draw target state.
	// Construction needs no GL. Initialization/use and teardown of allocated resources
	// require the caller's current context; Reset/cleanup is idempotent.
	class cGLStreamDrawing
	{
	public:
		cGLStreamDrawing(cGLGraphicsState &aState, cGpuProfiler &aProfiler) : mState(aState), mGpuProfiler(aProfiler) {}
		~cGLStreamDrawing() { Reset(); }
		void Init();
		void Reset();
		cGLStreamDrawing(const cGLStreamDrawing &) = delete;
		cGLStreamDrawing &operator=(const cGLStreamDrawing &) = delete;
		void DrawTriangles(const tVertexVec &avVertices, const cGLDrawTargetState &aTarget);

		void DrawLine(const cVector3f &avBegin, const cVector3f &avEnd, cColor aCol, const cGLDrawTargetState &aTarget);

		void DrawBoxMaxMin(const cVector3f &avMax, const cVector3f &avMin, cColor aCol, const cGLDrawTargetState &aTarget);

		void DrawSphere(const cVector3f &avPos, float afRadius, cColor aCol, const cGLDrawTargetState &aTarget);

		void DrawLine2D(const cVector2f &avBegin, const cVector2f &avEnd, float afZ, cColor aCol, const cGLDrawTargetState &aTarget);

		// Shared immutable triangle topology for all instanced effect sheets.
		void BindSheetIndices();

		const void *StreamIndices(const unsigned int *apIndices, int alCount);

		void DrawSpriteBatch(const cSpriteVertex *apVertices, size_t alCount,
			iTexture *apTexture, const cSpriteBlend &aBlend, const cGLDrawTargetState &aTarget);

	private:
		void StreamArray(unsigned int &alArrays, int alAttrib, int alElements,
			size_t alStride, size_t alOffset);

		void SetStreamArrays(unsigned int alArrays);

		void ImmediateBegin(int alTexUnits);

		void ImmediateVertex(const cVector3f &avPos, const cColor &aCol, const cVector3f *apTex = nullptr);

		void ImmediateDraw(GLenum aMode, const cGLDrawTargetState &aTarget);
		cGLGraphicsState &mState;
		cGpuProfiler &mGpuProfiler;
		GLuint mlSheetIndexBuffer = 0;
		GLuint mlStreamVertexArray = 0;
		GLuint mlStreamVertexBuffer = 0;
		GLuint mlStreamIndexBuffer = 0;
		size_t mlStreamVertexSize = 4 * 1024 * 1024;
		size_t mlStreamIndexSize = 1024 * 1024;
		size_t mlStreamVertexOffset = 0;
		size_t mlStreamIndexOffset = 0;
		unsigned int mlStreamArrays = 0;
		std::vector<float> mvImmediate;
		int mlImmediateTexUnits = 0;
	};
} // namespace hpl
#endif
