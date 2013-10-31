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

#include "CacheLevelFileSystem.h"
#include "CacheContext.h"
#include "OutOfCore.h"
#include "DataStrategy.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Serialization/Serialization.h>
#include <Util/References.h>
#include <algorithm>
#include <stdexcept>
#include <utility>

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
#include <Util/IO/FileUtils.h>
namespace Util {
class FileName;
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

namespace MinSG {
namespace OutOfCore {

CacheLevelFileSystem::CacheLevelFileSystem(CacheContext & cacheContext) :
	CacheLevel(0, cacheContext) {
}

CacheLevelFileSystem::~CacheLevelFileSystem() = default;

#ifdef MINSG_EXT_OUTOFCORE_DEBUG
void CacheLevelFileSystem::doVerify() const {
	for(const auto & object : getContext().getObjectsInLevel(*this)) {
		const Util::FileName & fileName = getContext().getContent(object)->getFileName();
		if (!Util::FileUtils::isFile(fileName)) {
			throw std::logic_error("File does not exist.");
		}
	}
}
#endif /* MINSG_EXT_OUTOFCORE_DEBUG */

bool CacheLevelFileSystem::doLoadCacheObject(CacheObject * object) {
	if (!getContext().isObjectStoredInLevel(object, *this)) {
		throw std::logic_error("Cache object is not stored in the lowest cache level.");
	}

	const Util::FileName fileName = getContext().getContent(object)->getFileName();
	Util::Reference<Rendering::Mesh> mesh = Rendering::Serialization::loadMesh(fileName);
	if (mesh.isNull()) {
		throw std::logic_error("Cache object could not be loaded.");
 	}
	getContext().setContent(object, mesh.get());
	return true;
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
