/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2015 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#ifndef PHYSICWORLD_H
#define PHYSICWORLD_H

#include <Util/ReferenceCounter.h>
#include <Util/StringIdentifier.h>
#include <Util/Factory/LambdaFactory.h>
#include <vector>

#include "CollisionShape.h"

namespace Geometry {
template<typename T_>class _Plane;
typedef _Plane<float> Plane;
template<typename T_> class _Vec3;
typedef _Vec3<float> Vec3;
template<typename T_> class _Box;
typedef _Box<float> Box;
template<typename T_> class _Sphere;
typedef _Sphere<float> Sphere;
template<typename T_> class _SRT;
typedef _SRT<float> SRT;
}
namespace Rendering {
class RenderingContext;
}
namespace MinSG {
class Node;
//! @ingroup ext
namespace Physics {

class PhysicWorld : public Util::ReferenceCounter<PhysicWorld>{
	public:
		MINSGAPI static PhysicWorld * createBulletWorld();

		//! create a new physic world
		PhysicWorld() = default;
		virtual ~PhysicWorld(){};
		
		// simulation
		virtual void cleanupWorld() = 0;
		virtual void stepSimulation(float time) = 0;
		virtual void renderPhysicWorld(Rendering::RenderingContext&) = 0;

		// world setup
		virtual void initNodeObserver(Node * rootNode) = 0;
		virtual void createGroundPlane(const Geometry::Plane& plane ) = 0;
		virtual void setGravity(const Geometry::Vec3& gravity) = 0;
		virtual const Geometry::Vec3 getGravity() = 0;
		
		// physics object properties
		virtual void markAsKinematicObject(Node& node, bool b) = 0;
		virtual void removeNode(Node* node) = 0;
		virtual void setMass(Node& node, float mass) = 0;
		virtual void setShape(Node& node, Util::Reference<CollisionShape> shape) = 0;
		virtual void setFriction(Node& node, float fric) = 0;
		virtual void setRollingFriction(Node& node, float rollfric) = 0;
		virtual void setLinearDamping(Node& node, float) = 0;
		virtual void setAngularDamping(Node& node, float) = 0;
		virtual void updateLocalSurfaceVelocity(Node* node, const Geometry::Vec3& localForce) = 0;
		
		// interaction
		virtual void setLinearVelocity(Node& node,const Geometry::Vec3&) = 0;
		virtual void setAngularVelocity(Node& node,const Geometry::Vec3&) = 0;
		
		// collision shapes
		MINSGAPI static const Util::StringIdentifier SHAPE_AABB;
		MINSGAPI static const Util::StringIdentifier SHAPE_SPHERE;
		MINSGAPI static const Util::StringIdentifier SHAPE_COMPOSED;
	protected:
		Util::LambdaFactory<CollisionShape*, Util::StringIdentifier> shapeFactory;
	public:
		template<typename ...Args>
		Util::Reference<CollisionShape> createShape(const Util::StringIdentifier& id, Args... args) { return shapeFactory.create(id, args...);	}
		
		MINSGAPI Util::Reference<CollisionShape> createShape_AABB(const Geometry::Box& aabb);
		MINSGAPI Util::Reference<CollisionShape> createShape_Sphere(const Geometry::Sphere& s);
		MINSGAPI Util::Reference<CollisionShape> createShape_Composed(const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& shapes);


		// constraints
		virtual void addConstraint_p2p(Node& nodeA, const Geometry::Vec3& pivotLocalA, Node& nodeB,const Geometry::Vec3& pivotLocalB)= 0;
		virtual void addConstraint_hinge(Node& nodeA, const Geometry::Vec3& pivotLocalA, const Geometry::Vec3& dirLocalA,Node& nodeB, const Geometry::Vec3& pivotLocalB, const Geometry::Vec3& dirLocalB)= 0;
		virtual void removeConstraints(Node& node)=0;
		virtual void removeConstraintBetweenNodes(Node& nodeA,Node& nodeB)=0;
};


}
}


#endif /* PHYSICWORLD_H */

#endif /* MINSG_EXT_PHYSICS */
