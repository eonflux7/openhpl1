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
#include "impl/GLSLProgram.h"
#include "impl/SDLTexture.h"
#include "system/LowLevelSystem.h"
#include "system/String.h"
#include "math/Math.h"

#include <cstdio>
#include <vector>

namespace hpl{

	//////////////////////////////////////////////////////////////////////////
	// STATIC DATA
	//////////////////////////////////////////////////////////////////////////

	tString cGLSLProgram::msShaderDir = "shaders/";
	GLuint cGLSLProgram::mlPipeline = 0;
	cGLSLProgram* cGLSLProgram::mpBoundVertex = nullptr;
	cGLSLProgram* cGLSLProgram::mpBoundFragment = nullptr;

	static const char* kVertexPreamble =
		"#version 330 compatibility\n"
		"#define VARYING out\n";

	static const char* kFragmentPreamble =
		"#version 330 compatibility\n"
		"#define VARYING in\n"
		"#define OUTPUT out vec4 outColor;\n";

	// Programs that are variants of a shared file: the engine's program name
	// (without "_vp"/"_fp"), the file (without "hpl1_" and the extension) and
	// the #defines it is compiled with. Other names load the file of the
	// same name.
	struct cShaderVariant
	{
		const char *msName;
		const char *msFile;
		const char *msDefines;
	};

	static const cShaderVariant kShaderVariants[] = {
		{"Diffuse_Light",				"light",	""},
		{"Diffuse_Light_Spot",			"light",	"SPOT"},
		{"DiffuseSpec_Light",			"light",	"SPECULAR"},
		{"DiffuseSpec_Light_Spot",		"light",	"SPECULAR SPOT"},
		{"Bump_Light",					"light",	"NORMAL_MAP"},
		{"Bump_Light_Spot",				"light",	"NORMAL_MAP SPOT"},
		{"BumpSpec_Light",				"light",	"NORMAL_MAP SPECULAR"},
		{"BumpSpec_Light_Spot",			"light",	"NORMAL_MAP SPECULAR SPOT"},
		{"BumpColorSpec_Light",			"light",	"NORMAL_MAP SPECULAR SPECULAR_MAP"},
		{"BumpColorSpec_Light_Spot",	"light",	"NORMAL_MAP SPECULAR SPECULAR_MAP SPOT"},
		{"Fog_Trans_Alpha",				"Fog_Trans",	""},
		{"Fog_Trans_Mod",				"Fog_Trans",	"MOD"},
		{"Fog_Trans_ModX2",				"Fog_Trans",	"MODX2"},
		{"PostEffect_Blur_2D",			"PostEffect_Blur",	""},
		{"PostEffect_Blur_Rect",		"PostEffect_Blur",	"RECT"},
	};

	static const int kMaxIncludeDepth = 8;

	static const int kMaxSamplers = 8;

	//-----------------------------------------------------------------------

	static bool ReadTextFile(const tString& asPath, tString& asOut)
	{
		FILE *pFile = fopen(asPath.c_str(), "rb");
		if(pFile==nullptr) return false;

		fseek(pFile,0,SEEK_END);
		long lSize = ftell(pFile);
		fseek(pFile,0,SEEK_SET);

		asOut.resize(lSize);
		size_t lRead = fread(&asOut[0],1,lSize,pFile);
		fclose(pFile);

		return lRead == (size_t)lSize;
	}

	//-----------------------------------------------------------------------

	// "core/programs/Diffuse_Light_Spot_fp.cg" -> "hpl1_light", "SPOT"
	static tString CgNameToShaderBase(const tString& asCgFile, tString& asDefines)
	{
		tString sName = cString::GetFileName(asCgFile);
		sName = cString::SetFileExt(sName,"");

		size_t lPos = sName.find("_vp");
		if(lPos == tString::npos) lPos = sName.find("_fp");
		if(lPos != tString::npos) sName.erase(lPos,3);

		asDefines = "";
		for(const cShaderVariant& variant : kShaderVariants)
		{
			if(sName == variant.msName)
			{
				sName = variant.msFile;
				asDefines = variant.msDefines;
				break;
			}
		}

		return "hpl1_" + sName;
	}

