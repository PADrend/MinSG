/*
	This file is part of the MinSG library extension Physics.
	Copyright (C) 2013 Mouns Almarrani
	Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PHYSICS
#ifndef MINSG_PHYSICHELPER_H_
#define MINSG_PHYSICHELPER_H_

// Forward declarations
class btMatrix3x3;
class btVector3;
class btTransform;
namespace Geometry {
template<typename T_> class _Matrix3x3;
typedef _Matrix3x3<float> Matrix3x3;
template<typename T_> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4;
template<typename T_> class _Vec3;
typedef _Vec3<float> Vec3;
template<typename T_> class _SRT;
typedef _SRT<float> SRT;
}
namespace Util{
class Color4f;
}

namespace MinSG {
namespace Physics{

MINSGAPI btMatrix3x3 toBtMatrix3x3(const Geometry::Matrix3x3& m);
MINSGAPI btTransform toBtTransform(const Geometry::SRT& s);
MINSGAPI btVector3 toBtVector3(const Geometry::Vec3& v);

MINSGAPI Geometry::Vec3 toVec3(const btVector3& btv);
MINSGAPI Geometry::Matrix3x3 toMatrix3x3(const btMatrix3x3& btm);
MINSGAPI Geometry::SRT toSRT(const btTransform& t);

MINSGAPI Util::Color4f toColor4f(const btVector3& btv);
}
}



#endif // MINSG_PHYSICHELPER_H_
#endif // MINSG_EXT_PHYSICS
