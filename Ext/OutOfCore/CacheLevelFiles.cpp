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

#include "CacheLevelFiles.h"
#include "CacheContext.h"
#include "DataStrategy.h"
#include "OutOfCore.h"
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/IO/TemporaryDirectory.h>
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/Utils.h>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace MinSG {
namespace OutOfCore {

CacheLevelFiles::CacheLevelFiles(uint64_t cacheSize, CacheContext & cacheContext) :
	CacheLevel(cacheSize, cacheContext),
	threadMutex(), threadSemaphore(),
	thread(), active(false),
	cacheDir("MinSG_OutOfCore"),
	internalMutex(),
	locations(), cacheObjectsToSave() {
}

CacheLevelFiles::~CacheLevelFiles() {
	{
		std::lock_guard<std::mutex> lock(threadMutex);
		active = false;
	}
	threadSemaphore.notify_all();
	thread.join();
}

void * CacheLevelFiles::threadRun(void * data) {
	CacheLevelFiles * level = static_cast<CacheLevelFiles *>(data);
	while(true) {
		{
			std::unique_lock<std::mutex> lock(level->threadMutex);
			level->threadSemaphore.wait(lock);
		}
		{
			std::lock_guard<std::mutex> lock(level->threadMutex);
			if(!level->active) {
				break;
			}
		}

		{
			std::lock_guard<std::mutex> lock(level->internalMutex);
			if(level->cacheObjectsToSave.empty()) {
				throw std::logic_error("Semaphore value does not reflect internal data structure's size.");
			}
			CacheObject * objectToSave = level->cacheObjectsToSave.cbegin()->first;
			Rendering::Mesh * mesh = level->cacheObjectsToSave.cbegin()->second.get();
			const Util::FileName path(level->cacheDir.getPath().toString() + '/' + Util::StringUtils::toString(reinterpret_cast<uintptr_t>(objectToSave)) + ".mmf");
			if (!Rendering::Serialization::saveMesh(mesh, path)) {
				throw std::logic_error("Unable to store the cache object.");
			}
			const uint32_t fileSize = Util::FileUtils::fileSize(path);
			level->locations.insert(std::make_pair(objectToSave, std::make_pair(path, fileSize)));
			level->cacheObjectsToSave.erase(level->cacheObjectsToSave.begin());
		}
	}
	return nullptr;
}

void CacheLevelFiles::doAddCacheObject(CacheObject * object) {
	std::lock_guard<std::mutex> lock(internalMutex);
	Util::Reference<Rendering::Mesh> meshClone = getContext().getContent(object)->clone();
	meshClone->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());
	const bool inserted = cacheObjectsToSave.insert(std::make_pair(object, std::move(meshClone))).second;
	if(!inserted) {
		throw std::logic_error("Cache object is already being saved.");
	}
	threadSemaphore.notify_all();
}

void CacheLevelFiles::doRemoveCacheObject(CacheObject * object) {
	std::lock_guard<std::mutex> lock(internalMutex);
	// Check if the cache object is already saved.
	const auto savedObject = locations.find(object);
	if(savedObject != locations.cend()) {
		const Util::FileName & path = savedObject->second.first;
		if(!Util::FileUtils::remove(path)) {
			throw std::logic_error("Unable to delete the cache object.");
		}
		locations.erase(savedObject);
	}
	// Check if the cache object is waiting for being saved.
	cacheObjectsToSave.erase(object);
}

bool CacheLevelFiles::doLoadCacheObject(CacheObject * object) {
	{
		std::lock_guard<std::mutex> lock(internalMutex);
		// Check if the cache object is already saved.
		const auto savedObject = locations.find(object);
		if(savedObject != locations.cend()) {
			const Util::FileName & path = savedObject->second.first;
			Util::Reference<Rendering::Mesh> mesh = Rendering::Serialization::loadMesh(path);
			if (mesh.isNull()) {
				throw std::logic_error("Cache object could not be loaded.");
			}
			getContext().setContent(object, mesh.get());
			return true;
		}
		// Check if the cache object is waiting for being saved.
		const auto waitingObject = cacheObjectsToSave.find(object);
		if(waitingObject != cacheObjectsToSave.cend()) {
			Util::Reference<Rendering::Mesh> meshClone = waitingObject->second.get()->clone();
			meshClone->setDataStrategy(&getDataStrategy());
			getContext().setContent(object, meshClone.get());
			return true;
		}
	}
	// Load the missing object directly from the lower level cache level.
	if(getLower() == nullptr) {
		throw std::logic_error("No lower cache level.");
	}
	if(!getLower()->loadCacheObject(object)) {
		return false;
	}

	const auto maxMemory = 0.95 * getOverallMemory();
	removeUnimportantCacheObjects(maxMemory);
	addCacheObject(object);
	return true;
}

uint64_t CacheLevelFiles::getCacheObjectSize(CacheObject * object) const {
	std::lock_guard<std::mutex> lock(internalMutex);
	// Check if the cache object is already saved.
	const auto savedObject = locations.find(object);
	if(savedObject != locations.cend()) {
		return savedObject->second.second;
	}
	// Check if the cache object is waiting for being saved.
	const auto waitingObject = cacheObjectsToSave.find(object);
	if(waitingObject != cacheObjectsToSave.cend()) {
		const Rendering::Mesh * mesh = waitingObject->second.get();
		return	sizeof(Rendering::Mesh)
				+ mesh->_getVertexData().getVertexCount() * mesh->_getVertexData().getVertexDescription().getVertexSize()
				+ mesh->_getIndexData().getIndexCount() * sizeof(uint32_t);
	}
	return 0;
}

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
void CacheLevelFiles::doVerify() const {
	std::lock_guard<std::mutex> lock(internalMutex);
	for(const auto & objectToSave : cacheObjectsToSave) {
		// Check that no cache object from cacheObjectsToSave is inside locations
		if(locations.count(objectToSave.first) > 0) {
			throw std::logic_error("Cache object from cacheObjectsToSave is in locations.");
		}
	}
	for(const auto & location : locations) {
		// Check that no cache object from locations is inside cacheObjectsToSave
		if(cacheObjectsToSave.count(location.first) > 0) {
			throw std::logic_error("Cache object from locations is in cacheObjectsToSave.");
		}
		const Util::FileName & fileName = location.second.first;
		if (!Util::FileUtils::isFile(fileName)) {
			throw std::logic_error("File does not exist.");
		}
		const uint32_t fileSize = location.second.second;
		if (Util::FileUtils::fileSize(fileName) != fileSize) {
			throw std::logic_error("Wrong file size.");
		}
	}
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

void CacheLevelFiles::init() {
	std::lock_guard<std::mutex> lock(threadMutex);
	if(!active) {
		active = true;
		thread = std::thread(std::bind(&CacheLevelFiles::threadRun, this));
	}
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
