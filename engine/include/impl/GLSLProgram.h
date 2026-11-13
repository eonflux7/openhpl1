// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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
		// Raw GL clients restore program/material bindings or invalidate them
		// here before returning to engine draws. Apply then rebinds both.
		static void ClearAppliedProgram(){ mpAppliedProgram = nullptr; }
		static void SetShaderDirectory(const tString& asDir){ msShaderDir = asDir; }
		static const tString& GetShaderDirectory(){ return msShaderDir; }

		/// The program selected as engine state (Bind/UnBind), used by PrepareDraw.
		static cGLSLProgram* GetBoundProgram(){ return mpBoundProgram; }
		/// The program whose GL variant Apply() last made current.
		static cGLSLProgram* GetAppliedProgram(){ return mpAppliedProgram; }
		bool Apply(unsigned int alArrayMask=0, const float* apLayers=nullptr);
		/// Re-bind the variant last used by Apply, without re-applying values.
		void RebindCurrent();

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
		void ApplyBindings(GLuint alHandle);
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
		GLuint mlCurrentHandle = 0;
		unsigned int mlCurrentMask = 0;
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
		static cGLSLProgram* mpAppliedProgram;
	};

};
#endif // HPL_GLSLPROGRAM_H
