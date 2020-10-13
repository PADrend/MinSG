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

//! @ingroup ext
namespace PathTracing {
class PathTracer {
public:    
  MINSGAPI PathTracer();  
  PathTracer(const PathTracer& other) = delete;  
  MINSGAPI ~PathTracer();  
  
  /**
   * 
   */
  MINSGAPI void download( Util::PixelAccessor& image, float gamma=2.2);
  /**
   * Resets the path tracer.
   */
  MINSGAPI void reset();  
  MINSGAPI void start();
  MINSGAPI void pause();
  
  MINSGAPI void setScene(GroupNode* scene);
  MINSGAPI void setCamera(AbstractCameraNode* camera);
  
  MINSGAPI void setMaxBounces(uint32_t maxBounces);
  MINSGAPI void setSeed(uint32_t seed);
  MINSGAPI void setUseGlobalLight(bool useGlobalLight);
  MINSGAPI void setAntiAliasing(bool antiAliasing);
  MINSGAPI void setResolution(const Geometry::Vec2i& resolution);
  MINSGAPI void setMaxSamples(uint32_t maxSamples);
  MINSGAPI void setThreadCount(uint32_t count);
  MINSGAPI void setTileSize(uint32_t size);
  MINSGAPI bool isFinished() const;
  MINSGAPI uint32_t getSamplesPerPixel() const;
private:
	class pimpl;
	std::unique_ptr<pimpl> impl;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_PATHTRACER_H_ */
#endif /* MINSG_EXT_PATHTRACING */
