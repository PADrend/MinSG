/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2012-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelGenerator.h"

#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Plane.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/Texture/Texture.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Timer.h>
#include <random>
#include <set>

namespace MinSG {
namespace BlueSurfels {
	
std::vector<SurfelGenerator::Surfel> SurfelGenerator::extractSurfelsFromTextures(
													Util::PixelAccessor & pos,
													Util::PixelAccessor & normal,
													Util::PixelAccessor & color,
													Util::PixelAccessor & size
													)const{
	Util::Timer t;
	t.reset();

	const uint32_t width = pos.getWidth();
	const uint32_t height = pos.getHeight();
	
	std::vector<Surfel> surfels;

	for(uint32_t x=0;x<width;++x){
		for(uint32_t y=0;y<height;++y){
			const Util::Color4f p = pos.readColor4f(x,y);
			if(p.getA()>0){
				const Util::Color4f n = normal.readColor4f(x,y);
				surfels.emplace_back(Geometry::Vec3(p.getR(),p.getG(),p.getB()),
									Geometry::Vec3(n.getR(),n.getG(),n.getB()),
									color.readColor4f(x,y),size.readSingleValueFloat(x,y));
			}
		}
	}
	if(benchmarkingEnabled){
		benchmarkResults["t_createInitialSet"] = t.getSeconds();
		t.reset();
	}

	return surfels;
}
	
Util::Reference<Rendering::Mesh> SurfelGenerator::buildBlueSurfels(const std::vector<Surfel> & surfels)const{
	if(benchmarkingEnabled)
		benchmarkResults["num_initialSetSize"] = surfels.size();

	Util::Timer t;
	t.reset();

	Geometry::Box bb;
	bb.invalidate();
	
	// calculate bounding box and initialize sourceSurfels-mapping
	std::vector<size_t> freeSurfelIds;
	freeSurfelIds.reserve( surfels.size() );
	{	
		size_t i=0;
		for(const auto & surfel : surfels){
			bb.include(surfel.pos);
			freeSurfelIds.emplace_back(i);
			++i;
		}
	}

	if(benchmarkingEnabled){
		benchmarkResults["t_bbCalculation"] = t.getSeconds();
		t.reset();
	}

	
	// generate the mesh
	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb = new Rendering::MeshUtils::MeshBuilder(vertexDescription);
	
	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
		size_t surfelId;
		OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelId(i) {}
	};
//	const Geometry::Box & boundingBox, float minimumBoxSize, uint32_t maximumPoints
	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,8);
	size_t surfelCount = 0;

	static std::default_random_engine engine;
	
	struct _{
		static std::vector<size_t> extractRandomSurfelIds(std::vector<size_t> & freeIds,uint32_t num ){
			std::vector<size_t> result;
			result.reserve(num);
			std::uniform_int_distribution<size_t> rand(0, freeIds.size()*100 );
			for(;num>0 && !freeIds.empty(); --num ){
				const size_t i = rand(engine)%freeIds.size();
				result.emplace_back( freeIds[i] );
				freeIds[i] = freeIds.back();
				freeIds.pop_back();
			}
			return result;
		}
	};
	
	// add initial point
	{
		auto initialPoints = _::extractRandomSurfelIds(freeSurfelIds,1);
		size_t surfelId = initialPoints.front();
//		freeSurfelIds.erase(surfelId);
		auto & surfel = surfels[surfelId];
		mb->position(surfel.pos);
		mb->normal(surfel.normal);
		mb->color(surfel.color);
		mb->addVertex();
		++surfelCount;
		octree.insert(OctreeEntry(surfelId,surfel.pos));
	}
	
	struct QS {
		bool operator()(const std::pair<size_t,float>&a,const std::pair<size_t,float> &b)const{
			return a.second<b.second;
		}
	} qualitySorter;

	std::vector<std::pair<size_t,float>> sortedSubset; // surfelId , distance to nearest other sample


