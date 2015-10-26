/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2013,2015 Claudius J�hn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#include "PhysicWorld.h"
#include "Bullet/BtPhysicWorld.h"
#include "../../Core/Nodes/Node.h"
//#include "../../Helper/StdNodeVisitors.h"
//#include "../../Core/NodeAttributeModifier.h"
//#include <Util/StringUtils.h>
#include <Geometry/Box.h>
#include <Geometry/Sphere.h>
#include <Geometry/SRT.h>

namespace MinSG {
namespace Physics {

//
//
//const Util::StringIdentifier PhysicWorld::SHAPE_TYPE("type");
//
//const char* const PhysicWorld::SHAPE_TYPE_BOX = "bb";
//const char* const PhysicWorld::SHAPE_TYPE_CONVEX_HULL = "cHull";
//const char* const PhysicWorld::SHAPE_TYPE_STATIC_TRIANGLE_MESH = "mesh";
//const char* const PhysicWorld::SHAPE_TYPE_SPHERE ="sphere";

const Util::StringIdentifier PhysicWorld::SHAPE_AABB("aabb");
const Util::StringIdentifier PhysicWorld::SHAPE_SPHERE("sphere");
const Util::StringIdentifier PhysicWorld::SHAPE_COMPOSED("composed");

//! (static)
PhysicWorld * PhysicWorld::createBulletWorld(){
	return new BtPhysicWorld;
}

Util::Reference<CollisionShape> PhysicWorld::createShape_AABB(const Geometry::Box& aabb) {
	return shapeFactory.create<const Geometry::Box&>(SHAPE_AABB, aabb);
}

Util::Reference<CollisionShape> PhysicWorld::createShape_Sphere(const Geometry::Sphere& s) {
	return shapeFactory.create<const Geometry::Sphere&>(SHAPE_SPHERE, s);
}

Util::Reference<CollisionShape> PhysicWorld::createShape_Composed(const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& shapes) {
	return shapeFactory.create<const std::vector<std::pair<Util::Reference<CollisionShape>,Geometry::SRT>>& >(SHAPE_COMPOSED, shapes);
}


}
}
#endif //MINSG_EXT_PHYSICS
