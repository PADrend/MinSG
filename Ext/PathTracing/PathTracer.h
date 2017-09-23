/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#ifndef MINSG_EXT_TRIANGLETREES
#error "MINSG_EXT_PATHTRACING requires MINSG_EXT_TRIANGLETREES."
#endif 

#ifndef MINSG_EXT_PATHTRACING_PATHTRACER_H_
#define MINSG_EXT_PATHTRACING_PATHTRACER_H_

#include <memory>

namespace Geometry {
template<typename T_> class _Vec2;
typedef _Vec2<int32_t> Vec2i;
}

namespace Util {
class PixelAccessor;
} 

namespace MinSG {
class GroupNode;
class AbstractCameraNode;

namespace PathTracing {
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
  
  void setScene(GroupNode* scene);
  void setCamera(AbstractCameraNode* camera);
  
  void setMaxBounces(uint32_t maxBounces);
  void setSeed(uint32_t seed);
  void setUseGlobalLight(bool useGlobalLight);
  void setAntiAliasing(bool antiAliasing);
  void setResolution(const Geometry::Vec2i& resolution);
  void setMaxSamples(uint32_t maxSamples);
  void setThreadCount(uint32_t count);
  void setTileSize(uint32_t size);
  bool isFinished() const;
  uint32_t getSamplesPerPixel() const;
private:
	class pimpl;
	std::unique_ptr<pimpl> impl;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_PATHTRACER_H_ */
#endif /* MINSG_EXT_PATHTRACING */
