/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2011 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#include "OutOfCore.h"
#include "CacheManager.h"
#include "DataStrategy.h"
#include "Definitions.h"
#include "ImportHandler.h"
#include "MeshAttributeSerialization.h"
#include "CacheLevel.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Statistics.h"
#include "../../SceneManagement/Importer/ImporterTools.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Serialization/Serialization.h>
#include <cstdint>

namespace MinSG {
namespace OutOfCore {

//! Stores the status of the out-of-core system. If @a setUp was called at least once, it is enabled.
static bool systemEnabled = false;

CacheManager & getCacheManager() {
	// Singleton
	static CacheManager manager;
	return manager;
}

DataStrategy & getDataStrategy() {
	// Singleton
	static DataStrategy strategy(getCacheManager());
	return strategy;
}

// Trigger the CacheManager in each frame and update the OutOfCore statistics.
class TriggerCallback {
	private:
		CacheManager & cacheManager;

	public:
		TriggerCallback(CacheManager & outOfCoreCacheManager) :
			 cacheManager(outOfCoreCacheManager) {
		}

		bool operator()(FrameContext & frameContext) {
			static const Statistics::eventType_t EVENT_TYPE_OUTOFCORE_BEGIN = 8;
			static const Statistics::eventType_t EVENT_TYPE_OUTOFCORE_END = 9;

			Statistics & statistics = frameContext.getStatistics();

			statistics.pushEvent(EVENT_TYPE_OUTOFCORE_BEGIN, 1.0);
			cacheManager.trigger();
			statistics.pushEvent(EVENT_TYPE_OUTOFCORE_END, 1.0);

			cacheManager.updateStatistics(statistics);

			// Execute again next frame.
			return false;
		}
};

void setUp(FrameContext & context) {
	CacheManager & manager = getCacheManager();

	// Let meshes that are loaded by the SceneManager be handled by the OutOfCore system.
	std::unique_ptr<SceneManagement::MeshImportHandler> handler(new ImportHandler);
	SceneManagement::ImporterTools::setMeshImportHandler(std::move(handler));
	initMeshAttributeSerialization();

	context.addEndFrameListener(TriggerCallback(manager));

	systemEnabled = true;
}

void shutDown() {
	systemEnabled = false;

	// Restore default import handler for meshes.
	std::unique_ptr<SceneManagement::MeshImportHandler> handler(new SceneManagement::MeshImportHandler);
	SceneManagement::ImporterTools::setMeshImportHandler(std::move(handler));

	getCacheManager().clear();
}

bool isSystemEnabled() {
	return systemEnabled;
}

Rendering::Mesh * addMesh(const Util::FileName & meshFile, const Geometry::Box & meshBB) {
	if(systemEnabled) {
		auto mesh = new Rendering::Mesh;
		mesh->_getVertexData()._setBoundingBox(meshBB);
		mesh->setDataStrategy(&getDataStrategy());
		mesh->setFileName(meshFile);

		getCacheManager().addFileSystemObject(mesh);

		return mesh;
	} else {
		Rendering::Mesh * mesh = Rendering::Serialization::loadMesh(meshFile);
		return mesh;
	}
}

}
}

#endif /* MINSG_EXT_OUTOFCORE */
