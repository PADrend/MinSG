/*
	This file is part of the MinSG library extension SphericalSampling.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SPHERICALSAMPLING

#ifndef MINSG_SPHERICALSAMPLING_STATISTICS_H_
#define MINSG_SPHERICALSAMPLING_STATISTICS_H_

#include <cstdint>

namespace MinSG {
class Statistics;
namespace SphericalSampling {

//! Singleton holder object for SphericalSampling related counters.
class Statistics {
	private:
		explicit Statistics(MinSG::Statistics & statistics);
		Statistics(Statistics &&) = delete;
		Statistics(const Statistics &) = delete;
		Statistics & operator=(Statistics &&) = delete;
		Statistics & operator=(const Statistics &) = delete;

		//! Key of visited spheres counter
		uint32_t visitedSpheresCounter;
		//! Key of entered spheres counter
		uint32_t enteredSpheresCounter;
	public:
		//! Return singleton instance.
		static Statistics & instance(MinSG::Statistics & statistics);

		uint32_t getVisitedSpheresCounter() const {
			return visitedSpheresCounter;
		}
		uint32_t getEnteredSpheresCounter() const {
			return enteredSpheresCounter;
		}
};

}
}

#endif /* MINSG_SPHERICALSAMPLING_STATISTICS_H_ */

#endif /* MINSG_EXT_SPHERICALSAMPLING */
