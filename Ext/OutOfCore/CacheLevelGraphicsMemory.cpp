/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "CacheLevelGraphicsMemory.h"
#include "CacheContext.h"
#include "CacheManager.h"
#include "OutOfCore.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Util/Timer.h>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace MinSG {
namespace OutOfCore {

CacheLevelGraphicsMemory::CacheLevelGraphicsMemory(uint64_t cacheSize, CacheContext & cacheContext) :
	CacheLevel(cacheSize, cacheContext) {
}

CacheLevelGraphicsMemory::~CacheLevelGraphicsMemory() = default;

void CacheLevelGraphicsMemory::doAddCacheObject(CacheObject * object) {
	Rendering::Mesh * mesh = getContext().getContent(object);

	if (mesh->isUsingIndexData() && !mesh->_getIndexData().isUploaded()) {
		if (!mesh->_getIndexData().hasLocalData()) {
			throw std::logic_error("Cache object has no index data in main memory.");
		}
		if (!mesh->_getIndexData().upload()) {
			throw std::logic_error("Cannot upload index data of cache object.");
		}
	}
	if (!mesh->_getVertexData().isUploaded()) {
		if (!mesh->_getVertexData().hasLocalData()) {
			throw std::logic_error("Cache object has no vertex data in main memory.");
		}
		if (!mesh->_getVertexData().upload()) {
			throw std::logic_error("Cannot upload vertex data of cache object.");
		}
	}
}

void CacheLevelGraphicsMemory::doRemoveCacheObject(CacheObject * object) {
	Rendering::Mesh * mesh = getContext().getContent(object);

	mesh->_getVertexData().removeGlBuffer();
	mesh->_getIndexData().removeGlBuffer();
}

uint64_t CacheLevelGraphicsMemory::getCacheObjectSize(CacheObject * object) const {
	const Rendering::Mesh * mesh = getContext().getContent(object);
	const Rendering::MeshVertexData & vertexData = mesh->_getVertexData();
	const Rendering::MeshIndexData & indexData = mesh->_getIndexData();
	return	vertexData.getVertexCount() * vertexData.getVertexDescription().getVertexSize()
			+ indexData.getIndexCount() * sizeof(uint32_t);
}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
void CacheLevelGraphicsMemory::doVerify() const {
	for(const auto & object : getContext().getObjectsInLevel(*this)) {
		const Rendering::Mesh * mesh = getContext().getContent(object);
		if (mesh->isUsingIndexData() && !mesh->_getIndexData().isUploaded()) {
			throw std::logic_error("Index data not uploaded.");
		}
		if (!mesh->_getVertexData().isUploaded()) {
			throw std::logic_error("Vertex data not uploaded.");
		}
	}
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

void CacheLevelGraphicsMemory::doWork() {
	if(getLower() == nullptr) {
		throw std::logic_error("No lower cache level.");
	}

	Util::Timer duration;
	duration.reset();

	const auto maxMemory = 0.95 * getOverallMemory();

	const uint64_t tenMilliseconds = 10000000;
	while(duration.getNanoseconds() < tenMilliseconds) {
		if(getUsedMemory() < 0.8 * getOverallMemory()) {
			// Prefetch
			CacheObject * request = getContext().getMostImportantMissingObject(*this);
			if(request != nullptr && getLower()->loadCacheObject(request)) {
				const auto objectSize = getCacheObjectSize(request);
				if(objectSize > 0.5 * getOverallMemory()) {
					getCacheManager().removeLargeCacheObject(request, levelId, objectSize);
				} else if(getUsedMemory() + objectSize < maxMemory) {
					addCacheObject(request);
				}
				/* FIXME missing else case with break? wrong constants?
				 * assume: getUsedMemory() = 0.7 * getOverallMemory()
				 * assume: objectSize = 0.4 * getOverallMemory()
				 * ==> spending 10 ms with doing nothing */
			} else {
				break;
			}
		} else if(!getContext().isTargetStateReached(*this)) {
			CacheObject * request = getContext().getMostImportantMissingObject(*this);
			if(request != nullptr && getLower()->loadCacheObject(request)) {
				/* FIXME
				 * assume: in priorities = 1,2,4
				 * assume: request priority = 3
				 * assume: to move in requested object, two objects have to be removed
				 * ==> 2 -> out, 4 -> out, 3 -> in
				 * ==> next frame: reverse action 3 -> out, 2 -> in
				 * add priority as parameter to removeUnimportantCacheObjects? */
				removeUnimportantCacheObjects(maxMemory - getCacheObjectSize(request));
				addCacheObject(request);
			}
			// No else: Cache object might have been removed from main memory in between.
		} else {
			break;
		}
	}
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
