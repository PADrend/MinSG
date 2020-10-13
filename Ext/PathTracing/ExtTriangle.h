/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING
#ifndef MINSG_EXT_PATHTRACING_TRIANGLE_H_
#define MINSG_EXT_PATHTRACING_TRIANGLE_H_

#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Triangle.h>

namespace Geometry {
typedef Triangle<_Vec3<float>> Triangle_3f;
typedef Triangle<_Vec2<float>> Triangle_2f;
}

namespace MinSG {
class GeometryNode;
namespace PathTracing {
class Material;
class SurfacePoint;

class ExtTriangle {
public:  
  MINSGAPI SurfacePoint getSurfacePoint(float u, float v) const;
  
  Geometry::Triangle_3f pos = {{0,0,0},{0,0,0},{0,0,0}};
  Geometry::Triangle_3f normal = {{0,0,0},{0,0,0},{0,0,0}};
  Geometry::Triangle_3f color = {{0,0,0},{0,0,0},{0,0,0}};
  Geometry::Triangle_2f texCoord = {{0,0},{0,0},{0,0}};
  
  GeometryNode* source = nullptr;
  Material* material = nullptr;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_TRIANGLE_H_ */
#endif /* MINSG_EXT_PATHTRACING */