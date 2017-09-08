/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#ifndef MINSG_EXT_RAYCASTING
#error "MINSG_EXT_PATHTRACING requires MINSG_EXT_RAYCASTING."
#endif /* MINSG_EXT_RAYCASTING */

#ifndef MINSG_EXT_PATHTRACING_H_
#define MINSG_EXT_PATHTRACING_H_

#include <Util/References.h>

#include <vector>
#include <cstdint>
#include <random>

namespace Geometry {
template<typename T_> class _Vec3;
template<typename vec_t> class _Ray;
typedef _Ray<_Vec3<float>> Ray3;
}

namespace Util {
class PixelAccessor;
class Color4f;
} 

namespace MinSG {
class GroupNode;
class GeometryNode;
class LightNode;
class AbstractCameraNode;

namespace PathTracing {
struct SurfaceProperties;
struct Emitter;

class PathTracer {
public:  
  PathTracer();
  /**
   * 
   */
  void trace(AbstractCameraNode* camera, GroupNode* scene, Util::PixelAccessor& frameBuffer);
  /**
   * Resets the path tracer.
   */
  void reset();
  
  void setMaxBounces(uint32_t maxBounces) { this->maxBounces = maxBounces; }
  void setSeed(uint32_t seed) { this->seed = seed; }
  void setUseGlobalLight(bool useGlobalLight) { this->useGlobalLight = useGlobalLight; }
  void setEmittersOnly(bool emittersOnly) { this->emittersOnly = emittersOnly; }
  void setAntiAliasing(bool antiAliasing) { this->antiAliasing = antiAliasing; }
private:
  Util::Color4f getRadiance(GroupNode* scene, const std::vector<Emitter>& lights, const Geometry::Ray3& ray, uint32_t bounce);  
  Util::Color4f sampleDirectLight(GroupNode* scene, const std::vector<Emitter>& lights, const Geometry::Ray3& ray, const SurfaceProperties& surface);
  Geometry::Ray3 sampleHemisphere(const SurfaceProperties& surface, const Geometry::Ray3& ray);
  
  uint32_t spp;
  uint32_t seed;
  uint32_t maxBounces = 4;
  bool useGlobalLight = true;
  bool emittersOnly = false;
  bool antiAliasing = true;
  std::default_random_engine rng;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_H_ */
#endif /* MINSG_EXT_PATHTRACING */
