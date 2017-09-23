/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "ExtTriangle.h"
#include "Material.h"
#include "SurfacePoint.h"

#include "../../Core/Nodes/GeometryNode.h"

namespace MinSG {
namespace PathTracing {
  
inline Util::Color4f vecToColor(const Geometry::Vec3& vec) {
  return {vec.x(), vec.y(), vec.z(), 1};
}
      
SurfacePoint ExtTriangle::getSurfacePoint(float u, float v) const {  
	SurfacePoint surface;
  surface.pos = pos.calcPoint(u, v);
  surface.normal = pos.calcNormal();//normal.calcPoint(u, v).normalize();
  surface.tangent = pos.getEdgeAB().normalize();
	surface.bitangent = surface.normal.cross(surface.tangent);
  auto tc = texCoord.calcPoint(u,v);
  
  surface.albedo = vecToColor(color.calcPoint(u, v));
  surface.albedo += material->getDiffuse(tc);
  auto nrm = material->getNormal(tc);
  if(!nrm.isZero()) 
    surface.normal = surface.localToWorld(nrm).normalize();
  surface.emission = material->getEmission();
    
  return surface;
}
  
}
}

#endif /* MINSG_EXT_PATHTRACING */