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

#ifndef MINSG_EXT_PATHTRACING_PATHTRACER_H_
#define MINSG_EXT_PATHTRACING_PATHTRACER_H_

#include <Util/References.h>
#include <Util/Graphics/Color.h>
#include <Geometry/Vec2.h>

#include <vector>
#include <deque>
#include <cstdint>
#include <random>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace Geometry {
template<typename T_> class _Vec3;
template<typename T_> class _Vec2;
template<typename vec_t> class _Ray;
typedef _Ray<_Vec3<float>> Ray3;
typedef _Vec2<float> Vec2;
typedef _Vec3<float> Vec3;
}

namespace Util {
class PixelAccessor;
} 

namespace MinSG {
class GroupNode;
class GeometryNode;
class LightNode;
class AbstractCameraNode;

namespace PathTracing {
class SurfacePoint;
class Light;
class Tile;

class PathTracer {
public:    
  PathTracer();  
  PathTracer(const PathTracer& other) = delete;  
  ~PathTracer();  
  
  /**
   * 
   */
  void download( Util::PixelAccessor& image, float gamma=2.2);
  /**
   * Resets the path tracer.
   */
  void reset();  
  
  void start();
  
  void pause();
  
  void trace(Tile* tile);
  
  void setScene(GroupNode* scene);
  void setCamera(AbstractCameraNode* camera);
  
  void setMaxBounces(uint32_t maxBounces) { this->maxBounces = maxBounces; needsReset = true;}
  void setSeed(uint32_t seed) { this->seed = seed; needsReset = true;}
  void setUseGlobalLight(bool useGlobalLight) { this->useGlobalLight = useGlobalLight; needsReset = true; }
  void setAntiAliasing(bool antiAliasing) { this->antiAliasing = antiAliasing; needsReset = true; }
  void setResolution(const Geometry::Vec2i& resolution) { this->resolution = resolution; needsReset = true; }
  void setMaxSamples(uint32_t maxSamples) { this->maxSamples = maxSamples; needsReset = true;}
  void setThreadCount(uint32_t count) { this->threadCount = count; needsReset = true; }
  bool isFinished() const { return finished; }
  uint32_t getSamplesPerPixel() const { return spp; }
private:
  Util::Color4f getRadiance(const Geometry::Ray3& primaryRay);  
  Util::Color4f sampleLight(const Geometry::Ray3& ray, const SurfacePoint& surface);
  float sample1D();
  Geometry::Vec2 sample2D();
  Geometry::Vec3 sample3D();
  void doWork();
  
  uint32_t seed;
  uint32_t maxSamples = 1024;
  uint32_t maxBounces = 4;
  uint32_t threadCount = 8;
  uint32_t tileCount;
  bool useGlobalLight = true;
  bool antiAliasing = true;
  Geometry::Vec2 resolution;
  bool needsReset = true;
  bool paused = false;
  std::atomic_bool finished;
  std::atomic_uint finishedCount;
  std::atomic_uint spp;
  
  Util::Reference<GroupNode> scene;
  Util::Reference<AbstractCameraNode> camera;
  std::vector<std::unique_ptr<Light>> sceneLights;
  std::deque<std::unique_ptr<Tile>> tileQueue;
  
  std::default_random_engine rng;
  
  std::vector<std::thread> threads;
  std::mutex queueMutex;
  std::condition_variable condition;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_PATHTRACER_H_ */
#endif /* MINSG_EXT_PATHTRACING */
