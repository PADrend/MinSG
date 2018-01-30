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


namespace MinSG{
namespace BlueSurfels {


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


float getMeterPerPixel(MinSG::FrameContext & context, MinSG::Node * node) {
  static const Geometry::Vec3 X_AXIS(1,0,0);
  static const Geometry::Vec3 Z_AXIS(0,0,1);
	const auto camera = context.getCamera();
	const Geometry::Rect viewport(camera->getViewport());
	// get world position of node & camera
	const auto node_pos_ws = node->getWorldBB().getCenter();
	const auto cam_pos_ws = camera->getWorldOrigin();
	// get camera direction
	const auto cam_dir_ws = Transformations::localDirToWorldDir(*camera, -Z_AXIS ).normalize();
	// get planes defined by camera/node origin & camera direction
	const Geometry::Plane cameraPlane(cam_pos_ws, cam_dir_ws);
	const Geometry::Plane nodePlane(node_pos_ws, cam_dir_ws);
	// get approximate radius of bounding sphere
	const float bs_radius = node->getWorldBB().getExtentMax() * 0.5f;	
	const float dist_c2n = std::max(camera->getNearPlane(), nodePlane.getOffset() - cameraPlane.getOffset() - bs_radius);	
	if(dist_c2n > camera->getFarPlane())
		return std::numeric_limits<float>::max();
	// compute 1m vector in clipping space located at node
	const auto one_meter_vector_ss = Geometry::project(Geometry::Vec3{1,0,-dist_c2n}, camera->getFrustum().getProjectionMatrix(), viewport);
	const float scale = node->getWorldTransformationSRT().getScale();
	const float pixel_per_meter = (one_meter_vector_ss.x() - viewport.getWidth()/2) * scale;	
	return pixel_per_meter > 0 ? 1.0f/pixel_per_meter : std::numeric_limits<float>::max();
}

}
}
#endif // MINSG_EXT_BLUE_SURFELS
