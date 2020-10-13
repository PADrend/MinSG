/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING
#ifndef MINSG_EXT_PATHTRACING_SURFACEPOINT_H_
#define MINSG_EXT_PATHTRACING_SURFACEPOINT_H_

#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>
#include <Util/Graphics/Color.h>

namespace MinSG {
class GeometryNode;
namespace PathTracing {

struct BSDFSample {
	Util::Color4f f;
	Geometry::Vec3 wi;
	float pdf;
};

class SurfacePoint {
public:
	Geometry::Vec3 pos;
	Geometry::Vec3 normal;
	Geometry::Vec3 tangent;
	Geometry::Vec3 bitangent;
	Geometry::Vec2 texcoord;
	Util::Color4f albedo;
	Util::Color4f emission;
	Geometry::Vec3 worldToLocal(const Geometry::Vec3& v) const {
		return {v.dot(tangent), v.dot(bitangent), v.dot(normal)};
	}
	Geometry::Vec3 localToWorld(const Geometry::Vec3& v) const {
		return {
			v.x() * tangent.x() + v.y() * bitangent.x() + v.z() * normal.x(),
			v.x() * tangent.y() + v.y() * bitangent.y() + v.z() * normal.y(),
			v.x() * tangent.z() + v.y() * bitangent.z() + v.z() * normal.z()
		};
	}
  
  MINSGAPI BSDFSample getBSDF(const Geometry::Vec3 &woWorld, const Geometry::Vec3 &wiWorld) const;
  MINSGAPI BSDFSample sampleBSDF(const Geometry::Vec3 &woWorld, const Geometry::Vec2& sample) const;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_SURFACEPOINT_H_ */
#endif /* MINSG_EXT_PATHTRACING */
