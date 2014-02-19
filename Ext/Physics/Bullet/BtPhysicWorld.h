/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef BTPHYSICWORLD_H
#define BTPHYSICWORLD_H
#include "Helper.h"
#include "BtPhysicObject.h"
#include "../PhysicWorld.h"
#include <Util/References.h>
#include <Util/Utils.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_GCC(-Wunused-parameter)
COMPILER_WARN_OFF_GCC(-Wunused-variable)
COMPILER_WARN_OFF_GCC(-Woverloaded-virtual)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wcast-qual)
#include <LinearMath/btConvexHullComputer.h>
#include <btBulletDynamicsCommon.h>
COMPILER_WARN_POP

namespace MinSG {
class GroupNode;
namespace Physics {

class ShapeContainer;

//! BtPhysicWorld---------|>PhysicWorld
class BtPhysicWorld: public PhysicWorld{
	private:
		bool simulationIsActive;
		btBroadphaseInterface* broadphase;
		btDefaultCollisionConfiguration* collisionConfiguration;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolver* solver;
		btDiscreteDynamicsWorld* dynamicsWorld;
		btAlignedObjectArray<btCollisionShape*>	collisionShapes;

		void onNodeTransformed(Node * node);
		void onNodeAdded(Node * node);
		btRigidBody * createRigidBody(BtPhysicObject& physObj,ShapeContainer* shape);
		void initCollisionCallbacks(BtPhysicObject& physObj);

	public:
		BtPhysicWorld();
		virtual ~BtPhysicWorld() = default;
		void stepSimulation(float time) override;
		void addNodeToPhyiscWorld(Node *node)override;
		void removeNode(Node *node)override;
		void initNodeObserver(Node * rootNode)override;
		void cleanupWorld() override;
		void createGroundPlane(const Geometry::Plane& plane ) override;
		void setGravity(const Geometry::Vec3& gravity)override;
		const Geometry::Vec3 getGravity()override;
		void updateMass(Node* node, float mass) override;
		void updateFriction(Node* _node, float fric) override;
		void updateRollingFriction(Node* _node, float rollfric) override;
		void updateShape(Node* node, Util::GenericAttributeMap * description) override;


		void updateLocalSurfaceVelocity(Node* node, const Geometry::Vec3& localForce) override;
		void updateConstraintPivot(Node* node, const std::string &name) override;

		//debug!!!!
		void renderPhysicWorld(Rendering::RenderingContext& rctxt) override;
		void applyP2PConstraint(Node* nodeA, Node* nodeB, const Geometry::Vec3& pivotLocalA) override;
		virtual void applyHingeConstraint(Node* nodeA, Node* nodeB, const Geometry::Vec3& pivotLocalA, const Geometry::Vec3& dir ) override;
		void removeConstraints(Node* node) override;
		virtual void removeConstraintBetweenNodes(Node* nodeA,Node* nodeB)override;

};
}
}




#endif /* BTPHYSICWORLD_H */

#endif /* MINSG_EXT_PHYSICS */
