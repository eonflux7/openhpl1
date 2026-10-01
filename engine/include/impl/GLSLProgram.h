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
#ifndef HPL_GLSLPROGRAM_H
#define HPL_GLSLPROGRAM_H

#include <map>

#include <glad/gl.h>

#include "graphics/GPUProgram.h"
#include "system/SystemTypes.h"

namespace hpl {

	/**
	 * Replacement for the original Cg program class.
	 *
	 * The engine treats vertex and fragment programs as independent objects
	 * (the Cg model), so each one is compiled as a separable GLSL program and
	 * attached to a shared program pipeline when bound.
	 *
	 * Requests for "Foo_vp.cg" / "Foo_fp.cg" load "hpl1_Foo.vertex" /
	 * "hpl1_Foo.fragment" from the shader directory, or a shared file with
	 * #defines for the names listed in kShaderVariants. Shader files can
	 * #include "file" from the shader directory.
	 */
	class cGLSLProgram : public iGpuProgram
	{
	public:
		cGLSLProgram(tString asName, eGpuProgramType aType);
		~cGLSLProgram() override;

		bool Reload() override;
		void Unload() override;
		void Destroy() override;

		tString GetProgramName(){ return msName; }

		bool CreateFromFile(const tString& asFile, const tString& asEntry="main") override;

		void Bind() override;
		void UnBind() override;

		bool SetFloat(const tString& asName, float afX) override;
		bool SetVec2f(const tString& asName, float afX,float afY) override;
		bool SetVec3f(const tString& asName, float afX,float afY,float afZ) override;
		bool SetVec4f(const tString& asName, float afX,float afY,float afZ, float afW) override;

		bool SetMatrixf(const tString& asName, const cMatrixf& mMtx) override;
		bool SetMatrixf(const tString& asName, eGpuProgramMatrix mType,
									eGpuProgramMatrixOp mOp) override;

		bool SetTexture(const tString& asName,iTexture* apTexture, bool abAutoDisable=true) override;
		bool SetTextureToUnit(int alUnit, iTexture* apTexture) override;

		/// GLSL SPECIFIC //////////////////////

		GLuint GetHandle(){ return mlProgram; }

		static bool InitPipeline();
		static void ExitPipeline();

		static void SetShaderDirectory(const tString& asDir){ msShaderDir = asDir; }
		static const tString& GetShaderDirectory(){ return msShaderDir; }

		/**
		 * Compiles "hpl1_<asName>.vertex" or ".fragment" from the shader
		 * directory as a separable program, for passes that are drawn outside
		 * the engine's program objects. The caller deletes it.
		 */
		static GLuint CreateStage(GLenum aStage, const tString& asName);

	private:
		GLint GetUniform(const tString& asName);
		static GLuint CompileStage(GLenum aStage, const tString& asFile, const tString& asDefines);
		static void BindStage(GLbitfield aStageBit, GLuint alProgram);
		static void UpdateFragmentStage();

		tString msName;
		GLuint mlProgram;
		std::map<tString, GLint> m_mapUniforms;

		// Vertex programs only: the fragment shader written to pair with this
		// vertex shader, used when no fragment program is bound. Cg vertex
		// programs could feed fixed-function fragment processing; GLSL ones
		// with custom outputs cannot.
		GLuint mlCompanionFragment;

		static tString msShaderDir;
		static GLuint mlPipeline;
		static cGLSLProgram* mpBoundVertex;
		static cGLSLProgram* mpBoundFragment;
	};

};
#endif // HPL_GLSLPROGRAM_H
