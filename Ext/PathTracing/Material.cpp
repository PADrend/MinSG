/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "Material.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/GroupState.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/States/ShaderUniformState.h"
#include "../../Core/States/ShaderState.h"

#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>

#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Uniform.h>

#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelAccessor.h>

#include <deque>

namespace MinSG {
namespace PathTracing {

using namespace Util;
using namespace Rendering;

template<class State_t>
std::deque<State_t *> collectAllStatesUpwards(Node* node) {
	std::deque<State_t *> states;
	for(;node != nullptr; node = node->getParent()) {
		for(const auto & state : node->getStates()){
			State_t * specificState = dynamic_cast<State_t *>(state);
			GroupState * grpState = dynamic_cast<GroupState *>(state);
      if(grpState) {
        for(const auto & state : grpState->getStates()){
    			State_t * specificState = dynamic_cast<State_t *>(state.get());
    			if(specificState)
  					states.push_back(specificState);
    		}
      } else if(specificState) {
        states.push_back(specificState);
      }  
		}
	}
	return states;
}

template<class State_t>
State_t * getFirstStateUpwards(Node* node) {
  auto states = collectAllStatesUpwards<State_t>(node);
  return states.empty() ? nullptr : states.front();
}

Texture* getTexture(Node* node, int32_t unit) {
  if(unit < 0)
    return nullptr;
  auto states = collectAllStatesUpwards<TextureState>(node);
  for(auto state : states) {
    if(state->getTextureUnit() == unit)
      return state->getTexture();
  }
	return nullptr;
}

Uniform getUniform(Node* node, const std::string& uniformName) {
  auto states = collectAllStatesUpwards<State>(node);
  for(auto state : states) {
    auto shaderState = dynamic_cast<ShaderState*>(state);
    auto uniformState = dynamic_cast<ShaderUniformState*>(state);
    if(shaderState && shaderState->hasUniform(uniformName)) {
      return shaderState->getUniform(uniformName);
    } else if(uniformState && uniformState->hasUniform(uniformName)) {
      return uniformState->getUniform(uniformName);
    }
  }
	return {};
}

int32_t getTextureUnit(Node* node, const std::string& uniformName) {
  auto uniform = getUniform(node, uniformName);
  if(uniform.isNull() || uniform.getNumValues() != 1 || uniform.getType() != Uniform::UNIFORM_INT)
	 return -1;
  return *reinterpret_cast<const int32_t*>(uniform.getData());
}

Reference<PixelAccessor> createPixelAccessor(Texture* tex) {
  if(!tex)
    return {nullptr};    
  if(!tex->getLocalBitmap()) {
    WARN("PathTracing: All textures need to be downloaded!");
    return {nullptr}; 
  } 
  return PixelAccessor::create(tex->getLocalBitmap());
}

Util::Color4f sampleTexture(const TextureWrapper& tex, const Geometry::Vec2& uv) {
  if(tex.texture.isNull())
    return tex.constantValue;
    
  // TODO: filtering
  return tex.texture->readColor4f(
    static_cast<int32_t>(uv.x() * tex.texture->getWidth()) % tex.texture->getWidth(), 
    static_cast<int32_t>(uv.y() * tex.texture->getHeight()) % tex.texture->getHeight());
}

inline Geometry::Vec3 colorToVec(const Util::Color4f& col) {
	return {col.r(), col.g(), col.b()};
}

Material::Material() : emission(0,0,0,0) {};

Material::Material(const TextureWrapper& diffuse, const TextureWrapper& normal, const TextureWrapper& specular, 
  const TextureWrapper& shininess, const Util::Color4f& emission) 
    : diffuse(diffuse), normal(normal), specular(specular), shininess(shininess), emission(emission) {}

Util::Color4f Material::getDiffuse(const Geometry::Vec2& uv) const {
  return sampleTexture(diffuse, uv);
}

Geometry::Vec3 Material::getNormal(const Geometry::Vec2& uv) const {
  auto nrm = colorToVec(sampleTexture(normal, uv));
	if(normal.texture.isNotNull())
		nrm = nrm * 2 - Geometry::Vec3(1,1,1);
  return nrm;
}

Util::Color4f Material::getSpecular(const Geometry::Vec2& uv) const {
  return sampleTexture(specular, uv);
}

float Material::getShininess(const Geometry::Vec2& uv) const {
  return sampleTexture(shininess, uv).r();
}

Util::Color4f Material::getEmission() const {
  return emission;
}

bool Material::isEmissive() const {
  return emission.r() + emission.g() + emission.b() > 0;
}

Material* Material::createFromNode(Node* node) {
  Material* material = new Material;
  // get base material
  auto matState = getFirstStateUpwards<MaterialState>(node);
  if(matState) {
    auto mat = matState->getParameters();
    material->diffuse.constantValue = mat.getDiffuse();
    material->specular.constantValue = mat.getSpecular();
    material->shininess.constantValue.setR(mat.getShininess());
    material->emission = mat.getEmission();
  }
  // get diffuse texture
  material->diffuse.texture = createPixelAccessor(getTexture(node, 0));
  // find normal texture
  int32_t nrmUnit = getTextureUnit(node, "sg_normalMap");
	material->normal.constantValue.set(0, 0, 0, 0);
  material->normal.texture = createPixelAccessor(getTexture(node, nrmUnit));
  // find specular texture
  int32_t specUnit = getTextureUnit(node, "sg_specularMap");
  material->specular.texture = createPixelAccessor(getTexture(node, specUnit));
  // find shininess texture
  int32_t shinyUnit = getTextureUnit(node, "sg_shininessMap");
  material->shininess.texture = createPixelAccessor(getTexture(node, shinyUnit));
  
  return material;
}

}
}
#endif