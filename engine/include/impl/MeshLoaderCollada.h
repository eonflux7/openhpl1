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
#ifndef HPL_MESH_LOADER_COLLADA_H
#define HPL_MESH_LOADER_COLLADA_H


#include "math/MathTypes.h"
#include "math/Quaternion.h"
#include "resources/MeshLoader.h"
#include "graphics/VertexBuffer.h"
#include "physics/PhysicsJoint.h"
#include "system/MemoryManager.h"

class TiXmlElement;

namespace hpl {

	class cMesh;
	class cNode3D;
	class iVertexBuffer;
	class cBone;
	class cAnimation;
	class cAnimationTrack;
	class cSkeleton;
	class cWorld3D;
	class cSector;
	class cMeshEntity;
	class iPhysicsBody;
	class cColliderEntity;
	class cMeshJoint;
	class cSystem;

	// HELP CLASSES

	class cColladaImage
	{
	public:
		tString msId;
		tString msName;

		tString msSource;
	};

	typedef std::vector<cColladaImage> tColladaImageVec;

	class cColladaTexture
	{
	public:
		tString msId;
		tString msName;

		tString msImage;
	};

	typedef std::vector<cColladaTexture> tColladaTextureVec;

	class cColladaMaterial
	{
	public:
		tString msId;
		tString msName;

		tString msTexture;
		cColor mDiffuseColor;
	};

	typedef std::vector<cColladaMaterial> tColladaMaterialVec;

	class cColladaLight
	{
	public:
		tString msId;
		tString msName;
		tString msType;

		cColor mDiffuseColor;
		float mfAngle;
	};

	typedef std::vector<cColladaLight> tColladaLightVec;

	class cColladaVtxArray
	{
	public:
		cColladaVtxArray() : mbIsInVertex(false){}

		tString msId;
		tString msType;
		bool mbIsInVertex;

		tVector3fVec mvArray;
	};

	typedef std::vector<cColladaVtxArray> tColladaVtxArrayVec;

	class cColladaVtxIndex
	{
	public:
		int mlVtx;
		int mlNorm;
		int mlTex;
	};

	typedef std::vector<cColladaVtxIndex> tColladaVtxIndexVec;

	class cColladaExtraVtx
	{
	public:
		cColladaExtraVtx(int alVtx,int alNorm,int alTex,int alNewVtx)
		{
			mlVtx = alVtx;
			mlNorm = alNorm;
			mlTex = alTex;
			mlNewVtx = alNewVtx;
		}

		cColladaExtraVtx(){}

		int mlVtx;
		int mlNorm;
		int mlTex;

		int mlNewVtx;

		bool Equals(const cColladaVtxIndex& aData)
		{
			if(mlVtx == aData.mlVtx && mlNorm == aData.mlNorm && mlTex == aData.mlTex)
			{
				return true;
			}

			return false;
		}
	};

	typedef std::list<cColladaExtraVtx> tColladaExtraVtxList;
	typedef tColladaExtraVtxList::iterator tColladaExtraVtxListIt;

	typedef std::vector<tColladaExtraVtxList> tColladaExtraVtxListVec;

	class cColladaGeometry
	{
	public:
		cColladaGeometry() : mlPosArrayIdx(-1),mlNormArrayIdx(-1),mlTexArrayIdx(-1),
						mlPosIdxNum(-1),mlNormIdxNum(-1),mlTexIdxNum(-1)  {}

		void Clear()
		{
			mvIndices.clear();

			for(int i=0; i<(int)mvArrayVec.size();i++)
			{
				mvArrayVec[i].mvArray.clear();
			}
			/*mvVertexVec.clear();
			mvIndexVec.clear();
			mvExtraVtxVec.clear();*/
		}

		tString msId;
		tString msName;

		tVertexVec mvVertexVec;
		tUIntVec mvIndexVec;
		tColladaExtraVtxListVec mvExtraVtxVec;
		tFloatVec mvTangents;

		tString msMaterial;


		//THe following are ONLY used when loading from a geometry element.!!
		tColladaVtxArrayVec mvArrayVec;
		tColladaVtxIndexVec mvIndices;

		int mlPosIdxNum; //The position in the triangle element
		int mlNormIdxNum; //for eternal use only
		int mlTexIdxNum;

