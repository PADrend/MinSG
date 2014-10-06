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

	return surfels;
}
	
Util::Reference<Rendering::Mesh> SurfelGenerator::buildBlueSurfels(const std::vector<Surfel> & surfels)const{
	Geometry::Box bb;
	bb.invalidate();
	
	// calculate bounding box and initialize sourceSurfels-mapping
//	std::deque<size_t> sourceSurfelIds(surfels.size());
	std::set<size_t> freeSurfelIds;
	{	
		size_t i=0;
		for(const auto & surfel : surfels){
			bb.include(surfel.pos);
			freeSurfelIds.insert(i);
			++i;
		}
	}

	
	
	// generate the mesh
	Rendering::VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb = new Rendering::MeshUtils::MeshBuilder(vd);
	
	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
		size_t surfelId;
		OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelId(i) {}
	};
//	const Geometry::Box & boundingBox, float minimumBoxSize, uint32_t maximumPoints
	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,8);
	size_t surfelCount = 0;
   
	static std::default_random_engine engine;
       
	struct _{
		static std::vector<size_t> getRandomSurfelIds(const std::set<size_t> & idSet,uint32_t num ){
			std::vector<size_t> result;
			result.reserve(num);
			if(!idSet.empty()){
				std::uniform_int_distribution<size_t> rand(0, *idSet.rbegin());
				 
				while(num-->0)
					result.emplace_back( *idSet.lower_bound(rand(engine)) );
			}
			return result;
		}
	};
	
	
	
	// add initial point
	{
		auto initialPoints = _::getRandomSurfelIds(freeSurfelIds,1);
		size_t surfelId = initialPoints.front();
		freeSurfelIds.erase(surfelId);
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
//	std::vector<size_t> newSurfelIds;

	std::cout << "Overall number:" << surfels.size();
 Util::Timer t;
 t.reset();
	unsigned int acceptSamples=1;
	unsigned int samplesPerRound=160;
	unsigned int round = 1;
	while(surfelCount<maxAbsSurfels && freeSurfelIds.size()>samplesPerRound){
		const auto randomSubset = _::getRandomSurfelIds(freeSurfelIds,samplesPerRound);
		for(const auto& vId : randomSubset){
			std::deque<OctreeEntry> closest;
			const Geometry::Vec3 & vPos = surfels[vId].pos;
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
			freeSurfelIds.erase(vId);
			const Surfel & v = surfels[vId];

			mb->position(v.pos);
			mb->normal(v.normal);
			mb->color(v.color);
			mb->addVertex();
			octree.insert(OctreeEntry(vId,v.pos));
			++surfelCount;
		}
		// remove worst samples

	// removal strategy 1
//	
//		for(size_t i=static_cast<size_t>(sortedSubset.size()*(1.0f-reusalRate));i>0;--i){
//			freeSurfelIds.erase(sortedSubset[i].first);
//		}
//	
//		// removal strategy 2
//		for(size_t i=std::min(static_cast<size_t>(acceptSamples),sortedSubset.size());i>0;--i)
//			freeSurfelIds.erase(sortedSubset[i].first);
		
		sortedSubset.clear();
//
		if( (round%500) == 0){
			std::cout << "Round:"<<round<<" #Samples:"<< surfelCount<<" : "<<t.getMilliseconds()<<" ms; samplesPerRound "<<samplesPerRound<<" accept:"<<acceptSamples<<"\n";
			samplesPerRound = std::max(samplesPerRound*0.5f,20.0f);
			acceptSamples = std::min( static_cast<unsigned int>(samplesPerRound*0.3), acceptSamples+1);
 			t.reset();
		}
		++round;
	}
	
	std::cout << " \t target number:" << maxAbsSurfels;
	std::cout << " \t created:" << surfelCount;
	std::cout << std::endl;

	
//	// sort surfels by size (and shuffle)
//	{
//		struct S {
//			const std::vector<Surfel> & surfels;
//			S(const std::vector<Surfel> & v) : surfels(v){}
//			bool operator()(size_t aId, size_t bId)const{
//				const auto & a = surfels[aId];
//				const auto & b = surfels[bId];
//				return a.size!=b.size ? a.size>b.size : 
//						(static_cast<uint32_t>(a.pos.x()*10000.0) ^ 
//						static_cast<uint32_t>(a.pos.y()*10000.0) ^ 
//						static_cast<uint32_t>(a.pos.z()*10000.0)) % 1273 >
//						(static_cast<uint32_t>(b.pos.x()*10000.0) ^ 
//						static_cast<uint32_t>(b.pos.y()*10000.0) ^ 
//						static_cast<uint32_t>(b.pos.z()*10000.0)) % 1273;
//			}
//		} sorter(surfels);
//		std::sort(sourceSurfelIds.begin(),sourceSurfelIds.end(),sorter);
//	}
//
//
//	
//	// generate the mesh
//	Rendering::VertexDescription vd;
//	vd.appendPosition3D();
//	vd.appendNormalByte();
//	vd.appendColorRGBAByte();
//	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb = new Rendering::MeshUtils::MeshBuilder(vd);
//	
//	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
//		size_t index;
//		OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), index(i) {}
//	};
////	const Geometry::Box & boundingBox, float minimumBoxSize, uint32_t maximumPoints
//	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,8);
//	size_t surfelCount = 0;
//
//	// add initial point
//	{
//		size_t surfelId = sourceSurfelIds.front();
//		sourceSurfelIds.pop_front();
//		auto & surfel = surfels[surfelId];
//		mb->position(surfel.pos);
//		mb->normal(surfel.normal);
//		mb->color(surfel.color);
//		mb->addVertex();
//		++surfelCount;
//		octree.insert(OctreeEntry(surfelId,surfel.pos));
//	}
//	
//	struct QS {
//		bool operator()(const std::pair<size_t,float>&a,const std::pair<size_t,float> &b)const{
//			return a.second<b.second;
//		}
//	} qualitySorter;
//
//	std::vector<std::pair<size_t,float>> samples; // surfelId , distance to nearest other sample
//	std::vector<size_t> newSurfelIds;
//
//	std::cout << "Overall number:" << surfels.size();
//// Util::Timer t;
//// t.reset();
//	unsigned int acceptSamples=1;
//	unsigned int samplesPerRound=160;
//	unsigned int round = 1;
//	while(surfelCount<maxAbsSurfels && sourceSurfelIds.size()>samplesPerRound){
//		for(size_t i=0;i<samplesPerRound;++i){
//			const size_t vId = sourceSurfelIds.front();
//			sourceSurfelIds.pop_front();
//			std::deque<OctreeEntry> closest;
//			const Geometry::Vec3 & vPos = surfels[vId].pos;
//			octree.getClosestPoints(vPos, 1, closest);
//			samples.emplace_back(vId,vPos.distanceSquared(closest.front().getPosition()));
//		}
//		// highest quality at the back
//		std::sort(samples.begin(),samples.end(),qualitySorter);
//		
//		newSurfelIds.clear();
//		for(unsigned int j = 0;j<acceptSamples && !samples.empty(); ++j ){
//			auto & sample = samples.back();
//			samples.pop_back();
//			const Surfel & v = surfels[sample.first];
//			bool ok = true;
////			for(const auto & recentSurfelId : newSurfelIds){
////				if(v.pos.distanceSquared( surfels[recentSurfelId].pos )<sample.second*2.0 ){
////					sourceSurfelIds.push_back(sample.first); // keep sample for next round
////					ok = false;
////					break;
////				}
////			}
//			if(ok){
//				newSurfelIds.push_back(sample.first);
//				mb->position(v.pos);
//				mb->normal(v.normal);
//				mb->color(v.color);
//				mb->addVertex();
//				octree.insert(OctreeEntry(sample.first,v.pos));
//				++surfelCount;
//			}
//		}
//
//		if( (round%10000) == 100){
//			std::default_random_engine rEngine;
//			std::shuffle(sourceSurfelIds.begin(),sourceSurfelIds.end(),rEngine );
//			//std::cout << "Shuffle\n";
//		}
//		if( (round%500) == 0){
//			//std::cout << "Round:"<<round<<" #Samples:"<< surfelCount<<" : "<<t.getMilliseconds()<<" ms; samplesPerRound "<<samplesPerRound<<" accept:"<<acceptSamples<<"\n";
//			samplesPerRound = std::max(samplesPerRound*0.5f,20.0f);
//			acceptSamples = std::min( static_cast<unsigned int>(samplesPerRound*0.3), acceptSamples+1);
//// 			t.reset();
//		}
//
//		for(size_t i=static_cast<size_t>(samples.size()*reusalRate);i>0;--i){
//			const size_t vId = samples.back().first;
//			samples.pop_back();
//			sourceSurfelIds.push_back(vId);
//		}
//		samples.clear();
//		++round;
//	}
//	
//	std::cout << " \t target number:" << maxAbsSurfels;
//	std::cout << " \t created:" << surfelCount;
//	std::cout << std::endl;

	auto mesh = mb->buildMesh();
	mesh->setDrawMode(Rendering::Mesh::DRAW_POINTS);

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
			octree.getClosestPoints(pos, 60, closestNeighbours);
			float f=0.0f;
			float sum = 0.0f;
			for(const auto& neighbour : closestNeighbours){
				float v = surfels[ neighbour.surfelId ].normal.dot(normal);
				if(v>=-0.00001){
					f += std::abs(v);
					sum+=1.0;
					
				}
//				if( surfels[ neighbour.index ].normal.dot(normal) > 0.9 )
			}
			Util::Color4f c = colorAccessor->getColor4f(vIndex);
			c.a( f/sum );
			colorAccessor->setColor( vIndex, c );
//			colorAccessor->setColor( vIndex, Util::Color4f(f/10.0,f/10.0,f/10.0,1.0 ));			
		}
		
		
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

}
}
#endif /* MINSG_EXT_BLUE_SURFELS */
