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
#include "ai/AStar.h"

#include "ai/AINodeContainer.h"

#include "math/Math.h"

#include "system/LowLevelSystem.h"
#include <cmath>
#include <limits>


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// NODE
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cAStarNode::cAStarNode(cAINode *apAINode)
	{
		mfCost = mfDistance = std::numeric_limits<float>::infinity();
		mpParent = nullptr;
		mpAINode = apAINode;
	}

	//-----------------------------------------------------------------------

	bool cAStarNodeCompare::operator()(cAStarNode* apNodeA,cAStarNode* apNodeB) const
	{
		if(apNodeA->mfCost != apNodeB->mfCost) return apNodeA->mfCost < apNodeB->mfCost;
		return std::less<cAINode*>()(apNodeA->mpAINode, apNodeB->mpAINode);
	}


	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cAStarHandler::cAStarHandler(cAINodeContainer *apContainer)
	{
		mlMaxIterations = -1;

		mpContainer = apContainer;

		mpCallback = nullptr;
		mpGoalNode = nullptr;
	}

	//-----------------------------------------------------------------------

	cAStarHandler::~cAStarHandler() = default;

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cAStarHandler::GetPath(const cVector3f& avStart, const cVector3f& avGoal,tAINodeList *apNodeList)
	{
		m_setOpenList.clear();
		m_mapNodes.clear();
		m_setGoalNodes.clear();
		mpGoalNode = nullptr;

		if(mpContainer->FreePath(avStart,avGoal,3)) return true;

		//Set goal position
		mvGoal = avGoal;

		float fMaxHeight = mpContainer->GetMaxHeight()*1.5f;

		////////////////////////////////////////////////
		//Find nodes reachable from the start and goal position (use double 2*2 distance)
		float fMaxDist = mpContainer->GetMaxEdgeDistance()*2; //float fMaxDist = mpContainer->GetMaxEdgeDistance()*mpContainer->GetMaxEdgeDistance()*4;

		/////////////////////
		//Check with Start
		//Log(" Get Start\n");
		cAINodeIterator startNodeIt =  mpContainer->GetNodeIterator(avStart,fMaxDist);
		while(startNodeIt.HasNext())
		{
			cAINode *pAINode = startNodeIt.Next();
			//Log("Check node: %s\n",pAINode->GetName().c_str());

			float fHeight = fabs(avStart.y - pAINode->GetPosition().y);
			float fDist = cMath::Vector3Dist(avStart,pAINode->GetPosition()); //float fDist = cMath::Vector3DistSqr(avStart,pAINode->GetPosition());
			if(fDist < fMaxDist && fHeight <= fMaxHeight)
			{
				//Check if path is clear
				if(mpContainer->FreePath(avStart,pAINode->GetPosition(),-1,
										eAIFreePathFlag_SkipDynamic))
				{
					AddOpenNode(pAINode,nullptr,fDist);
				}
			}
		}
		//Log(" Found start\n");

		////////////////////////////////
		//Check with Goal
		//Log(" Get Goal\n");
		cAINodeIterator goalNodeIt =  mpContainer->GetNodeIterator(avGoal,fMaxDist);
		while(goalNodeIt.HasNext())
		{
			cAINode *pAINode = goalNodeIt.Next();
			//Log("Check node: %s\n",pAINode->GetName().c_str());

			float fHeight = fabs(avGoal.y - pAINode->GetPosition().y);
			float fDist = cMath::Vector3Dist(avGoal,pAINode->GetPosition()); //fDist = cMath::Vector3DistSqr(avGoal,pAINode->GetPosition());
			if(fDist < fMaxDist && fHeight <= fMaxHeight)
			{
				//Check if path is clear
				if(mpContainer->FreePath(avGoal,pAINode->GetPosition(),3))
				{
					m_setGoalNodes.insert(pAINode);
				}
			}
		}
		//Log(" Found goal\n");


		////////////////////////////////////////////////
		//Iterate the algorithm
		IterateAlgorithm();

		////////////////////////////////////////////////
		//Check if goal was found, if so build path.
		if(mpGoalNode)
		{
			if(apNodeList)
			{
				cAStarNode *pParentNode = mpGoalNode;
				while(pParentNode != nullptr)
				{
					apNodeList->push_back(pParentNode->mpAINode);
					pParentNode = pParentNode->mpParent;
				}
			}

			return true;
		}
		else
		{
			 return false;
		}
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cAStarHandler::IterateAlgorithm()
	{
		int lIterationCount=0;
		while(m_setOpenList.empty()==false && (mlMaxIterations <0 || lIterationCount < mlMaxIterations))
		{
			cAStarNode *pNode = GetBestNode();
			cAINode *pAINode = pNode->mpAINode;

			//////////////////////
			// Check if current node can reach goal
			if(IsGoalNode(pAINode))
			{
				mpGoalNode = pNode;
				break;
			}

			/////////////////////
			//Add nodes connected to current
			int lEdgeCount = pAINode->GetEdgeNum();
			for(int i=0; i< lEdgeCount; ++i)
			{
				cAINodeEdge *pEdge = pAINode->GetEdge(i);

				if(mpCallback == nullptr || mpCallback->CanAddNode(pAINode, pEdge->mpNode))
				{
					AddOpenNode(pEdge->mpNode, pNode, pEdge->mfDistance);
				}
			}

			++lIterationCount;
		}
	}


	//-----------------------------------------------------------------------

	void cAStarHandler::AddOpenNode(cAINode *apAINode, cAStarNode *apParent, float afDistance)
	{
		if(!apAINode || !std::isfinite(afDistance) || afDistance < 0) return;
		float fDistance = Cost(afDistance, apAINode, apParent);
		if(!std::isfinite(fDistance) || fDistance < 0) return;
		auto result = m_mapNodes.try_emplace(apAINode, apAINode);
		cAStarNode *pNode = &result.first->second;
		if(!result.second && fDistance >= pNode->mfDistance) return;

		// Erase before changing the ordering key; cheaper closed nodes reopen too.
		m_setOpenList.erase(pNode);
		pNode->mfDistance = fDistance;
		pNode->mfCost = fDistance + Heuristic(apAINode->GetPosition(), mvGoal);
		pNode->mpParent = apParent;
		m_setOpenList.insert(pNode);
	}

	//-----------------------------------------------------------------------

	cAStarNode* cAStarHandler::GetBestNode()
	{
		auto it = m_setOpenList.begin();
		cAStarNode *pNode = *it;
		m_setOpenList.erase(it);
		return pNode;
	}

	//-----------------------------------------------------------------------

	float cAStarHandler::Cost(float afDistance, cAINode *apAINode, cAStarNode *apParent)
	{
		if(apParent)
		{
			float fHeight = (1+fabs(apAINode->GetPosition().y - apParent->mpAINode->GetPosition().y));
			// Penalize this edge, rather than rescaling the entire route.
			return apParent->mfDistance + afDistance * fHeight;
		}
		else
			return afDistance;
	}

	//-----------------------------------------------------------------------

	float cAStarHandler::Heuristic(const cVector3f& avStart, const cVector3f& avGoal)
	{
		//return cMath::Vector3DistSqr(avStart, avGoal);
		return cMath::Vector3Dist(avStart, avGoal);
	}

	//-----------------------------------------------------------------------

	bool cAStarHandler::IsGoalNode(cAINode *apAINode)
	{
		tAINodeSetIt it = m_setGoalNodes.find(apAINode);
		if(it == m_setGoalNodes.end()) return false;

		return true;
	}

	//-----------------------------------------------------------------------
}
