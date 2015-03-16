/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "CacheContext.h"
#include "CacheLevel.h"
#include "CacheObject.h"
#include <Rendering/Mesh/Mesh.h>
#include <functional>
#include <mutex>

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
#include "CacheObjectPriority.h"
#include <cassert>
#include <ostream>
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

namespace MinSG {
namespace OutOfCore {

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
static std::ostream & operator<<(std::ostream & stream, const CacheObjectPriority & prio) {
    return stream << 
			prio.getUserPriority() << '/' <<
			prio.getUsageFrameNumber() << '/' <<
			prio.getUsageCount();
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

CacheContext::CacheContext() :
		cacheObjectsMutex(),
		sortedCacheObjects(), sortedCacheObjectsBuffer(),
		containersSameBegin(0), 
		updatedCacheObjects(), additionalCacheObjects(0),
		firstMissingCache(), lastContainedCache(),
		contentMutex() {
	firstMissingCache.fill(0);
	lastContainedCache.fill(0);
}

CacheContext::~CacheContext() = default;

void CacheContext::addObject(CacheObject * object) {
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	assert(object->updated);
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	object->updated = true;
	updatedCacheObjects.push_back(object);
	++additionalCacheObjects;
	containersSameBegin = 0;
}

void CacheContext::removeObject(CacheObject * object) {
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);
	if(object->updated) {
		updatedCacheObjects.erase(std::remove(updatedCacheObjects.begin(), updatedCacheObjects.end(), object),
								  updatedCacheObjects.end());
	}
	sortedCacheObjects.erase(std::remove(sortedCacheObjects.begin(), sortedCacheObjects.end(), object),
							 sortedCacheObjects.end());
	sortedCacheObjectsBuffer.erase(std::remove(sortedCacheObjectsBuffer.begin(), sortedCacheObjectsBuffer.end(), object),
								   sortedCacheObjectsBuffer.end());
	containersSameBegin = 0;
}

void CacheContext::onEndFrame(const std::vector<CacheLevel *> & levels) {
	for(const auto & level : levels) {
		level->lockContainer();
	}
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);
	if(updatedCacheObjects.empty()) {
		for(const auto & level : levels) {
			level->unlockContainer();
		}
		return;
	}
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	for(const auto & element : updatedCacheObjects) {
		assert(element->updated);
	}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	std::sort(updatedCacheObjects.begin(), updatedCacheObjects.end(), CacheObjectCompare());

	const bool numCacheObjectsChanged = (additionalCacheObjects != 0);
	const auto oldContainersSameBegin = containersSameBegin;

	if(sortedCacheObjects.empty()) {
		// There are no cache objects yet. Simply replace both arrays.
		sortedCacheObjects = updatedCacheObjects;
		sortedCacheObjectsBuffer = updatedCacheObjects;
		containersSameBegin = std::distance(sortedCacheObjects.cbegin(), sortedCacheObjects.cend());
		additionalCacheObjects = 0;
	} else if(numCacheObjectsChanged) {
		// New cache objects were added. Update the first array in place.
		sortedCacheObjects.reserve(sortedCacheObjects.size() + additionalCacheObjects);
		sortedCacheObjects.erase(std::remove_if(sortedCacheObjects.begin(), 
												sortedCacheObjects.end(), 
												[](const CacheObject * object) { return object->updated; }),
								 sortedCacheObjects.end());
		const auto oldSortedSize = sortedCacheObjects.size();
		sortedCacheObjects.insert(sortedCacheObjects.end(), updatedCacheObjects.begin(), updatedCacheObjects.end());
		std::inplace_merge(sortedCacheObjects.begin(),
						   std::next(sortedCacheObjects.begin(), static_cast<std::ptrdiff_t>(oldSortedSize)),
						   sortedCacheObjects.end(),
						   CacheObjectCompare());
		// Copy the first array to the second array.
		sortedCacheObjectsBuffer = sortedCacheObjects;
		containersSameBegin = std::distance(sortedCacheObjects.cbegin(), sortedCacheObjects.cend());
		additionalCacheObjects = 0;
	} else {
		auto oldIt = sortedCacheObjects.cbegin();
		const auto oldEnd = sortedCacheObjects.cend();
		auto newIt = updatedCacheObjects.cbegin();
		const auto newEnd = updatedCacheObjects.cend();

		auto outputIt = sortedCacheObjectsBuffer.begin();

		auto numUpdatesToSearch = updatedCacheObjects.size();
		containersSameBegin = 0;

		CacheObjectCompare prioCompare;
		while(oldIt != oldEnd && newIt != newEnd) {
			if(numUpdatesToSearch > 0 && (*oldIt)->updated) {
				// Skip cache object that has been updated
				++oldIt;
				--numUpdatesToSearch;
				continue;
			}
			// No need to check for same cache objects here
			if(prioCompare(*newIt, *oldIt)) {
				*outputIt++ = *newIt++;
			} else {
				*outputIt++ = *oldIt++;
			}
		}
		
		if(oldIt != oldEnd) {
			for(; oldIt != oldEnd; ++oldIt) {
				if(numUpdatesToSearch == 0) {
					// If the last changed cache object has been found,
					// copy only the range up to the old marker position.
					const auto changedEnd = std::prev(sortedCacheObjects.cend(), oldContainersSameBegin);
					if(oldIt < changedEnd) {
						std::copy(oldIt, changedEnd, outputIt);
					}
					containersSameBegin = std::distance(outputIt, sortedCacheObjectsBuffer.end());
					break;
				}
				if((*oldIt)->updated) {
					// Skip cache object that has been updated
					--numUpdatesToSearch;
					continue;
				}
				*outputIt++ = *oldIt;
			}
		} else {
			std::copy(newIt, newEnd, outputIt);
		}

		using std::swap;
		swap(sortedCacheObjects, sortedCacheObjectsBuffer);
	}
	for(const auto & cacheObject : updatedCacheObjects) {
		cacheObject->updated = false;
	}
	updatedCacheObjects.clear();
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	for(const auto & element : sortedCacheObjects) {
		assert(!element->updated);
	}
	assert(std::is_sorted(sortedCacheObjects.cbegin(), sortedCacheObjects.cend(), CacheObjectCompare()));
	assert(std::equal(std::prev(sortedCacheObjectsBuffer.cend(), containersSameBegin), sortedCacheObjectsBuffer.cend(), std::prev(sortedCacheObjects.cend(), containersSameBegin)));
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

