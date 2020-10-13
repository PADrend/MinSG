/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHECONTEXT_H_
#define OUTOFCORE_CACHECONTEXT_H_

#include "Definitions.h"
#include <array>
#include <memory>
#include <mutex>
#include <vector>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace OutOfCore {
class CacheLevel;
class CacheObject;
class CacheObjectPriority;

/**
 * @brief Context for holding global cache information
 * 
 * Structure holding information needed by different entities in the
 * out-of-core system (e.g. CacheManager, CacheLevel). The class takes care of
 * providing exclusive access to the required data structures, which makes it
 * thread-safe.
 *
 * @author Benjamin Eikel
 * @date 2012-12-05
 */
class CacheContext {
	private:
		//! Guard for @a sortedCacheObjects and @a updatedCacheObjects
		mutable std::mutex cacheObjectsMutex;

		typedef std::vector<CacheObject *> container_t;
		typedef std::vector<CacheObject *>::const_iterator object_pos_t;
		typedef std::vector<CacheObject *>::const_reverse_iterator 
															rev_object_pos_t;

		/**
		 * Container of cache objects sorted by priority. It is not updated
		 * immediately when the priority of a cache object changes. The
		 * container is updated only once after the frame for performance
		 * reasons.
		 */
		container_t sortedCacheObjects;

		/**
		 * Buffer for @a sortedCacheObjects used for circumventing the need to
		 * create a new array every frame.
		 */
		container_t sortedCacheObjectsBuffer;

		/**
		 * Position in @a sortedCacheObjects after the last cache object that
		 * has been updated in the previous frame. From this position to the
		 * end of the container there have been no updates. It is initialized
		 * to the end of the container and moved gradually to the front. The
		 * position is counted from the end of the container, meaning a value
		 * of zero describing the end. If cache object are added or removed,
		 * it is reset to the end. The @a sortedCacheObjects and
		 * @a sortedCacheObjectsBuffer arrays are the same beginning from this
		 * position to the end.
		 */
		std::ptrdiff_t containersSameBegin;

		/**
		 * Container that collects the cache objects that are updated during a
		 * frame.
		 */
		std::vector<CacheObject *> updatedCacheObjects;

		/**
		 * Counter for the additional capacity needed to store cache objects.
		 * Increased by one for every added cache object.
		 */
		std::size_t additionalCacheObjects;

		/**
		 * For each cache level, the cached position of the @b first cache
		 * object inside the array of sorted cache objects, which is <b>not
		 * contained</b> in that cache level. The position is counted from
		 * front to back in @a sortedCacheObjects (zero meaning @c begin).
		 */
		mutable std::array<std::ptrdiff_t, maxNumCacheLevels> firstMissingCache;

		/**
		 * For each cache level, the cached position of the @b last cache
		 * object inside the array of sorted cache objects, which is
		 * @b contained in that cache level. The position is counted from back
		 * to front in @a sortedCacheObjects (zero meaning @c rbegin).
		 */
		mutable std::array<std::ptrdiff_t, maxNumCacheLevels> lastContainedCache;

		//! Guard for the content of cache objects
		mutable std::mutex contentMutex;

		/**
		 * Get the first missing cache object for the given cache level.
		 * 
		 * @param level Cache level
		 * @return Valid position inside @a sortedCacheObjects if a missing
		 * cache object is found, or @codesortedCacheObjects.cend()@endcode
		 * otherwise.
		 * @note @a cacheObjectsMutex and the cache level have to be locked
		 */
		MINSGAPI object_pos_t getFirstMissing(const CacheLevel & level) const;

		/**
		 * Get the last contained cache object for the given cache level.
		 * 
		 * @param level Cache level
		 * @return Valid position inside @a sortedCacheObjects if a contained
		 * cache object is found, or @codesortedCacheObjects.crend()@endcode
		 * otherwise.
		 * @note @a cacheObjectsMutex and the cache level have to be locked
		 */
		MINSGAPI rev_object_pos_t getLastContained(const CacheLevel & level) const;
	public:
		MINSGAPI CacheContext();

		MINSGAPI ~CacheContext();

