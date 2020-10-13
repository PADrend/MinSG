/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_CACHEOBJECT_H_
#define OUTOFCORE_CACHEOBJECT_H_

#include "CacheObjectPriority.h"
#include "Definitions.h"
#include <Util/References.h>
#include <cstdint>
#include <bitset>

namespace Rendering {
class Mesh;
}

namespace MinSG {
namespace OutOfCore {

/**
 * Capsule for a Mesh that is handled inside a cache system.
 * The cache system may be extended by allowing capsules with other content here.
 *
 * @author Benjamin Eikel
 * @date 2011-02-21
 */
class CacheObject {
	private:
		friend class CacheContext;
		friend struct CacheObjectCompare;

		//! Content of the capsule that is the real data stored in memory.
		const Util::Reference<Rendering::Mesh> content;

		//! Priority for this cache object. It is used to sort cache objects.
		CacheObjectPriority priority;

		//! Maximum identifier of a cache level storing this cache object.
		cacheLevelId_t highestLevelStored;

		//! Flag storing if the cache object was changed in the current frame.
		bool updated;

		CacheObject(const CacheObject &) = delete;
		CacheObject(CacheObject &&) = delete;
		CacheObject & operator=(const CacheObject &) = delete;
		CacheObject & operator=(CacheObject &&) = delete;

		const CacheObjectPriority & getPriority() const {
			return priority;
		}

		void setPriority(const CacheObjectPriority & newPriority) {
			priority = newPriority;
		}

		Rendering::Mesh * getContent() const {
			return content.get();
		}

		bool isContainedIn(cacheLevelId_t levelId) const {
			return levelId <= highestLevelStored;
		}

		cacheLevelId_t getHighestLevelStored() const {
			return highestLevelStored;
		}

		void setHighestLevelStored(cacheLevelId_t levelId) {
			highestLevelStored = levelId;
		}

	public:
		MINSGAPI CacheObject(Rendering::Mesh * mesh);

		MINSGAPI ~CacheObject();
};

//! Structure used to sort cache objects by decreasing priority in STL containers.
struct CacheObjectCompare {
		bool operator()(const CacheObject * a, const CacheObject * b) const {
			const CacheObjectPriority & prioA = a->getPriority();
			const CacheObjectPriority & prioB = b->getPriority();
			return prioB < prioA || (!(prioA < prioB) && b < a);
		}
};

}
}

#endif /* OUTOFCORE_CACHEOBJECT_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
