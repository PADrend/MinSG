/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHELEVELGRAPHICSMEMORY_H_
#define OUTOFCORE_CACHELEVELGRAPHICSMEMORY_H_

#include "CacheLevel.h"
#include <cstddef>
#include <set>
#include <cstdint>
#include <vector>

namespace MinSG {
namespace OutOfCore {

/**
 * Specialized cache level for storing cache objects in and retrieving cache objects from graphics memory (GPU memory).
 *
 * @author Benjamin Eikel
 * @date 2011-02-24
 */
class CacheLevelGraphicsMemory : public CacheLevel {
	private:
		//! Upload the cache object to graphics memory.
		void doAddCacheObject(CacheObject * object) override;

		//! Delete the cache object from graphics memory.
		void doRemoveCacheObject(CacheObject * object) override;

		//! Do nothing
		bool doLoadCacheObject(CacheObject * /*object*/) override {
			return false;
		}

		//! Return the memory size of the cache object.
		uint64_t getCacheObjectSize(CacheObject * object) const override;

		//! Fetch cache objects from below and upload the to GPU memory.
		void doWork() override;

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		//! Check all cache objects stored in this cache level for inconsistencies.
		void doVerify() const override;
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	public:
		CacheLevelGraphicsMemory(uint64_t cacheSize, CacheContext & cacheContext);
		virtual ~CacheLevelGraphicsMemory();
};

}
}

#endif /* OUTOFCORE_CACHELEVELGRAPHICSMEMORY_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
