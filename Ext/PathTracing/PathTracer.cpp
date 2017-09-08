/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "PathTracer.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/TextureState.h"
#include "../../Core/States/GroupState.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Core/Transformations.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../RayCasting/RayCaster.h"
#include "../TriangleTrees/TriangleTree.h"
#include "../TriangleTrees/TriangleAccessor.h"
#include "../TriangleTrees/ABTree.h"
#include "../TriangleTrees/ABTreeBuilder.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/TriangleAccessor.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>

#include <Geometry/Tools.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Line.h>
#include <Geometry/Triangle.h>
#include <Geometry/Convert.h>
#include <Geometry/Plane.h>

#include <Util/Graphics/PixelAccessor.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/GenericAttribute.h>
#include <Util/StringIdentifier.h>
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <Util/Macros.h>

#include <deque>
#include <limits>
#include <unordered_set>

namespace MinSG {
namespace PathTracing {

static const float bias = 1e-4f;
	
using namespace TriangleTrees;
using namespace RayCasting;
using namespace Rendering;

typedef RayCaster<float>::intersection_t Intersection_t;
typedef RayCaster<float>::intersection_packet_t IntersectionPacket_t;
typedef std::vector<Geometry::Ray3> RayVector_t;
typedef std::vector<Emitter> LightVector_t;
typedef LightParameters::lightType_t Light_t;

static const auto idTriangleTree = NodeAttributeModifier::create("TriangleTree", NodeAttributeModifier::PRIVATE_ATTRIBUTE);
typedef Util::WrapperAttribute<std::unique_ptr<const TriangleTree>> TriangleTreeWrapper_t;

static bool hasTriangleTree(GeometryNode * node) {
	return node->isAttributeSet(idTriangleTree);
}

static const TriangleTree * getTriangleTree(Node * node) {
	auto attr = node->getAttribute<TriangleTreeWrapper_t>(idTriangleTree);	
	return attr->get().get();
}

static void storeTriangleTree(Node * node, const TriangleTree * tree) {
	node->setAttribute(idTriangleTree, new TriangleTreeWrapper_t(tree));
}

struct SurfaceProperties {
	Geometry::Vec3 pos;
	Geometry::Vec3 normal;
	Geometry::Vec3 tangent;
	Geometry::Vec2 texcoord;
	Util::Color4f albedo;
	Util::Color4f emission;
};

struct Emitter {
	Emitter() {}
	Emitter(GeometryNode* geometry, LightNode* light, Util::Color4f emission) : 
		geometry(geometry), light(light), emission(emission) {}
	GeometryNode* geometry = nullptr;
	LightNode* light = nullptr;
	Util::Color4f emission;
};

std::pair<uint32_t, Geometry::Triangle_f> getTriangleAtPoint(GeometryNode* geoNode, const Geometry::Vec3& pos) {
	uint32_t tIdx = std::numeric_limits<uint32_t>::max();
	Geometry::Triangle_f triangle({0,0,0},{0,0,0},{0,0,0});
	float dist = 1;	
	
	const TriangleTree* triangleTree;
	if(!hasTriangleTree(geoNode)) {
		TriangleTrees::ABTreeBuilder treeBuilder(32, 0.5f);
		triangleTree = treeBuilder.buildTriangleTree(geoNode->getMesh());
		storeTriangleTree(geoNode, triangleTree);
	} else {
		triangleTree = getTriangleTree(geoNode);
	}
		
	// TODO: check normals
	std::deque<const TriangleTree*> queue{triangleTree};
	while(!queue.empty()) {
		const TriangleTree* node = queue.front();		
		queue.pop_front();
		
		for(uint32_t i=0; i<node->getTriangleCount(); ++i) {
			auto ta = node->getTriangle(i);
			float d = ta.getTriangle().distanceSquared(pos);
			if(d < dist) {
				dist = d;
				tIdx = ta.getTriangleIndex();
				triangle = ta.getTriangle();
			}
		}
		if(!node->isLeaf()) {
			for(const auto* child : node->getChildren()) {
				if(child->getBound().getDistanceSquared(pos) < bias) {
					queue.push_back(child);
				}
			}
		}
	}
	return std::make_pair(tIdx, triangle);
}

SurfaceProperties getSurfaceProperties(const Geometry::Ray3& ray, const Intersection_t& hit) {
	SurfaceProperties surface;
	if(!hit.first) {
		WARN("PathTracer: Invalid intersection.");
		return surface;
	}
	
	auto geoNode = hit.first;
	auto mesh = geoNode->getMesh();
	
	surface.pos = ray.getPoint(hit.second);
	Geometry::Vec3 localPos = Transformations::worldPosToLocalPos(*geoNode, surface.pos);
	uint32_t tIndex;
	Geometry::Triangle_f triangle({0,0,0},{0,0,0},{0,0,0});
	
	std::tie(tIndex, triangle) = getTriangleAtPoint(geoNode, localPos);
	
	if(tIndex > mesh->getPrimitiveCount()) {
		WARN("PathTracer: Could not find intersecting triangle.");
		return surface;
	}
	auto bc = triangle.calcBarycentricCoordinates(localPos);
	surface.normal = Transformations::localDirToWorldDir(*geoNode, triangle.calcNormal());
	surface.tangent = triangle.getEdgeAB().getNormalized();
	auto& vertexData = mesh->openVertexData();
	
	auto tAcc = MeshUtils::TriangleAccessor::create(mesh);
	uint32_t t0,t1,t2;
	std::tie(t0,t1,t2) = tAcc->getIndices(tIndex);	
	
	auto states = geoNode->getStates();
	MaterialState* mat = nullptr;
	TextureState* tex = nullptr;
	// TODO: normal texture
	for(uint32_t i=0; i<states.size(); ++i) {
		auto gs = dynamic_cast<GroupState*>(states[i]);
		auto ms = dynamic_cast<MaterialState*>(states[i]);
		auto ts = dynamic_cast<TextureState*>(states[i]);
		if(gs && gs->isActive()) {
			for(auto s : gs->getStates())
				states.push_back(s.get());
		} else if(ms && ms->isActive()) {
			mat = ms;
		} else if(ts && ts->isActive() && ts->getTextureUnit() == 0) {
			tex = ts;
		}
	}
	
	// sample texture
	if(mesh->getVertexDescription().hasAttribute(VertexAttributeIds::TEXCOORD0)) {
		auto tAcc = TexCoordAttributeAccessor::create(vertexData, VertexAttributeIds::TEXCOORD0);
		auto tc0 = tAcc->getCoordinate(t0);
		auto tc1 = tAcc->getCoordinate(t1);
		auto tc2 = tAcc->getCoordinate(t2);
		surface.texcoord = tc0 * bc.x() + tc1 * bc.y()  + tc2 * bc.z();
	}
	
	if(mat) {
		surface.albedo = mat->getParameters().getDiffuse();
		surface.emission = mat->getParameters().getEmission();
	} else if(mesh->getVertexDescription().hasAttribute(VertexAttributeIds::COLOR)) {
		auto cAcc = ColorAttributeAccessor::create(vertexData, VertexAttributeIds::COLOR);
		Util::Color4f col0 = cAcc->getColor4f(t0);
		Util::Color4f col1 = cAcc->getColor4f(t1);
		Util::Color4f col2 = cAcc->getColor4f(t2);		
		surface.albedo = col0 * bc.x() + col1 * bc.y()  + col2 * bc.z();
	} else {
		surface.albedo = Util::Color4f(1,1,1,1);
	}
	
	if(tex && tex->getTexture()->getLocalBitmap()) {
		auto pAcc = Util::PixelAccessor::create(tex->getTexture()->getLocalBitmap());
		// TODO: filtering
		surface.albedo *= pAcc->readColor4f(
			static_cast<int32_t>(surface.texcoord.x() * pAcc->getWidth()) % pAcc->getWidth(), 
			static_cast<int32_t>(surface.texcoord.y() * pAcc->getHeight()) % pAcc->getHeight());
	}	
	
	return surface;
}

PathTracer::PathTracer() {
	std::random_device r;
	setSeed(r());
	reset();
}


Geometry::Ray3 PathTracer::sampleHemisphere(const SurfaceProperties& surface, const Geometry::Ray3& ray) {
	Geometry::Ray3 outRay;
	outRay.setOrigin(surface.pos + surface.normal*bias);
	//const float reflMean = (surface.albedo.r() + surface.albedo.g() + surface.albedo.b()) / 3.0f;
	std::uniform_real_distribution<float> random(0, 1);
	//if(random(rng) < reflMean) {
		// cosine-weighted importance sample hemisphere
		const float _2pr1 = M_PI * 2.0 * random(rng);
		const float sr2 = std::sqrt(random(rng));
		// make coord frame coefficients (z in normal direction)
		const float x = std::cos(_2pr1) * sr2;
		const float y = std::sin(_2pr1) * sr2;
		const float z = std::sqrt(1.0f - (sr2 * sr2));
		//auto normal = surface.normal.dot(ray.getDirection()) >= 0 ? surface.normal : -surface.normal;
		
		outRay.setDirection({
			(surface.tangent * x) +
			(surface.normal.cross(surface.tangent) * y) +
			(surface.normal * z)
		});
	//}
	return outRay;
}

std::pair<Geometry::Ray3,float> getLightRay(LightNode* light, const SurfaceProperties& surface) {
	Geometry::Ray3 lightRay;
	float dist;
	auto lightPos = light->getWorldOrigin();
	auto lightDir = Transformations::localDirToWorldDir(*light, {0,0,-1});
	auto surfaceToLight = light->getType() == Light_t::DIRECTIONAL ? -lightDir : (lightPos - surface.pos).getNormalized();	
	float cutoff = std::cos(Geometry::Convert::degToRad(light->getCutoff()));
	if(surfaceToLight.dot(surface.normal) >= 0 && (light->getType() != Light_t::SPOT || lightDir.dot(-surfaceToLight) > cutoff)) {
		lightRay.setDirection(surfaceToLight);
		lightRay.setOrigin(surface.pos + surfaceToLight*bias);		
	} 
	if(light->getType() == Light_t::DIRECTIONAL) {
		Geometry::Plane plane(light->getWorldOrigin(), Transformations::localDirToWorldDir(*light, {0,0,-1}));
		dist = plane.planeTest(surface.pos);
	} else {
		dist = surface.pos.distance(light->getWorldOrigin());
	}
	return std::make_pair(lightRay, dist);
}

std::pair<Geometry::Ray3,float> sampleGeometryRay(GeometryNode* geo, const SurfaceProperties& surface, std::default_random_engine& rng) {
	Geometry::Ray3 lightRay;
	float dist;
	std::uniform_real_distribution<float> random(0,1);
	auto bb = geo->getWorldBB();
	auto pos = bb.getMin() + Geometry::Vec3(random(rng)*bb.getExtentX(),random(rng)*bb.getExtentY(),random(rng)*bb.getExtentZ());
	auto surfaceToLight = (pos - surface.pos).getNormalized();
	if(surfaceToLight.dot(surface.normal) >= 0) {
		lightRay.setDirection(surfaceToLight);
		lightRay.setOrigin(surface.pos + surfaceToLight*bias);
		dist = surface.pos.distance(pos);
	}		
	return std::make_pair(lightRay, dist);
}

Util::Color4f PathTracer::sampleDirectLight(GroupNode* scene, const std::vector<Emitter>& lights, const Geometry::Ray3& ray, const SurfaceProperties& surface) {
  static RayCaster<float> rayCaster;
	Util::Color4f radiance;
	Geometry::Ray3 lightRay;
	float lightDist;
	uint32_t startIdx = 0;
	
	// random direct light ray
	std::uniform_int_distribution<uint32_t> randomLight(0, lights.size()-1);
	startIdx = randomLight(rng);
	Emitter emitter;
		
	for(uint32_t i=0; i<lights.size() && lightRay.getDirection().isZero(); ++i) {
		emitter = lights[(startIdx + i)%lights.size()];
		if(emitter.light)
			std::tie(lightRay,lightDist) = getLightRay(emitter.light, surface);
		else if(emitter.geometry)
			std::tie(lightRay,lightDist) = sampleGeometryRay(emitter.geometry, surface, rng);
	}
	
	if(!lightRay.getDirection().isZero()) {
		auto hit = rayCaster.castRays(scene, {lightRay}).front();
		float attenuation = 0;
		if(emitter.light) {
			auto light = emitter.light;
			if(lightDist <= hit.second) {
				attenuation = 1.0f / (light->getConstantAttenuation() + light->getLinearAttenuation() * lightDist + light->getQuadraticAttenuation() * lightDist * lightDist);
			}
		} else if(emitter.geometry && emitter.geometry == hit.first) {
			auto triangle = getTriangleAtPoint(emitter.geometry, lightRay.getOrigin() + lightRay.getDirection()*hit.second).second;
			auto normal = Transformations::localDirToWorldDir(*emitter.geometry, triangle.calcNormal());
			float cosArea = normal.dot(-lightRay.getDirection());// * triangle.calcArea() * emitter.geometry->getWorldTransformationSRT().getScale();
			attenuation = cosArea / lightDist * lightDist;
		}
		radiance += emitter.emission * attenuation * surface.albedo * std::abs(lightRay.getDirection().dot(surface.normal)) / M_PI;
	}
		
	return radiance;
}

Util::Color4f PathTracer::getRadiance(GroupNode* scene, const std::vector<Emitter>& lights, const Geometry::Ray3& ray, uint32_t bounce) {	
  static RayCaster<float> rayCaster;
	Util::Color4f radiance;
	auto hit = rayCaster.castRays(scene, {ray}).front();
	if(!hit.first)
		return radiance;
		
	SurfaceProperties surface = getSurfaceProperties(ray, hit);
	
	// local emission for first hit
	//if(bounce == 0) {
		if(surface.normal.dot(-ray.getDirection()) > 0) // only emit in surface normal direction
			radiance = surface.emission;
	//}
	
	// get incoming direct light
	radiance += sampleDirectLight(scene, lights, ray, surface);
	
	if(bounce >= maxBounces)
		return radiance;
	
	// get reflection ray
	const float reflMean = 1;//(surface.albedo.r() + surface.albedo.g() + surface.albedo.b()) / 3.0f;
	auto reflRay = sampleHemisphere(surface, ray);
	if(!reflRay.getDirection().isZero()) {
		radiance += surface.albedo * reflMean * getRadiance(scene, lights, reflRay, bounce+1);
	}
	
	return radiance;
}

void PathTracer::trace(AbstractCameraNode* camera, GroupNode* scene, Util::PixelAccessor& frameBuffer) {
	
	// build helper structures
	for(auto node : MinSG::collectNodes<GeometryNode>(scene)) {		
		if(!hasTriangleTree(node)) {
			TriangleTrees::ABTreeBuilder treeBuilder(32, 0.5f);
			auto triangleTree = treeBuilder.buildTriangleTree(node->getMesh());
			storeTriangleTree(node, triangleTree);
		} 
	}		
	
	// collect lights
	LightVector_t lights;
	if(!emittersOnly) {
		std::unordered_set<LightNode*> lightSet;
		if(useGlobalLight) {
			for(auto state : collectStatesUpwards<LightingState>(scene)) {
				if(state->isActive() && state->getEnableLight())
					lightSet.emplace(state->getLight());
			}
		}
		for(auto state : collectStates<LightingState>(scene)) {
			if(state->isActive() && state->getEnableLight())
				lightSet.emplace(state->getLight());
		}
		for(auto light : lightSet) {
			lights.emplace_back(nullptr, light, light->getDiffuseLightColor());
		}
	}
	for(auto geoNode : collectNodes<GeometryNode>(scene)) {
		auto states = geoNode->getStates();		
		for(uint32_t i=0; i<states.size(); ++i) {
			auto gs = dynamic_cast<GroupState*>(states[i]);
			auto ms = dynamic_cast<MaterialState*>(states[i]);
			if(gs && gs->isActive()) {
				for(auto s : gs->getStates())
					states.push_back(s.get());
			} else if(ms && ms->isActive()) {
				auto emission = ms->getParameters().getEmission();
				if((emission.r() + emission.g() + emission.b()) > 0) {
					lights.emplace_back(geoNode, nullptr, emission);
					break;
				}
			} 
		}
	}
  
  // generate primary rays
	//std::uniform_real_distribution<float> jitter(-0.5f,0.5f);
	std::normal_distribution<float> jitter(0.0f,0.25f);
  RayVector_t rays;
  for(uint32_t y=0; y<frameBuffer.getHeight(); ++y) {
    for(uint32_t x=0; x<frameBuffer.getWidth(); ++x) {
      auto worldToScreen = camera->getFrustum().getProjectionMatrix() * camera->getWorldTransformationMatrix().inverse();
      Geometry::Vec3 win = antiAliasing ? Geometry::Vec3(x + jitter(rng),y + jitter(rng),0) : Geometry::Vec3(x,y,0);
      Geometry::Rect vp(camera->getViewport());
      Geometry::Vec3 target = Geometry::unProject<float>(win, worldToScreen, vp);
      
      Geometry::Ray3 ray;
      ray.setOrigin(camera->getWorldOrigin());
      ray.setDirection((target - camera->getWorldOrigin()).getNormalized());
      rays.push_back(ray);
    }
  }
	
  for(uint32_t y=0; y<frameBuffer.getHeight(); ++y) {
    for(uint32_t x=0; x<frameBuffer.getWidth(); ++x) {
      uint32_t index = y*frameBuffer.getWidth() + x;
			auto oldColor = frameBuffer.readColor4f(x, y);
			Util::Color4f radiance = getRadiance(scene, lights, rays[index], 0);
			Util::Color4f accum(radiance, oldColor, spp/(spp+1.0f));
			frameBuffer.writeColor(x, y, accum);				
    }
  }
	++spp;
}

void PathTracer::reset() {
  spp = 0;
	rng.seed(seed);
}

}
}

#endif /* MINSG_EXT_PATHTRACING */