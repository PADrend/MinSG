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
#include "Light.h"
#include "SurfacePoint.h"

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
#include <string.h>
#include <iomanip>

//#define LOGGING

#ifdef LOGGING
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(msg) std::cout << std::setprecision(5) << __FILENAME__ << ":" << __LINE__ << ":" << __func__ << ": " << msg << std::endl;
#else
#define LOG(msg) 
#endif

namespace MinSG {
namespace PathTracing {

static const float bias = 1e-4f;
	
using namespace TriangleTrees;
using namespace RayCasting;
using namespace Rendering;

static const auto idSolidGeoTree = NodeAttributeModifier::create("SolidGeoTree", NodeAttributeModifier::PRIVATE_ATTRIBUTE);
static const auto idTriangleTree = NodeAttributeModifier::create("TriangleTree", NodeAttributeModifier::PRIVATE_ATTRIBUTE);
typedef Util::WrapperAttribute<std::unique_ptr<const TriangleTree>> TriangleTreeWrapper_t;

//static bool hasTriangleTree(GeometryNode * node) {
//	return node->isAttributeSet(idTriangleTree);
//}

/*static const TriangleTree * getTriangleTree(Node * node) {
	auto attr = node->getAttribute<TriangleTreeWrapper_t>(idTriangleTree);	
	return attr->get().get();
}*/

static void storeTriangleTree(Node * node, const TriangleTree * tree) {
	node->setAttribute(idTriangleTree, new TriangleTreeWrapper_t(tree));
}

static inline bool isBlack(const Util::Color4f& color) {
	return color.r() == 0 && color.g() == 0 && color.b() == 0;
}


class Tile {
public:
	static const uint32_t SIZE = 64;
  Geometry::Vec2i offset;
	uint32_t spp = 0;
  std::array<std::array<Util::Color4f, SIZE>, SIZE> data;
};

PathTracer::PathTracer() {
	std::random_device r;
	setSeed(r());
}

PathTracer::~PathTracer() = default;  

Util::Color4f PathTracer::sampleLight(const Geometry::Ray3& ray, const SurfacePoint& surface) {
  static RayCaster<float> rayCaster;
	Util::Color4f radiance;
	
	// random light
	// TODO: better light importance sampling
	Light* light = sceneLights[sample1D()*sceneLights.size()].get();
	auto sample = sample3D();
	auto lightSample = light->sampleIncidentRadiance(surface, sample);
	LOG("Estimate direct: " << sample << " -> Li: " << lightSample.l << ", wi: " << lightSample.wi << ", pdf: " << lightSample.pdf);
	
	if(!isBlack(lightSample.l) && lightSample.pdf > 0) {
		
		auto bsdf = surface.getBSDF(-ray.getDirection(), lightSample.wi);
		bsdf.f *= std::abs(lightSample.wi.dot(surface.normal));
		LOG("surf f*dot: " << bsdf.f << " pdf: " << bsdf.pdf << " normal: " << surface.normal);
		
		if(!isBlack(bsdf.f)) {
			// test visibility
			Geometry::Ray3 lightRay(surface.pos + surface.normal * bias, lightSample.wi);
			auto hit = rayCaster.castRays(scene.get(), {lightRay}).front();
			if(hit.second < lightSample.dist - bias) {
				// Light source is not visible
				lightSample.l.set(0, 0, 0, 1);
				LOG("  shadow ray blocked");
			} else {				
				LOG("  shadow ray unoccluded");
			}
			
			if(!isBlack(lightSample.l)) {
				if(light->isDeltaLight()) {
					radiance += bsdf.f * lightSample.l / lightSample.pdf;
				} else {
					float weight = (lightSample.pdf * lightSample.pdf) / ( bsdf.pdf * bsdf.pdf + lightSample.pdf * lightSample.pdf);
					radiance += bsdf.f * weight * lightSample.l / lightSample.pdf;
					LOG("Ld: " << radiance << ", weight: " << weight);
				}
			}
		}		
	}
	
  // Sample BSDF with multiple importance sampling
	/*if(!light->isDeltaLight()) {
		auto bsdf = surface.sample(-ray.getDirection(), sample2D());
		bsdf.f *= std::abs(bsdf.wi.dot(surface.normal));
		LOG("  BSDF / phase sampling f: " << bsdf.f << ", scatteringPdf: " << bsdf.pdf);
		if(!isBlack(bsdf.f) && bsdf.pdf > 0) {
			float weight = (lightSample.pdf * lightSample.pdf) / ( bsdf.pdf * bsdf.pdf + lightSample.pdf * lightSample.pdf);
			
		}
	}*/
		
	return radiance * sceneLights.size();
}

Util::Color4f PathTracer::getRadiance(const Geometry::Ray3& primaryRay) {	
  static RayCaster<float> rayCaster;
	Util::Color4f radiance(0,0,0,0), beta(1,1,1,1);
	Geometry::Ray3 ray(primaryRay);	
		
	for(uint32_t bounces = 0; bounces <= maxBounces; ++bounces) {	
		LOG("bounce " << bounces << ", current L = " << radiance << ", beta = " << beta << ", ray = (" << ray.getOrigin() << ", " << ray.getDirection() << ")");
		
		auto hit = rayCaster.castRays(scene.get(), {ray}).front();
		if(!hit.first)
			break;
			
		auto surface = SurfacePoint::getSurfaceAt(hit.first, ray.getPoint(hit.second));

		// local emission for first hit
		if(bounces == 0) {
			if(surface.normal.dot(-ray.getDirection()) > 0) {
				// only emit in surface normal direction
				radiance += beta * surface.emission;
				LOG("Added Le -> L = " << radiance);
			}
		}
		
		// get incoming direct light
		auto Ld = beta * sampleLight(ray, surface);
		LOG("Sampled direct lighting Ld = " << Ld);
		radiance += Ld;
		
		// sample BSDF
		auto bsdf = surface.sampleBSDF(-ray.getDirection(), sample2D());
		LOG("Sampled BSDF, f = " << bsdf.f << ", pdf = " << bsdf.pdf << " surface = " << surface.albedo);
		if(isBlack(bsdf.f) || bsdf.pdf == 0)
			break;
		beta *= bsdf.f * std::abs(bsdf.wi.dot(surface.normal)) / bsdf.pdf;
		LOG("Updated beta = " << beta);
			
		// create reflection ray
		ray.setOrigin(surface.pos + surface.normal * bias);
		ray.setDirection(bsdf.wi);
		
		// TODO: terminate path with russian roulette
	}
	
	return radiance;
}

void PathTracer::download(Util::PixelAccessor& image, float gamma) {
	if(finished) {
		// wait for threads when finished
		condition.notify_all();
		for(auto& t : threads) t.join();
		threads.clear();
	}
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		
		for(auto& tile : tileQueue) {
			for(uint32_t y = 0; y < Tile::SIZE; ++y) {
				for(uint32_t x = 0; x < Tile::SIZE; ++x) {
					auto pixel = tile->data[x][y];
					if(tile->spp > 0) {
						pixel /= tile->spp;
						pixel.a(1);
					} else {
						pixel.set(0,0,0,0);
					}
					
					// gamma correction
					if(gamma > 0) {
						pixel.r(std::pow(pixel.r(),1.0/gamma));
						pixel.g(std::pow(pixel.g(),1.0/gamma));
						pixel.b(std::pow(pixel.b(),1.0/gamma));
					}
					
					Geometry::Vec2i imageCoords = tile->offset + Geometry::Vec2i(x,y);
					if(imageCoords.x() < image.getWidth() && imageCoords.y() < image.getHeight())
						image.writeColor(imageCoords.x(), imageCoords.y(), pixel);
				}
			}
		}
	}
}

void PathTracer::start() {
	if(scene.isNull()) {
		WARN("PathTracer: PathTracer has no scene.");
		return;
	}
	if(camera.isNull()) {
		WARN("PathTracer: PathTracer has no camera.");
		return;
	}
	if(needsReset)
		reset();
	paused = false;
	
	if(threads.empty() || finished) {
		threads.clear();
		for(uint32_t i=0; i<threadCount; ++i)
			threads.emplace_back(std::thread(std::bind(&PathTracer::doWork, this)));
	}
	condition.notify_all();
}

void PathTracer::pause() {
	paused = true;
}

void PathTracer::reset() {
	needsReset = false;
	rng.seed(seed);
	if(scene.isNull()) {
		WARN("PathTracer: PathTracer has no scene.");
		return;
	}
	if(camera.isNull()) {
		WARN("PathTracer: PathTracer has no camera.");
		return;
	}
	
	// wait for all threads to finish
	finished = true;
	condition.notify_all();
	for(auto& t : threads) t.join();
	threads.clear();
	finishedCount = 0;
	spp = 0;
	
	// generate tiles in spiral pattern
	Geometry::Vec2i tileDim(std::ceil(resolution.x()/Tile::SIZE), std::ceil(resolution.y()/Tile::SIZE));
	uint32_t maxTileDim = std::max(tileDim.x(), tileDim.y()); 
	
	Geometry::Vec2i tileCoord(tileDim.x()/2, tileDim.y()/2);
	uint32_t steps = 1;
	Geometry::Vec2i dir(1,0);
	
	tileQueue.clear();
	while(std::max(tileCoord.x(), tileCoord.y()) <= maxTileDim) {
		for(uint32_t d=0;d<2;++d) {
			for(uint32_t s=0;s<steps;++s) {
				// Create Tile
				Geometry::Vec2i offset = Geometry::Vec2(tileCoord) * Tile::SIZE;
				if(offset.x() < resolution.x() && offset.y() < resolution.y()) {
					auto tile = new Tile;
					tile->offset = offset;
					tileQueue.emplace_back(tile);
				}
				// move
				tileCoord += dir;
			}
			// rotate right
			dir.setValue(dir.y(), -dir.x());
		}
		++steps;
	}
	tileCount = tileQueue.size();
	
	finished = false;
}

void PathTracer::trace(Tile* tile) {	  
  // generate primary rays
  for(uint32_t ty=0; ty<Tile::SIZE; ++ty) {
    for(uint32_t tx=0; tx<Tile::SIZE; ++tx) {
			uint32_t x = tile->offset.x() + tx;
			uint32_t y = tile->offset.y() + ty;
			auto sample = sample3D();
			sample.z(0);
      auto worldToScreen = camera->getFrustum().getProjectionMatrix() * camera->getWorldTransformationMatrix().inverse();
      Geometry::Vec3 win = Geometry::Vec3(x,y,0) + (antiAliasing ? sample : Geometry::Vec3(0.5f,0.5f,0));
      Geometry::Rect vp(camera->getViewport());
      Geometry::Vec3 target = Geometry::unProject<float>(win, worldToScreen, vp);
      
      Geometry::Ray3 ray;
      ray.setOrigin(camera->getWorldOrigin());
      ray.setDirection((target - camera->getWorldOrigin()).getNormalized());
			
			Util::Color4f radiance = getRadiance(ray);
			tile->data[tx][ty] += radiance;
    }
  }
	
	++tile->spp;	
	if(tile->spp > maxSamples) {
		++finishedCount;
		if(finishedCount >= tileCount) {
			finished = true;
			condition.notify_all();
		}
		return;
	}
	auto tmp = tile->spp-1;
	spp.compare_exchange_strong(tmp, tmp+1);
}

void PathTracer::doWork() {
	while(true) {
		std::unique_ptr<Tile> tile;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			while(!finished && paused && tileQueue.empty()) {
				condition.wait(lock);
			}
			if(finished)
				return;
				
			tile = std::move(tileQueue.front());
			tileQueue.pop_front();
		}
		
		trace(tile.get());
		
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			tileQueue.emplace_back(std::move(tile));
		}
		condition.notify_one();
	}
}

