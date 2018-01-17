/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2012-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2016-2017 Sascha Brandt <myeti@mail.uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelGenerator.h"
#include "SurfelAnalysis.h"

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

class TextureSurfelBuilder : public SurfelGenerator::SurfelBuilder {
private:
	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb;
	Util::PixelAccessor* normal;
	Util::PixelAccessor* color;
public:
	TextureSurfelBuilder(const Rendering::VertexDescription& vd, Util::PixelAccessor* _normal, Util::PixelAccessor*  _color) : normal(_normal), color(_color) {
		mb = new Rendering::MeshUtils::MeshBuilder(vd);
	}
	uint32_t addSurfel(const SurfelGenerator::Surfel& s) {
		uint32_t x = s.getIndex() % normal->getWidth();
		uint32_t y = s.getIndex() / normal->getWidth();
		mb->position(s.pos);
		auto n = normal->readColor4f(x,y);
		mb->normal(Geometry::Vec3(n.r(), n.g(), n.b()));
		mb->color(color->readColor4f(x,y));
		return mb->addVertex();
	};
	Rendering::Mesh* buildMesh() {
		return mb->buildMesh();
	};
};
	
class MeshSurfelBuilder : public SurfelGenerator::SurfelBuilder {
private:
	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb;
	Rendering::NormalAttributeAccessor* normal;
	Rendering::ColorAttributeAccessor* color;
public:
	MeshSurfelBuilder(const Rendering::VertexDescription& vd, Rendering::NormalAttributeAccessor* _normal, Rendering::ColorAttributeAccessor* _color) 
		: normal(_normal), color(_color) {
		mb = new Rendering::MeshUtils::MeshBuilder(vd);
	}
	uint32_t addSurfel(const SurfelGenerator::Surfel& s) {
		mb->position(s.pos);
		mb->normal(normal->getNormal(s.getIndex()));
		mb->color(color->getColor4f(s.getIndex()));
		return mb->addVertex();
	};
	Rendering::Mesh* buildMesh() {
		return mb->buildMesh();
	};
};

std::vector<SurfelGenerator::Surfel> SurfelGenerator::extractSurfelsFromTextures(
													Util::PixelAccessor & pos,
													Util::PixelAccessor & normal,
													Util::PixelAccessor & color
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
				const uint32_t index = y*width + x;
				surfels.emplace_back(Geometry::Vec3(p.getR(),p.getG(),p.getB()), index);
			}
		}
	}
	if(parameters.benchmarkingEnabled){
		benchmarkResults["t_createInitialSet"] = t.getSeconds();
		t.reset();
	}

	return surfels;
}
	
SurfelGenerator::SurfelResult SurfelGenerator::buildBlueSurfels(const std::vector<Surfel> & surfels, SurfelBuilder* sb)const{
	if(parameters.benchmarkingEnabled)
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

	if(parameters.benchmarkingEnabled){
		benchmarkResults["t_bbCalculation"] = t.getSeconds();
		t.reset();
	}
	
	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
		size_t surfelId;
		OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelId(i) {}
	};
//	const Geometry::Box & boundingBox, float minimumBoxSize, uint32_t maximumPoints
	Geometry::PointOctree<OctreeEntry> octree(bb,bb.getExtentMax()*0.01,8);
	size_t surfelCount = 0;
	float minDist = bb.getDiameterSquared();

	static std::default_random_engine engine;
	if(parameters.seed>0) 
		engine.seed(parameters.seed);
	
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
		size_t surfelId;
		auto initialPoints = _::extractRandomSurfelIds(freeSurfelIds,1);
		surfelId = initialPoints.front();