	if(false){	// pure random
		const auto randomSubset = _::extractRandomSurfelIds(freeSurfelIds,maxAbsSurfels);
		for(const auto& vId : randomSubset){
			const Surfel & v = surfels[vId];
			mb->position(v.pos);
			mb->normal(v.normal);
			mb->color(v.color);
			mb->addVertex();
			octree.insert(OctreeEntry(vId,v.pos));
			++surfelCount;
		}
	}
	else{
	unsigned int acceptSamples=1;
//	unsigned int samplesPerRound=/*160*/ 1000; // hq
	unsigned int samplesPerRound=160; 
	unsigned int round = 1;

	if(benchmarkingEnabled)
		benchmarkResults["num_initialSamples"] = samplesPerRound;

	std::deque<OctreeEntry> closest;

	Util::Timer tRound;
	while(surfelCount<maxAbsSurfels && freeSurfelIds.size()>samplesPerRound){
		const auto randomSubset = _::extractRandomSurfelIds(freeSurfelIds,samplesPerRound);
		for(const auto& vId : randomSubset){
			const Geometry::Vec3 & vPos = surfels[vId].pos;
			closest.clear();
			octree.getClosestPoints(vPos, 1, closest);
			sortedSubset.emplace_back(vId,vPos.distanceSquared(closest.front().getPosition()));
		}
		// highest quality at the back
		std::sort(sortedSubset.begin(),sortedSubset.end(),qualitySorter);
		
		// accept best samples
		for(unsigned int j = 0;j<acceptSamples && !sortedSubset.empty(); ++j ){
			auto & sample = sortedSubset.back();
			sortedSubset.pop_back();
			const size_t vId = sample.first;
			const Surfel & v = surfels[vId];
			mb->position(v.pos);
			mb->normal(v.normal);
			mb->color(v.color);
			mb->addVertex();
			octree.insert(OctreeEntry(vId,v.pos));
			++surfelCount;
		}

		// remove one (bad) surfel to prevent using duplicate surfels even if the requested number of surfels is too large.
		if(!sortedSubset.empty())
			sortedSubset.pop_back();

		// put back unused samples
		for(const auto& entry : sortedSubset)
			freeSurfelIds.emplace_back( entry.first );


		// removal strategy 1
//	
//		for(size_t i=static_cast<size_t>(sortedSubset.size()*(1.0f-reusalRate));i>0;--i){
//			freeSurfelIds.erase(sortedSubset[i].first);
//		}

		sortedSubset.clear();
		if( (round%500) == 0){
			std::cout << "Round:"<<round<<" #Samples:"<< surfelCount<<" : "<<tRound.getMilliseconds()<<" ms; samplesPerRound "<<samplesPerRound<<" accept:"<<acceptSamples<<"\n";
			samplesPerRound = std::max(samplesPerRound*0.5f,20.0f);
			acceptSamples = std::min( static_cast<unsigned int>(samplesPerRound*0.3), acceptSamples+1);
			tRound.reset();
		}
		++round;
	}
	}
	if(benchmarkingEnabled){
		benchmarkResults["t_sampling"] = t.getSeconds();
		benchmarkResults["num_Surfels"] = surfelCount;
		benchmarkResults["num_targetSurfels"] = maxAbsSurfels;
		t.reset();
	}

	auto mesh = mb->buildMesh();
	mesh->setDrawMode(Rendering::Mesh::DRAW_POINTS);
	if(benchmarkingEnabled){
		benchmarkResults["t_buildMesh"] = t.getSeconds();
		t.reset();
	}

	// guess size
	if(mesh->getVertexCount()>0){
		Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor(Rendering::PositionAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION));
		Util::Reference<Rendering::NormalAttributeAccessor> normalAccessor(Rendering::NormalAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::NORMAL));
		Util::Reference<Rendering::ColorAttributeAccessor> colorAccessor(Rendering::ColorAttributeAccessor::create(mesh->openVertexData(), Rendering::VertexAttributeIds::COLOR));
		std::deque<OctreeEntry> closestNeighbours;
		const size_t endIndex = mesh->getVertexCount();
		for(size_t vIndex = 0 ; vIndex<endIndex; ++vIndex){
			const auto pos = positionAccessor->getPosition(vIndex);
			const auto normal = normalAccessor->getNormal(vIndex);

			closestNeighbours.clear();
			octree.getClosestPoints(pos, 20, closestNeighbours);

			float f = 0.0f, sum = 0.0f;
			for(const auto& neighbour : closestNeighbours){
				float v = surfels[ neighbour.surfelId ].normal.dot(normal);
				if(v>=-0.00001){
					f += std::abs(v);
					sum+=1.0;
					
				}
			}
			Util::Color4f c = colorAccessor->getColor4f(vIndex);
			c.a( f/sum );
			colorAccessor->setColor( vIndex, c );
//			colorAccessor->setColor( vIndex, Util::Color4f(f/10.0,f/10.0,f/10.0,1.0 ));			
		}
		
	}
	if(benchmarkingEnabled){
		benchmarkResults["t_guessSizes"] = t.getSeconds();
		t.reset();
	}
	return mesh;
}


std::pair<Util::Reference<Rendering::Mesh>,float> SurfelGenerator::createSurfels(
									Util::PixelAccessor & pos,
									Util::PixelAccessor & normal,
									Util::PixelAccessor & color,
									Util::PixelAccessor & size
									)const{

	const auto surfels = extractSurfelsFromTextures(pos,normal,color,size);
	const float relativeSize = static_cast<float>(surfels.size()) / (pos.getWidth() * pos.getHeight());
	if(surfels.empty()){
		WARN("trying to create surfels from empty textures, returning empty mesh");
		return std::make_pair(new Rendering::Mesh,0.0);
	}
	return std::make_pair(buildBlueSurfels(surfels),relativeSize);
}

float SurfelGenerator::getMedianOfNthClosestNeighbours(Rendering::Mesh& mesh, size_t prefixLength, size_t nThNeighbour){
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

void SurfelGenerator::setVertexDescription(const Rendering::VertexDescription& vd) {
	if(	!vd.hasAttribute(Rendering::VertexAttributeIds::POSITION) ||
			!vd.hasAttribute(Rendering::VertexAttributeIds::NORMAL) ||
			!vd.hasAttribute(Rendering::VertexAttributeIds::COLOR) ) {
		WARN("SurfelGenerator requires position, normal and color vertex attributes");
		return;
	}
	vertexDescription = vd;
}

}
}
#endif /* MINSG_EXT_BLUE_SURFELS */
