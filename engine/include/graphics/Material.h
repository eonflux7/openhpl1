// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_MATERIAL_H
#define HPL_MATERIAL_H

#include <vector>
#include <optional>
#include "graphics/Texture.h"
#include "system/SystemTypes.h"
#include "resources/ResourceImage.h"
#include "resources/ResourceBase.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/GPUProgram.h"

class TiXmlElement;

namespace hpl {


#define kMaxProgramNum (5)

	enum eMaterialTexture
	{
		eMaterialTexture_Diffuse,
		eMaterialTexture_NMap,
		eMaterialTexture_Specular,
		eMaterialTexture_Alpha,
		eMaterialTexture_Illumination,
		eMaterialTexture_CubeMap,
		eMaterialTexture_Refraction,
		eMaterialTexture_LastEnum
	};

	enum eMaterialPicture
	{
		eMaterialPicture_Image,
		eMaterialPicture_Texture,
		eMaterialPicture_LastEnum
	};

	enum eMaterialRenderType
	{
		eMaterialRenderType_Z,
		eMaterialRenderType_Light,
		eMaterialRenderType_Diffuse,
		eMaterialRenderType_LastEnum
	};

	enum eMaterialBlendMode
	{
		eMaterialBlendMode_None,
		eMaterialBlendMode_Add,
		eMaterialBlendMode_Mul,
		eMaterialBlendMode_MulX2,
		eMaterialBlendMode_Replace,
		eMaterialBlendMode_Alpha,
		eMaterialBlendMode_DestAlphaAdd,
		eMaterialBlendMode_LastEnum
	};

	enum eMaterialAlphaMode
	{
		eMaterialAlphaMode_Solid,
		eMaterialAlphaMode_Trans,
		eMaterialAlphaMode_LastEnum,
	};

	//! Determines what color channels are going to be affected
	enum eMaterialChannelMode
	{
		eMaterialChannelMode_RGBA,
		eMaterialChannelMode_RGB,
		eMaterialChannelMode_A,
		eMaterialChannelMode_Z,
		eMaterialChannelMode_LastEnum,
	};

	class cRenderer3D;
	class cRenderSettings;
	class cTextureManager;
	class cImageManager;
	class cGpuProgramManager;
	class iLight;
	class iCamera;
	class iLight3D;

	class iMaterialProgramSetup
	{
	public:
		virtual void Setup(iGpuProgram *apProgram,cRenderSettings* apRenderSettings)=0;
		virtual void SetupMatrix(cMatrixf *apModelMatrix, cRenderSettings* apRenderSettings){}
	};

	class cTextureType
	{
	public:
		cTextureType(tString asSuffix, eMaterialTexture aType): msSuffix(asSuffix), mType(aType){}

		tString msSuffix;
		eMaterialTexture mType;
	};

	typedef std::list<cTextureType> tTextureTypeList;
	typedef tTextureTypeList::iterator tTextureTypeListIt;

	class iMaterial : public iResourceBase
	{
	public:
		iMaterial(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
				cImageManager* apImageManager, cTextureManager *apTextureManager,
				cGpuProgramManager* apProgramManager,
				eMaterialPicture aPicture, cRenderer3D *apRenderer3D);
		~iMaterial() override;

		//resources stuff.
		bool Reload() override{ return false;}
		void Unload() override{}
		void Destroy() override{}

		virtual void Compile()=0;

		virtual void Update(float afTimeStep){}

		//The new render system stuff
		virtual iGpuProgram* GetProgram(eMaterialRenderType aType, int alPass, iLight3D *apLight){return nullptr;}
		virtual bool ProgramUsesLight(eMaterialRenderType aType, int alPass, iLight3D *apLight){return false;}
		virtual bool ProgramUsesEye(eMaterialRenderType aType, int alPass, iLight3D *apLight){return false;}
		virtual iMaterialProgramSetup * GetProgramSetup(eMaterialRenderType aType, int alPass, iLight3D *apLight){return nullptr;}


		virtual eMaterialAlphaMode GetAlphaMode(eMaterialRenderType aType, int alPass, iLight3D *apLight){return eMaterialAlphaMode_Solid;}
		virtual eMaterialBlendMode GetBlendMode(eMaterialRenderType aType, int alPass, iLight3D *apLight){return eMaterialBlendMode_Replace;}
		virtual eMaterialChannelMode GetChannelMode(eMaterialRenderType aType, int alPass, iLight3D *apLight){return eMaterialChannelMode_RGBA;}

