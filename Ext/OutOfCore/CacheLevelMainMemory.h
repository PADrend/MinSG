/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHELEVELMAINMEMORY_H_
#define OUTOFCORE_CACHELEVELMAINMEMORY_H_

#include "CacheLevel.h"
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace MinSG {
namespace OutOfCore {

/**
 * Specialized cache level for storing cache objects in and retrieving cache objects from main memory (CPU memory).
 *
 * @author Benjamin Eikel
 * @date 2011-02-23
 */
class CacheLevelMainMemory : public CacheLevel {
	private:
		//! Guard for @a thread and @a active
		std::mutex threadMutex;

		//! Semaphore used to put the worker thread to sleep when there is no work to do.
		std::condition_variable threadSemaphore;

		//! Parallel thread of execution that is used to load cache objects from lower cache levels.
		std::thread thread;

		//! Status of the cache level's thread.
		bool active;

		//! Helper function that is executed by the thread.
		MINSGAPI static void * threadRun(void * data);

		//! Store the cache object in main memory.
		MINSGAPI void doAddCacheObject(CacheObject * object) override;

		//! Delete the cache object from main memory.
		MINSGAPI void doRemoveCacheObject(CacheObject * object) override;

		//! Return @c true if the mesh is contained in main memory.
		MINSGAPI bool doLoadCacheObject(CacheObject * object) override;

		//! Return the memory size of the cache object.
		MINSGAPI uint64_t getCacheObjectSize(CacheObject * object) const override;

		//! If necessary, wake up the worker thread.
		MINSGAPI void doWork() override;

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		//! Check all cache objects stored in this cache level for inconsistencies.
		MINSGAPI void doVerify() const override;
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	public:
		MINSGAPI CacheLevelMainMemory(uint64_t cacheSize, CacheContext & cacheContext);
		MINSGAPI virtual ~CacheLevelMainMemory();

		//! Start the worker thread
		MINSGAPI void init() override;
};

}
}

#endif /* OUTOFCORE_CACHELEVELMAINMEMORY_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
