/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "Light.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/Transformations.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/TriangleAccessor.h>

#include <Geometry/Tools.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Line.h>
#include <Geometry/Triangle.h>
#include <Geometry/Convert.h>
#include <Geometry/Plane.h>

#include <limits>

namespace MinSG {
namespace PathTracing {
  
using namespace Rendering;

/*******************************************************
 * MinSGLight
 *******************************************************/
 
MinSGLight::MinSGLight(LightNode* node) : Light(), node(node) {}

Light::SampleResult MinSGLight::sampleIncidentRadiance(const SurfacePoint& surface, const Geometry::Vec3 &sample) const {
  SampleResult result;
  auto lightPos = node->getWorldOrigin(); 
  float dist = std::numeric_limits<float>::max();
  auto lightDir = Transformations::localDirToWorldDir(*node.get(), {0,0,-1});
  
  float attenuation = 1.0f;
  
  if(node->getType() != LightParameters::lightType_t::DIRECTIONAL) {
    dist = surface.pos.distance(lightPos);
    attenuation /= (node->getConstantAttenuation() + node->getLinearAttenuation() * dist + node->getQuadraticAttenuation() * dist * dist);
    result.wi = (lightPos - surface.pos).getNormalized();
  } else {
    result.wi = -lightDir;
  }
  
  if(node->getType() == LightParameters::lightType_t::SPOT) {
    float spotDot = result.wi.dot(-lightDir);
    attenuation *= spotDot < Geometry::Convert::degToRad(node->getCutoff()) ? std::pow(spotDot, node->getExponent()) : 0;
  }
  
  result.l = (node->getAmbientLightColor() + node->getDiffuseLightColor()) * attenuation;
  result.pdf = 1;
  result.dist = dist;
  return result;
}

 /*******************************************************
  * DiffuseAreaLight
  *******************************************************/
DiffuseAreaLight::DiffuseAreaLight(GeometryNode* node, Util::Color4f emission) : Light(), node(node), emission(emission) {}

Light::SampleResult DiffuseAreaLight::sampleIncidentRadiance(const SurfacePoint& surface, const Geometry::Vec3 &sample) const {
  SampleResult result;
  auto mesh = node->getMesh();
  auto ta = MeshUtils::TriangleAccessor::create(mesh);
  auto t = ta->getTriangle(sample.z() * mesh->getPrimitiveCount());
  //float su0 = std::sqrt(sample.x());
  //Geometry::Vec2 b(1-su0, sample.y() * su0);
  //Geometry::Vec3 p = t.getVertexA() * b.x() + t.getVertexB() * b.y() + t.getVertexC() * (1.0f - b.x() - b.y());
  //auto tPos = Transformations::localPosToWorldPos(*node.get(), p);
  auto tPos = Transformations::localPosToWorldPos(*node.get(), t.calcPoint(sample.x(), sample.y()));
  float area = t.calcArea() * node->getWorldTransformationSRT().getScale();
  auto normal = Transformations::localDirToWorldDir(*node.get(), t.calcNormal());
  //std::cout << "SampleTriangle: Triangle: " << t << ", Pos: " << surface.pos << " tPos: " << tPos << ", A: " << area << ", Normal: " << normal << std::endl;   
  
  result.dist = surface.pos.distance(tPos);
  result.wi = (tPos - surface.pos).getNormalized();
  result.l = normal.dot(-result.wi) > 0 ? emission : Util::Color4f(0,0,0,0);
  result.pdf = area > 0 ? 1.0f / area : 0.0f;
  result.pdf *= 1.0f/mesh->getPrimitiveCount();
  // convert to solid angle
  result.pdf *= (result.dist * result.dist) / std::abs(normal.dot(-result.wi));
  if(std::isinf(result.pdf)) result.pdf = 0;
  return result;
}

}
}

#endif /* MINSG_EXT_PATHTRACING */
