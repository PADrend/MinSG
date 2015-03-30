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

#ifndef PHYSICWORLD_H
#define PHYSICWORLD_H

#include "PhysicObject.h"
#include <Util/ReferenceCounter.h>

namespace Geometry {
template<typename T_>class _Plane;
typedef _Plane<float> Plane;
template<typename T_> class _Vec3;
typedef _Vec3<float> Vec3;
template<typename T_> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class RenderingContext;
}
namespace MinSG {
class Node;
namespace Physics {

class CollisionShape;

class PhysicWorld : public Util::ReferenceCounter<PhysicWorld>{
	public:
//		//---------------Description keys-------------------
//		static const Util::StringIdentifier SHAPE_TYPE;
//		static const char* const SHAPE_TYPE_BOX;
//		static const char* const SHAPE_TYPE_CONVEX_HULL;
//		static const char* const SHAPE_TYPE_STATIC_TRIANGLE_MESH ;
//		static const char* const SHAPE_TYPE_SPHERE;

		static PhysicWorld * createBulletWorld();

		//! create a new physic world
		PhysicWorld() = default;
		virtual ~PhysicWorld(){};
		virtual void stepSimulation(float time) = 0;
		virtual void applyProperties(Node& node)= 0;
		virtual void cleanupWorld() = 0;
		virtual void initNodeObserver(Node * rootNode)=0;
		virtual void createGroundPlane(const Geometry::Plane& plane ) = 0;
		virtual void removeNode(Node *node) = 0;
		virtual void setGravity(const Geometry::Vec3& gravity)=0;
		virtual const Geometry::Vec3 getGravity()=0;

		virtual void updateMass(Node* node, float mass) = 0;
		virtual void updateShape(Node* node, Util::Reference<CollisionShape> shape)= 0;
		virtual void updateFriction(Node* node, float fric) = 0;
		virtual void updateRollingFriction(Node* node, float rollfric) = 0;
		virtual void updateLocalSurfaceVelocity(Node* node, const Geometry::Vec3& localForce) = 0;

		virtual Util::Reference<CollisionShape> createShape_AABB(const Geometry::Box& aabb) = 0;
		
		//Debug!!!!!!!
		virtual void renderPhysicWorld(Rendering::RenderingContext&) = 0;

		virtual void applyP2PConstraint(Node* nodeA, Node* nodeB, const Geometry::Vec3& pivotLocalA)= 0;
		virtual void applyHingeConstraint(Node* nodeA, Node* nodeB, const Geometry::Vec3& pivotLocalA, const Geometry::Vec3& dir )= 0;
		virtual void removeConstraints(Node* node)=0;
		virtual void removeConstraintBetweenNodes(Node* nodeA,Node* nodeB)=0;
};


}
}


#endif /* PHYSICWORLD_H */

#endif /* MINSG_EXT_PHYSICS */
