/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_STATISTICS_H_
#define MINSG_SVS_STATISTICS_H_

#include <cstdint>

namespace MinSG {
class Statistics;
namespace SVS {

//! Singleton holder object for SVS related counters.
class Statistics {
	private:
		MINSGAPI explicit Statistics(MinSG::Statistics & statistics);
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
		MINSGAPI static Statistics & instance(MinSG::Statistics & statistics);

		uint32_t getVisitedSpheresCounter() const {
			return visitedSpheresCounter;
		}
		uint32_t getEnteredSpheresCounter() const {
			return enteredSpheresCounter;
		}
};

}
}

#endif /* MINSG_SVS_STATISTICS_H_ */

#endif /* MINSG_EXT_SVS */
