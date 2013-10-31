/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "CacheManager.h"
#include "CacheLevel.h"
#include "CacheLevelFiles.h"
#include "CacheLevelFileSystem.h"
#include "CacheLevelGraphicsMemory.h"
#include "CacheLevelMainMemory.h"
#include "CacheObject.h"
#include "Definitions.h"
#include "OutOfCore.h"
#include "../../Core/Statistics.h"
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Serialization/Serialization.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace OutOfCore {

CacheManager::CacheManager() :
		objects(), context(),
		meshToObject(), levels(), frameNumber(0) {
	levels.reserve(maxNumCacheLevels);
}

CacheManager::~CacheManager() = default;

uint16_t CacheManager::setUserPriority(Rendering::Mesh * mesh, uint16_t userPriority) {
	if(!isSystemEnabled()) {
		return 0;
	}
	if (levels.empty()) {
		throw std::logic_error("There are no cache levels.");
	}
	const auto it = meshToObject.find(mesh);
	if (it == meshToObject.end()) {
		throw std::logic_error("Unknown mesh requested.");
	}
	CacheObject * object = it->second;
	return context.updateUserPriority(object, userPriority);
}

void CacheManager::meshDisplay(Rendering::Mesh * mesh) {
	if (levels.empty()) {
		throw std::logic_error("There are no cache levels.");
	}
	const auto it = meshToObject.find(mesh);
	if (it == meshToObject.end()) {
		throw std::logic_error("Unknown mesh requested.");
	}
	CacheObject * object = it->second;
	context.updateFrameNumber(object, frameNumber);
}

cacheLevelId_t CacheManager::addCacheLevel(CacheLevelType type, uint64_t size) {
	if (levels.size() == maxNumCacheLevels) {
		throw std::logic_error("Adding cache level failed. The maximum number of cache levels has been exceeded.");
	}
	CacheLevel * previousHighest = nullptr;
	if(!levels.empty()) {
		previousHighest = levels.back().get();
	}
	switch(type) {
		case CacheLevelType::FILE_SYSTEM:
			levels.emplace_back(new CacheLevelFileSystem(context));
			break;
		case CacheLevelType::FILES:
			levels.emplace_back(new CacheLevelFiles(size, context));
			break;
		case CacheLevelType::MAIN_MEMORY:
			levels.emplace_back(new CacheLevelMainMemory(size, context));
			break;
		case CacheLevelType::GRAPHICS_MEMORY:
			levels.emplace_back(new CacheLevelGraphicsMemory(size, context));
			break;
		default:
			throw std::invalid_argument("Adding cache level failed. Invalid cache level type.");
	}
	if(previousHighest != nullptr) {
		previousHighest->setUpper(levels.back().get());
	}
	levels.back()->init();
	return levels.size() - 1;
}

void CacheManager::clear() {
	for(const auto & cacheObject : objects) {
		context.getContent(cacheObject.get())->setDataStrategy(Rendering::MeshDataStrategy::getDefaultStrategy());
	}
	// Remove levels from bottom to top.
	for(auto rIt = levels.rbegin(); rIt != levels.rend(); ++rIt) {
		rIt->reset();
	}
	levels.clear();
	objects.clear();
}

void CacheManager::addFileSystemObject(Rendering::Mesh * mesh) {
	if (levels.empty()) {
		throw std::logic_error("There are no cache levels.");
	}
	if (meshToObject.count(mesh) > 0) {
		// Object already exists
		throw std::logic_error("Cache object has been registered before.");
	}
	// Check if the lowest cache level is a file system cache level.
	CacheLevelFileSystem * level = dynamic_cast<CacheLevelFileSystem *>(levels.front().get());
	if (level == nullptr) {
		throw std::logic_error("Lowest cache level is no file system cache level.");
	}
	auto object = new CacheObject(mesh);
	objects.emplace_back(object);
	context.addObject(object);
	meshToObject.insert(std::make_pair(mesh, object));
	level->addCacheObject(object);
}

void CacheManager::removeLargeCacheObject(CacheObject * object, cacheLevelId_t levelId, uint64_t size) {
	Rendering::Mesh * mesh = context.getContent(object);

	Util::Reference<Rendering::Mesh> newMesh = Rendering::Serialization::loadMesh(mesh->getFileName());
	if (newMesh.isNull()) {
		throw std::logic_error("Mesh could not be loaded.");
	}
	newMesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());

	mesh->setDataStrategy(Rendering::SimpleMeshDataStrategy::getPureLocalStrategy());
	meshToObject.erase(mesh);
	for(const auto & level : levels) {
		level->removeCacheObject(object);
	}
	context.removeObject(object);
	objects.erase(std::remove_if(objects.begin(), 
								 objects.end(), 
								 [&object](const std::unique_ptr<CacheObject> & candidate) { return candidate.get() == object; }),
				  objects.end());

	mesh->swap(*newMesh.get());

	std::cout	<< "Warning: Cache object is too large for cache level " << static_cast<int>(levelId) << ".\n"
				<< "         Size: " << size << " Bytes, File: " << mesh->getFileName() << '\n'
				<< "         The cache object has been removed from the out-of-core system." << std::endl;
}

void CacheManager::trigger() {
	std::vector<CacheLevel *> levelsCopy;
	levelsCopy.reserve(levels.size());
	for(const auto & level : levels) {
		levelsCopy.push_back(level.get());
	}
	context.onEndFrame(levelsCopy);

	for(const auto & level : levels) {
		try {
			level->work();
		} catch(const std::exception & e) {
			WARN("Failure in cache level " + Util::StringUtils::toString<uint32_t>(level->getLevelId()) + ":\n" + e.what());
		}
	}
	++frameNumber;
}

void CacheManager::updateStatistics(Statistics & statistics) {
	static std::vector<uint32_t> counterKeys;
	if(counterKeys.empty()) {
		// Set the descriptions for statistics once here.
		for(auto & level : levels) {
			counterKeys.push_back(statistics.addCounter("Cache " + Util::StringUtils::toString<uint32_t>(level->getLevelId()) + ": Used memory", "MiBytes"));
		}
	}
	const double mebibyte = 1048576.0;
	for(cacheLevelId_t level = 0; level < levels.size(); ++level) {
		statistics.setValue(counterKeys[level], static_cast<double>(levels[level]->getUsedMemory()) / mebibyte);
	}
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