		int mlPosArrayIdx; //The index for array containing positions
		int mlNormArrayIdx; //The index for array containing normals
		int mlTexArrayIdx; //The index for array containing texcoords
	};

	typedef std::vector<cColladaGeometry> tColladaGeometryVec;

	class cColladaJointPair
	{
	public:
		cColladaJointPair(){}
		cColladaJointPair(int alJoint, int alWeight)
		{
			mlJoint = alJoint;
			mlWeight = alWeight;
		}

		int mlJoint;
		int mlWeight;
	};

	typedef std::list<cColladaJointPair> tColladaJointPairList;
	typedef tColladaJointPairList::iterator tColladaJointPairListIt;

	typedef std::vector<tColladaJointPairList> tColladaJointPairListVec;

	class cColladaController
	{
	public:
		cColladaController() : mlJointPairIdx(-1), mlWeightPairIdx(-1){}

		tString msTarget;
		tString msId;

		cMatrixf m_mtxBindShapeMatrix;

		int mlJointPairIdx;
		int mlWeightPairIdx;

		tStringVec mvJoints;
		tFloatVec mvWeights;
		tMatrixfVec mvMatrices;

		tColladaJointPairListVec mvPairs;
	};

	typedef std::vector<cColladaController> tColladaControllerVec;

	class cColladaChannel
	{
	public:
		tString msId;
		tString msTarget;
		tString msSource;
	};

	typedef std::vector<cColladaChannel> tColladaChannelVec;

	class cColladaSampler
	{
	public:
		tString msId;
		tString msTimeArray;
		tString msValueArray;

		tString msTarget;
	};

	typedef std::vector<cColladaSampler> tColladaSamplerVec;

	class cColladaAnimSource
	{
	public:
		tString msId;
		tFloatVec mvValues;
	};

	typedef std::vector<cColladaAnimSource> tColladaAnimSourceVec;


	// A key of an animation read from glTF: the node's local translation and
	// rotation, mfTime seconds from the start of the animation.
	class cColladaBakedKey
	{
	public:
		cColladaBakedKey() : mbExact(false) {}

		float mfTime;
		cVector3f mvTrans;
		cQuaternion mqRot;

		//The engine's key, from an unedited converted file
		bool mbExact;
		cVector3f mvExactTrans;
		cQuaternion mqExactRot;
	};

	typedef std::vector<cColladaBakedKey> tColladaBakedKeyVec;

	class cColladaAnimation
	{
	public:
		tString msId;
		//tString msName;
		tString msTargetNode;

		tColladaChannelVec mvChannels;
		tColladaSamplerVec mvSamplers;

		tColladaAnimSourceVec mvSources;

		//glTF animations have keys instead of channels, samplers and sources.
		tColladaBakedKeyVec mvBakedKeys;

		tFloatVec* GetSourceVec(const tString& asId)
		{
			for(size_t i=0; i< mvSources.size(); i++)
			{
				if(mvSources[i].msId == asId)
				{
					return &mvSources[i].mvValues;
				}
			}

			return nullptr;
		}
	};

	typedef std::vector<cColladaAnimation> tColladaAnimationVec;

	class cColladaTransform
	{
	public:
		tString msSid;
		tString msType;
		tFloatVec mvValues;
	};

	typedef std::list<cColladaTransform> tColladaTransformList;
	typedef tColladaTransformList::iterator tColladaTransformListIt;


	class cColladaNode;
	typedef std::list<cColladaNode*> tColladaNodeList;
	typedef tColladaNodeList::iterator tColladaNodeListIt;

	class cColladaNode
	{
	public:
		cColladaNode() : mlCount(0), pParent(nullptr), mvScale(1,1,1), mbSourceIsFile(false) {}

		tString msId;
		tString msName;
		tString msType;

		tString msSource;
		bool mbSourceIsFile;

		cMatrixf m_mtxTransform;
		cMatrixf m_mtxWorldTransform;

		cColladaNode *pParent;

		cVector3f mvScale;

		int mlCount;

		tColladaNodeList mlstChildren;

		tColladaTransformList mlstTransforms;

		/*void DeleteChildren()
		{
			STLDeleteAll(mlstChildren);
		}*/

