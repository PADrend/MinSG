/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelAnalysis.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Transformations.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Box.h>
#include <Geometry/Plane.h>
#include <Geometry/Tools.h>

#include <algorithm>

namespace MinSG{
namespace BlueSurfels {
	
static const Util::StringIdentifier SURFEL_ATTRIBUTE("surfels");
static const Util::StringIdentifier SURFEL_PACKING_ATTRIBUTE("surfelPacking");
static const Util::StringIdentifier SURFEL_SURFACE_ATTRIBUTE("surfelSurface");
static const Util::StringIdentifier SURFEL_MEDIAN_ATTRIBUTE("surfelMedianDist");

std::vector<float> getProgressiveMinimalMinimalVertexDistances(Rendering::Mesh& mesh){

	Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor(Rendering::PositionAttributeAccessor::create(mesh.openVertexData(), Rendering::VertexAttributeIds::POSITION));
	
	Geometry::Box bb = mesh.getBoundingBox();
	bb.resizeRel( 1.01 );
	
	typedef Geometry::Point<Geometry::Vec3> P;
	Geometry::PointOctree<P> octree(bb,bb.getExtentMax()*0.01,8);
	
	float currentClosestDistance = bb.getDiameter();
	
	const size_t endIndex = mesh.getVertexCount();

	std::vector<float> progressiveClosestDistances;
	progressiveClosestDistances.reserve(endIndex-1);
	
	std::deque<P> closestNeighbors;
	for(size_t vIndex = 0 ; vIndex<endIndex; ++vIndex){
		const auto pos = positionAccessor->getPosition(vIndex);
	
		closestNeighbors.clear();
		octree.getClosestPoints(pos, 1, closestNeighbors);
			
		if(!closestNeighbors.empty()){
			const float d = pos.distance(closestNeighbors.front().getPosition());
			if(d<currentClosestDistance)
				currentClosestDistance = d;
			progressiveClosestDistances.push_back(currentClosestDistance);
		}
			
		octree.insert( P(pos) );
	}
	
	
	return progressiveClosestDistances;

}

std::vector<float> getMinimalVertexDistances(Rendering::Mesh& mesh,size_t prefixLength){

	Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor(Rendering::PositionAttributeAccessor::create(mesh.openVertexData(), Rendering::VertexAttributeIds::POSITION));
	
	Geometry::Box bb = mesh.getBoundingBox();
	bb.resizeRel( 1.01 );
	
	class P : public Geometry::Point<Geometry::Vec3>{
	public:
		size_t vIndex;
		P(Geometry::Vec3 p,size_t _vIndex) : Geometry::Point<Geometry::Vec3>( std::move(p)),vIndex(_vIndex){}
	};
	Geometry::PointOctree<P> octree(bb,bb.getExtentMax()*0.01,8);
	
	const size_t endIndex = std::min(static_cast<size_t>(mesh.getVertexCount()),prefixLength);

	for(size_t vIndex = 0 ; vIndex<endIndex; ++vIndex)
		octree.insert( P(positionAccessor->getPosition(vIndex),vIndex) );
	
		
	std::vector<float> closestDistances;
	closestDistances.reserve(endIndex-1);
	
	std::deque<P> closestNeighbors;
	for(size_t vIndex = 0 ; vIndex<endIndex; ++vIndex){
		const auto pos = positionAccessor->getPosition(vIndex);
		closestNeighbors.clear();
		octree.getClosestPoints(pos, 2, closestNeighbors); // this point and closest neighbor
		if(closestNeighbors.size()==2){ // get other point
			if(closestNeighbors[0].vIndex==vIndex)
				closestDistances.push_back(pos.distance(closestNeighbors[1].getPosition()));
			else
				closestDistances.push_back(pos.distance(closestNeighbors[0].getPosition()));
		}
	}
	return closestDistances;
}

float getMedianOfNthClosestNeighbours(Rendering::Mesh& mesh, size_t prefixLength, size_t nThNeighbour){
	if(mesh.getVertexCount()<=nThNeighbour)
		return 0;
	
	auto positionAccessor = Rendering::PositionAttributeAccessor::create(mesh.openVertexData(), Rendering::VertexAttributeIds::POSITION);
	const size_t endIndex = std::min(static_cast<size_t>(mesh.getVertexCount()),prefixLength);
	
	
	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
		size_t surfelId;
		OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelId(i) {}
	};

	const auto bb = mesh.getBoundingBox();
	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,8);
	
	std::deque<OctreeEntry> closestNeighbours;
	for(size_t vIndex = 0; vIndex<endIndex; ++vIndex)
		octree.insert( OctreeEntry(vIndex, positionAccessor->getPosition(vIndex)) );

