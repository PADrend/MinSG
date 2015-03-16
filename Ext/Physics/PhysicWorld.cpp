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

#include "PhysicWorld.h"
#include "Bullet/BtPhysicWorld.h"
#include "../../Core/Nodes/Node.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../Core/NodeAttributeModifier.h"
#include <Util/StringUtils.h>

namespace MinSG {
namespace Physics {



const Util::StringIdentifier PhysicWorld::SHAPE_TYPE("type");

const char* const PhysicWorld::SHAPE_TYPE_BOX = "bb";
const char* const PhysicWorld::SHAPE_TYPE_CONVEX_HULL = "cHull";
const char* const PhysicWorld::SHAPE_TYPE_STATIC_TRIANGLE_MESH = "mesh";
const char* const PhysicWorld::SHAPE_TYPE_SPHERE ="sphere";

//! (static)
PhysicWorld * PhysicWorld::createBulletWorld(){
	return new BtPhysicWorld;
}


}
}
#endif //MINSG_EXT_PHYSICS