		cColladaNode* CreateChild()
		{
			cColladaNode *pNode = hplNew( cColladaNode, () );
			mlstChildren.push_back(pNode);
			pNode->pParent = this;
			return pNode;
		}

		cColladaTransform* GetTransform(const tString &asSid)
		{
			tColladaTransformListIt it = mlstTransforms.begin();
			for(; it != mlstTransforms.end(); it++)
			{
				 cColladaTransform &Trans = *it;
				 if(Trans.msSid == asSid)
				 {
					 return &Trans;
				 }
			}
			return nullptr;
		}
	};

	class cColladaScene
	{
	public:
		cColladaScene()
		{
			mRoot.m_mtxTransform = cMatrixf::Identity;
			mRoot.m_mtxWorldTransform = cMatrixf::Identity;

			mfDeltaTime =0;
		}

		~cColladaScene()
		{
			//mRoot.DeleteChildren();
			STLDeleteAll(mlstNodes);
		}

		void ResetNodes()
		{
			STLDeleteAll(mlstNodes);
			mRoot.mlstChildren.clear();
		}

		cColladaNode* GetNode(const tString asId)
		{
			tColladaNodeListIt it = mlstNodes.begin();
			for(; it != mlstNodes.end();++it)
			{
				cColladaNode *pNode = *it;
				if(pNode->msId == asId || pNode->msName == asId)
				{
					return pNode;
				}
			}

			return nullptr;
		}

		cColladaNode* GetNodeFromSource(const tString asSource)
		{
			tColladaNodeListIt it = mlstNodes.begin();
			for(; it != mlstNodes.end();++it)
			{
				cColladaNode *pNode = *it;
				if(pNode->msSource == asSource)
				{
					return pNode;
				}
			}

			return nullptr;
		}

		float mfStartTime;
		float mfEndTime;
		float mfDeltaTime;

		cColladaNode mRoot;

		tColladaNodeList mlstNodes;
	};

	// Everything the loader reads from a model file before it creates meshes,
	// worlds and animations (the contents of a Collada cache).
	class cColladaData
	{
	public:
		tColladaImageVec mvImages;
		tColladaTextureVec mvTextures;
		tColladaMaterialVec mvMaterials;
		tColladaLightVec mvLights;
		tColladaGeometryVec mvGeometries;
		tColladaControllerVec mvControllers;
		tColladaAnimationVec mvAnimations;
		cColladaScene mScene;
	};

	// Loads Collada (.dae) and glTF 2.0 (.glb, .gltf) models. Both are read
	// into the same intermediate data, which the meshes, worlds and
	// animations are created from, so the naming conventions of the original
	// Collada files (colliders, joints, areas, lights, references...) apply
	// to glTF files as well. See docs/modding.md.
	class cMeshLoaderCollada : public iMeshLoader
	{
	public:
		cMeshLoaderCollada(iLowLevelGraphics *apLowLevelGraphics);
		~cMeshLoaderCollada() override;

		// Model conversion (tools/model_tool.cpp). These do not need graphics
		// and neither read nor write caches.

		// Reads a Collada or glTF file.
		bool LoadData(const tString &asFile, cColladaData &aData);
		// Writes data read from a Collada file as glTF (.glb or .gltf).
		// Problems that may make the glTF file differ are added to apWarnings.
		bool SaveGltf(cColladaData &aData, const tString &asFile, const tString &asSourceName,
			tStringVec *apWarnings);
		// The animation tracks the loader creates for the data, or nullptr if
		// there are none; with abRestPoses also the tracks of the bones that
		// are not animated, like a loaded mesh. Also returns the skeleton, if
		// any. The caller deletes both.
		cAnimation *CreateAnimation(cColladaData &aData, bool abRestPoses, cSkeleton **apSkeleton);
		// The material file name of a geometry ("" if none).
		tString GetGeometryMaterialFile(cColladaData &aData, const cColladaGeometry &aGeometry);

		// Collada: whether the scale of a node is kept out of its transform
		// (lights, colliders and areas keep their size in the scale).
		static bool ScaleIsSeparate(const cColladaNode *apNode, const tColladaLightVec *apLights);
		// Sets mvTangents from the vertices and triangles.
		static void CreateTangents(cColladaGeometry &aGeometry);