		virtual iTexture* GetTexture(int alUnit,eMaterialRenderType aType, int alPass, iLight3D *apLight){ return nullptr;}

		virtual int GetNumOfPasses(eMaterialRenderType aType, iLight3D *apLight){ return 0;}

		virtual bool UsesType(eMaterialRenderType aType){return false;}

		bool HasAlpha(){ return mbHasAlpha;}
		void SetHasAlpha(bool abX){ mbHasAlpha= abX; }

		bool GetDepthTest(){ return mbDepthTest;}
		void SetDepthTest(bool abX){ mbDepthTest = abX;}

		float GetValue() { return mfValue;}
		void SetValue(float afX) { mfValue = afX;}

		virtual bool LoadData(TiXmlElement* apRootElem){ return true; }

		// Sprite materials describe one texture and blend state; world materials
		// use the pass descriptions above.
		virtual std::optional<cSpriteBlend> GetSpriteBlend() const { return std::nullopt; }

		iTexture* GetTexture(eMaterialTexture aType);
		cRect2f GetTextureOffset(eMaterialTexture aType);

		void SetTexture(iTexture* apTex,eMaterialTexture aType){ mvTexture[aType] = apTex; }
		void SetImage(cResourceImage* apImg,eMaterialTexture aType){
			mvImage[aType] = apImg;	}
		cResourceImage* GetImage(eMaterialTexture aType){ return mvImage[aType];}

		void SetProgram(iGpuProgram* apProgram, unsigned int alNum){
						mpProgram[alNum] = apProgram;}
		iGpuProgram* GetStoredProgram(unsigned int alNum){
						return mpProgram[alNum];}

		/**
		 * return true if the material is transparent
		 * \return
		 */
		virtual bool IsTransparent() { return mbIsTransparent; }
		virtual bool IsGlowing() { return mbIsGlowing; }
		/**
		 * return true if the material has a light pass
		 * \return
		 */
		virtual bool UsesLights(){ return mbUsesLights; }

		virtual tTextureTypeList GetTextureTypes(){
			tTextureTypeList vTypes;
			vTypes.push_back(cTextureType("",eMaterialTexture_Diffuse));
			return vTypes;
		}

		/**
		 * The type of the material
		 * \param alId
		 */
		void SetId(int alId){ mlId = alId;}
		int GetId(){ return mlId;}

		virtual iGpuProgram* GetRefractionProgram(){ return nullptr;}

		virtual bool GetRefractionUsesDiffuse(){ return false;}
		virtual eMaterialTexture GetRefractionDiffuseTexture(){ return eMaterialTexture_Diffuse;}

		virtual bool GetRefractionUsesEye(){ return false;}
		virtual bool GetRefractionUsesTime(){ return false;}
		virtual bool GetRefractionSkipsStandardTrans(){ return false;}

		const tString& GetPhysicsMaterial(){ return msPhysicsMaterial;}
		void SetPhysicsMaterial(const tString& asName){ msPhysicsMaterial = asName;}

	protected:
		iLowLevelGraphics* mpLowLevelGraphics;
		cImageManager* mpImageManager;
		cTextureManager* mpTextureManager;
		cRenderer3D* mpRenderer3D;
		cRenderSettings *mpRenderSettings;
		cGpuProgramManager* mpProgramManager;

		bool mbIsTransparent;
		bool mbIsGlowing;
		bool mbUsesLights;
		bool mbHasAlpha;
		bool mbDepthTest;
		float mfValue;

		int mlId;

		tString msPhysicsMaterial;

		tTextureVec mvTexture;
		tResourceImageVec mvImage;

		eMaterialPicture mPicture;

		iGpuProgram *mpProgram[kMaxProgramNum];


	};

	typedef std::vector<iMaterial*> tMaterialVec;

	class iMaterialType
	{
	public:
		virtual ~iMaterialType(){}
		virtual bool IsCorrect(tString asName)=0;
		virtual iMaterial* Create(const tString& asName,iLowLevelGraphics* apLowLevelGraphics,
			cImageManager* apImageManager, cTextureManager *apTextureManager,
			cGpuProgramManager* apProgramManager,
			eMaterialPicture aPicture, cRenderer3D *apRenderer3D)=0;
	};

	typedef std::list<iMaterialType*> tMaterialTypeList;
	typedef tMaterialTypeList::iterator tMaterialTypeListIt;

};
#endif // HPL_MATERIAL_H