	//-----------------------------------------------------------------------

	// Appends asFile with its #include "name" lines replaced by the named
	// files from the shader directory, each file once. Every file gets its
	// own source number in #line, so compile errors read "<number>(<line>)"
	// with the number indexing avFiles.
	static bool ExpandIncludes(const tString& asDir, const tString& asFile,
								std::vector<tString>& avFiles, tString& asOut, int alDepth)
	{
		tString sSource;
		if(ReadTextFile(asDir + asFile, sSource)==false)
		{
			Error("GLSL: Couldn't read '%s'\n",(asDir + asFile).c_str());
			return false;
		}

		int lFileNum = (int)avFiles.size();
		avFiles.push_back(asFile);

		char sLine[64];
		snprintf(sLine, sizeof(sLine), "#line 1 %d\n", lFileNum);
		asOut += sLine;

		size_t lStart = 0;
		int lLineNum = 1;
		while(lStart < sSource.size())
		{
			size_t lEnd = sSource.find('\n', lStart);
			if(lEnd == tString::npos) lEnd = sSource.size();
			tString sText = sSource.substr(lStart, lEnd - lStart);
			lStart = lEnd + 1;

			size_t lFirst = sText.find_first_not_of(" \t");
			if(lFirst == tString::npos || sText.compare(lFirst, 8, "#include") != 0)
			{
				asOut += sText;
				asOut += "\n";
				++lLineNum;
				continue;
			}

			size_t lOpen = sText.find('"', lFirst);
			size_t lClose = lOpen == tString::npos ? tString::npos : sText.find('"', lOpen + 1);
			if(lClose == tString::npos)
			{
				Error("GLSL: %s(%d): expected #include \"file\"\n", asFile.c_str(), lLineNum);
				return false;
			}
			tString sInclude = sText.substr(lOpen + 1, lClose - lOpen - 1);

			bool bIncluded = false;
			for(const tString& sFile : avFiles) bIncluded = bIncluded || sFile == sInclude;
			if(!bIncluded)
			{
				if(alDepth >= kMaxIncludeDepth)
				{
					Error("GLSL: %s(%d): includes nested too deep\n", asFile.c_str(), lLineNum);
					return false;
				}
				if(!ExpandIncludes(asDir, sInclude, avFiles, asOut, alDepth + 1)) return false;
			}

			++lLineNum;
			snprintf(sLine, sizeof(sLine), "#line %d %d\n", lLineNum, lFileNum);
			asOut += sLine;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cGLSLProgram::cGLSLProgram(tString asName,eGpuProgramType aType)
	: iGpuProgram(asName, aType)
	{
		msName = asName;
		mlProgram = 0;
		mlCompanionFragment = 0;
	}

	cGLSLProgram::~cGLSLProgram()
	{
		if(mpBoundVertex == this) mpBoundVertex = nullptr;
		if(mpBoundFragment == this) mpBoundFragment = nullptr;

		if(mlProgram) glDeleteProgram(mlProgram);
		if(mlCompanionFragment) glDeleteProgram(mlCompanionFragment);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cGLSLProgram::InitPipeline()
	{
		if(!GLAD_GL_VERSION_4_1 && !GLAD_GL_ARB_separate_shader_objects)
		{
			Error("OpenGL separate shader objects are not supported!\n");
			return false;
		}

		glGenProgramPipelines(1,&mlPipeline);
		glUseProgram(0);
		// Bound only while a stage is active, see UpdateFragmentStage().
		glBindProgramPipeline(0);
		return true;
	}

	void cGLSLProgram::ExitPipeline()
	{
		if(mlPipeline) glDeleteProgramPipelines(1,&mlPipeline);
		mlPipeline = 0;
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CreateStage(GLenum aStage, const tString& asName)
	{
		return CompileStage(aStage, "hpl1_" + asName +
									(aStage == GL_VERTEX_SHADER ? ".vertex" : ".fragment"), "");
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::Reload()
	{
		return false;
	}

	void cGLSLProgram::Unload()
	{
	}

	void cGLSLProgram::Destroy()
	{
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::CreateFromFile(const tString &asFile, const tString &asEntry)
	{
		tString sDefines;
		tString sBase = CgNameToShaderBase(asFile, sDefines);

		if(mProgramType == eGpuProgramType_Vertex)
		{
			mlProgram = CompileStage(GL_VERTEX_SHADER, sBase + ".vertex", sDefines);
			// Optional: not every vertex shader has a companion.
			FILE *pFile = fopen((msShaderDir + sBase + ".fragment").c_str(),"rb");
			if(pFile)
			{
				fclose(pFile);
				mlCompanionFragment = CompileStage(GL_FRAGMENT_SHADER, sBase + ".fragment", sDefines);
			}
		}
		else
		{
			mlProgram = CompileStage(GL_FRAGMENT_SHADER, sBase + ".fragment", sDefines);
		}

		if(mlProgram==0)
		{
			Error("GLSL: Couldn't create program for '%s'\n",asFile.c_str());
			return false;
		}

		if(mbDebugInfo) Log("GLSL: '%s' -> '%s' (%s)\n",msName.c_str(),sBase.c_str(),sDefines.c_str());

		return true;
	}

	//-----------------------------------------------------------------------

	void cGLSLProgram::Bind()
	{
		if(mProgramType == eGpuProgramType_Vertex)
		{
			mpBoundVertex = this;
			BindStage(GL_VERTEX_SHADER_BIT, mlProgram);
		}
		else
		{
			mpBoundFragment = this;
		}
		UpdateFragmentStage();
	}

	//-----------------------------------------------------------------------

	void cGLSLProgram::UnBind()
	{
		if(mProgramType == eGpuProgramType_Vertex)
		{
			if(mpBoundVertex == this) mpBoundVertex = nullptr;
			BindStage(GL_VERTEX_SHADER_BIT, 0);
		}
		else
		{
			if(mpBoundFragment == this) mpBoundFragment = nullptr;
		}
		UpdateFragmentStage();
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetFloat(const tString& asName, float afX)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform1f(mlProgram, lLoc, afX);
		return true;
	}

	bool cGLSLProgram::SetVec2f(const tString& asName, float afX,float afY)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform2f(mlProgram, lLoc, afX, afY);
		return true;
	}

	bool cGLSLProgram::SetVec3f(const tString& asName, float afX,float afY,float afZ)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform3f(mlProgram, lLoc, afX, afY, afZ);
		return true;
	}

	bool cGLSLProgram::SetVec4f(const tString& asName, float afX,float afY,float afZ, float afW)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform4f(mlProgram, lLoc, afX, afY, afZ, afW);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetMatrixf(const tString& asName, const cMatrixf& mMtx)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		// cMatrixf is row-major
		glProgramUniformMatrix4fv(mlProgram, lLoc, 1, GL_TRUE, &mMtx.m[0][0]);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetMatrixf(const tString& asName, eGpuProgramMatrix mType,
									eGpuProgramMatrixOp mOp)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;

		// Like cgGLSetStateMatrixParameter: sample the fixed-function
		// matrix stacks now. GL matrices are column-major.
		float vModelView[16], vProj[16], vResult[16];

		switch(mType)
		{
		case eGpuProgramMatrix_View:
			glGetFloatv(GL_MODELVIEW_MATRIX, vResult);
			break;
		case eGpuProgramMatrix_Projection:
			glGetFloatv(GL_PROJECTION_MATRIX, vResult);
			break;
		case eGpuProgramMatrix_Texture:
			glGetFloatv(GL_TEXTURE_MATRIX, vResult);
			break;
		case eGpuProgramMatrix_ViewProjection:
			glGetFloatv(GL_MODELVIEW_MATRIX, vModelView);
			glGetFloatv(GL_PROJECTION_MATRIX, vProj);
			for(int c=0; c<4; ++c)
				for(int r=0; r<4; ++r)
				{
					float fSum = 0;
					for(int k=0; k<4; ++k) fSum += vProj[k*4 + r] * vModelView[c*4 + k];
					vResult[c*4 + r] = fSum;
				}
			break;
		default:
			return false;
		}

		// Matrix ops are kept in cMatrixf (row-major) form.
		cMatrixf mtxResult;
		for(int c=0; c<4; ++c)
			for(int r=0; r<4; ++r)
				mtxResult.m[r][c] = vResult[c*4 + r];

		switch(mOp)
		{
		case eGpuProgramMatrixOp_Identity:			break;
		case eGpuProgramMatrixOp_Inverse:			mtxResult = cMath::MatrixInverse(mtxResult); break;
		case eGpuProgramMatrixOp_Transpose:			mtxResult = mtxResult.GetTranspose(); break;
		case eGpuProgramMatrixOp_InverseTranspose:	mtxResult = cMath::MatrixInverse(mtxResult).GetTranspose(); break;
		default:									break;
		}

		glProgramUniformMatrix4fv(mlProgram, lLoc, 1, GL_TRUE, &mtxResult.m[0][0]);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetTexture(const tString& asName,iTexture* apTexture, bool abAutoDisable)
	{
		// Samplers are named texN and fixed to unit N.
		if(asName.size() < 4 || asName.compare(0,3,"tex") != 0) return false;
		int lUnit = cString::ToInt(asName.substr(3).c_str(),-1);
		if(lUnit < 0) return false;

		return SetTextureToUnit(lUnit, apTexture);
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetTextureToUnit(int alUnit, iTexture* apTexture)
	{
		if(alUnit < 0 || alUnit >= kMaxSamplers) return false;
		if(apTexture==nullptr) return true;

		GLenum Target = GL_TEXTURE_2D;
		switch(apTexture->GetTarget())
		{
		case eTextureTarget_1D:			Target = GL_TEXTURE_1D; break;
		case eTextureTarget_2D:			Target = GL_TEXTURE_2D; break;
		case eTextureTarget_Rect:		Target = GL_TEXTURE_RECTANGLE; break;
		case eTextureTarget_CubeMap:	Target = GL_TEXTURE_CUBE_MAP; break;
		case eTextureTarget_3D:			Target = GL_TEXTURE_3D; break;
		default: break;
		}

		GLint lPrevUnit;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &lPrevUnit);
		glActiveTexture(GL_TEXTURE0 + alUnit);
		glBindTexture(Target, static_cast<cSDLTexture*>(apTexture)->GetTextureHandle());
		glActiveTexture(lPrevUnit);

		return true;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	GLint cGLSLProgram::GetUniform(const tString& asName)
	{
		if(mlProgram==0) return -1;

		std::map<tString,GLint>::iterator it = m_mapUniforms.find(asName);
		if(it != m_mapUniforms.end()) return it->second;

		GLint lLoc = glGetUniformLocation(mlProgram, asName.c_str());
		m_mapUniforms.insert(std::map<tString,GLint>::value_type(asName,lLoc));
		return lLoc;
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CompileStage(GLenum aStage, const tString& asFile, const tString& asDefines)
	{
		tString sSource = aStage == GL_VERTEX_SHADER ? kVertexPreamble : kFragmentPreamble;

		tStringVec vDefines;
		cString::GetStringVec(asDefines, vDefines, nullptr);
		for(const tString& sDefine : vDefines) sSource += "#define " + sDefine + "\n";

		// Source number 0 is the preamble above.
		std::vector<tString> vFiles(1, "(preamble)");
		if(ExpandIncludes(msShaderDir, asFile, vFiles, sSource, 0)==false) return 0;

		tString sName = asFile;
		if(asDefines != "") sName += " (" + asDefines + ")";

		const char* pSource = sSource.c_str();
		GLuint lShader = glCreateShader(aStage);
		glShaderSource(lShader, 1, &pSource, nullptr);
		glCompileShader(lShader);

		GLint lOk = GL_FALSE;
		glGetShaderiv(lShader, GL_COMPILE_STATUS, &lOk);
		if(!lOk)
		{
			char sLog[4096];
			glGetShaderInfoLog(lShader, sizeof(sLog), nullptr, sLog);
			tString sFiles;
			for(size_t i=0; i<vFiles.size(); ++i)
				sFiles += "  " + cString::ToString((int)i) + " = " + vFiles[i] + "\n";
			Error("GLSL: Compiling '%s' failed:\n%s\nSource numbers:\n%s",sName.c_str(),sLog,sFiles.c_str());
			glDeleteShader(lShader);
			return 0;
		}

		GLuint lProgram = glCreateProgram();
		glProgramParameteri(lProgram, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glAttachShader(lProgram, lShader);
		glLinkProgram(lProgram);
		glDetachShader(lProgram, lShader);
		glDeleteShader(lShader);

		glGetProgramiv(lProgram, GL_LINK_STATUS, &lOk);
		if(!lOk)
		{
			char sLog[4096];
			glGetProgramInfoLog(lProgram, sizeof(sLog), nullptr, sLog);
			Error("GLSL: Linking '%s' failed:\n%s\n",sName.c_str(),sLog);
			glDeleteProgram(lProgram);
			return 0;
		}

		// texN samplers always read texture unit N
		for(int i=0; i<kMaxSamplers; ++i)
		{
			char sName[8];
			snprintf(sName, sizeof(sName), "tex%d", i);
			GLint lLoc = glGetUniformLocation(lProgram, sName);
			if(lLoc >= 0) glProgramUniform1i(lProgram, lLoc, i);
		}

		return lProgram;
	}

	//-----------------------------------------------------------------------

	void cGLSLProgram::BindStage(GLbitfield aStageBit, GLuint alProgram)
	{
		glUseProgramStages(mlPipeline, aStageBit, alProgram);
	}

	//-----------------------------------------------------------------------

	void cGLSLProgram::UpdateFragmentStage()
	{
		GLuint lFragment = 0;
		if(mpBoundFragment)		lFragment = mpBoundFragment->mlProgram;
		else if(mpBoundVertex)	lFragment = mpBoundVertex->mlCompanionFragment;

		BindStage(GL_FRAGMENT_SHADER_BIT, lFragment);

		// An empty pipeline is not "fixed function" for every driver (NVIDIA
		// rejects draws with it), so only keep it bound while it has a stage.
		bool bAnyStage = lFragment != 0 || (mpBoundVertex && mpBoundVertex->mlProgram);
		glBindProgramPipeline(bAnyStage ? mlPipeline : 0);

		if(bAnyStage && mbDebugInfo)
		{
			static int lReported = 0;
			glValidateProgramPipeline(mlPipeline);
			GLint lValid = GL_TRUE;
			glGetProgramPipelineiv(mlPipeline, GL_VALIDATE_STATUS, &lValid);
			if(!lValid && lReported < 10)
			{
				++lReported;
				char sLog[2048] = "";
				glGetProgramPipelineInfoLog(mlPipeline, sizeof(sLog), nullptr, sLog);
				Log("GLSL: invalid pipeline (vertex '%s', fragment '%s'): %s\n",
					mpBoundVertex ? mpBoundVertex->msName.c_str() : "-",
					mpBoundFragment ? mpBoundFragment->msName.c_str() : "-", sLog);
			}
		}
	}

	//-----------------------------------------------------------------------
}
