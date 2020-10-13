/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHELEVELFILES_H_
#define OUTOFCORE_CACHELEVELFILES_H_

#include "CacheLevel.h"
#include <Util/IO/TemporaryDirectory.h>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <utility>
#include <vector>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace OutOfCore {

/**
 * Specialized cache level for storing cache objects in and retrieving cache objects from files.
 *
 * @author Benjamin Eikel
 * @date 2011-02-23
 */
class CacheLevelFiles : public CacheLevel {
	private:
		//! Guard for @a thread and @a active
		std::mutex threadMutex;

		//! Semaphore used to put the worker thread to sleep when there is no work to do.
		std::condition_variable threadSemaphore;

		//! Parallel thread of execution that is for writing cache objects to disk.
		std::thread thread;

		//! Status of the cache level's thread.
		bool active;

		//! Helper function that is executed by the thread.
		MINSGAPI static void * threadRun(void * data);

		//! Directory for storing the cache objects.
		const Util::TemporaryDirectory cacheDir;

		//! Guard for @a locations and @a cacheObjectsToSave
		mutable std::mutex internalMutex;

		//! Mapping from cache objects to their locations.
		std::unordered_map<CacheObject *, std::pair<Util::FileName, uint32_t>> locations;

		//! Pending cache objects that have to be written to disk.
		mutable std::unordered_map<CacheObject *, Util::Reference<Rendering::Mesh>> cacheObjectsToSave;

		//! Create a file storing the cache object.
		MINSGAPI void doAddCacheObject(CacheObject * object) override;

		//! Remove the file storing the cache object.
		MINSGAPI void doRemoveCacheObject(CacheObject * object) override;

		//! Load a cache object from a file.
		MINSGAPI bool doLoadCacheObject(CacheObject * object) override;

		//! Do nothing
		void doWork() override {
		}

		//! Return the file size of the cache object.
		MINSGAPI uint64_t getCacheObjectSize(CacheObject * object) const override;

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		//! Check all cache objects stored in this cache level for inconsistencies.
		MINSGAPI void doVerify() const override;
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	public:
		MINSGAPI CacheLevelFiles(uint64_t cacheSize, CacheContext & cacheContext);
		MINSGAPI virtual ~CacheLevelFiles();

		//! Start the worker thread
		MINSGAPI void init() override;
};

}
}

#endif /* OUTOFCORE_CACHELEVELFILES_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
