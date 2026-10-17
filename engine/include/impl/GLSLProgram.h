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
#include <array>
#include <map>
#include <set>
#include <vector>

#include <glad/gl.h>

#include "graphics/GPUProgram.h"
#include "system/SystemTypes.h"

namespace hpl {

	class iLowLevelGraphics;

	// A linked vertex/fragment program. Reload swaps a complete pair atomically.
	class cGLSLProgram : public iGpuProgram
	{
	public:
		cGLSLProgram(tString asName, bool abState=true);
		~cGLSLProgram() override;

		bool Reload() override;
		void Unload() override;
		void Destroy() override;

		tString GetProgramName(){ return msName; }

		bool CreateFromFile(const tString& asFile, const tString& asDefines="") override;

		void Bind() override;
		void UnBind() override;

		bool SetTexture(const tString& asName,iTexture* apTexture, bool abAutoDisable=true) override;
		bool SetTextureToUnit(int alUnit, iTexture* apTexture) override;

		/// GLSL SPECIFIC //////////////////////

		/// Vertex attribute locations: hpl_Vertex, hpl_Normal, hpl_Color and
		/// hpl_TexCoord and hpl_Tangent in vertex shaders.
		enum eVertexAttrib
		{
			eVertexAttrib_Position = 0,
			eVertexAttrib_Normal = 1,
			eVertexAttrib_Color = 2,
			eVertexAttrib_TexCoord = 4,
			eVertexAttrib_Tangent = 5,
			eVertexAttrib_LastEnum = 6
		};

		/// Uniform buffer binding of the hpl_Frame block (hpl1_frame.glsl).
		static const GLuint kFrameBlockBinding = 0;
		/// Uniform buffer binding of the hpl_State block, which has the alpha
		/// test settings every fragment shader applies (see CompileSource()).
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

		static bool InitPrograms(iLowLevelGraphics *apLowLevelGraphics);
		static void ExitPrograms();
		static void SetShaderDirectory(const tString& asDir){ msShaderDir = asDir; }
		static const tString& GetShaderDirectory(){ return msShaderDir; }

		static cGLSLProgram* GetBoundProgram(){ return mpBoundProgram; }
		bool Apply(unsigned int alArrayMask=0, const float* apLayers=nullptr);

		// Recompile complete pairs after a source/include edit, at most twice
		// a second. Failed edits retain the previous program and its values.
		static bool ReloadChangedFiles();

	private:
		struct cSourceFile
		{
			tString msPath;
			std::filesystem::file_time_type mTime;
		};

		static GLuint CreateLinked(const tString& asName, const tString& asDefines="", bool abState=false,
			const tString& asVertex="", std::vector<tString>* apFiles=nullptr);
		bool CompileProgram();
		static GLuint CompileShader(GLenum aStage, const tString& asFile, const tString& asDefines,
									bool abState, std::vector<tString> *apFiles=nullptr);
		static GLuint CompileSource(GLenum aStage, tString asSource, const tString& asName,
								const std::vector<tString>& avFiles);
		static void WatchFiles(std::vector<cSourceFile>& avWatched, const std::vector<tString>& avFiles);
		static bool FilesChanged(std::vector<cSourceFile>& avWatched);

		tString msName;
		tString msBase;
		tString msVertex;
		tString msDefines;
		GLuint mlProgram;
		struct cTextureVariant {
			GLuint handle = 0;
			std::array<GLint, 8> layers;
		};
		std::map<unsigned int, cTextureVariant> mTextureVariants;
		unsigned int mlSampler2DMask = 0;
		cTextureVariant LinkTextureVariant(unsigned int alMask, std::vector<tString>& avFiles);

		// The files the program was compiled from, for reloading.
		std::vector<cSourceFile> mvSources;

		static tString msShaderDir;
		static iLowLevelGraphics* mpLowLevelGraphics;
		static bool mbReloadEnabled;
		static std::set<cGLSLProgram*> msetPrograms;
		GLuint mlMaterialBuffer = 0;
		bool mbState;
		static cGLSLProgram* mpBoundProgram;
	};

};
#endif // HPL_GLSLPROGRAM_H
