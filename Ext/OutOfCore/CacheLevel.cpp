/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "CacheLevel.h"
#include "CacheContext.h"
#include "Definitions.h"
#include <Util/Timer.h>
#include <functional>
#include <mutex>
#include <utility>

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
#include <Util/Macros.h>
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

namespace MinSG {
namespace OutOfCore {

cacheLevelId_t CacheLevel::levelCount = 0;

CacheLevel::CacheLevel(uint64_t cacheSize, CacheContext & cacheContext) :
	containerMutex(),
	memoryOverall(cacheSize), memoryUsed(0), numCacheObjects(0),
	upper(nullptr), lower(nullptr), context(cacheContext),
	lastWorkDuration(0.0), levelId(levelCount++) {
}

CacheLevel::~CacheLevel() = default;

void CacheLevel::removeUnimportantCacheObjects(uint64_t maximumMemory) {
	while(getUsedMemory() > maximumMemory) {
		CacheObject * unimportant = context.getLeastImportantStoredObject(*this);
		if(unimportant == nullptr) {
			// No cache objects left to remove.
			// FIXME may this cause a fill level > 100% ?
			return;
		}
		removeCacheObject(unimportant);
	}
}

uint64_t CacheLevel::getUsedMemory() const {
	std::lock_guard<std::mutex> containerLock(containerMutex);
	return memoryUsed;
}

void CacheLevel::addCacheObject(CacheObject * object) {
	std::lock_guard<std::mutex> containerLock(containerMutex);
	context.addObjectToLevel(object, *this);
	doAddCacheObject(object);
	memoryUsed += getCacheObjectSize(object);
	++numCacheObjects;
}

void CacheLevel::removeCacheObject(CacheObject * object) {
	std::lock_guard<std::mutex> containerLock(containerMutex);
	--numCacheObjects;
	memoryUsed -= getCacheObjectSize(object);
	doRemoveCacheObject(object);
	context.removeObjectFromLevel(object, *this);
}

bool CacheLevel::loadCacheObject(CacheObject * object) {
	return doLoadCacheObject(object);
}

std::size_t CacheLevel::getNumObjects() const {
	std::lock_guard<std::mutex> containerLock(containerMutex);
	return numCacheObjects;
}

void CacheLevel::lockContainer() const {
	containerMutex.lock();
}

void CacheLevel::unlockContainer() const {
	containerMutex.unlock();
}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
void CacheLevel::verify() const {
	if(getUsedMemory() > getOverallMemory()) {
		throw std::logic_error("Cache level is overfilled.");
	}

	std::lock_guard<std::mutex> containerLock(containerMutex);
	if(numCacheObjects != context.getObjectsInLevel(*this).size()) {
		throw std::logic_error("Internal cache object counter is invalid.");
	}
	doVerify();
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

void CacheLevel::work() {
	Util::Timer workTimer;
	workTimer.reset();
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	verify();
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

	doWork();

	workTimer.stop();
	lastWorkDuration = workTimer.getMilliseconds();
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