		// glTF: the local transform of a node from its translation, rotation
		// (x,y,z,w) and scale, as the reader makes it.
		static cMatrixf GltfTransform(const float *apTrans, const float *apRotation, const float *apScale,
			bool abScaleIsSeparate);
		// glTF: a light's colour and spot angle (degrees), as the reader makes them.
		static cColor GltfLightColor(const float *apColor, float afIntensity);
		static float GltfSpotAngle(float afOuterConeAngle);
		// glTF: a hash of an animation track's times, translations and
		// rotations, so that a reader knows they have not been edited.
		static tString GltfKeyHash(const tFloatVec &avTimes, const tFloatVec &avTrans, const tFloatVec &avRot);

		cMesh* LoadMesh(const tString& asFile, tMeshLoadFlag aFlags) override;
		bool SaveMesh(cMesh* apMesh,const tString& asFile) override{return false;}

		cWorld3D* LoadWorld(const tString& asFile, cScene* apScene,tWorldLoadFlag aFlags) override;

		cAnimation* LoadAnimation(const tString& asFile) override;


		bool IsSupported(const tString asFileType) override;

		void AddSupportedTypes(tStringVec* avFileTypes) override;

	private:
		bool mbZToY;

		tWorldLoadFlag mFlags;

		tString GetParentName(cColladaNode *apNode, tColladaGeometryVec *apColladaGeometries);

		void CreateMeshJoint(cMeshJoint* apJoint,ePhysicsJointType aJointType,cBoundingVolume &aBV,
							tStringVec &avStrings,cColladaNode* apNode,cColladaScene &aColladaScene,
							tColladaGeometryVec &avColladaGeom);

		void CreateHierarchyNodes(cMesh *apMesh, cNode3D* mpParentNode,
			cColladaNode* apColladaNode,
			tColladaGeometryVec &avColladaGeom);

		cColladaGeometry* GetGeometry(const tString& asId, tColladaGeometryVec &avGeomVec);
		cColladaLight* GetLight(const tString& asId, tColladaLightVec &avLightVec);

		cMeshEntity* CreateStaticMeshEntity(cColladaNode* apNode, cWorld3D *apWorld,
											cColladaGeometry *apGeom,bool abInRoomGroup,
											tColladaMaterialVec &avColladaMaterials,
											tColladaTextureVec &avColladaTextures,
											tColladaImageVec &avColladaImages);

		cColliderEntity* CreateStaticCollider(cColladaNode* apNode, cWorld3D *apWorld,
											cColladaGeometry *apGeom,
											tColladaMaterialVec &avColladaMaterials,
											tColladaTextureVec &avColladaTextures,
											tColladaImageVec &avColladaImages,
											bool abCharacterCollider);


		void AddSceneObjects(cColladaNode* apNode, cWorld3D *apWorld,
								tColladaGeometryVec &avColladaGeometries,tColladaLightVec &avColladaLights,
								tColladaMaterialVec &avColladaMaterials, tColladaTextureVec &avColladaTextures,
								tColladaImageVec &avColladaImages,cColladaScene *apColladaScene);

		void AddSectorChildren(cColladaNode* apNode, tString asSector, cWorld3D *apWorld,
								tColladaGeometryVec &avColladaGeometries,tColladaLightVec &avColladaLights,
								tColladaMaterialVec &avColladaMaterials,tColladaTextureVec &avColladaTextures,
								tColladaImageVec &avColladaImages);



		cAnimationTrack* CreateAnimTrack(cAnimation *apAnimation , cSkeleton *apSkeleton,
						cColladaAnimation &aAnim, cColladaScene *apScene);
		cAnimationTrack *CreateBakedAnimTrack(cAnimation *apAnimation, cSkeleton *apSkeleton,
			cColladaAnimation &aAnim);

		cSkeleton *CreateSkeleton(tColladaControllerVec &avControllers, cColladaScene &aScene);
		void AddRestPoseTracks(cAnimation *apAnimation, cSkeleton *apSkeleton, cColladaScene &aScene);

		void CalcLocalMatrixRec(cBone* apBone, cMatrixf a_mtxParentGlobal, int alDepth);

		void CreateSkeletonBone(cColladaNode* apColladaNode, cBone* apParentBone);

