/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "DataStrategy.h"
#include "CacheManager.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Util/Graphics/ColorLibrary.h>
#include <limits>

namespace MinSG {
namespace OutOfCore {

void DataStrategy::assureLocalVertexData(Rendering::Mesh * mesh) {
	CacheContext & cacheContext = cacheManager.getCacheContext();
	cacheContext.lockContentMutex();

	// Make this first check here to prevent an unnecessary priority change.
	if(!mesh->_getVertexData().empty() && mesh->_getVertexData().hasLocalData()) {
		cacheContext.unlockContentMutex();
		return;
	}
	// Give the cache object the highest priority.
	const auto oldUserPiority = cacheManager.setUserPriority(mesh, std::numeric_limits<uint16_t>::max());
	// Wait until the cache object is available in main memory.
	while(mesh->_getVertexData().empty() || !mesh->_getVertexData().hasLocalData()) {
		cacheContext.unlockContentMutex();
		cacheManager.trigger();
		cacheContext.lockContentMutex();
	}
	cacheManager.setUserPriority(mesh, oldUserPiority);

	cacheContext.unlockContentMutex();
}

void DataStrategy::assureLocalIndexData(Rendering::Mesh * mesh) {
	CacheContext & cacheContext = cacheManager.getCacheContext();
	cacheContext.lockContentMutex();

	// Make this first check here to prevent an unnecessary priority change.
	if(!mesh->_getIndexData().empty() && mesh->_getIndexData().hasLocalData()) {
		cacheContext.unlockContentMutex();
		return;
	}
	// Give the cache object the highest priority.
	const auto oldUserPiority = cacheManager.setUserPriority(mesh, std::numeric_limits<uint16_t>::max());
	// Wait until the cache object is available in main memory.
	while(mesh->_getIndexData().empty() || !mesh->_getIndexData().hasLocalData()) {
		cacheContext.lockContentMutex();
		cacheManager.trigger();
		cacheContext.unlockContentMutex();
	}
	cacheManager.setUserPriority(mesh, oldUserPiority);

	cacheContext.unlockContentMutex();
}

void DataStrategy::displayMesh(Rendering::RenderingContext & context, Rendering::Mesh * m, uint32_t startIndex, uint32_t indexCount) {
	CacheContext & cacheContext = cacheManager.getCacheContext();
	cacheContext.lockContentMutex();

	// Make sure the CacheManager is called even if the mesh is not really displayed.
	// Otherwise it might happen that the mesh will never be swapped in again after it was swapped out once.
	cacheManager.meshDisplay(m);

	if(missingMode == WAIT_DISPLAY) {
		while(m->_getVertexData().empty() ||
				(!m->_getVertexData().isUploaded() && !m->_getVertexData().hasLocalData()) ||
				(m->isUsingIndexData() && 
					(m->_getIndexData().empty() ||
					(!m->_getIndexData().isUploaded() && !m->_getIndexData().hasLocalData())))) {
			cacheContext.unlockContentMutex();
			cacheManager.trigger();
			cacheContext.lockContentMutex();
		}
	}

	const Rendering::MeshVertexData & vd = m->_getVertexData();
	const Rendering::MeshIndexData & id = m->_getIndexData();
	if (vd.empty() || (m->isUsingIndexData() && id.empty())) {
		if(missingMode == NO_WAIT_DISPLAY_COLORED_BOX) {
			Rendering::drawBox(context, vd.getBoundingBox(), Util::ColorLibrary::RED);
		} else if(missingMode == NO_WAIT_DISPLAY_DEPTH_BOX) {
			// Disable color writes
			context.pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
			Rendering::drawBox(context, vd.getBoundingBox());
			context.popColorBuffer();
		}
		// Nothing has to be done here for mode NO_WAIT_DO_NOTHING
		cacheContext.unlockContentMutex();
		return;
	}
	if ((!vd.isUploaded() && !vd.hasLocalData()) || (m->isUsingIndexData() && (!id.isUploaded() && !id.hasLocalData()))) {
		if(missingMode == NO_WAIT_DISPLAY_COLORED_BOX) {
			Rendering::drawBox(context, vd.getBoundingBox(), Util::ColorLibrary::YELLOW);
		} else if(missingMode == NO_WAIT_DISPLAY_DEPTH_BOX) {
			// Disable color writes
			context.pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
			Rendering::drawBox(context, vd.getBoundingBox());
			context.popColorBuffer();
		}
		// Nothing has to be done here for mode NO_WAIT_DO_NOTHING
		cacheContext.unlockContentMutex();
		return;
	}
	DataStrategy::doDisplayMesh(context, m, startIndex, indexCount);

	cacheContext.unlockContentMutex();
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
