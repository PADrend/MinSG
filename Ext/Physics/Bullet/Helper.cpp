/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS

#include "Helper.h"
#include <Geometry/Matrix3x3.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/SRT.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF_CLANG(-W#warnings)
COMPILER_WARN_OFF_GCC(-Wunused-variable)
COMPILER_WARN_OFF_GCC(-Wswitch-default)
COMPILER_WARN_OFF_GCC(-Wunused-parameter)
COMPILER_WARN_OFF_GCC(-Woverloaded-virtual)
COMPILER_WARN_OFF_GCC(-Wshadow)
COMPILER_WARN_OFF_GCC(-Wold-style-cast)
COMPILER_WARN_OFF_GCC(-Wcast-qual)
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btTransform.h>
COMPILER_WARN_POP

namespace MinSG {
namespace Physics{

btMatrix3x3 toBtMatrix3x3(const Geometry::Matrix3x3& m){
    btMatrix3x3 btM;
    btM.setValue(	m.at(0), m.at(1), m.at(2),
					m.at(3), m.at(4), m.at(5),
					m.at(6), m.at(7), m.at(8));
	return btM;
}

btTransform toBtTransform(const Geometry::SRT& s){
	btTransform t;
	t.setIdentity();
	t.setOrigin( toBtVector3(s.getTranslation()) );
	t.setBasis( toBtMatrix3x3(s.getRotation()) );
	return t;
}

btVector3 toBtVector3(const Geometry::Vec3& v){
    return btVector3(v.x(),v.y(),v.z());
}

Geometry::Vec3 toVec3(const btVector3& btv){
    return Geometry::Vec3(btv.getX(), btv.getY(), btv.getZ());
}


Geometry::Matrix3x3 toMatrix3x3(const btMatrix3x3& btm){
    Geometry::Matrix3x3 tmp;
    for(int i = 0; i<3; ++i)
        tmp.setRow(i,toVec3(btm.getRow(i)));
    return tmp;
}


Geometry::SRT toSRT(const btTransform& t){
	return Geometry::SRT(toVec3(t.getOrigin()), toMatrix3x3(t.getBasis()),1.0);
}


}

}




#endif // MINSG_EXT_PHYSICS
