/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_SAMPLEPOINT_H_
#define MINSG_SVS_SAMPLEPOINT_H_

#include <memory>

namespace Geometry {
template<typename T_> class _Vec3;
typedef _Vec3<float> Vec3f;
}
namespace MinSG {
namespace VisibilitySubdivision {
class VisibilityVector;
}
namespace SVS {

/**
 * A position on a sphere that is used for sampling.
 * Storage for the value sampled at that position.
 *
 * @author Benjamin Eikel
 * @date 2012-01-17
 */
class SamplePoint {
	private:
		// Use Pimpl idiom
		struct Implementation;
		std::unique_ptr<Implementation> impl;

	public:
		/**
		 * Create a new sample position with an empty value.
		 *
		 * @param pos Position on the unit sphere
		 */
		MINSGAPI SamplePoint(const Geometry::Vec3f & pos);
		MINSGAPI SamplePoint(SamplePoint &&);
		MINSGAPI SamplePoint(const SamplePoint &);
		MINSGAPI ~SamplePoint();
		MINSGAPI SamplePoint & operator=(SamplePoint &&);
		MINSGAPI SamplePoint & operator=(const SamplePoint &);

		//! Equality comparison
		MINSGAPI bool operator==(const SamplePoint & other) const;

		//! Retrieve the sample position.
		MINSGAPI const Geometry::Vec3f & getPosition() const;

		//! Retrieve the value stored at this position.
		MINSGAPI const VisibilitySubdivision::VisibilityVector & getValue() const;
		//! Store a new value for this position.
		MINSGAPI void setValue(const VisibilitySubdivision::VisibilityVector & vv);

		/**
		 * Calculate the amount of memory that is required to store the sample.
		 * 
		 * @return Overall amount of memory in bytes
		 */
		MINSGAPI size_t getMemoryUsage() const;
};

}
}

#endif /* MINSG_SVS_SAMPLEPOINT_H_ */

#endif /* MINSG_EXT_SVS */
