/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHEOBJECTPRIORITY_H_
#define OUTOFCORE_CACHEOBJECTPRIORITY_H_

#include <cstdint>

namespace MinSG {
namespace OutOfCore {

class CacheObjectPriority {
	private:
		//! Frame number in which the cache object was last used. This has the second most influence.
		uint32_t usageFrameNumber;

		//! Number of times the cache object was used in frame @a usageFrameNumber. This has the least influence.
		uint16_t usageCount;

		//! Number that can be set by the user. This has the highest influence.
		uint16_t userPriority;

	public:
		CacheObjectPriority() :
			usageFrameNumber(0), usageCount(0), userPriority(0) {
		}

		//! Constructor taking priorities from highest to lowest influence.
		CacheObjectPriority(uint16_t newUserPriority, uint32_t newUsageFrameNumber, uint16_t newUsageCount) :
			usageFrameNumber(newUsageFrameNumber), usageCount(newUsageCount), userPriority(newUserPriority) {
		}

		bool operator<(const CacheObjectPriority & other) const {
			return userPriority < other.userPriority || (!(other.userPriority < userPriority) 
					&& (usageFrameNumber < other.usageFrameNumber || (!(other.usageFrameNumber < usageFrameNumber) 
					&& usageCount < other.usageCount)));
		}

		bool operator==(const CacheObjectPriority & other) const {
			return userPriority == other.userPriority 
					&& usageFrameNumber == other.usageFrameNumber 
					&& usageCount == other.usageCount;
		}

		uint16_t getUserPriority() const {
			return userPriority;
		}
		void setUserPriority(uint16_t newUserPriority) {
			userPriority = newUserPriority;
		}

		uint32_t getUsageCount() const {
			return usageCount;
		}
		void setUsageCount(uint32_t newUsageCount) {
			usageCount = newUsageCount;
		}

		uint32_t getUsageFrameNumber() const {
			return usageFrameNumber;
		}
		void setUsageFrameNumber(uint32_t newUsageFrameNumber) {
			usageFrameNumber = newUsageFrameNumber;
		}
};

}
}

#endif /* OUTOFCORE_CACHEOBJECTPRIORITY_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
