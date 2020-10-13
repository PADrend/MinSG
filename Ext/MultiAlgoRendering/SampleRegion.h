/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_SAMPLEREGION_H
#define MAR_SAMPLEREGION_H

#include "MultiAlgoGroupNode.h"
#include "SampleStorage.h"
#include "SampleContext.h"
#include "Utils.h"

#include <Util/ReferenceCounter.h>
#include <Util/Macros.h>
#include <Util/Graphics/ColorLibrary.h>

#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Geometry/VecN.h>

#include <limits>
#include <vector>
#include <set>

namespace MinSG {
namespace MAR {

/*!
 * 0 = good, 1 = bad
 */
class SampleQuality {
private:

	float quality;

public:

	SampleQuality() : quality(-1) {}

	void invalidate() {
		quality = -1;
	}
	bool isValid() const {
		return quality != -1;
	}
	float get() const {
		return quality;
	}
	void set(float _quality) {
		assert((_quality >= 0 && _quality <= 1) || _quality == -1);
		quality = _quality;
		std::cerr << quality << std::endl;
	}
};

class SampleRegion : public Util::ReferenceCounter<SampleRegion>
{

public:
	class SortB2F {
		Geometry::Vec3f pos;
	public:
		SortB2F(Geometry::Vec3f _pos) : pos(std::move(_pos)) {}
		bool operator() (const ref_t & a, const ref_t & b) const {
			return b->getBounds().getDistanceSquared(pos) > a->getBounds().getDistanceSquared(pos);
		}
	};

	//! constructor
public:
	
	//! @name Serialization
	//@{
		MINSGAPI static SampleRegion * create(std::istream & in);
		MINSGAPI void write(std::ostream & out)const;
	//@}
		
	SampleRegion(const SampleStorage * storage, Geometry::Box _bounds, int _depth = 0) :
		Util::ReferenceCounter<SampleRegion>(),
		sampleCount(0),
		sampleQuality(),
		bounds(std::move(_bounds)),
		depth(_depth),
		temporaryPosition(std::numeric_limits<float>::max())
	{
		std::deque<SamplePoint> samples;
		storage->getStorage().collectPointsWithinBox(bounds, samples);
		sampleCount = samples.size();
	}

	//! sample count
private:
	uint32_t sampleCount;
public:
	uint32_t getSampleCount() const {
		return sampleCount;
	}

	//! sample quality
private:
	mutable SampleQuality sampleQuality;
public:
	MINSGAPI static const float INVALID_QUALITY;
	const SampleQuality & getSampleQuality(const SampleContext * context) const
	{
		if(!sampleQuality.isValid() && sampleCount > 0)
		{
			const SampleStorage * storage = context->getSampleStorage();
			std::deque<SamplePoint> samples;
			storage->getStorage().collectPointsWithinBox(bounds, samples);
			Geometry::VecNf avg(samples.begin()->data->errors.size());
			for(const auto & sp : samples) {
				avg += Geometry::VecNf(sp.data->errors.begin(), sp.data->errors.end());
			}
			avg /= samples.size();

			Geometry::VecNf avgdiff(samples.begin()->data->errors.size());
			for(const auto & sp : samples) {
				avgdiff += (avg - Geometry::VecNf(sp.data->errors.begin(), sp.data->errors.end())).getAbs();
			}
			avgdiff /= samples.size();

//             sampleQuality.set(avgdiff.length(avgdiff.size()));
//             sampleQuality.set(avgdiff.length(Geometry::VecNf::MAXIMUM_NORM));
			sampleQuality.set(avgdiff.avg() / std::max(depth, 1u));
		}
		return sampleQuality;
	}
	std::string debug(const SampleContext * context) const {
		const SampleStorage * storage = context->getSampleStorage();
		std::deque<SamplePoint> samples;
		storage->getStorage().collectPointsWithinBox(bounds, samples);
		uint32_t vecSize = samples.begin()->data->times.size();
		Geometry::VecNf avg(vecSize);
		for(const auto & sp : samples)
			avg += Geometry::VecNf(sp.data->times.begin(), sp.data->times.end());
		avg /= samples.size();

		Geometry::VecNf var(vecSize);
		for(const auto & sp : samples) {
			Geometry::VecNf x(sp.data->times.begin(), sp.data->times.end());
			x -= avg;
			var += x*x;
		}
		var /= samples.size()-1;
		
		std::stringstream ss;
		for(const auto & v : var)
			ss << v << std::endl;
		return ss.str();
	}

	//! bounds
private:
	Geometry::Box bounds;
public:
	const uint32_t depth;
	const Geometry::Box & getBounds() const {
		return bounds;
	}

	//! samples &  positions
private:

	std::deque<SampleResult::ref_t> temporaryResults;
	Geometry::Vec3f temporaryPosition;

public:
	
	Geometry::Vec3f createSamplePosition(const Geometry::Vec3f & position) {
		assert(temporaryResults.empty());
		temporaryPosition = position;
		return temporaryPosition;
	}

	Geometry::Vec3f createSamplePosition(const SampleContext * context) {
		assert(temporaryResults.empty());
		temporaryPosition = context->getSamplePositionGenerator()->generateSamplePosition(context, this->bounds);
		return temporaryPosition;
	}

	void addSample(const SampleResult::ref_t & sampleResult) {
		assert(bounds.contains(temporaryPosition));
		temporaryResults.push_back(sampleResult);
	}

	void finalizeSample(const SampleContext * context) {
		assert(bounds.contains(temporaryPosition));
		sampleCount++;
		sampleQuality.invalidate();
        std::deque<SamplePoint> d;
        context->getSampleStorage()->getStorage().collectPoints(d);
        context->getSampleStorage()->addResults(temporaryPosition, d.size(), temporaryResults);
		temporaryResults.clear();
		temporaryPosition.setValue(std::numeric_limits<float>::max());
	}



private:

	MINSGAPI SampleRegion(uint32_t);
	MINSGAPI SampleRegion(const SampleRegion&);
	MINSGAPI SampleRegion & operator= (const SampleRegion &);

};

}
}

#endif // SAMPLEREGION_H
#endif