	if(numCacheObjectsChanged) {
		// Fully reset pointers if the number of cache objects has been changed
		firstMissingCache.fill(0);
		lastContainedCache.fill(0);
	} else {
		// Always reset pointers for level zero (file system)
		firstMissingCache[0] = 0;
		lastContainedCache[0] = 0;

		// Check if the pointers point into the area that has not been changed
		const auto minSameBegin = std::min(oldContainersSameBegin, containersSameBegin);
		const auto numCacheObjects = static_cast<std::ptrdiff_t>(sortedCacheObjects.size());
		for(cacheLevelId_t levelId = 1; levelId < levels.size(); ++levelId) {
			// Only reset the pointers if they point into the changed area
			const auto firstMissingRev = numCacheObjects - firstMissingCache[levelId];
			if(firstMissingRev > minSameBegin) {
				firstMissingCache[levelId] = 0;
			}
			if(lastContainedCache[levelId] > minSameBegin) {
				lastContainedCache[levelId] = 0;
			}
		}
	}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	for(const auto & level : levels) {
		const auto levelId = level->getLevelId();
		const auto levelContains = std::bind(&CacheObject::isContainedIn, std::placeholders::_1, levelId);
		assert(std::all_of(sortedCacheObjects.cbegin(), std::next(sortedCacheObjects.cbegin(), firstMissingCache[levelId]), levelContains));
		assert(std::none_of(sortedCacheObjects.crbegin(), std::next(sortedCacheObjects.crbegin(), lastContainedCache[levelId]), levelContains));
	}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

	for(const auto & level : levels) {
		level->unlockContainer();
	}
}

uint16_t CacheContext::updateUserPriority(CacheObject * object, uint16_t userPriority) {
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);

	const CacheObjectPriority oldPriority = object->getPriority();
	if (oldPriority.getUserPriority() == userPriority) {
		// Do nothing if the priority has not changed.
		return oldPriority.getUserPriority();
	}
	CacheObjectPriority newPriority(oldPriority);
	newPriority.setUserPriority(userPriority);
	object->setPriority(newPriority);

	if(!object->updated) {
		updatedCacheObjects.push_back(object);
		object->updated = true;
	}

	return oldPriority.getUserPriority();
}

void CacheContext::updateFrameNumber(CacheObject * object, uint32_t frameNumber) {
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);

	CacheObjectPriority newPriority(object->getPriority());
	if (newPriority.getUsageFrameNumber() == frameNumber) {
		newPriority.setUsageCount(newPriority.getUsageCount() + 1);
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		assert(object->updated);
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	} else {
		newPriority.setUsageFrameNumber(frameNumber);
		newPriority.setUsageCount(1);
	}
	object->setPriority(newPriority);

	if(!object->updated) {
		updatedCacheObjects.push_back(object);
		object->updated = true;
	}
}

