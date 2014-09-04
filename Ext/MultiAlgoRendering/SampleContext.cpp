/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#include "SampleContext.h"
#include "Utils.h"
#include "SampleRegion.h"
#include "SampleStorage.h"
#include "../../Core/Nodes/AbstractCameraNode.h"
#include <Geometry/BoxHelper.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/ColorLibrary.h>
#include <random>

namespace MinSG {
namespace MAR {

//! sample position generators

Geometry::Vec3f NiceRandomPositionGenerator::generateSamplePosition(const SampleContext * context, Geometry::Box bounds) {
	static std::default_random_engine engine;
	Geometry::Vec3f best;
	float maxDist = 0;
	for (int i = 0; i < 100; i++) {
		Geometry::Vec3f tmp(
			std::uniform_real_distribution<float>(bounds.getMinX(), bounds.getMaxX())(engine),
			std::uniform_real_distribution<float>(bounds.getMinY(), bounds.getMaxY())(engine),
			std::uniform_real_distribution<float>(bounds.getMinZ(), bounds.getMaxZ())(engine)
		);
		std::deque<SamplePoint> q;
		context->getSampleStorage()->getStorage().getClosestPoints(tmp, 1, q);
		if (q.size() == 0) { // Octree is empty
			best = tmp;
			break;
		} else {
			float d = (q.front().getPosition() - tmp).lengthSquared();
			if (d > maxDist) {
				best = tmp;
				maxDist = d;
			}
		}
	}

	return best;
}

//! sample context

//! static, serialization
SampleContext * SampleContext::create(std::istream & in) {
	auto sc = new SampleContext();

	sc->storage = SampleStorage::create(in);
	uint64_t size = MAR::read<uint64_t>(in);
	for(uint64_t x = 0; x < size; ++x)
		sc->regions.push_back(SampleRegion::create(in));
	return sc;
}

//! static, serialization
void SampleContext::write(std::ostream & out) const {
	storage->write(out);
	MAR::write<uint64_t>(out, regions.size());
	for(const auto & r : regions)
		r->write(out);
}

//! private, serialization
SampleContext::SampleContext(): Util::ReferenceCounter<SampleContext>(), storage(), samplePosGen(new NiceRandomPositionGenerator()) {}

SampleContext::SampleContext(const Geometry::Box & bounds): Util::ReferenceCounter<SampleContext>(), storage(), samplePosGen(new NiceRandomPositionGenerator()) {
	storage = new SampleStorage(bounds);
	regions.push_back(new SampleRegion(storage.get(), bounds));
}

SampleContext::~SampleContext(){}

SampleStorage * SampleContext::getSampleStorage() const{
	return storage.get();
}

uint32_t SampleContext::getRegionCount() const {
	return regions.size();
}

SamplePositionGenerator * SampleContext::getSamplePositionGenerator() const{
	return samplePosGen.get();
}

const SampleContext::regions_t SampleContext::getSampleRegions() const{
	return regions;
}


SampleRegion* SampleContext::getMinSampleRegion() const {
	SampleRegion* region = nullptr;
	uint32_t minSamples = std::numeric_limits<uint32_t>::max();
	for(const auto & r : regions) {
		float count = r->getSampleCount();
		if(count < minSamples) {
			minSamples = count;
			region = r.get();
		}
	}
	if(region==nullptr) {
		WARN("no region found");
	}
	return region;
}

SampleRegion* SampleContext::getSampleRegionAtPosition(const Geometry::Vec3f & position) const {
	for(const auto & r : regions) {
		if(r->getBounds().contains(position))
			return r.get();
	}
	return nullptr;
}

size_t SampleContext::getMemoryUsage()const{
	size_t mem = 0;
	mem += sizeof(SampleContext);
	mem += storage->getMemoryUsage();
	return mem;
}

void SampleContext::splitLowQualityRegion() {
	SampleRegion::ref_t region = nullptr;
	float minQuality = -1;
	for(const auto & r : regions) {
		SampleQuality quality = r->getSampleQuality(this);
		if(quality.isValid() && quality.get() > minQuality) {
			minQuality = quality.get();
			region = r;
		}
	}
	if(region.isNull()) {
		WARN("no region to split");
		return;
	}
	const auto boxes = Geometry::Helper::splitBoxCubeLike(region->getBounds());
	regions.erase(std::find(begin(regions), end(regions), region));
	for(const auto & box : boxes) {
		regions.push_back(new SampleRegion(storage.get(), box, region->depth+1));
	}
}

void SampleContext::displaySamples(FrameContext & fc)const {

	Rendering::MaterialParameters mat;
	mat.setDiffuse(Util::Color4f(1,0,0,1));
	fc.getRenderingContext().pushAndSetMaterial(mat);

	this->storage->displaySamples(fc);

	fc.getRenderingContext().popMaterial();
}

static const Geometry::Box & getBounds2(const SampleRegion::ref_t & region) {
	return region->getBounds();
}
static const Util::Color4f getColor2(const SampleRegion::ref_t & region, const SampleContext * sc, float intensity) {
	return Util::Color4f(Util::Color4f(Util::ColorLibrary::GREEN), Util::Color4f(Util::ColorLibrary::RED), region->getSampleQuality(sc).get()*intensity);
}

void SampleContext::displayRegions(FrameContext & frameContext, float alpha, float redGreenThreshold) const {
	regions_t tmp(begin(regions), end(regions));
	std::sort(begin(tmp), end(tmp), SampleRegion::SortB2F(frameContext.getCamera()->getWorldOrigin()));

	debugDisplay<regions_t>(tmp, frameContext, alpha, &getBounds2, std::bind(&getColor2, std::placeholders::_1, this, redGreenThreshold));
}

}
}

#endif
