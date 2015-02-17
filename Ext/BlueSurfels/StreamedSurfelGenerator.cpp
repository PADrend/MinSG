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

#include "StreamedSurfelGenerator.h"

#include <Geometry/Point.h>
#include <Geometry/PointOctree.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeAccessors.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/MeshUtils/StreamMeshDataStrategy.h>
#include <Rendering/Texture/Texture.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Timer.h>
#include <Util/Macros.h>
#include <random>
#include <set>
#include <functional>
#include <stack>

namespace MinSG {
namespace BlueSurfels {
	
enum InternalState {
	Initial, Processing, Finished
};

struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
	size_t surfelId;
	OctreeEntry(size_t i,const Geometry::Vec3 & p) : Geometry::Point<Geometry::Vec3f>(p), surfelId(i) {}
};

struct QS {
	bool operator()(const std::pair<size_t,float>&a,const std::pair<size_t,float> &b)const{
		return a.second<b.second;
	}
};

typedef std::function<void(StreamedSurfelGenerator::State*)> StateFn_t;

struct StreamedSurfelGenerator::State {
	std::vector<SurfelGenerator::Surfel> surfels;
	Util::Reference<Rendering::Mesh> mesh;
	float relCoverage;
	InternalState state = Initial;
	Util::Reference<Rendering::MeshUtils::MeshBuilder> mb;

	StateFn_t process;

