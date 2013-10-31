/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#include "SampleRegion.h"

#include <Util/Macros.h>

#include <limits>
#include <cassert>
#include <cstdint>

namespace MinSG {
namespace MAR {

const float SampleRegion::INVALID_QUALITY = -1.0f;

SampleRegion * SampleRegion::create(std::istream & in) {
	auto sr = new SampleRegion(MAR::read<uint32_t>(in));
	sr->bounds = MAR::read< Geometry::Box >(in);
	sr->sampleCount = MAR::read<uint32_t>(in);
	sr->sampleQuality.set(MAR::read<float>(in));
	return sr;
}

void SampleRegion::write(std::ostream & out) const {
	MAR::write<uint32_t>(out, depth);
	MAR::write<Geometry::Box>(out, bounds);
	MAR::write<uint32_t>(out, sampleCount);
	MAR::write<float>(out, sampleQuality.get());
}

SampleRegion::SampleRegion(uint32_t _depth)  : Util::ReferenceCounter<SampleRegion>(),
	sampleCount(0), sampleQuality(), bounds(), depth(_depth), temporaryPosition(std::numeric_limits<float>::max()){}

}
}

#endif
