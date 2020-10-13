/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHELEVEL_H_
#define OUTOFCORE_CACHELEVEL_H_

#include "Definitions.h"
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <vector>

namespace MinSG {
namespace OutOfCore {
class CacheContext;
class CacheObject;

/**
 * Representation of one cache level inside the cache hierarchy.
 *
 * @author Benjamin Eikel
 * @date 2011-02-21
 */
class CacheLevel {
	private:
		//! Counter for cache levels that is incremented for each object that is created.
		static cacheLevelId_t levelCount;

		//! Guard for @a memoryUsed and @a numCacheObjects
		mutable std::mutex containerMutex;

		//! Overall cache size in bytes.
		const uint64_t memoryOverall;

		//! Used cache size in bytes.
		uint64_t memoryUsed;

		//! Counter for the number of cache objects stored in this cache level.
		std::size_t numCacheObjects;

		//! Pointer to the cache level that is directly above this level or @c nullptr if this is the highest level.
		CacheLevel * upper;

		//! Pointer to the cache level that is directly below this level or @c nullptr if this is the lowest level.
		CacheLevel * lower;

		//! Structure holding shared global information.
		CacheContext & context;

		//! Duration in milliseconds of the last call to work().
		double lastWorkDuration;

		/**
		 * Add the given cache object to this cache level.
		 * Really store the data of the cache object inside this cache level.
		 *
		 * @param object Cache object to add
		 * @throw std::exception if an error occurred
		 * @note called by addCacheObject()
		 */
		virtual void doAddCacheObject(CacheObject * object) = 0;

		/**
		 * Remove the given cache object from this cache level.
		 * Really delete the data of the cache object inside this cache level.
		 *
		 * @param object Cache object to remove
		 * @throw std::exception if an error occurred
		 * @note called by removeCacheObject()
		 */
		virtual void doRemoveCacheObject(CacheObject * object) = 0;

		/**
		 * Load a given cache object stored in this cache level into main memory.
		 * 
		 * @param object Cache object to load
		 * @throw std::exception if an error occurred
		 * @note called by loadCacheObject()
		 */
		virtual bool doLoadCacheObject(CacheObject * object) = 0;

		/**
		 * Function that has to be implemented in subclasses to do the real work.
		 * This function is called by the cache level from @a work() once every frame.
		 * Therefore this function should not consume much time.
		 *
		 * @throw std::exception if an error occurred
		 */
		virtual void doWork() = 0;

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		/**
		 * Check the internal data structures.
		 * Verify that the state that is stored in this cache level is the same as the real state of the cache objects.
		 * For example, a cache level storing files could check if the files really exist for all cache objects in this cache level.
		 *
		 * @throw std::exception if there are inconsistencies
		 */
		MINSGAPI void verify() const;

		/**
		 * Check the internal data structures.
		 * Verify that the state that is stored in this cache level is the same as the real state of the cache objects.
		 * For example, a cache level storing files could check if the files really exist for all cache objects in this cache level.
		 *
		 * @throw std::exception if there are inconsistencies
		 * @note @a containerMutex is locked when the function is called
		 */
		virtual void doVerify() const = 0;
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
	protected:
		//! Identifier of this level that is unique inside the cache hierarchy.
		const cacheLevelId_t levelId;

		MINSGAPI CacheLevel(uint64_t cacheSize, CacheContext & cacheContext);

		CacheLevel * getLower() {
			return lower;
		}

		CacheContext & getContext() {
			return context;
		}
		const CacheContext & getContext() const {
			return context;
		}

		/**
		 * Return the size of the given cache object for this cache level (e.g. size in memory, file size).
		 *
		 * @param object Cache object to retrieve the size of.
		 * @return The size of the requested cache object in bytes or zero if the size is not known.
		 */
		virtual uint64_t getCacheObjectSize(CacheObject * object) const = 0;

		/**
		 * Remove unimportant cache objects until the given maximum memory
		 * usage is reached.
		 * 
		 * @param maximumMemory Maximum amount of memory in bytes that is to
		 * be used
		 */
		MINSGAPI void removeUnimportantCacheObjects(uint64_t maximumMemory);

	public:
		MINSGAPI virtual ~CacheLevel();

		cacheLevelId_t getLevelId() const {
			return levelId;
		}

		uint64_t getOverallMemory() const {
			return memoryOverall;
		}

		MINSGAPI uint64_t getUsedMemory() const;

		uint64_t getFreeMemory() const {
			return memoryOverall - getUsedMemory();
		}

		/**
		 * Add the given cache object to this cache level.
		 * Update the internal data structures of this cache level with the new status.
		 *
		 * @param object Cache object to add
		 * @throw std::exception if an error occurred
		 */
		MINSGAPI void addCacheObject(CacheObject * object);

		/**
		 * Remove the given cache object from this cache level.
		 * Update the internal data structures of this cache level with the new status.
		 *
		 * @param object Cache object to remove
		 * @throw std::exception if an error occurred
		 */
		MINSGAPI void removeCacheObject(CacheObject * object);

		/**
		 * Load a given cache object stored in this cache level into main memory.
		 * 
		 * @param object Cache object to load
		 * @throw std::exception if an error occurred
		 */
		MINSGAPI bool loadCacheObject(CacheObject * object);

		//! Return the number of cache objects that are stored inside this cache level.
		MINSGAPI std::size_t getNumObjects() const;

		//! Lock @a containerMutex. Must be used only by CacheContext.
		MINSGAPI void lockContainer() const;

		//! Unlock @a containerMutex. Must be used only by CacheContext.
		MINSGAPI void unlockContainer() const;

		//! Return the duration in milliseconds of the last call to @a work.
		double getLastWorkDuration() const {
			return lastWorkDuration;
		}

		/**
		 * Associate a cache level above this object.
		 * This is only possible if the given cache level has not been associated before and this object has no upper cache level yet.
		 *
		 * @note There is no setLower function.
		 * The cache level given as upper level to this function is modified so that its lower pointer points to this object.
		 * Therefore the cache hierarchy has to be built from bottom to top.
		 * @param newUpper Cache level that is placed directly above this object. Note that the given cache level is also modified.
		 * @throw std::logic_error in case of an error (e.g. one of the cache levels was already associated)
		 */
		void setUpper(CacheLevel * newUpper) {
			if (newUpper == nullptr || upper != nullptr || newUpper->upper != nullptr || newUpper->lower != nullptr || newUpper->levelId != levelId + 1) {
				throw std::logic_error("One of the cache levels already has been associated before.");
			}
			upper = newUpper;
			newUpper->lower = this;
		}

		/**
		 * Do all work that has to be done:
		 * - check the deliveries and add them to this cache level
		 * - handle requests from the upper cache level
		 * - prefetch cache objects if possible
		 *
		 * This function is called by the cache manager once every frame.
		 * Therefore this function should not consume much time.
		 *
		 * @throw std::exception if an error occurred
		 */
		MINSGAPI void work();

		/**
		 * Function called once at the beginning.
		 * This function can be overwritten by subclasses to perform work that
		 * cannot be done in the constructor (e.g. starting an additional
		 * thread).
		 */
		virtual void init() {
		}
};

}
}

#endif /* OUTOFCORE_CACHELEVEL_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