//		freeSurfelIds.erase(surfelId);
		const auto & surfel = surfels[surfelId];
		sb->addSurfel(surfel);
		++surfelCount;
		octree.insert(OctreeEntry(surfelId,surfel.pos));
	}

	typedef std::pair<float,size_t> DistSurfelPair_t;
	std::vector<DistSurfelPair_t> sortedSubset; // surfelId , distance to nearest other sample

	if(parameters.pureRandomStrategy){	// pure random
		const auto randomSubset = _::extractRandomSurfelIds(freeSurfelIds,parameters.maxAbsSurfels);
		for(const auto& vId : randomSubset){
			const auto & surfel = surfels[vId];
			sb->addSurfel(surfel);
			++surfelCount;
		}
	}
	else{
	unsigned int acceptSamples=1;
	unsigned int samplesPerRound = parameters.samplesPerRound; 
	unsigned int round = 1; 

	if(parameters.benchmarkingEnabled)
		benchmarkResults["num_initialSamples"] = samplesPerRound;

	std::deque<OctreeEntry> closest;

	Util::Timer tRound;
	while(surfelCount<parameters.maxAbsSurfels && freeSurfelIds.size()>samplesPerRound){
		const auto randomSubset = _::extractRandomSurfelIds(freeSurfelIds,samplesPerRound);
		for(const auto& vId : randomSubset){
			const Geometry::Vec3 & vPos = surfels[vId].pos;
			closest.clear();
			octree.getClosestPoints(vPos, 1, closest);
			sortedSubset.emplace_back(vPos.distanceSquared(closest.front().getPosition()),vId);
		}
		// highest quality at the back
		std::partial_sort(sortedSubset.rbegin(), sortedSubset.rbegin()+acceptSamples+1,sortedSubset.rend(),std::greater<DistSurfelPair_t>());
		
		// accept best samples
		for(unsigned int j = 0;j<acceptSamples && !sortedSubset.empty(); ++j ){
			auto & sample = sortedSubset.back();
			sortedSubset.pop_back();
			const size_t vId = sample.second;
			const auto & surfel = surfels[vId];
			sb->addSurfel(surfel);
			octree.insert(OctreeEntry(vId,surfel.pos));
			if(surfelCount < parameters.medianDistCount) {
				minDist = std::min(minDist, sample.first);
			}
			++surfelCount;
		}

		// remove one (bad) surfel to prevent using duplicate surfels even if the requested number of surfels is too large.
		if(!sortedSubset.empty())
			sortedSubset.pop_back();

		// put back unused samples
		for(const auto& entry : sortedSubset)
			freeSurfelIds.emplace_back( entry.second );
			
		sortedSubset.clear();
		if( (round%500) == 0){
			//std::cout << "Round:"<<round<<" #Samples:"<< surfelCount<<" : "<<tRound.getMilliseconds()<<" ms; samplesPerRound "<<samplesPerRound<<" accept:"<<acceptSamples<<"\n";
			samplesPerRound = std::max(samplesPerRound*0.5f,20.0f);
			acceptSamples = std::min( static_cast<unsigned int>(samplesPerRound*0.3), acceptSamples+1);
			tRound.reset();
		}
		++round;
	}
	}
	
	if(parameters.benchmarkingEnabled){
		benchmarkResults["t_sampling"] = t.getSeconds();
		benchmarkResults["num_Surfels"] = surfelCount;
		benchmarkResults["num_targetSurfels"] = parameters.maxAbsSurfels;
		t.reset();
	}

	auto mesh = sb->buildMesh();
	mesh->setDrawMode(Rendering::Mesh::DRAW_POINTS);
	if(parameters.benchmarkingEnabled){
		benchmarkResults["t_buildMesh"] = t.getSeconds();
		t.reset();
	}

	float medianDist = getMedianOfNthClosestNeighbours(*mesh,parameters.medianDistCount,2);
	return {mesh, std::sqrt(minDist), medianDist};
}

SurfelGenerator::SurfelResult SurfelGenerator::createSurfels(
									Util::PixelAccessor & pos,
									Util::PixelAccessor & normal,
									Util::PixelAccessor & color
									)const{

	const auto surfels = extractSurfelsFromTextures(pos,normal,color);
	if(surfels.empty()){
		WARN("trying to create surfels from empty textures, returning empty mesh");
		return {new Rendering::Mesh,0.0,0.0};
	}
	TextureSurfelBuilder sb(vertexDescription, &normal, &color);
	return buildBlueSurfels(surfels, &sb);
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

SurfelGenerator::SurfelResult SurfelGenerator::createSurfelsFromMesh(Rendering::Mesh& mesh) const {
	auto vd = mesh.getVertexDescription();
	if(	!vd.hasAttribute(Rendering::VertexAttributeIds::POSITION) ||
			!vd.hasAttribute(Rendering::VertexAttributeIds::NORMAL) ||
			!vd.hasAttribute(Rendering::VertexAttributeIds::COLOR) ) {
		WARN("SurfelGenerator requires position, normal and color vertex attributes");
		return {};
	}
	auto& vertexData = mesh.openVertexData();
	
	auto posAcc = Rendering::PositionAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::POSITION);
	auto nrmAcc = Rendering::NormalAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::NORMAL);
	auto colAcc = Rendering::ColorAttributeAccessor::create(vertexData, Rendering::VertexAttributeIds::COLOR);
	
	std::vector<Surfel> surfels;
	surfels.reserve(mesh.getVertexCount());
	for(uint32_t i=0; i<mesh.getVertexCount(); ++i) {
		surfels.emplace_back(posAcc->getPosition(i), i);
	}
	MeshSurfelBuilder sb(vd, nrmAcc.get(), colAcc.get());	
	return buildBlueSurfels(surfels, &sb);
}

}
}
#endif /* MINSG_EXT_BLUE_SURFELS */
