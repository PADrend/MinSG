/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_SAMPLECONTEXT_H
#define MAR_SAMPLECONTEXT_H

#include "MultiAlgoGroupNode.h"

#include <Util/BidirectionalMap.h> // just for Reference hashfunction
#include <Util/Macros.h>
#include <Util/ReferenceCounter.h>
#include <Util/References.h>

#include <vector>
#include <unordered_set>
#include <limits>
#include <cstdint>
#include <cassert>

namespace MinSG {

class FrameContext;

namespace MAR {

class SampleStorage;
class SampleRegion;
class SampleContext;

//! sample position generators

class SamplePositionGenerator{
public:
	virtual Geometry::Vec3f generateSamplePosition(const SampleContext * context, Geometry::Box bounds) = 0;
	virtual ~SamplePositionGenerator(){}
};

class NiceRandomPositionGenerator:public SamplePositionGenerator{
public:
	MINSGAPI virtual Geometry::Vec3f generateSamplePosition(const SampleContext * context, Geometry::Box bounds) override;
	virtual ~NiceRandomPositionGenerator(){}
};

//! sample context

class SampleContext : public Util::ReferenceCounter<SampleContext>{

private:
		MINSGAPI SampleContext();

		Util::Reference<SampleStorage> storage;

		std::unique_ptr<SamplePositionGenerator> samplePosGen;

		typedef std::vector<Util::Reference<SampleRegion>> regions_t;
		regions_t regions;

	public:

		//! @name Serialization
		//@{
			MINSGAPI static SampleContext * create(std::istream & in);
			MINSGAPI void write(std::ostream & out)const;
		//@}
			
		MINSGAPI SampleContext(const Geometry::Box & bounds);
		MINSGAPI virtual ~SampleContext();
		
		MINSGAPI const regions_t getSampleRegions() const;

		MINSGAPI SampleStorage * getSampleStorage() const;

		MINSGAPI uint32_t getRegionCount() const;

		MINSGAPI SampleRegion* getMinSampleRegion() const;

		MINSGAPI SampleRegion* getSampleRegionAtPosition(const Geometry::Vec3f & position) const;

		MINSGAPI void splitLowQualityRegion();
		
		MINSGAPI size_t getMemoryUsage()const;

		MINSGAPI SamplePositionGenerator * getSamplePositionGenerator() const;

		MINSGAPI void displaySamples(FrameContext & fc) const;
		MINSGAPI void displayRegions(FrameContext & fc, float alpha, float redGreenThreshold) const;
};

}
}

#endif // SAMPLECONTEXT_H
#endif
