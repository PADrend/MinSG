/*
	This file is part of the MinSG library extension SphericalSampling.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SPHERICALSAMPLING

#include "SamplePoint.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include <Geometry/Vec3.h>
#include <stdexcept>

namespace MinSG {
namespace SphericalSampling {


struct SamplePoint::Implementation {
	Geometry::Vec3f position;
	VisibilitySubdivision::VisibilityVector value;

	Implementation(Geometry::Vec3f pos) : position(std::move(pos)), value() {
	}
};

SamplePoint::SamplePoint(const Geometry::Vec3f & pos) :
		impl(new Implementation(pos)) {
	const float length = pos.lengthSquared();
	if(length < 0.999f || length > 1.001f) {
		throw std::invalid_argument("Unit vector expected.");
	}
}

SamplePoint::SamplePoint(SamplePoint &&) = default;

SamplePoint::SamplePoint(const SamplePoint & other) : 
		impl(new Implementation(*other.impl)) {
}

SamplePoint::~SamplePoint() = default;

SamplePoint & SamplePoint::operator=(SamplePoint &&) = default;

SamplePoint & SamplePoint::operator=(const SamplePoint & other) {
	*impl = *other.impl;
	return *this;
}

bool SamplePoint::operator==(const SamplePoint & other) const {
	return impl->position == other.impl->position && 
			impl->value == other.impl->value;
}

const Geometry::Vec3f & SamplePoint::getPosition() const {
	return impl->position;
}

const VisibilitySubdivision::VisibilityVector & SamplePoint::getValue() const {
	return impl->value;
}

void SamplePoint::setValue(const VisibilitySubdivision::VisibilityVector & vv) {
	impl->value = vv;
}

size_t SamplePoint::getMemoryUsage() const {
	return sizeof(SamplePoint) + 
			sizeof(Implementation) +
			impl->value.getVisibleNodeCount() * sizeof(VisibilitySubdivision::VisibilityVector::node_benefits_pair_t);
}

}
}

#endif /* MINSG_EXT_SPHERICALSAMPLING */
