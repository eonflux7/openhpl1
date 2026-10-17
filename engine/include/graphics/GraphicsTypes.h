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
#ifndef HPL_GRAPHICSTYPES_H
#define HPL_GRAPHICSTYPES_H

#include <list>
#include <vector>
#include "graphics/Color.h"
#include "math/MathTypes.h"

namespace hpl {

	#define MAX_TEXTUREUNITS (8)
	#define MAX_NUM_OF_LIGHTS (30)


	enum eBlendFunc
	{
		eBlendFunc_Zero,
		eBlendFunc_One,
		eBlendFunc_SrcColor,
		eBlendFunc_OneMinusSrcColor,
		eBlendFunc_DestColor,
		eBlendFunc_OneMinusDestColor,
		eBlendFunc_SrcAlpha,
		eBlendFunc_OneMinusSrcAlpha,
		eBlendFunc_DestAlpha,
		eBlendFunc_OneMinusDestAlpha,
		eBlendFunc_SrcAlphaSaturate,
		eBlendFunc_LastEnum
	};

	enum eTileRotation
	{
		eTileRotation_0,
		eTileRotation_90,
		eTileRotation_180,
		eTileRotation_270,
		eTileRotation_LastEnum
	};

	enum ePrimitiveType
	{
		ePrimitiveType_Tri,
		ePrimitiveType_Quad,
		ePrimitiveType_LastEnum
	};

	typedef tFlag tAnimTransformFlag;

	#define eAnimTransformFlag_Translate	(0x00000001)
	#define eAnimTransformFlag_Scale		(0x00000002)
	#define eAnimTransformFlag_Rotate		(0x00000004)

	#define klNumOfAnimTransformFlags (3)

	const tAnimTransformFlag kvAnimTransformFlags[] = {eAnimTransformFlag_Translate,
					eAnimTransformFlag_Scale, eAnimTransformFlag_Rotate};

	//-----------------------------------------

	enum eColorDataFormat
	{
		eColorDataFormat_RGB,
		eColorDataFormat_RGBA,
		eColorDataFormat_ALPHA,
		eColorDataFormat_BGR,
		eColorDataFormat_BGRA,
		eColorDataFormat_LastEnum
	};

	//---------------------------------------

	enum eFontAlign
	{
		eFontAlign_Left,
		eFontAlign_Right,
		eFontAlign_Center,
		eFontAlign_LastEnum
	};

	//---------------------------------------

	class cKeyFrame
	{
	public:
		cVector3f trans;
		cVector3f scale;
		cQuaternion rotation;
		float time;
	};

	typedef std::vector<cKeyFrame*> tKeyFramePtrVec;
	typedef tKeyFramePtrVec::iterator tKeyFramePtrVecIt;

	typedef std::vector<cKeyFrame> tKeyFrameVec;
	typedef tKeyFrameVec::iterator tKeyFrameVecIt;

	//---------------------------------------

	enum eAnimationEventType
	{
		eAnimationEventType_PlaySound,
		eAnimationEventType_LastEnum
	};

	//---------------------------------------

	class cVertexBonePair
	{
	public:
		cVertexBonePair(unsigned int aVtx, unsigned int aBone, float aW)
		{
			vtxIdx = aVtx;
			boneIdx = aBone;
			weight = aW;
		}
		cVertexBonePair(){}


		unsigned int vtxIdx;
		unsigned int boneIdx;
		float weight;
	};


	typedef std::vector<cVertexBonePair> tVertexBonePairVec;
	typedef tVertexBonePairVec::iterator tVertexBonePairVecIt;

	//---------------------------------------

	class cVertex
	{
	public:
		cVertex():pos(0),tex(0),col(0){}
		cVertex(const cVector3f &avPos,const cVector3f &avTex,const cColor &aCol )
		{
			pos = avPos;
			tex = avTex;
			col = aCol;
		}

		cVertex(const cVector3f &avPos,const cColor &aCol )
		{
			pos = avPos;
			col = aCol;
		}

		cVector3f pos;
		cVector3f tex;
		cVector3f tan;
		cVector3f norm;
		cColor col;
	};

	/**
	 * What a light looks like beyond its colour, reach and pictures
	 * (Milestone 5). The defaults are the original: a light of the game's
	 * data, with its falloff ramp and, for a spotlight, its picture.
	 */
	class cLightLook
	{
	public:
		/// Scales the amount of light. The light colour itself stops at 1.
		float mfBrightness = 1.0f;
		/// Multiplies the light's colour.
		cColor mTint = cColor(1,1,1,1);

		/**
		 * Falloff by the distance (one over its square) in place of the
		 * light's ramp, fading out at the light's reach.
		 */
		bool mbDistanceFalloff = false;
		/// The distance, as a share of the reach, at which the light is as bright as its colour says.
		float mfEvenAt = 0.4f;
		/// How many times brighter than that it gets at most, nearer.
		float mfNearBoost = 2.5f;

		/**
		 * Spotlights: a beam made by the shader (a hotspot in a dimmer
		 * field with a soft edge) in place of the light's picture.
		 */
		bool mbBeam = false;
		/// The hotspot's size, as a share of the cone.
		float mfBeamHotspot = 0.3f;
		/// How bright the field around the hotspot is: 0 none, 1 as bright as the hotspot.
		float mfBeamField = 0.5f;
		/// The beam's size, as a share of the cone.
		float mfBeamWidth = 1.0f;

		/**
		 * Moves the place the light shines from, along the world's axes:
		 * a flame that sways. The light itself stays where it is (what it
		 * reaches, a spotlight's cone).
		 */
		cVector3f mvOffset = cVector3f(0,0,0);

		/**
		 * A light that is long, not a point: a fluorescent tube. From the
		 * tube's middle to one of its ends, along the world's axes; zero
		 * for a point. Only the highlights show it.
		 */
		cVector3f mvTubeAxis = cVector3f(0,0,0);
		/// Where the tube's middle is, from the light, along the world's axes.
		cVector3f mvTubeOffset = cVector3f(0,0,0);

		bool HasTube() const { return mvTubeAxis.x != 0 || mvTubeAxis.y != 0 || mvTubeAxis.z != 0; }
	};

	typedef std::list<cVertex> tVertexList;
	typedef tVertexList::iterator tVertexListIt;

	typedef std::vector<cVertex> tVertexVec;
	typedef tVertexVec::iterator tVertexVecIt;
}

#endif // HPL_GRAPHICSTYPES_H