		iVertexBuffer *CreateVertexBuffer(cColladaGeometry & aGeometry,
											eVertexBufferUsageType aUsageType);
											//tColladaExtraVtxListVec &vExtraVtxVec);

		bool FillStructures(const tString &asFile,
							tColladaImageVec *apColladaImageVec,
							tColladaTextureVec *apColladaTextureVec,
							tColladaMaterialVec *apColladaMaterialVec,
							tColladaLightVec *apColladaLightVec,
							tColladaGeometryVec *apColladaGeometryVec,
							tColladaControllerVec *apColladaControllerVec,
							tColladaAnimationVec *apColladaAnimVec,
							cColladaScene *apColladaScene,
							bool abCache);

		bool FillStructuresFromGltf(const tString &asFile,
			tColladaImageVec *apColladaImageVec,
			tColladaTextureVec *apColladaTextureVec,
			tColladaMaterialVec *apColladaMaterialVec,
			tColladaLightVec *apColladaLightVec,
			tColladaGeometryVec *apColladaGeometryVec,
			tColladaControllerVec *apColladaControllerVec,
			tColladaAnimationVec *apColladaAnimVec,
			cColladaScene *apColladaScene);

		bool SaveStructures(const tString &asFile,
							tColladaImageVec *apColladaImageVec,
							tColladaTextureVec *apColladaTextureVec,
							tColladaMaterialVec *apColladaMaterialVec,
							tColladaLightVec *apColladaLightVec,
							tColladaGeometryVec *apColladaGeometryVec,
							tColladaControllerVec *apColladaControllerVec,
							tColladaAnimationVec *apColladaAnimVec,
							cColladaScene *apColladaScene);

		bool LoadStructures(const tString &asFile,
							tColladaImageVec *apColladaImageVec,
							tColladaTextureVec *apColladaTextureVec,
							tColladaMaterialVec *apColladaMaterialVec,
							tColladaLightVec *apColladaLightVec,
							tColladaGeometryVec *apColladaGeometryVec,
							tColladaControllerVec *apColladaControllerVec,
							tColladaAnimationVec *apColladaAnimVec,
							cColladaScene *apColladaScene);

		void LoadColladaScene(TiXmlElement* apRootElem,cColladaNode *apParentNode, cColladaScene *apScene,
								tColladaLightVec *apColladaLightVec);

		void LoadAnimations(TiXmlElement* apRootElem, tColladaAnimationVec &avAnimations,
								cColladaScene *apColladaScene);

		void LoadImages(TiXmlElement* apRootElem, tColladaImageVec &avColladaImageVec);

		void LoadTextures(TiXmlElement* apRootElem, tColladaTextureVec &avColladaTextureVec);

		void LoadMaterials(TiXmlElement* apRootElem, tColladaMaterialVec &avColladaMaterialVec);

		void LoadLights(TiXmlElement* apRootElem, tColladaLightVec &avColladaLightVec);

		void LoadGeometry(TiXmlElement* apRootElem, tColladaGeometryVec &avColladaGeometryVec, cColladaScene *apColladaScene);
		void LoadVertexData(TiXmlElement* apSourceElem, tVector3fVec &avVtxVec);

		void LoadControllers(TiXmlElement* apRootElem, tColladaControllerVec &avColladaControllerVec,
							tColladaGeometryVec *apColladaGeometryVec);
		void LoadJointData(TiXmlElement* apSourceElem, cColladaController &aController);

		//Helpers
		void SplitVertices(cColladaGeometry &aGeometry,tColladaExtraVtxListVec &avExtraVtxVec,
							tVertexVec &avVertexVec, tUIntVec &avIndexVec);


		void FillVertexVec(const char* apChars,tVector3fVec &avVtxVec, int alElements, int alVtxCount);
		tString GetTopString(const tString asPath);
		tString GetMaterialTextureFile(const tString &asMaterial, tColladaMaterialVec &avColladaMaterialVec,
																tColladaTextureVec &avColladaTextureVec,
																tColladaImageVec &avColladaImageVec);

		cVector3f GetVectorPos(const cVector3f &avVec);
		cVector3f GetVectorPosFromPtr(float *apVec);
		cVector3f GetVectorScaleFromPtr(float *apVec);
	};

};
#endif // HPL_MESH_LOADER_COLLADA_H
