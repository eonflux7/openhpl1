// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SUB_MESH_H
#define HPL_SUB_MESH_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class iMaterial;
	class iVertexBuffer;

	class cMesh;

	class cMaterialManager;

	class cSubMesh
	{
	friend class cMesh;
	friend class cSubMeshEntity;
	public:
		cSubMesh(const tString &asName,cMaterialManager* apMaterialManager);
		~cSubMesh();

		void SetMaterial(iMaterial* apMaterial);
		void SetVertexBuffer(iVertexBuffer* apVtxBuffer);

		//Renderable implementation.
		iMaterial *GetMaterial();
		iVertexBuffer* GetVertexBuffer();

		tString GetName(){ return msName;}

		//Vertex-Bone pairs
		void ResizeVertexBonePairs(int alSize);
		int GetVertexBonePairNum();
		cVertexBonePair& GetVertexBonePair(int alNum);

		void AddVertexBonePair(const cVertexBonePair &aPair);
		void ClearVertexBonePairs();

		void CompileBonePairs();

		const tString& GetGroup(){ return msGroup;}
		void SetGroup(const tString& asGroup){ msGroup =asGroup;}

		const tString& GetNodeName(){ return msNodeName;}
		void SetNodeName(const tString& asNodeName){ msNodeName = asNodeName;}

		void SetModelScale(const cVector3f& avScale){ mvModelScale = avScale;}
		cVector3f GetModelScale(){ return mvModelScale;}

		const cMatrixf& GetLocalTransform(){ return m_mtxLocalTransform;}
		void SetLocalTransform(const cMatrixf& a_mtxTrans){ m_mtxLocalTransform = a_mtxTrans;}

		bool GetIsOneSided(){ return mbIsOneSided;}
		const cVector3f& GetOneSidedNormal(){ return mvOneSidedNormal;}

		void Compile();
	private:
		void CheckOneSided();


		iMaterial* mpMaterial;
		iVertexBuffer* mpVtxBuffer;

		cMatrixf m_mtxLocalTransform;

		tVertexBonePairVec mvVtxBonePairs;

		tString msGroup;
		tString msNodeName;

		float *mpVertexWeights;
		unsigned char *mpVertexBones;

		cVector3f mvModelScale;

		tString msName;

		bool mbIsOneSided;
		cVector3f mvOneSidedNormal;

		cMaterialManager* mpMaterialManager;

		cMesh* mpParent;
	};

};
#endif // HPL_SUB_MESH_H