CacheContext::object_pos_t CacheContext::getFirstMissing(const CacheLevel & level) const {
	const auto levelId = level.getLevelId();
	const auto levelContains = std::bind(&CacheObject::isContainedIn, std::placeholders::_1, levelId);
	const auto cachedBegin = std::next(sortedCacheObjects.cbegin(), firstMissingCache[levelId]);
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	// Verify that the cached value is valid
	assert(std::all_of(sortedCacheObjects.cbegin(), cachedBegin, levelContains));
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	// Use the cached value to start the search.
	auto firstMissingObj = std::find_if_not(cachedBegin, sortedCacheObjects.cend(), levelContains);
	// Update the cached value
	firstMissingCache[levelId] = std::distance(sortedCacheObjects.cbegin(), firstMissingObj);
	return firstMissingObj;
}

CacheContext::rev_object_pos_t CacheContext::getLastContained(const CacheLevel & level) const {
	const auto levelId = level.getLevelId();
	const auto levelContains = std::bind(&CacheObject::isContainedIn, std::placeholders::_1, levelId);
	const auto cachedBegin = std::next(sortedCacheObjects.crbegin(), lastContainedCache[levelId]);
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	// Verify that the cached value is valid
// 	assert(std::none_of(sortedCacheObjects.crbegin(), cachedBegin, levelContains));
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	// Use the cached value to start the search.
	auto lastContainedObj = std::find_if(cachedBegin, sortedCacheObjects.crend(), levelContains);
	// Update the cached value
	lastContainedCache[levelId] = std::distance(sortedCacheObjects.crbegin(), lastContainedObj);
	return lastContainedObj;
}

CacheObject * CacheContext::getMostImportantMissingObject(const CacheLevel & level) {
	level.lockContainer();
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);
	const auto firstMissingObj = getFirstMissing(level);
	level.unlockContainer();
	if(firstMissingObj == sortedCacheObjects.cend()) {
		return nullptr;
	}
	return *firstMissingObj;
}

CacheObject * CacheContext::getLeastImportantStoredObject(const CacheLevel & level) {
	level.lockContainer();
	std::lock_guard<std::mutex> contentLock(contentMutex);
	std::lock_guard<std::mutex> cacheObjectsLock(cacheObjectsMutex);
	const auto levelId = level.getLevelId();
	CacheObject * result = nullptr;
	while(result == nullptr) {
		const auto lastContainedObj = getLastContained(level);
		if(lastContainedObj == sortedCacheObjects.crend()) {
			break;
		}
		if((*lastContainedObj)->getHighestLevelStored() == levelId) {
			result = *lastContainedObj;
			break;
		}
		// Do not return a cache object that is stored in a higher cache level.
		// This makes sure that the requesting cache level is allowed to remove the cache object.
		++lastContainedCache[levelId];
	}
	level.unlockContainer();
	return result;
}

bool CacheContext::isTargetStateReached(const CacheLevel & level) const {
	level.lockContainer();
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);

	const auto firstMissingPos = std::distance(sortedCacheObjects.cbegin(), getFirstMissing(level));
	const auto lastContainedPos = std::distance(sortedCacheObjects.cbegin(), getLastContained(level).base());

	level.unlockContainer();
	return lastContainedPos < firstMissingPos;
}

Rendering::Mesh * CacheContext::getContent(CacheObject * object) {
	std::lock_guard<std::mutex> lock(contentMutex);
	return object->getContent();
}

const Rendering::Mesh * CacheContext::getContent(CacheObject * object) const {
	std::lock_guard<std::mutex> lock(contentMutex);
	return object->getContent();
}

void CacheContext::setContent(CacheObject * object, Rendering::Mesh * newContent) {
	std::lock_guard<std::mutex> lock(contentMutex);
	
	Rendering::Mesh * content = object->getContent();
	
	content->_getIndexData().swap(newContent->_getIndexData());
	content->_getVertexData().swap(newContent->_getVertexData());
	content->setDrawMode(newContent->getDrawMode());
	content->setUseIndexData(newContent->isUsingIndexData());
	// Do not change fileName and dataStrategy
}

void CacheContext::lockContentMutex() {
	contentMutex.lock();
}

void CacheContext::unlockContentMutex() {
	contentMutex.unlock();
}

