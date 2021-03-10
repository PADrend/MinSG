/*
	This file is part of the MinSG library extension PathTracing.
	Copyright (C) 2017 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PATHTRACING

#include "PathTracer.h"
#include "Light.h"
#include "SurfacePoint.h"
#include "SurfacePoint.h"
#include "Material.h"
#include "ExtTriangle.h"
#include "RayCaster.h"
#include "TreeBuilder.h"

#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/States/LightingState.h"
#include "../../Core/States/GroupState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/Transformations.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../TriangleTrees/SolidTree.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/TriangleAccessor.h>
#include <Rendering/MeshUtils/LocalMeshDataHolder.h>
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

#include <Util/References.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/GenericAttribute.h>
#include <Util/StringIdentifier.h>
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <Util/Macros.h>

#include <vector>
#include <deque>
#include <cstdint>
#include <random>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <unordered_set>
#include <string.h>
#include <iomanip>

//#define LOGGING 2

#ifdef LOGGING
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(level, msg) if(level <= LOGGING) { \
	std::stringstream ss; \
	ss << std::setprecision(5) << __FILENAME__ << ":" << __LINE__ << ":" << __func__ << ":" << std::this_thread::get_id() << ": " << msg << std::endl; \
	std::cout << ss.str(); \
}
#else
#define LOG(level, msg) 
#endif

namespace MinSG {
namespace PathTracing {
	
using namespace Rendering;

static const float bias = 1e-4f;

static inline bool isBlack(const Util::Color4f& color) {
	return color.r() == 0 && color.g() == 0 && color.b() == 0;
}

class Tile {
public:
	Tile(uint32_t tileSize) : tileSize(tileSize) { data.resize(tileSize*tileSize, {0,0,0,0}); } 
	void set(uint32_t x, uint32_t y, const Util::Color4f& value) { data[y*tileSize + x] = value; }
	void add(uint32_t x, uint32_t y, const Util::Color4f& value) { data[y*tileSize + x] += value; }
	Util::Color4f get(uint32_t x, uint32_t y) { return data[y*tileSize + x]; }
	uint32_t tileSize;
	uint32_t spp = 0;
	Geometry::Vec2i offset;
  std::vector<Util::Color4f> data;
};

class PathTracer::pimpl {
public:
	// ----------------------------------
	// Threading
	void start();
	void pause();
	void reset();  
	void doWork();
	std::atomic_bool paused;
	std::atomic_bool finished;
	std::atomic_uint finishedCount;
	std::atomic_uint spp;
	std::vector<std::thread> threads;
	std::mutex queueMutex;
	std::condition_variable condition;
	std::deque<std::unique_ptr<Tile>> tileQueue;
	uint32_t tileCount;
	uint32_t tileSize = 16;
	uint32_t threadCount = 8;
	
	// ----------------------------------
	// Path tracing	
	void trace(Tile* tile);
	Util::Color4f getRadiance(const Geometry::Ray3& primaryRay);  
	Util::Color4f sampleLight(const Geometry::Ray3& ray, const SurfacePoint& surface);
	void download( Util::PixelAccessor& image, float gamma);
	uint32_t maxSamples = 1024;
	uint32_t maxBounces = 4;
	bool useGlobalLight = true;
	bool antiAliasing = true;
	Geometry::Vec2 resolution;
	
	// ----------------------------------	
	// Scene
	void setScene(GroupNode* scene); 
	Util::Reference<GroupNode> scene;
	Util::Reference<AbstractCameraNode> camera;
	bool needsReset = true;
	std::vector<MeshUtils::LocalMeshDataHolder> meshDataHolders;
	std::vector<std::unique_ptr<Light>> sceneLights;
	std::vector<std::unique_ptr<Material>> materialLibrary;
	SolidTree_ExtTriangle triangleTree;
	
	// ----------------------------------
	// Sampling	
	float sample1D();
	Geometry::Vec2 sample2D();
	Geometry::Vec3 sample3D();
	uint32_t seed;
	std::default_random_engine rng;
};

/*******************************************************************************
 * Threading
 *******************************************************************************/
void PathTracer::pimpl::start() {
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
	
	auto tCount = std::max(std::min(threadCount, tileCount), 1U);
	if(threads.empty() || finished) {
		threads.clear();
		for(uint32_t i=0; i<tCount; ++i)
			threads.emplace_back(std::thread(std::bind(&PathTracer::pimpl::doWork, this)));
	}
	condition.notify_all();
}
//-------------------------------------------------------------------------

