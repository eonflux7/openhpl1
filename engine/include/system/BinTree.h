// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_BINTREE_H
#define HPL_BINTREE_H

#include <list>

namespace hpl {

	enum eBinTreeNode {
		eBinTreeNode_Left,
		eBinTreeNode_Right
	};



	// TREE NODE CLASS

	template<class T> class BinTreeNode {
	public:
		T* GetData()
		{
			return &mData;
		}

		BinTreeNode(T aData, BinTreeNode<T> *aParent, eBinTreeNode aParentDir)
		{
			for(int i=0;i<2;i++) mChild[i]=nullptr;
			mData = aData;
			mParent = aParent;
			mParentDir = aParentDir;
		}

		BinTreeNode<T>* AddChild(eBinTreeNode i, T aData)
		{
			if(mChild[i]==nullptr){
				mChild[i] = new BinTreeNode<T>(aData, this,i);
				return mChild[i];
			}
			return nullptr;
		}

		BinTreeNode<T>* GetChild(eBinTreeNode i)
		{
			return mChild[i];
		}

		BinTreeNode<T>* GetParent()
		{
			return mParent;
		}

	private:
		BinTreeNode<T>* mChild[2];
		BinTreeNode<T>* mParent;
		T mData;
		eBinTreeNode mParentDir;
	};

	// MAIN TREE CLASS

	template<class T> class BinTree {
	public:
		BinTree(){
			mlNumOfNodes =0;
			mFirstNode = nullptr;
		}

		~BinTree()
		{
			Clear();
		}

		/**
		 * Clears the entire tree
		 * \return number of nodes deleted
		 */
		int Clear()
		{
			mlNum=0;
			DeleteNode(mFirstNode);
			mFirstNode = nullptr;
			return mlNum;
		}

		/**
		 * Insert a node to the tree.
		 * \todo only works to set the root node.
		 * \param aData the data to insert
		 * \return
		 */
		BinTreeNode<T>* Insert(T aData)
		{
			if(mFirstNode==nullptr)
			{
				mFirstNode = new BinTreeNode<T>(aData, nullptr,eBinTreeNode_Left);
				mlNumOfNodes++;

				return mFirstNode;
			}

			//Insertion other then at the root is not supported!
			BinTreeNode<T>* Node = mFirstNode;
			eBinTreeNode c;
			while(true)
			{
					c = eBinTreeNode_Left;

				if(Node->GetChild(c)==nullptr)
				{
					Node = Node->AddChild(c, aData);
					break;
				}
				else
				{
					Node = Node->GetChild(c);
				}
			}
			mlNumOfNodes++;

			return Node;
		}

		/**
		 * Insert a node into a certain node in the tree
		 * \param aData the data to insert
		 * \param aNode the node to insert the data in
		 * \param aChild what child to insert at
		 * \return
		 */
		BinTreeNode<T>* InsertAt(T aData,BinTreeNode<T>* aNode, eBinTreeNode aChild=eBinTreeNode_Left)
		{
			if(aNode == nullptr)return nullptr;

			if(aNode->GetChild(aChild)!=nullptr) {
				aChild = aChild==eBinTreeNode_Left ? eBinTreeNode_Right : eBinTreeNode_Left;
				if(aNode->GetChild(aChild)!=nullptr)return nullptr;
			}

			return aNode->AddChild(aChild, aData);
		}

		/**
		 * Get the size of the tree
		 * \return
		 */
		int Size()
		{
			return mlNumOfNodes;
		}

		const std::list<BinTreeNode<T>*>& GetLeafList()
		{
			mlstNodes.clear();
			PopulateLeafList(mFirstNode);
			 return  mlstNodes;
		}

		/**
		 * Get a list of all the nodes in the tree
		 * \return
		 */
		const std::list<BinTreeNode<T>*>& GetNodeList()
		{
			mlstNodes.clear();
			PopulateNodeList(mFirstNode);
			return  mlstNodes;
		}

	private:
		int mlNumOfNodes;
		BinTreeNode<T>* mFirstNode;
		int mlNum;

		std::list<BinTreeNode<T>*> mlstNodes;

		void DeleteNode(BinTreeNode<T>* aNode)
		{
			if(aNode==nullptr)	return;

			DeleteNode(aNode->GetChild(eBinTreeNode_Left));
			DeleteNode(aNode->GetChild(eBinTreeNode_Right));

			delete aNode;
			mlNum++;
		}

		void PopulateNodeList(BinTreeNode<T>* aNode)
		{
			if(aNode==nullptr)	return;

			PopulateNodeList(aNode->GetChild(eBinTreeNode_Left));
			mlstNodes.push_back(aNode);
			PopulateNodeList(aNode->GetChild(eBinTreeNode_Right));
		}

		void PopulateLeafList(BinTreeNode<T>* aNode)
		{
			if(aNode==nullptr)	return;

			if(aNode->GetChild(eBinTreeNode_Left)==nullptr &&
				aNode->GetChild(eBinTreeNode_Right)==nullptr)
			{
				mlstNodes.push_back(aNode);
			}

			PopulateLeafList(aNode->GetChild(eBinTreeNode_Left));
			PopulateLeafList(aNode->GetChild(eBinTreeNode_Right));
		}


	};

};
#endif // HPL_BINTREE_H