void CacheContext::addObjectToLevel(CacheObject * object, const CacheLevel & level) {
	{
		std::lock_guard<std::mutex> lock(contentMutex);
		const auto levelId = level.getLevelId();
		if(levelId != 0 && object->getHighestLevelStored() == levelId) {
			throw std::logic_error("Cache object is already stored in the given cache level.");
		} else if(object->getHighestLevelStored() > levelId) {
			throw std::logic_error("Cache object is already stored in an upper cache level.");
		} else if(levelId != 0 && object->getHighestLevelStored() != levelId - 1) {
			throw std::logic_error("Cache object is not stored in the previous cache level.");
		}
		object->setHighestLevelStored(levelId);
	}
	
	
	
	
// void afterRequestedObjectAdded(const CacheLevel & level, CacheObject * object) {
	const auto levelId = level.getLevelId();
	if(levelId != 0) {
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);
	auto firstMissingObj = std::next(sortedCacheObjects.cbegin(), firstMissingCache[levelId]);
	// When pointing still to the same cache object, the cache can be used.
	if(object == *firstMissingObj) {
		// Invalidate the other pointer if necessary
		const auto addedObjectRevPos = std::distance(firstMissingObj, sortedCacheObjects.cend());
		if(addedObjectRevPos == lastContainedCache[levelId]) {
			--lastContainedCache[levelId];
		} /*else if(addedObjectRevPos < lastContainedCache[levelId]) {
			lastContainedCache[levelId] = 0;
		}*/

		// The added cache object is skipped during the next search
		getFirstMissing(level);
	} /*else {
		firstMissingCache[levelId] = 0;
		lastContainedCache[levelId] = 0;
	}*/

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	// Verify that the cached values are valid
	const auto levelContains = std::bind(&CacheObject::isContainedIn, std::placeholders::_1, levelId);
	assert(std::all_of(sortedCacheObjects.cbegin(), std::next(sortedCacheObjects.cbegin(), firstMissingCache[levelId]), levelContains));
// 	assert(std::none_of(sortedCacheObjects.crbegin(), std::next(sortedCacheObjects.crbegin(), lastContainedCache[levelId]), levelContains));
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	}
}

void CacheContext::removeObjectFromLevel(CacheObject * object, const CacheLevel & level) {
	{
		std::lock_guard<std::mutex> lock(contentMutex);
		const auto levelId = level.getLevelId();
		if(object->getHighestLevelStored() > levelId) {
			throw std::logic_error("Cache object is still stored in an upper cache level.");
		} else if(object->getHighestLevelStored() < levelId) {
			throw std::logic_error("Cache object is not stored in the given cache level.");
		}
		if(levelId != 0) {
			object->setHighestLevelStored(levelId - 1);
		}
	}
	
	
	const auto levelId = level.getLevelId();
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);

	auto lastContainedObj = std::next(sortedCacheObjects.crbegin(), lastContainedCache[levelId]);
	// When pointing still to the same cache object, the cache can be used.
	if(object == *lastContainedObj) {
		// Invalidate the other pointer if necessary
		const auto removedObjectPos = std::distance(sortedCacheObjects.cbegin(), lastContainedObj.base());
		if(removedObjectPos == firstMissingCache[levelId]) {
			--firstMissingCache[levelId];
		} /*else if(removedObjectPos < firstMissingCache[levelId]) {
			firstMissingCache[levelId] = 0;
		}*/

		// The removed cache object is skipped during the next search
		getLastContained(level);
	} /*else {
		firstMissingCache[levelId] = 0;
		lastContainedCache[levelId] = 0;
	}*/

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
	// Verify that the cached values are valid
	const auto levelContains = std::bind(&CacheObject::isContainedIn, std::placeholders::_1, levelId);
	assert(std::all_of(sortedCacheObjects.cbegin(), std::next(sortedCacheObjects.cbegin(), firstMissingCache[levelId]), levelContains));
// 	assert(std::none_of(sortedCacheObjects.crbegin(), std::next(sortedCacheObjects.crbegin(), lastContainedCache[levelId]), levelContains));
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
}

bool CacheContext::isObjectStoredInLevel(const CacheObject * object, const CacheLevel & level) const {
	std::lock_guard<std::mutex> lock(contentMutex);
	return object->isContainedIn(level.getLevelId());
}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
std::vector<CacheObject *> CacheContext::getObjectsInLevel(const CacheLevel & level) const {
	const auto levelId = level.getLevelId();
	const auto levelContains = std::bind(&CacheObject::isContainedIn, std::placeholders::_1, levelId);
	std::vector<CacheObject *> objectsInLevel;
	std::lock_guard<std::mutex> lock(cacheObjectsMutex);
	std::copy_if(sortedCacheObjects.cbegin(), sortedCacheObjects.cend(), std::back_inserter(objectsInLevel), levelContains);
	return objectsInLevel;
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

}
}

#endif /* MINSG_EXT_OUTOFCORE */
