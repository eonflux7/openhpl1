// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_ENTITY_LOADER_OBJECT_H
#define HPL_ENTITY_LOADER_OBJECT_H

#include "system/SystemTypes.h"
#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"

#include "resources/Resources.h"

class TiXmlElement;

namespace hpl {

	class iPhysicsBody;
	class iPhysicsJoint;
	class iPhysicsWorld;
	class cMesh;
	class cMeshEntity;
	class cParticleSystem3D;
	class cBillboard;
	class cBeam;
	class cSoundEntity;
	class iLight3D;
	class iHapticShape;

	class cEntityLoader_Object : public iEntity3DLoader
	{
	public:
		cEntityLoader_Object(const tString &asName) : iEntity3DLoader(asName){}
		~cEntityLoader_Object() override{}

		iEntity3D* Load(const tString &asName, TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,
					cWorld3D *apWorld, const tString &asFileName, bool abLoadReferences) override;

	protected:
		virtual void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld){};
		virtual void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld){};

		void SetBodyProperties(iPhysicsBody *apBody, TiXmlElement *apPhysicsElem);
		void SetJointProperties(iPhysicsJoint *apJoint, TiXmlElement *apJointElem,cWorld3D *apWorld);

		void LoadController(iPhysicsJoint *apJoint,iPhysicsWorld *apPhysicsWorld, TiXmlElement *apElem);

		eAnimationEventType GetAnimationEventType(const char* apString);

		tString msSubType;
		tString msName;

		tString msFileName;

		std::vector<iPhysicsBody*> mvBodies;
		std::vector<iPhysicsJoint*> mvJoints;

		std::vector<iHapticShape*> mvHapticShapes;

		std::vector<iLight3D*> mvLights;
		std::vector<cParticleSystem3D*> mvParticleSystems;
		std::vector<cBillboard*> mvBillboards;
		std::vector<cBeam*> mvBeams;
		std::vector<cSoundEntity*> mvSoundEntities;

		cMeshEntity *mpEntity;
		cMesh *mpMesh;
	};

};
#endif // HPL_ENTITY_LOADER_OBJECT_H
