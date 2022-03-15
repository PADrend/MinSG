/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "CacheLevelMainMemory.h"
#include "CacheContext.h"
#include "CacheManager.h"
#include "OutOfCore.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace MinSG {
namespace OutOfCore {

CacheLevelMainMemory::CacheLevelMainMemory(uint64_t cacheSize, CacheContext & cacheContext) :
	CacheLevel(cacheSize, cacheContext), 
	threadMutex(), threadSemaphore(),
	thread(), active(false) {
}

CacheLevelMainMemory::~CacheLevelMainMemory() {
	{
		std::lock_guard<std::mutex> lock(threadMutex);
		active = false;
	}
	threadSemaphore.notify_all();
	thread.join();
}

void * CacheLevelMainMemory::threadRun(void * data) {
	CacheLevelMainMemory * level = static_cast<CacheLevelMainMemory *>(data);
	const uint64_t maxMemory = static_cast<uint64_t>(0.95 * level->getOverallMemory());
	while(true) {
		if(level->getLower() == nullptr) {
			continue;
		}
		bool workDone = false;
		{
			std::lock_guard<std::mutex> lock(level->threadMutex);
			if(!level->active) {
				break;
			}

			// Keep lock here to make sure that the thread is not stopped while working with an object.
			if(level->getUsedMemory() < 0.8 * level->getOverallMemory()) {
				// Prefetch
				CacheObject * request = level->getContext().getMostImportantMissingObject(*level);
				if(request != nullptr && level->getLower()->loadCacheObject(request)) {
					const auto objectSize = level->getCacheObjectSize(request);
					if(objectSize > 0.5 * level->getOverallMemory()) {
						getCacheManager().removeLargeCacheObject(request, level->levelId, objectSize);
					} else if(level->getUsedMemory() + objectSize < maxMemory) {
						level->addCacheObject(request);
					}
					workDone = true;
				}
			} else if(!level->getContext().isTargetStateReached(*level)) {
				CacheObject * request = level->getContext().getMostImportantMissingObject(*level);
				if(request != nullptr && level->getLower()->loadCacheObject(request)) {
					level->removeUnimportantCacheObjects(maxMemory - level->getCacheObjectSize(request));
					level->addCacheObject(request);
					workDone = true;
				} else {
					throw std::logic_error("Error loading cache object.");
				}
			}
		}
		if(!workDone) {
			std::unique_lock<std::mutex> lock(level->threadMutex);
			level->threadSemaphore.wait(lock);
		}
	}
	return nullptr;
}

void CacheLevelMainMemory::doAddCacheObject(CacheObject * object) {
	Rendering::Mesh * mesh = getContext().getContent(object);
	if (mesh->isUsingIndexData() && !mesh->_getIndexData().hasLocalData()) {
		if (!mesh->_getIndexData().isUploaded()) {
			throw std::logic_error("Cache object has no index data in graphics memory.");
		}
		if (!mesh->_getIndexData().download()) {
			throw std::logic_error("Cannot download index data of cache object.");
		}
	}
	if (!mesh->_getVertexData().hasLocalData()) {
		if (!mesh->_getVertexData().isUploaded()) {
			throw std::logic_error("Cache object has no vertex data in graphics memory.");
		}
		if (!mesh->_getVertexData().download()) {
			throw std::logic_error("Cannot download vertex data of cache object.");
		}
	}
}

void CacheLevelMainMemory::doRemoveCacheObject(CacheObject * object) {
	Rendering::Mesh * mesh = getContext().getContent(object);

	mesh->_getVertexData().releaseLocalData();
	mesh->_getIndexData().releaseLocalData();
}

bool CacheLevelMainMemory::doLoadCacheObject(CacheObject * object) {
	return getContext().isObjectStoredInLevel(object, *this);
}

uint64_t CacheLevelMainMemory::getCacheObjectSize(CacheObject * object) const {
	const Rendering::Mesh * mesh = getContext().getContent(object);
	const Rendering::MeshVertexData & vertexData = mesh->_getVertexData();
	const Rendering::MeshIndexData & indexData = mesh->_getIndexData();
	return	sizeof(Rendering::Mesh)
			+ vertexData.getVertexCount() * vertexData.getVertexDescription().getVertexSize()
			+ indexData.getIndexCount() * sizeof(uint32_t);
}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
void CacheLevelMainMemory::doVerify() const {
	for(const auto & object : getContext().getObjectsInLevel(*this)) {
		const Rendering::Mesh * mesh = getContext().getContent(object);
		if(mesh->isUsingIndexData() && !mesh->_getIndexData().hasLocalData()) {
			throw std::logic_error("No local index data.");
		}
		if(!mesh->_getVertexData().hasLocalData()) {
			throw std::logic_error("No local vertex data.");
		}
	}
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

void CacheLevelMainMemory::doWork() {
	threadSemaphore.notify_all();
}

void CacheLevelMainMemory::init() {
	std::lock_guard<std::mutex> lock(threadMutex);
	if(!active) {
		active = true;
		thread = std::thread(std::bind(&CacheLevelMainMemory::threadRun, this));
	}
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
