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

#include <filesystem>
#include <map>
#include <set>
#include <vector>

#include <glad/gl.h>

#include "graphics/GPUProgram.h"
#include "system/SystemTypes.h"

namespace hpl {

	class iLowLevelGraphics;

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

		/// Vertex attribute locations: hpl_Vertex, hpl_Normal, hpl_Color and
		/// hpl_MultiTexCoord0..4 in vertex shaders.
		enum eVertexAttrib
		{
			eVertexAttrib_Position = 0,
			eVertexAttrib_Normal = 1,
			eVertexAttrib_Color = 2,
			eVertexAttrib_TexCoord0 = 4,
			eVertexAttrib_LastEnum = 9
		};

		/// Uniform buffer binding of the hpl_Frame block (hpl1_frame.glsl).
		static const GLuint kFrameBlockBinding = 0;
		/// Uniform buffer binding of the hpl_State block, which has the alpha
		/// test settings every fragment shader applies (see LinkStage()).
		static const GLuint kStateBlockBinding = 1;
		/// Texture unit of the hpl_ShadowMap sampler; hpl_ShadowMapDepth has
		/// the next one. Past the units the engine binds textures to.
		static const int kShadowMapUnit = 8;
		/// Texture unit of the hpl_SceneDepth sampler (hpl1_scene_depth.glsl):
		/// the depth of the 3D scene.
		static const int kSceneDepthUnit = 10;
		/// Texture unit of the hpl_SceneColor sampler: the colour of the 3D
		/// scene as it is drawn into, multisampled if the scene is
		/// (iLowLevelGraphics::BeginSceneColorRead).
		static const int kSceneColorUnit = 11;
		/// hpl_AlphaTest.x when the alpha test is done as coverage of the
		/// pixel's samples; 1 is the plain test, 0 none.
		static const int kAlphaToCoverage = 2;

		GLuint GetHandle(){ return mlProgram; }
		/// The fragment stage that goes with a vertex program, 0 for none.
		GLuint GetCompanionHandle(){ return mlCompanionFragment; }

		/// apLowLevelGraphics supplies the matrices for SetMatrixf.
		static bool InitPipeline(iLowLevelGraphics *apLowLevelGraphics);
		static void ExitPipeline();

		static void SetShaderDirectory(const tString& asDir){ msShaderDir = asDir; }
		static const tString& GetShaderDirectory(){ return msShaderDir; }

		/**
		 * Compiles "hpl1_<asName>.vertex" or ".fragment" from the shader
		 * directory as a separable program, for passes that are drawn outside
		 * the engine's program objects, with asDefines (separated by
		 * spaces) defined. The caller deletes it.
		 */
		static GLuint CreateStage(GLenum aStage, const tString& asName, const tString& asDefines="");

		/**
		 * Compiles source made by the engine as a separable program, with
		 * the preamble (and, for fragment shaders, the epilogue that clamps
		 * the colour; with abState it also follows the alpha test and
		 * colour space set through iLowLevelGraphics). The caller deletes it.
		 */
		static GLuint CompileSource(GLenum aStage, const tString& asSource, const tString& asName,
									bool abState);

		/// The programs bound for each stage (a fragment stage can be a
		/// vertex program's companion), 0 for none.
		static GLuint GetVertexStage();
		static GLuint GetFragmentStage();

		/// Puts programs in the engine's program pipeline, which stays bound;
		/// done before every draw, as Bind() and UnBind() only take note.
		static void SetPipelineStages(GLuint alVertex, GLuint alFragment);

		/**
		 * With OVERTURE_SHADER_RELOAD set, recompiles the programs whose
		 * files (or the files they include) changed on disk; checks at most
		 * twice a second. A program that does not compile keeps its old
		 * version. Returns true if a file changed, so that the owners of
		 * CreateStage() programs can create them again.
		 */
		static bool ReloadChangedFiles();

	private:
		struct cSourceFile
		{
			tString msPath;
			std::filesystem::file_time_type mTime;
		};

		GLint GetUniform(const tString& asName);
		bool CompileProgram();
		static GLuint CompileStage(GLenum aStage, const tString& asFile, const tString& asDefines,
									bool abState, std::vector<tString> *apFiles=nullptr);
		static GLuint LinkStage(GLenum aStage, tString asSource, const tString& asName,
								const std::vector<tString>& avFiles);
		static void WatchFiles(std::vector<cSourceFile>& avWatched, const std::vector<tString>& avFiles);
		static bool FilesChanged(std::vector<cSourceFile>& avWatched);

		tString msName;
		tString msBase;
		tString msDefines;
		GLuint mlProgram;
		std::map<tString, GLint> m_mapUniforms;

		// Vertex programs only: the fragment shader written to pair with this
		// vertex shader, used when no fragment program is bound. Cg vertex
		// programs could feed fixed-function fragment processing; GLSL ones
		// with custom outputs cannot.
		GLuint mlCompanionFragment;

		// The files the program was compiled from, for reloading.
		std::vector<cSourceFile> mvSources;

		static tString msShaderDir;
		static iLowLevelGraphics* mpLowLevelGraphics;
		static bool mbReloadEnabled;
		static std::set<cGLSLProgram*> msetPrograms;
		static std::vector<cSourceFile> mvStageSources;
		static GLuint mlPipeline;
		static cGLSLProgram* mpBoundVertex;
		static cGLSLProgram* mpBoundFragment;
		static GLuint mlPipelineVertex;
		static GLuint mlPipelineFragment;
	};

};
#endif // HPL_GLSLPROGRAM_H
