/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2015 Claudius J�hn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef BTPHYSICWORLD_H
#define BTPHYSICWORLD_H

#include <Util/Macros.h>
COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_GCC(-Woverloaded-virtual)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wcast-qual)
COMPILER_WARN_OFF_GCC(-Wunused)
COMPILER_WARN_OFF_GCC(-Wunused-parameter)
COMPILER_WARN_OFF_GCC(-Wunused-variable)
#include <LinearMath/btConvexHullComputer.h>
#include <btBulletDynamicsCommon.h>
#if (BT_BULLET_VERSION == 282) and !defined(BULLET_WARNING_PATCH)
#define BULLET_WARNING_PATCH
inline int _suppressUnusedVariableWarning(){  return btInfinityMask;} // on mingw, -Wunused-variable does not work here.
#endif
COMPILER_WARN_POP


#include "Helper.h"
#include "BtPhysicObject.h"
#include "../PhysicWorld.h"
#include <Util/References.h>

class btConstraint;

namespace MinSG {
class GroupNode;
namespace Physics {

class BtCollisionShape;

//! BtPhysicWorld---------|>PhysicWorld
class BtPhysicWorld : public PhysicWorld{
		bool simulationIsActive;
		btBroadphaseInterface* broadphase;
		btDefaultCollisionConfiguration* collisionConfiguration;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolver* solver;
		btDiscreteDynamicsWorld* dynamicsWorld;
		btAlignedObjectArray<btCollisionShape*>	collisionShapes;

		MINSGAPI void onNodeTransformed(Node * node);
		MINSGAPI btRigidBody * createRigidBody(BtPhysicObject& physObj,Util::Reference<CollisionShape> shape);
		MINSGAPI void initCollisionCallbacks(BtPhysicObject& physObj);

		std::vector<btRigidBody *> bodiesToRemove;
		std::vector<btTypedConstraint *> constraintsToRemove;
		std::set<Util::Reference<Node>> nodesToUpdate;
	public:
		MINSGAPI BtPhysicWorld();
		virtual ~BtPhysicWorld() = default;
		
		// simulation
		MINSGAPI void cleanupWorld() override;
		MINSGAPI void stepSimulation(float time) override;
		MINSGAPI void renderPhysicWorld(Rendering::RenderingContext& rctxt) override;
		
		// world setup
		MINSGAPI void initNodeObserver(Node * rootNode)override;
		MINSGAPI void createGroundPlane(const Geometry::Plane& plane ) override;
		MINSGAPI void setGravity(const Geometry::Vec3& gravity)override;
		MINSGAPI const Geometry::Vec3 getGravity()override;
		
		// physics objects
	private:
		MINSGAPI void applyProperties(Node& node);
	public:
		MINSGAPI void markAsKinematicObject(Node& node, bool b)override;
		MINSGAPI void removeNode(Node *node)override;
		MINSGAPI void setMass(Node& node, float mass) override;
		MINSGAPI void setFriction(Node& _node, float fric) override;
		MINSGAPI void setRollingFriction(Node& _node, float rollfric) override;
		MINSGAPI void setShape(Node& node, Util::Reference<CollisionShape> shape) override;
		MINSGAPI void setLinearDamping(Node& node, float) override;
		MINSGAPI void setAngularDamping(Node& node, float) override;
		MINSGAPI void updateLocalSurfaceVelocity(Node* node, const Geometry::Vec3& localForce) override;
		
		// collision shapes
		//Util::Reference<CollisionShape> createShape_AABB(const Geometry::Box& aabb)override;
		//Util::Reference<CollisionShape> createShape_Sphere(const Geometry::Sphere&)override;
		//Util::Reference<CollisionShape> createShape_Composed(const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& shapes)override;

		// constraints		
		MINSGAPI void addConstraint_p2p(Node& nodeA, const Geometry::Vec3& pivotLocalA, Node& nodeB,const Geometry::Vec3& pivotLocalB) override;
		MINSGAPI void addConstraint_hinge(Node& nodeA, const Geometry::Vec3& pivotLocalA, const Geometry::Vec3& dirLocalA,Node& nodeB, const Geometry::Vec3& pivotLocalB, const Geometry::Vec3& dirLocalB) override;
		MINSGAPI void removeConstraints(Node& node) override;
		MINSGAPI void removeConstraintBetweenNodes(Node& nodeA,Node& nodeB)override;

		// interaction
		MINSGAPI void setLinearVelocity(Node& node,const Geometry::Vec3&) override;
		MINSGAPI void setAngularVelocity(Node& node,const Geometry::Vec3&) override;

};
}
}

#endif /* BTPHYSICWORLD_H */

#endif /* MINSG_EXT_PHYSICS */
