/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHELEVELFILESYSTEM_H_
#define OUTOFCORE_CACHELEVELFILESYSTEM_H_

#include "CacheLevel.h"
#include <Util/References.h>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <set>
#include <stdexcept>

namespace MinSG {
namespace OutOfCore {

/**
 * Specialized cache level for file system or network read-only access.
 * It supports all file systems that are provided by Util::FileUtils.
 *
 * @author Benjamin Eikel
 * @date 2011-02-23
 */
class CacheLevelFileSystem : public CacheLevel {
	private:
		//! Do nothing
		void doAddCacheObject(CacheObject * /*object*/) override {
		}

		//! Do nothing
		void doRemoveCacheObject(CacheObject * /*object*/) override {
		}

		//! Load a single cache object from a file system, network location, or archive.
		bool doLoadCacheObject(CacheObject * object) override;

		//! Do nothing
		void doWork() override {
		}

		//! Always return zero
		uint64_t getCacheObjectSize(CacheObject * /*object*/) const override {
			return 0;
		}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		//! Check all cache objects stored in this cache level for inconsistencies.
		MINSGAPI void doVerify() const override;
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	public:
		MINSGAPI CacheLevelFileSystem(CacheContext & cacheContext);
		MINSGAPI virtual ~CacheLevelFileSystem();
};

}
}

#endif /* OUTOFCORE_CACHELEVELFILESYSTEM_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
