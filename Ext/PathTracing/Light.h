/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#ifndef MINSG_EXT_PATHTRACING_LIGHT_H_
#define MINSG_EXT_PATHTRACING_LIGHT_H_

#include "SurfacePoint.h"

#include <Util/References.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Vec3.h>

#include <cstdint>

namespace Rendering {
namespace MeshUtils {
class TriangleAccessor;
} 
} 

namespace MinSG {
class LightNode;
class GeometryNode;
  
namespace PathTracing {

class Light {
public:
  struct SampleResult {
    Util::Color4f l;
    Geometry::Vec3 wi;
    float pdf;
    float dist;
  };
  virtual ~Light() = default;
  
  virtual SampleResult sampleIncidentRadiance(const SurfacePoint& surface, const Geometry::Vec3& sample) const = 0;
  virtual bool isDeltaLight() const { return false; };
}; 

class MinSGLight : public Light {
public:
  MINSGAPI MinSGLight(LightNode* node);
  
  MINSGAPI virtual SampleResult sampleIncidentRadiance(const SurfacePoint& surface, const Geometry::Vec3& sample) const;
  virtual bool isDeltaLight() const { return true; };
private:
  Util::Reference<LightNode> node;
};

class DiffuseAreaLight : public Light  {
public:
  MINSGAPI DiffuseAreaLight(GeometryNode* node, Util::Color4f emission);
  
  MINSGAPI virtual SampleResult sampleIncidentRadiance(const SurfacePoint& surface, const Geometry::Vec3& sample) const;
private:
  Util::Reference<GeometryNode> node;
  Util::Reference<Rendering::MeshUtils::TriangleAccessor> tAcc;
  Util::Color4f emission;
};

}
}


#endif /* end of include guard: MINSG_EXT_PATHTRACING_LIGHT_H_ */
#endif /* MINSG_EXT_PATHTRACING */
