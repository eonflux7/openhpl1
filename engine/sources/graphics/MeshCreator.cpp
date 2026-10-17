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
#include "graphics/MeshCreator.h"
#include "system/String.h"
#include "system/LowLevelSystem.h"
#include "graphics/LowLevelGraphics.h"
#include "graphics/VertexBuffer.h"
#include "resources/Resources.h"
#include "graphics/Mesh.h"
#include "graphics/SubMesh.h"
#include "resources/MaterialManager.h"
#include "resources/AnimationManager.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cMeshCreator::cMeshCreator(iLowLevelGraphics *apLowLevelGraphics, cResources *apResources)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		mpResources = apResources;
	}

	//-----------------------------------------------------------------------

	cMeshCreator::~cMeshCreator()
	{
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cMesh* cMeshCreator::CreateBox(const tString &asName,cVector3f avSize, const tString &asMaterial)
	{
		cMesh *pMesh = hplNew( cMesh, (asName, mpResources->GetMaterialManager(),
										mpResources->GetAnimationManager()) );

		cSubMesh *pSubMesh = pMesh->CreateSubMesh("Main");

		iMaterial *pMat = mpResources->GetMaterialManager()->CreateMaterial(asMaterial);
		pSubMesh->SetMaterial(pMat);
		iVertexBuffer *pVtxBuff = CreateBoxVertexBuffer(avSize);
		pSubMesh->SetVertexBuffer(pVtxBuff);

		return pMesh;
	}

	//-----------------------------------------------------------------------

	iVertexBuffer* cMeshCreator::CreateSkyBoxVertexBuffer(float afSize)
	{
		iVertexBuffer* pSkyBox = mpLowLevelGraphics->CreateVertexBuffer(
										eVertexFlag_Color0 | eVertexFlag_Position | eVertexFlag_TexCoord,
										eVertexBufferDrawType_Tri,eVertexBufferUsageType_Static);

		float fSize = afSize;

		for(int x=-1; x<=1;x++)
			for(int y=-1; y<=1;y++)
				for(int z=-1; z<=1;z++)
				{
					if(x==0 && y==0 && z==0)continue;
					if(std::abs(x) + std::abs(y) + std::abs(z) > 1)continue;

					//Direction (could say inverse normal) of the quad.
					cVector3f vDir;
					cVector3f vSide;

					cVector3f vAdd[4];
					if(std::abs(x)){
						vDir.x = (float)x;

						vAdd[0].y = 1;vAdd[0].z = 1;
						vAdd[1].y = -1;vAdd[1].z = 1;
						vAdd[2].y = -1;vAdd[2].z = -1;
						vAdd[3].y = 1;vAdd[3].z = -1;
					}
					else if(std::abs(y)){
						vDir.y = (float)y;

						vAdd[0].z = 1;vAdd[0].x = 1;
						vAdd[1].z = -1;vAdd[1].x = 1;
						vAdd[2].z = -1;vAdd[2].x = -1;
						vAdd[3].z = 1;vAdd[3].x = -1;
					}
					else if(std::abs(z)){
						vAdd[0].y = 1;vAdd[0].x = 1;
						vAdd[1].y = 1;vAdd[1].x = -1;
						vAdd[2].y = -1;vAdd[2].x = -1;
						vAdd[3].y = -1;vAdd[3].x = 1;

						vDir.z = (float)z;
					}


					for(int i=0;i<4;i++)
					{
						int idx = i;
						if(x + y + z < 0) idx = 3-i;

						pSkyBox->AddColor(eVertexFlag_Color0, cColor(1,1,1,1));
						pSkyBox->AddVertex(eVertexFlag_Position, (vDir+vAdd[idx])*fSize);
						pSkyBox->AddVertex(eVertexFlag_TexCoord, vDir+vAdd[idx]);

						vSide = vDir+vAdd[idx];
					}
				}

		for(int face=0; face<6; ++face)
			for(int corner : {0,1,2,0,2,3}) pSkyBox->AddIndex(face*4 + corner);

		if(!pSkyBox->Compile(0))
		{
			hplDelete(pSkyBox);
			return nullptr;
		}
		return pSkyBox;
	}

	//-----------------------------------------------------------------------

	iVertexBuffer* cMeshCreator::CreateBoxVertexBuffer(cVector3f avSize)
	{
		iVertexBuffer* pBox = mpLowLevelGraphics->CreateVertexBuffer(
			eVertexFlag_Color0 | eVertexFlag_Position | eVertexFlag_TexCoord |
			eVertexFlag_Tangent | eVertexFlag_Normal,
			eVertexBufferDrawType_Tri,eVertexBufferUsageType_Static);

		avSize = avSize*0.5;

		int lVtxIdx =0;

		for(int x=-1; x<=1;x++)
			for(int y=-1; y<=1;y++)
				for(int z=-1; z<=1;z++)
				{
					if(x==0 && y==0 && z==0)continue;
					if(std::abs(x) + std::abs(y) + std::abs(z) > 1)continue;

					//Direction (could say inverse normal) of the quad.
					cVector3f vDir;
					cVector3f vSide;

					cVector3f vAdd[4];
					if(std::abs(x)){
						vDir.x = (float)x;

						vAdd[0].y = 1;vAdd[0].z = 1;
						vAdd[1].y = -1;vAdd[1].z = 1;
						vAdd[2].y = -1;vAdd[2].z = -1;
						vAdd[3].y = 1;vAdd[3].z = -1;
					}
					else if(std::abs(y)){
						vDir.y = (float)y;

						vAdd[0].z = 1;vAdd[0].x = 1;
						vAdd[1].z = -1;vAdd[1].x = 1;
						vAdd[2].z = -1;vAdd[2].x = -1;
						vAdd[3].z = 1;vAdd[3].x = -1;
					}
					else if(std::abs(z)){
						vAdd[0].y = 1;vAdd[0].x = 1;
						vAdd[1].y = 1;vAdd[1].x = -1;
						vAdd[2].y = -1;vAdd[2].x = -1;
						vAdd[3].y = -1;vAdd[3].x = 1;

						vDir.z = (float)z;
					}


					for(int i=0;i<4;i++)
					{
						int idx = GetBoxIdx(i,x,y,z);
						cVector3f vTex = GetBoxTex(i,x,y,z,vAdd);

						pBox->AddColor(eVertexFlag_Color0, cColor(1,1,1,1));
						pBox->AddVertex(eVertexFlag_Position, (vDir+vAdd[idx])*avSize);
						pBox->AddVertex(eVertexFlag_Normal, vDir);

						//texture coord
						cVector3f vCoord = cVector3f((vTex.x+1)*0.5f,(vTex.y+1)*0.5f,0);
						pBox->AddVertex(eVertexFlag_TexCoord,vCoord);

						vSide = vDir+vAdd[idx];
					}

					for(int i=0;i<3;i++)pBox->AddIndex(lVtxIdx + i);
					pBox->AddIndex(lVtxIdx + 2);
					pBox->AddIndex(lVtxIdx + 3);
					pBox->AddIndex(lVtxIdx + 0);

					lVtxIdx +=4;

				}

		if(!pBox->Compile(eVertexCompileFlag_CreateTangents))
		{
			hplDelete(pBox);
			return nullptr;
		}
		return pBox;
	}

	cVector3f cMeshCreator::GetBoxTex(int i,int x, int y, int z, cVector3f *vAdd)
	{
		cVector3f vTex;

		if(std::abs(x)){
			vTex.x = vAdd[i].z;
			vTex.y = vAdd[i].y;
		}
		else if(std::abs(y)){
			vTex.x = vAdd[i].x;
			vTex.y = vAdd[i].z;
		}
		else if(std::abs(z)){
			vTex.x = vAdd[i].x;
			vTex.y = vAdd[i].y;
		}

		//Inverse for negative directions
		if(x+y+z <0)
		{
			vTex.x = -vTex.x;
			vTex.y = -vTex.y;
		}

		return vTex;
	}
	int cMeshCreator::GetBoxIdx(int i,int x, int y, int z)
	{
		int idx = i;
		if(x + y + z > 0) idx = 3-i;

		return idx;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------


	//-----------------------------------------------------------------------

}
