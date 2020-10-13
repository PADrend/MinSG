/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHEMANAGER_H_
#define OUTOFCORE_CACHEMANAGER_H_

#include "CacheContext.h"
#include "Definitions.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Rendering {
class Mesh;
}
namespace MinSG {
class Statistics;
namespace OutOfCore {
class CacheLevel;
class CacheObject;

/**
 * Class to manage the cache levels and the positions of the cache objects inside these cache levels based on the given priorities.
 * It is the interface between the environment using Rendering::Mesh and the cache system using CacheObject.
 *
 * @author Benjamin Eikel
 * @date 2011-02-21
 */
class CacheManager {
	private:
		//! Container for cache objects.
		std::deque<std::unique_ptr<CacheObject>> objects;

		//! Structure holding shared global information.
		CacheContext context;

		//! Mapping to translate a mesh into its corresponding cache object.
		std::unordered_map<Rendering::Mesh *, CacheObject *> meshToObject;

		//! Cache hierarchy.
		std::vector<std::unique_ptr<CacheLevel>> levels;

		//! Frame counter that is incremented by one for each call of @a trigger().
		uint32_t frameNumber;

	public:
		MINSGAPI CacheManager();

		MINSGAPI ~CacheManager();

		/**
		 * Change the user priority of a mesh inside the cache hierarchy.
		 * All cache levels have to be traversed for this change.
		 * Therefore this is a rather costly operation.
		 * Nothing is done if the new priority is equal to the old priority.
		 *
		 * @param mesh Mesh to change the priority of.
		 * @param userPriority New priority for the given mesh.
		 * @return Previous user priority
		 * @throw std::exception if an error occurred (e.g. the given mesh is unknown).
		 */
		MINSGAPI uint16_t setUserPriority(Rendering::Mesh * mesh, uint16_t userPriority);

		/**
		 * Inform this manager that a mesh is displayed.
		 * This changes the priority of a mesh inside the cache hierarchy.
		 * All cache levels have to be traversed for this change.
		 * Therefore this is a rather costly operation.
		 *
		 * @param mesh Mesh that is displayed
		 * @throw std::exception if an error occurred (e.g. the given mesh is unknown).
		 */
		MINSGAPI void meshDisplay(Rendering::Mesh * mesh);

		/**
		 * Add a new level to the top of the cache hierarchy.
		 * For creating a cache hierarchy the levels have to be added from bottom (e.g. network) to top (e.g. graphics memory).
		 *
		 * @param type Type of the cache level to add
		 * @param size Size of the cache level in bytes
		 * @return Identifier of the new cache level
		 * @throw std::exception if an error occurred
		 */
		MINSGAPI cacheLevelId_t addCacheLevel(CacheLevelType type, uint64_t size);

		//! Remove all cache levels and cache objects.
		MINSGAPI void clear();

		/**
		 * Return the cache level with the given identifier.
		 *
		 * @return Pointer to a constant cache level.
		 */
		const CacheLevel * getCacheLevel(cacheLevelId_t levelId) const {
			if (levelId >= levels.size()) {
				return nullptr;
			}
			return levels[levelId].get();
		}

		/**
		 * Add a new mesh that is currently located at a file system location.
		 *
		 * @note The location, from which the mesh can be loaded, is stored inside the mesh.
		 * @param mesh Currently empty mesh.
		 * @throw std::exception in case of an error (e.g. there is no file system cache level).
		 */
		MINSGAPI void addFileSystemObject(Rendering::Mesh * mesh);

		/**
		 * Remove a cache object that is too large for the cache system. A
		 * warning message is generated for it and output on stdout. The cache
		 * object is remove from all entities in the out-of-core system and
		 * its data strategy is changed
		 * 
		 * @param object Cache object that is too large
		 * @param levelId Cache level that reports the large cache object
		 * @param size Size of the given cache object
		 */
		MINSGAPI void removeLargeCacheObject(CacheObject * object,
									cacheLevelId_t levelId,
									uint64_t size);

		/**
		 * Do the real work here: Swap cache objects in and out.
		 * This function is called by a frame listener before each frame.
		 */
		MINSGAPI void trigger();

		/**
		 * Tell the statistics object the fill levels of the cache levels.
		 *
		 * @param statistics Statistics object.
		 */
		MINSGAPI void updateStatistics(Statistics & statistics);

		//! Access the associated cache context.
		CacheContext & getCacheContext() {
			return context;
		}
};

}
}

#endif /* OUTOFCORE_CACHEMANAGER_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