	std::vector<float> nThClosestDistances;

	std::vector<float> distances;
	for(size_t vIndex = 0; vIndex<endIndex; ++vIndex){
		distances.clear();
		
		const auto pos = positionAccessor->getPosition(vIndex);
	
		closestNeighbours.clear();
		octree.getClosestPoints(pos, nThNeighbour+1, closestNeighbours);

		for(const auto& neighbour : closestNeighbours)
			distances.push_back( neighbour.getPosition().distance( pos ) );
		
		std::sort(distances.begin(), distances.end());
		nThClosestDistances.push_back( distances[nThNeighbour] );
	}

	std::sort(nThClosestDistances.begin(), nThClosestDistances.end());
	return nThClosestDistances[ nThClosestDistances.size()*0.5 ];
}

float getMeterPerPixel(AbstractCameraNode* camera, MinSG::Node * node) {
  static const Geometry::Vec3 Z_AXIS(0,0,1);
	const Geometry::Rect viewport(camera->getViewport());
	// get approximate radius of bounding sphere
	const float bs_radius = 0.0f;//node->getWorldBB().getExtentMax() * 0.5f;
  const auto cam_pos_ws = camera->getWorldOrigin();
  const auto node_pos_ws = node->getWorldBB().getClosestPoint(cam_pos_ws);//node->getWorldBB().getCenter();
  float dist_ws = (cam_pos_ws - node_pos_ws).dot(camera->getWorldTransformationSRT().getDirVector());
  dist_ws = camera->getFrustum().isOrthogonal() ? 1 : std::max(camera->getNearPlane(), dist_ws - camera->getNearPlane() - bs_radius);
  
  const auto l = camera->getFrustum().getLeft();
  const auto r = camera->getFrustum().getRight();
  const auto n = camera->getNearPlane();
  
	// compute 1m vector in clipping space located at node
  const float one_meter_ss = 2*n / ((r-l) * dist_ws);
	const float scale = node->getWorldTransformationSRT().getScale();
	const float pixel_per_meter = one_meter_ss * viewport.getWidth() * 0.5 * scale;
	return pixel_per_meter > 0 ? 1.0f/pixel_per_meter : std::numeric_limits<float>::max();
}

float computeSurfelPacking(Rendering::Mesh* mesh) {
	if(!mesh) return 0;
	uint32_t count = std::min(1000u, mesh->getVertexCount());
	//float r = getMedianOfNthClosestNeighbours(*mesh, count, 1);
	auto dist = getMinimalVertexDistances(*mesh, count);
	float r = *std::min_element(dist.begin(), dist.end());
  return r * r * static_cast<float>(count);
}

float getSurfelPacking(MinSG::Node* node, Rendering::Mesh* surfels) {
	if(!surfels)
		return 0;
	if(node->isInstance())
		node = node->getPrototype();
		
  auto surfelPackingAttr = node->findAttribute(SURFEL_PACKING_ATTRIBUTE);
	if(surfelPackingAttr)
    return surfelPackingAttr->toFloat();
		
	auto surfelSurfaceAttr = node->findAttribute(SURFEL_SURFACE_ATTRIBUTE);
	if(surfelSurfaceAttr) {
		float packing = surfelSurfaceAttr->toFloat();
		node->unsetAttribute(SURFEL_SURFACE_ATTRIBUTE);
		node->setAttribute(SURFEL_PACKING_ATTRIBUTE, Util::GenericAttribute::createNumber(packing));
		return packing;
	}
	
	// try to find deprecated 'surfelMedianDist' attribute & compute surface 
	auto surfelMedianAttr = node->findAttribute(SURFEL_MEDIAN_ATTRIBUTE);
	if(surfelMedianAttr) {
		uint32_t medianCount = std::min(1000U, surfels->getVertexCount());
		float median = surfelMedianAttr->toFloat();
		float packing = static_cast<float>(medianCount) * median * median;  
		node->unsetAttribute(SURFEL_MEDIAN_ATTRIBUTE);
		node->setAttribute(SURFEL_PACKING_ATTRIBUTE, Util::GenericAttribute::createNumber(packing));
		return packing;
	}

	float packing = computeSurfelPacking(surfels);
	node->setAttribute(SURFEL_PACKING_ATTRIBUTE, Util::GenericAttribute::createNumber(packing));
	return packing;
}

Rendering::Mesh* getSurfels(MinSG::Node * node) {
	auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));
	return surfelAttribute ? surfelAttribute->get() : nullptr;
}

}
}
#endif // MINSG_EXT_BLUE_SURFELS