	Geometry::PointOctree<OctreeEntry> octree = Geometry::PointOctree<OctreeEntry>(Geometry::Box(),0,0);
	uint32_t surfelCount;
	uint32_t maxAbssurfelCount;
	std::vector<size_t> freeSurfelIds;
	uint32_t vId;
};

std::vector<size_t> extractRandomSurfelIds(StreamedSurfelGenerator::State* state,uint32_t num ){
	static std::default_random_engine engine;
	std::vector<size_t> result;
	result.reserve(num);
	std::uniform_int_distribution<size_t> rand(0, state->freeSurfelIds.size()*100 );
	for(;num>0 && !state->freeSurfelIds.empty(); --num ){
		const size_t i = rand(engine)%state->freeSurfelIds.size();
		result.emplace_back( state->freeSurfelIds[i] );
		state->freeSurfelIds[i] = state->freeSurfelIds.back();
		state->freeSurfelIds.pop_back();
	}
	return result;
}

/*****************************************
 * States
 *****************************************/

void guessSizes(StreamedSurfelGenerator::State* state) {
	// guess size
	if(state->mesh->getVertexCount()>0){
		Util::Reference<Rendering::PositionAttributeAccessor> positionAccessor(Rendering::PositionAttributeAccessor::create(state->mesh->openVertexData(), Rendering::VertexAttributeIds::POSITION));
		Util::Reference<Rendering::NormalAttributeAccessor> normalAccessor(Rendering::NormalAttributeAccessor::create(state->mesh->openVertexData(), Rendering::VertexAttributeIds::NORMAL));
		Util::Reference<Rendering::ColorAttributeAccessor> colorAccessor(Rendering::ColorAttributeAccessor::create(state->mesh->openVertexData(), Rendering::VertexAttributeIds::COLOR));
		std::deque<OctreeEntry> closestNeighbours;
		const size_t endIndex = state->mesh->getVertexCount();
		for(size_t vIndex = 0 ; vIndex<endIndex; ++vIndex){
			const auto pos = positionAccessor->getPosition(vIndex);
			const auto normal = normalAccessor->getNormal(vIndex);

			closestNeighbours.clear();
			state->octree.getClosestPoints(pos, 20, closestNeighbours);

			float f = 0.0f, sum = 0.0f;
			for(const auto& neighbour : closestNeighbours){
				float v = state->surfels[ neighbour.surfelId ].normal.dot(normal);
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
	state->state = Finished;
}

void addRandomPoint(StreamedSurfelGenerator::State* state) {
	if(state->surfelCount >= state->maxAbssurfelCount || state->freeSurfelIds.empty()) {
		//state->state = Finished;
		state->process = guessSizes;
		return;
	}

	auto initialPoints = extractRandomSurfelIds(state,1);
	size_t surfelId = initialPoints.front();
//		freeSurfelIds.erase(surfelId);
	auto & surfel = state->surfels[surfelId];
	auto mb = state->mb;
	mb->position(surfel.pos);
	mb->normal(surfel.normal);
	mb->color(surfel.color);
	mb->addVertex();
	++state->surfelCount;
	state->octree.insert(OctreeEntry(surfelId,surfel.pos));

	state->process = addRandomPoint;
}

void addFurthestPoint(StreamedSurfelGenerator::State* state) {
	if(state->surfelCount >= state->maxAbssurfelCount || state->freeSurfelIds.empty()) {
		state->state = Finished;
		return;
	}
	unsigned int samplesPerRound=160;
	unsigned int acceptSamples=1;
	std::vector<std::pair<size_t,float>> sortedSubset; // surfelId , distance to nearest other sample
	std::deque<OctreeEntry> closest;

	const auto randomSubset = extractRandomSurfelIds(state,samplesPerRound);
	for(const auto& vId : randomSubset){
		const Geometry::Vec3 & vPos = state->surfels[vId].pos;
		closest.clear();
		state->octree.getClosestPoints(vPos, 1, closest);
		sortedSubset.emplace_back(vId,vPos.distanceSquared(closest.front().getPosition()));
	}
	// highest quality at the back
	std::sort(sortedSubset.begin(),sortedSubset.end(),QS());

	// accept best samples
	for(unsigned int j = 0;j<acceptSamples && !sortedSubset.empty(); ++j ){
		auto & sample = sortedSubset.back();
		sortedSubset.pop_back();
		const size_t vId = sample.first;
		const SurfelGenerator::Surfel & v = state->surfels[vId];
		auto mb = state->mb;
		mb->position(v.pos);
		mb->normal(v.normal);
		mb->color(v.color);
		mb->addVertex();
		state->octree.insert(OctreeEntry(vId,v.pos));
		++state->surfelCount;
	}
	// put back unused samples
	for(const auto& entry : sortedSubset)
		state->freeSurfelIds.emplace_back( entry.first );

	state->process = addFurthestPoint;
}

void addInitialPoint(StreamedSurfelGenerator::State* state) {
	auto initialPoints = extractRandomSurfelIds(state,1);
	size_t surfelId = initialPoints.front();
//		freeSurfelIds.erase(surfelId);
	auto & surfel = state->surfels[surfelId];
	auto mb = state->mb;
	mb->position(surfel.pos);
	mb->normal(surfel.normal);
	mb->color(surfel.color);
	mb->addVertex();
	++state->surfelCount;
	state->octree.insert(OctreeEntry(surfelId,surfel.pos));

	state->process = addFurthestPoint;
}

void calculateBB(StreamedSurfelGenerator::State* state) {
	Geometry::Box bb;
	bb.invalidate();

	// calculate bounding box and initialize sourceSurfels-mapping
	state->freeSurfelIds.reserve( state->surfels.size() );
	{
		size_t i=0;
		for(const auto & surfel : state->surfels){
			bb.include(surfel.pos);
			state->freeSurfelIds.emplace_back(i);
			++i;
		}
	}

	state->octree = Geometry::PointOctree<OctreeEntry>(bb,bb.getExtentMax()*0.01,8);
	state->surfelCount = 0;

	state->process = addInitialPoint;
}

/*****************************************
 * StreamedSurfelGenerator
 *****************************************/

StreamedSurfelGenerator::StreamedSurfelGenerator() :
		SurfelGenerator(), timeLimit_ms(10), state(new State) {}

StreamedSurfelGenerator::~StreamedSurfelGenerator() = default;

StreamedSurfelGenerator::StreamedSurfelGenerator(const StreamedSurfelGenerator& other)
		: SurfelGenerator(other), timeLimit_ms(other.timeLimit_ms), state(new State(*other.state.get())){}

StreamedSurfelGenerator::StreamedSurfelGenerator(StreamedSurfelGenerator &&) = default;

void StreamedSurfelGenerator::begin(Util::PixelAccessor& pos, Util::PixelAccessor& normal, Util::PixelAccessor& color, Util::PixelAccessor& size) {
	state.reset(new State);

	state->state = Processing;
	state->surfels = extractSurfelsFromTextures(pos, normal, color, size);
	state->relCoverage = static_cast<float>(state->surfels.size()) / (pos.getWidth() * pos.getHeight());
	state->process = calculateBB;
	state->maxAbssurfelCount = getMaxAbsSurfels();
	state->surfelCount = 0;

	Rendering::VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	Rendering::Mesh* mesh = new Rendering::Mesh(vd, std::min(static_cast<size_t>(getMaxAbsSurfels()), state->surfels.size()), 0);
	mesh->setUseIndexData(false);
	mesh->setDataStrategy(new Rendering::StreamMeshDataStrategy);
	mesh->setDrawMode(Rendering::Mesh::DRAW_POINTS);
	state->mesh = mesh;
	state->mb = new Rendering::MeshUtils::MeshBuilder(vd);
}

bool StreamedSurfelGenerator::step() {
	if(state->state == Initial) {
		WARN("StreamedSurfelGenerator was not initialized");
		return true;
	}
	if(state->state == Finished) {
		return true;
	}
	Util::Timer t;
	t.reset();

	while(t.getMilliseconds() < timeLimit_ms && state->state != Finished) {
		state->process(state.get());
	}

	auto mb = state->mb;
	if(!mb->isEmpty()) {
		auto mesh = state->mesh;
		Util::Reference<Rendering::Mesh> tmpMesh = mb->buildMesh();
		auto strategy = dynamic_cast<Rendering::StreamMeshDataStrategy*>(mesh->getDataStrategy());

		Rendering::MeshVertexData& vd = mesh->openVertexData();
		Rendering::MeshVertexData& srcData = tmpMesh->openVertexData();
		std::copy(srcData.data(), srcData.data() + srcData.dataSize(), vd.data() + strategy->getStreamStart() * vd.getVertexDescription().getVertexSize());

		strategy->uploadNextVertices(tmpMesh->getVertexCount());
	}

	return state->state == Finished;
}

std::pair<Util::Reference<Rendering::Mesh>, float> StreamedSurfelGenerator::getResult() {
	if(state->state == Initial) {
		WARN("StreamedSurfelGenerator was not initialized");
		return std::make_pair(new Rendering::Mesh,0.0);
	}
	return std::make_pair(state->mesh, state->relCoverage);
}

}
}

#endif /* MINSG_EXT_BLUE_SURFELS */