		//! Inform the context about a new cache object.
		MINSGAPI void addObject(CacheObject * object);

		//! Remove an existing cache object.
		MINSGAPI void removeObject(CacheObject * object);

		/**
		 * Inform the context that the frame has ended. It will perform
		 * maintenance work to restore the correct sort order of
		 * @a sortedCacheObjects incorporating the priority changes of the
		 * last frame.
		 */
		MINSGAPI void onEndFrame(const std::vector<CacheLevel *> & levels);

		/**
		 * Update the user priority of a cache object. If the new user
		 * priority is the same as the old one, nothing will be changed.
		 * 
		 * @param object Cache object to update
		 * @return Previous user priority
		 */
		MINSGAPI uint16_t updateUserPriority(CacheObject * object, 
									uint16_t userPriority);

		/**
		 * Update the frame number in which a cache object was used last. If
		 * the cache objects already has been used in that frame, its usage
		 * count is increased by one. Otherwise its usage frame number is
		 * updated.
		 * 
		 * @param object Cache object to update
		 * @param frameNumber Frame number in which the cache object was used
		 */
		MINSGAPI void updateFrameNumber(CacheObject * object, uint32_t frameNumber);

		/**
		 * Return the cache object with the highest priority that is not
		 * stored in the given cache level. If the cache level does store all
		 * cache objects, @c nullptr is returned.
		 * 
		 * @param level Cache level
		 * @return Most important missing cache object, or @c nullptr
		 */
		MINSGAPI CacheObject * getMostImportantMissingObject(const CacheLevel & level);

		/**
		 * Return the cache object with the smallest priority that is stored
		 * in the given cache level. If the cache level does not store any
		 * cache object, @c nullptr is returned. Additionally, it is checked
		 * that the cache object can be removed by the requesting cache level.
		 * 
		 * @param level Cache level
		 * @return Least important stored cache object, or @c nullptr
		 */
		MINSGAPI CacheObject * getLeastImportantStoredObject(const CacheLevel & level);

		/**
		 * Check if the target state for the given cache level has been
		 * reached. This means that the last stored cache object has at least
		 * the priority of the first missing cache object for that cache
		 * level. If the cache level is full already, it has no work to do.
		 * 
		 * @param level Cache level
		 * @return @c true if the cache level has reached the target state
		 */
		MINSGAPI bool isTargetStateReached(const CacheLevel & level) const;

		//! Access the content of the given cache object.
		MINSGAPI Rendering::Mesh * getContent(CacheObject * object);
		//! Read the content of the given cache object.
		MINSGAPI const Rendering::Mesh * getContent(CacheObject * object) const;
		//! Update the content of the given cache object.
		MINSGAPI void setContent(CacheObject * object, Rendering::Mesh * newContent);

		//! Lock @a contentMutex
		MINSGAPI void lockContentMutex();

		//! Unlock @a contentMutex
		MINSGAPI void unlockContentMutex();

		/**
		 * Inform the cache context that a cache object is to be added to a
		 * cache level.
		 * 
		 * @param object Cache object that will be added
		 * @param level Cache level that will contain the cache object
		 */
		MINSGAPI void addObjectToLevel(CacheObject * object, const CacheLevel & level);

		/**
		 * Inform the cache context that a cache object is to be remove from a
		 * cache level.
		 * 
		 * @param object Cache object that will be removed
		 * @param level Cache level that currently contains the cache object
		 */
		MINSGAPI void removeObjectFromLevel(CacheObject * object, const CacheLevel & level);

		/**
		 * Check if a cache level contains a specific cache object.
		 * 
		 * @param object Cache object
		 * @param level Cache level
		 * @return @c true if the given cache level contains the cache object,
		 * @c false otherwise
		 */
		MINSGAPI bool isObjectStoredInLevel(const CacheObject * object, const CacheLevel & level) const;
		
#ifdef MINSG_EXT_OUTOFCORE_DEBUG
		//! Return all cache objects that are stored in a cache level.
		MINSGAPI std::vector<CacheObject *> getObjectsInLevel(const CacheLevel & level) const;
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */
};

}
}

#endif /* OUTOFCORE_CACHECONTEXT_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
