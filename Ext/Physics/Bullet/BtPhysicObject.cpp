/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2009-2013 Claudius J�hn <claudius@uni-paderborn.de>
	Copyright (C) 2009-2013 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#include "BtPhysicObject.h"
#include "Helper.h"
#include<algorithm>

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
#include <btBulletDynamicsCommon.h>
#if (BT_BULLET_VERSION == 282) && !defined(BULLET_WARNING_PATCH)
#define BULLET_WARNING_PATCH
inline int _suppressUnusedVariableWarning(){  return btInfinityMask;} // on mingw, -Wunused-variable does not work here.
#endif
COMPILER_WARN_POP


namespace MinSG {
namespace Physics {

BtPhysicObject::~BtPhysicObject(){}

void BtPhysicObject::removeConstraint(BtConstraintObject& constraint){
    auto it = std::find(constraints.begin(),constraints.end(),&constraint);
    if(it!=constraints.end())
        constraints.erase(it);
}

void BtPhysicObject::setShape(Util::Reference<CollisionShape> _shape){
	BtCollisionShape* btShape = dynamic_cast<BtCollisionShape*>(_shape.get());
	if( _shape && !btShape )
		throw std::runtime_error("setShape: Invalid Shape type.");
	shape = btShape;
}

}
}


#endif // MINSG_EXT_PHYSICS

