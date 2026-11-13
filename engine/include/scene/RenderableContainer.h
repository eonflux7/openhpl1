// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_RENDERABLE_CONTAINER_H
#define HPL_RENDERABLE_CONTAINER_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	class iRenderable;
	class cFrustum;
	class cRenderList;

	class iRenderableContainer
	{
	public:
		virtual ~iRenderableContainer(){}
		/**
		 * Add a new object to the container.
		 * \param *apRenderable the object
		 * \param abStatic true if the object will not move, else false.
		 * \return true if success, else false
		 */
		virtual bool Add(iRenderable *apRenderable, bool abStatic)=0;
		/**
		 * Removes the object, only non static are guaranteed to be found.
		 * \param *apRenderable the object
		 * \return true if success, else false
		 */
		virtual bool Remove(iRenderable *apRenderable)=0;

		/**
		 * Populate the renderlist with all objects visible in a fostrum.
		 * \param apFostrum
		 * \param *apRenderList
		 */
		virtual void GetVisible(cFrustum* apFostrum,cRenderList *apRenderList)=0;

		/**
		* Sets up things like octrees so that the container is ready to use.
		*/
		virtual void Compile()=0;
	};
};
#endif // RENDERABLE_CONTAINER