void PathTracer::setCamera(AbstractCameraNode* camera_) {
	camera = camera_;
	needsReset = true;
}

void PathTracer::setScene(GroupNode* scene_) {
  static RayCaster<float> rayCaster;
  scene = scene_;
	needsReset = true;
	
	// create lights from scene
	sceneLights.clear();
	std::unordered_set<LightNode*> lightSet;
	
	// find global light sources
	if(useGlobalLight) {
		for(auto state : collectStatesUpwards<LightingState>(scene.get())) {
			if(state->isActive() && state->getEnableLight())
				lightSet.emplace(state->getLight());
		}
	}
	// find active MinSG light nodes
	for(auto state : collectStates<LightingState>(scene.get())) {
		if(state->isActive() && state->getEnableLight())
			lightSet.emplace(state->getLight());
	}
	for(auto light : lightSet) {
		sceneLights.emplace_back(new MinSGLight(light));
	}
	
	// find geometry nodes with emissive material
	for(auto geoNode : collectNodes<GeometryNode>(scene.get())) {
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
					sceneLights.emplace_back(new DiffuseAreaLight(geoNode, emission));
					break;
				}
			} 
		}
	}
	
	// build helper structures
	scene->unsetAttribute(idSolidGeoTree);
	rayCaster.castRays(scene.get(), {});
	for(auto node : MinSG::collectNodes<GeometryNode>(scene.get())) {		
		scene->unsetAttribute(idTriangleTree);
		//if(!hasTriangleTree(node)) {
			TriangleTrees::ABTreeBuilder treeBuilder(32, 0.5f);
			auto triangleTree = treeBuilder.buildTriangleTree(node->getMesh());
			storeTriangleTree(node, triangleTree);
		//} 
	}	
	
	// TODO: ensure that all mesh & texture data is downloaded
}

float PathTracer::sample1D() {
	static std::uniform_real_distribution<float> sampler(0,1);
	return sampler(rng);
}

Geometry::Vec2 PathTracer::sample2D() {
	static std::uniform_real_distribution<float> sampler(0,1);
	return {sampler(rng), sampler(rng)};
	//return {0.5, 0.5};
}

Geometry::Vec3 PathTracer::sample3D() {
	static std::uniform_real_distribution<float> sampler(0,1);
	return {sampler(rng), sampler(rng), sampler(rng)};
	//return {0.5, 0.5, 0.5};
}

}
}

#endif /* MINSG_EXT_PATHTRACING */