void PathTracer::pimpl::pause() {
	paused = true;
}
//-------------------------------------------------------------------------

void PathTracer::pimpl::reset() {
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
	Geometry::Vec2i tileDim(static_cast<int32_t>(std::ceil(resolution.x()/tileSize)), static_cast<int32_t>(std::ceil(resolution.y()/tileSize)));
	int32_t maxTileDim = std::max(tileDim.x(), tileDim.y()); 
	
	Geometry::Vec2i tileCoord(tileDim.x()/2, tileDim.y()/2);
	uint32_t steps = 1;
	Geometry::Vec2i dir(1,0);
	
	tileQueue.clear();
	while(std::max(tileCoord.x(), tileCoord.y()) <= maxTileDim) {
		for(uint32_t d=0;d<2;++d) {
			for(uint32_t s=0;s<steps;++s) {
				// Create Tile
				Geometry::Vec2i offset = Geometry::Vec2(tileCoord) * static_cast<float>(tileSize);
				if(offset.x() >= 0 && offset.y() >= 0 && offset.x() < resolution.x() && offset.y() < resolution.y()) {
					auto tile = new Tile(tileSize);
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
	tileCount = static_cast<uint32_t>(tileQueue.size());
	
	finished = false;
}
//-------------------------------------------------------------------------

void PathTracer::pimpl::doWork() {
	while(true) {
		std::unique_ptr<Tile> tile;
		{
			std::unique_lock<std::mutex> lock(queueMutex);
			while(!finished && (tileQueue.empty() || paused)) {
				LOG(1,"wait for it... ");
				condition.wait(lock);
				LOG(1,"wake up ");
			}
			if(finished)
				return;
			LOG(1,"Take one " << !tileQueue.empty());
				
			tile = std::move(tileQueue.front());
			tileQueue.pop_front();
			LOG(1,"pop " << tile->offset);
		}
		
		trace(tile.get());
		
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			LOG(1,"put back " << tile->offset);
			tileQueue.emplace_back(std::move(tile));
		}
		condition.notify_one();
	}
}
//-------------------------------------------------------------------------
 
/*******************************************************************************
 * Path tracing
 *******************************************************************************/
 
void PathTracer::pimpl::trace(Tile* tile) {	
	// generate primary rays
	for(uint32_t ty=0; ty<tileSize; ++ty) {
		for(uint32_t tx=0; tx<tileSize; ++tx) {
			uint32_t x = tile->offset.x() + tx;
			uint32_t y = tile->offset.y() + ty;
			if(x >= resolution.x() || y >= resolution.y())
				continue;
			auto sample = sample3D();
			sample.z(0);
			auto worldToScreen = camera->getFrustum().getProjectionMatrix() * camera->getWorldTransformationMatrix().inverse();
			Geometry::Vec3 win = Geometry::Vec3(static_cast<float>(x),static_cast<float>(y),0) + (antiAliasing ? sample : Geometry::Vec3(0.5f,0.5f,0));
			Geometry::Rect vp(camera->getViewport());
			Geometry::Vec3 target = Geometry::unProject<float>(win, worldToScreen, vp);

			Geometry::Ray3 ray;
			ray.setOrigin(camera->getWorldOrigin());
			ray.setDirection((target - camera->getWorldOrigin()).getNormalized());

			Util::Color4f radiance = getRadiance(ray);
			tile->add(tx, ty, radiance);
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
//-------------------------------------------------------------------------

Util::Color4f PathTracer::pimpl::sampleLight(const Geometry::Ray3& ray, const SurfacePoint& surface) {
  static RayCaster<float> rayCaster;
	Util::Color4f radiance;
	
	// random light
	// TODO: better light importance sampling
	Light* light = sceneLights[static_cast<size_t>(sample1D()*sceneLights.size())].get();
	auto sample = sample3D();
	auto lightSample = light->sampleIncidentRadiance(surface, sample);
	LOG(2,"Estimate direct: " << sample << " -> Li: " << lightSample.l << ", wi: " << lightSample.wi << ", pdf: " << lightSample.pdf);
	
	if(!isBlack(lightSample.l) && lightSample.pdf > 0) {
		
		auto bsdf = surface.getBSDF(-ray.getDirection(), lightSample.wi);
		bsdf.f *= std::abs(lightSample.wi.dot(surface.normal));
		LOG(2,"surf f*dot: " << bsdf.f << " pdf: " << bsdf.pdf << " normal: " << surface.normal);
		
		if(!isBlack(bsdf.f)) {
			// test visibility
			Geometry::Ray3 lightRay(surface.pos + surface.normal * bias, lightSample.wi);
			auto hit = rayCaster.castRays(triangleTree, {lightRay}, scene->getWorldBB()).front();			
			if(std::get<0>(hit) < lightSample.dist - bias) {
				// Light source is not visible
				lightSample.l.set(0, 0, 0, 1);
				LOG(2,"  shadow ray blocked");
			} else {				
				LOG(2,"  shadow ray unoccluded");
			}
			
			if(!isBlack(lightSample.l)) {
				if(light->isDeltaLight()) {
					radiance += bsdf.f * lightSample.l / lightSample.pdf;
				} else {
					float weight = (lightSample.pdf * lightSample.pdf) / ( bsdf.pdf * bsdf.pdf + lightSample.pdf * lightSample.pdf);
					radiance += bsdf.f * weight * lightSample.l / lightSample.pdf;
					LOG(2,"Ld: " << radiance << ", weight: " << weight);
				}
			}
		}
	}
	
  // Sample BSDF with multiple importance sampling
	/*if(!light->isDeltaLight()) {
		auto bsdf = surface.sample(-ray.getDirection(), sample2D());
		bsdf.f *= std::abs(bsdf.wi.dot(surface.normal));
		LOG(2,"  BSDF / phase sampling f: " << bsdf.f << ", scatteringPdf: " << bsdf.pdf);
		if(!isBlack(bsdf.f) && bsdf.pdf > 0) {
			float weight = (lightSample.pdf * lightSample.pdf) / ( bsdf.pdf * bsdf.pdf + lightSample.pdf * lightSample.pdf);
			
		}
	}*/
		
	return radiance * static_cast<float>(sceneLights.size());
}
//-------------------------------------------------------------------------

Util::Color4f PathTracer::pimpl::getRadiance(const Geometry::Ray3& primaryRay) {	
  static RayCaster<float> rayCaster;
	Util::Color4f radiance(0,0,0,0), beta(1,1,1,1);
	Geometry::Ray3 ray(primaryRay);	
		
	auto bounds = scene->getWorldBB();
	//auto diameter = bounds.getDiameter();
	
	for(uint32_t bounces = 0; bounces <= maxBounces; ++bounces) {	
		LOG(2,"bounce " << bounces << ", current L = " << radiance << ", beta = " << beta << ", ray = (" << ray.getOrigin() << ", " << ray.getDirection() << ")");
		
		float dist, u, v;
		ExtTriangle tri;
		std::tie(dist, u, v, tri) = rayCaster.castRays(triangleTree, {ray}, bounds).front();
		if(!tri.source)
			break;
			
		auto surface = tri.getSurfacePoint(u,v);

		// local emission for first hit
		if(bounces == 0) {
			if(surface.normal.dot(-ray.getDirection()) > 0) {
				// only emit in surface normal direction
				radiance += beta * surface.emission;
				LOG(2,"Added Le -> L = " << radiance);
			}
		}
		
		// get incoming direct light
		auto Ld = beta * sampleLight(ray, surface);
		LOG(2,"Sampled direct lighting Ld = " << Ld);
		radiance += Ld;
		
		// sample BSDF
		auto bsdf = surface.sampleBSDF(-ray.getDirection(), sample2D());
		LOG(2,"Sampled BSDF, f = " << bsdf.f << ", pdf = " << bsdf.pdf << " surface = " << surface.albedo);
		if(isBlack(bsdf.f) || bsdf.pdf == 0)
			break;
		beta *= bsdf.f * std::abs(bsdf.wi.dot(surface.normal)) / bsdf.pdf;
		LOG(2,"Updated beta = " << beta);
			
		// create reflection ray
		ray.setOrigin(surface.pos + surface.normal * bias);
		ray.setDirection(bsdf.wi);
		
		// TODO: terminate path with russian roulette
		LOG(2,"Radiance " << radiance);
	}
	
	return radiance;
}
//-------------------------------------------------------------------------

void PathTracer::pimpl::download(Util::PixelAccessor& image, float gamma) {
	if(finished) {
		// wait for threads when finished
		condition.notify_all();
		for(auto& t : threads) t.join();
		threads.clear();
	}
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		
		for(auto& tile : tileQueue) {
			for(uint32_t y = 0; y < tileSize; ++y) {
				for(uint32_t x = 0; x < tileSize; ++x) {
					auto pixel = tile->get(x,y);
					if(tile->spp > 0) {
						pixel /= static_cast<float>(tile->spp);
						pixel.a(1);
					} else {
						pixel.set(0,0,0,0);
					}
					
					// gamma correction
					if(gamma > 0) {
						pixel.r(static_cast<float>(std::pow(pixel.r(),1.0/gamma)));
						pixel.g(static_cast<float>(std::pow(pixel.g(),1.0/gamma)));
						pixel.b(static_cast<float>(std::pow(pixel.b(),1.0/gamma)));
					}
					
					Geometry::Vec2i imageCoords = tile->offset + Geometry::Vec2i(x,y);
					if(static_cast<uint32_t>(imageCoords.x()) < image.getWidth() && static_cast<uint32_t>(imageCoords.y()) < image.getHeight())
						image.writeColor(imageCoords.x(), imageCoords.y(), pixel);
				}
			}
		}
	}
}
//-------------------------------------------------------------------------

/*******************************************************************************
 * Scene
 *******************************************************************************/

void PathTracer::pimpl::setScene(GroupNode* scene_) {
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
 
 // build acceleration structure
 materialLibrary.clear();
 meshDataHolders.clear();
 triangleTree = buildSolidExtTree(scene.get(), materialLibrary);
 
 // ensure that all mesh & texture data is downloaded
 for(auto geoNode : collectNodes<GeometryNode>(scene.get())) {
	 //meshDataHolders.emplace_back(geoNode->getMesh());
 }
}
//-------------------------------------------------------------------------

/*******************************************************************************
 * Sampling
 *******************************************************************************/
 
float PathTracer::pimpl::sample1D() {
	static std::uniform_real_distribution<float> sampler(0,1);
	return sampler(rng);
}
//-------------------------------------------------------------------------

Geometry::Vec2 PathTracer::pimpl::sample2D() {
	static std::uniform_real_distribution<float> sampler(0,1);
	return {sampler(rng), sampler(rng)};
	//return {0.5, 0.5};
}
//-------------------------------------------------------------------------

Geometry::Vec3 PathTracer::pimpl::sample3D() {
	static std::uniform_real_distribution<float> sampler(0,1);
	return {sampler(rng), sampler(rng), sampler(rng)};
	//return {0.5, 0.5, 0.5};
} 
//-------------------------------------------------------------------------

/*******************************************************************************
 * PathTracer class
 *******************************************************************************/

PathTracer::PathTracer() : impl(new pimpl) {
	std::random_device r;
	setSeed(r());
}

PathTracer::~PathTracer() = default;  

void PathTracer::download(Util::PixelAccessor& image, float gamma) {
	impl->download(image, gamma);
}

void PathTracer::start() {
	impl->start();
}

void PathTracer::pause() {
	impl->pause();
}

void PathTracer::reset() {
	impl->reset();
}

void PathTracer::setScene(GroupNode* scene) {
	impl->setScene(scene);
}

void PathTracer::setCamera(AbstractCameraNode* camera) {
	impl->camera = camera;
	impl->needsReset = true;
}

void PathTracer::setMaxBounces(uint32_t maxBounces) { impl->maxBounces = maxBounces; impl->needsReset = true;}
void PathTracer::setSeed(uint32_t seed) { impl->seed = seed; impl->needsReset = true;}
void PathTracer::setUseGlobalLight(bool useGlobalLight) { impl->useGlobalLight = useGlobalLight; impl->needsReset = true; }
void PathTracer::setAntiAliasing(bool antiAliasing) { impl->antiAliasing = antiAliasing; impl->needsReset = true; }
void PathTracer::setResolution(const Geometry::Vec2i& resolution) { impl->resolution = resolution; impl->needsReset = true; }
void PathTracer::setMaxSamples(uint32_t maxSamples) { impl->maxSamples = maxSamples; impl->needsReset = true;}
void PathTracer::setThreadCount(uint32_t count) { impl->threadCount = count; impl->needsReset = true; }
void PathTracer::setTileSize(uint32_t size) { impl->tileSize = size; impl->needsReset = true; }
bool PathTracer::isFinished() const { return impl->finished; }
uint32_t PathTracer::getSamplesPerPixel() const { return impl->spp; }

}
}

#endif /* MINSG_EXT_PATHTRACING */