/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING
#ifndef MINSG_EXT_PATHTRACING_MATERIAL_H_
#define MINSG_EXT_PATHTRACING_MATERIAL_H_

#include <Util/References.h>
#include <Util/Graphics/Color.h>

#include <tuple>

namespace Geometry {
template<typename T_> class _Vec2;
template<typename T_> class _Vec3;
typedef _Vec2<float> Vec2;
typedef _Vec3<float> Vec3;
}
namespace Util {
class PixelAccessor;
} 
namespace MinSG {
class Node;
namespace PathTracing {

// TODO: BRDF maps? stored as SHs?
struct TextureWrapper {
  Util::Reference<Util::PixelAccessor> texture;
  Util::Color4f constantValue;
};

class Material {
public:
  MINSGAPI Material();
  MINSGAPI Material(const TextureWrapper& diffuse, const TextureWrapper& normal, const TextureWrapper& specular, 
    const TextureWrapper& shininess, const Util::Color4f& emission);
  
  MINSGAPI Util::Color4f getDiffuse(const Geometry::Vec2& uv) const;
  MINSGAPI Geometry::Vec3 getNormal(const Geometry::Vec2& uv) const;
  MINSGAPI Util::Color4f getSpecular(const Geometry::Vec2& uv) const;
  MINSGAPI float getShininess(const Geometry::Vec2& uv) const;
  MINSGAPI Util::Color4f getEmission() const;
  
  MINSGAPI bool isEmissive() const;
  
  MINSGAPI static Material* createFromNode(Node* node);
private:
  TextureWrapper diffuse;
  TextureWrapper normal;
  TextureWrapper specular;
  TextureWrapper shininess;
  Util::Color4f emission;
};
  
}
}

#endif /* end of include guard: MINSG_EXT_PATHTRACING_MATERIAL_H_ */
#endif /* MINSG_EXT_PATHTRACING */