/*
	This file is part of the MinSG library.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <Geometry/Box.h>
#include <MinSG/Core/FrameContext.h>
#include <MinSG/Ext/OutOfCore/CacheManager.h>
#include <MinSG/Ext/OutOfCore/CacheLevelFiles.h>
#include <MinSG/Ext/OutOfCore/CacheLevelFileSystem.h>
#include <MinSG/Ext/OutOfCore/CacheLevelMainMemory.h>
#include <MinSG/Ext/OutOfCore/CacheObjectPriority.h>
#include <MinSG/Ext/OutOfCore/Definitions.h>
#include <MinSG/Ext/OutOfCore/OutOfCore.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshIndexData.h>
#include <Rendering/Mesh/MeshVertexData.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Serialization/Serialization.h>
#include <Util/IO/FileName.h>
#include <Util/IO/TemporaryDirectory.h>
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/Timer.h>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <random>
#include <string>
#include <vector>

// Prevent warning
int test_OutOfCore();

#ifdef MINSG_EXT_OUTOFCORE
static const uint64_t kibibyte = 1024;

static void outputCacheLevelInformation() {
	MinSG::OutOfCore::CacheManager & manager = MinSG::OutOfCore::getCacheManager();
	for(MinSG::OutOfCore::cacheLevelId_t levelNumber = 0; levelNumber < MinSG::OutOfCore::maxNumCacheLevels; ++levelNumber) {
		const auto * level = manager.getCacheLevel(levelNumber);
		if(level == nullptr) {
			break;
		}
		std::cout <<	"Level " << static_cast<uint32_t>(levelNumber) << ": " <<
						"usedMemory=" << static_cast<double>(level->getUsedMemory()) / static_cast<double>(kibibyte) << " KiB\t" <<
						"objects=" << level->getNumObjects() << std::endl;
	}
}
#endif /* MINSG_EXT_OUTOFCORE */

int test_OutOfCore() {
#ifdef MINSG_EXT_OUTOFCORE
	const bool verbose = true;

	// Tests for MinSG::OutOfCore::CacheObjectPriority
	if(sizeof(MinSG::OutOfCore::CacheObjectPriority) != 8) {
		return EXIT_FAILURE;
	}
	if(!(MinSG::OutOfCore::CacheObjectPriority(1, 2, 3) == MinSG::OutOfCore::CacheObjectPriority(1, 2, 3))) {
		return EXIT_FAILURE;
	}
	if(!(MinSG::OutOfCore::CacheObjectPriority(1, 100, 100) < MinSG::OutOfCore::CacheObjectPriority(2, 0, 0))) {
		return EXIT_FAILURE;
	}
	if(!(MinSG::OutOfCore::CacheObjectPriority(2, 1, 100) < MinSG::OutOfCore::CacheObjectPriority(2, 2, 0))) {
		return EXIT_FAILURE;
	}
	if(!(MinSG::OutOfCore::CacheObjectPriority(2, 2, 1) < MinSG::OutOfCore::CacheObjectPriority(2, 2, 2))) {
		return EXIT_FAILURE;
	}

	std::default_random_engine engine;
	std::uniform_int_distribution<std::size_t> vertexCountDist(10, 1000);
	const uint32_t numMeshes = 30000;
	const Util::TemporaryDirectory tempDir("MinSGTest_OutOfCore");
	
	// Create empty meshes and save them into a subdirectory.
	{
		Rendering::VertexDescription vertexDesc;
		vertexDesc.appendPosition3D();
		
		for(uint_fast32_t i = 0; i < numMeshes; ++i) {
			Util::Reference<Rendering::Mesh> mesh = new Rendering::Mesh(vertexDesc, vertexCountDist(engine), 64);
			Rendering::MeshVertexData & vertexData = mesh->openVertexData();
			std::fill_n(vertexData.data(), vertexData.dataSize(), 0);
			vertexData.markAsChanged();
			Rendering::MeshIndexData & indexData = mesh->openIndexData();
			std::fill_n(indexData.data(), indexData.getIndexCount(), 0);
			indexData.markAsChanged();
			const std::string numberString = Util::StringUtils::toString<uint32_t>(i);
			Rendering::Serialization::saveMesh(mesh.get(), Util::FileName(tempDir.getPath().getDir() + numberString + ".mmf"));
		}
	}
	
	// Set up the OutOfCore system.
	MinSG::FrameContext frameContext;
	
	MinSG::OutOfCore::setUp(frameContext);

	MinSG::OutOfCore::CacheManager & manager = MinSG::OutOfCore::getCacheManager();
	manager.addCacheLevel(MinSG::OutOfCore::CacheLevelType::FILE_SYSTEM, 0);
	manager.addCacheLevel(MinSG::OutOfCore::CacheLevelType::FILES, 512 * kibibyte);
	manager.addCacheLevel(MinSG::OutOfCore::CacheLevelType::MAIN_MEMORY, 256 * kibibyte);
	
	Util::Timer addTimer;
	addTimer.reset();
	std::cout << "Adding meshes ..." << std::flush;
	
	// Add the meshes to the OutOfCore system.
	std::vector<Util::Reference<Rendering::Mesh> > meshes;
	meshes.reserve(numMeshes);
	static const Geometry::Box boundingBox(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	for(uint_fast32_t i = 0; i < numMeshes; ++i) {
		const std::string numberString = Util::StringUtils::toString<uint32_t>(i);
		meshes.push_back(MinSG::OutOfCore::addMesh(Util::FileName(tempDir.getPath().getDir() + numberString + ".mmf"), boundingBox));
	}
	
	manager.trigger();
	
	addTimer.stop();
	std::cout << " done (" << addTimer.getSeconds() << " s)" << std::endl;
	
	Util::Timer displayTimer;
	Util::Timer assureLocalTimer;
	Util::Timer overallTimer;
	overallTimer.reset();

	uint32_t frame = 0;

	{
		// Simulate frames to get the OutOfCore system working.
		std::uniform_int_distribution<std::size_t> indexDist(0, meshes.size() - 1);
		for(; frame < 10; ++frame) {
			std::cout << "Executing frame " << frame << " ..." << std::flush;
			frameContext.beginFrame();
			displayTimer.reset();
			// Simulate display of meshes to change the priorities of the system.
			for(uint32_t i = 0; i < meshes.size() / 2; ++i) {
				const uint32_t meshIndex = indexDist(engine);
				Rendering::Mesh * mesh = meshes[meshIndex].get();
				manager.meshDisplay(mesh);
			}
			manager.trigger();
			displayTimer.stop();
			assureLocalTimer.reset();
			for(uint32_t i = 0; i < 10; ++i) {
				const uint32_t meshIndex = indexDist(engine);
				
				Rendering::Mesh * mesh = meshes[meshIndex].get();
				const Rendering::MeshVertexData & vd = mesh->openVertexData();
				const Rendering::MeshIndexData & id = mesh->openIndexData();
				
				if (!vd.hasLocalData() || !id.hasLocalData()) {
					std::cout << "Error: Mesh has no local data." << std::endl;
					return EXIT_FAILURE;
				}
			}
			assureLocalTimer.stop();
			frameContext.endFrame();
			std::cout << " done (display: " << displayTimer.getSeconds() << " s, assureLocal: " << assureLocalTimer.getSeconds() << " s)" << std::endl;

			if(verbose) {
				outputCacheLevelInformation();
			}
		}
	}

	for(uint32_t round = 0; round < 10; ++round) {
		Util::Timer addAgainTimer;
		addAgainTimer.reset();
		std::cout << "Adding additional meshes ..." << std::flush;
		
		// Simulate loading a second scene by adding meshes again.
		meshes.reserve(meshes.size() + 3 * numMeshes);
		for(uint_fast32_t i = 0; i < numMeshes; ++i) {
			const std::string numberString = Util::StringUtils::toString<uint32_t>(i);
			meshes.push_back(MinSG::OutOfCore::addMesh(Util::FileName(tempDir.getPath().getDir() + numberString + ".mmf"), boundingBox));
			meshes.push_back(MinSG::OutOfCore::addMesh(Util::FileName(tempDir.getPath().getDir() + numberString + ".mmf"), boundingBox));
			meshes.push_back(MinSG::OutOfCore::addMesh(Util::FileName(tempDir.getPath().getDir() + numberString + ".mmf"), boundingBox));
		}
		
		manager.trigger();
		
		addAgainTimer.stop();
		std::cout << " done (" << addAgainTimer.getSeconds() << " s)" << std::endl;

		// Simulate frames to get the OutOfCore system working.
		std::normal_distribution<double> indexDist(meshes.size() / 2, std::sqrt(meshes.size() / 2));
		const auto untilFrame = frame + 5;
		for(; frame < untilFrame; ++frame) {
			std::cout << "Executing frame " << frame << " ..." << std::flush;
			frameContext.beginFrame();
			displayTimer.reset();
			// Simulate display of meshes to change the priorities of the system.
			for(uint32_t i = 0; i < meshes.size() / 10; ++i) {
				const std::size_t meshIndex = std::max(static_cast<std::size_t>(0), std::min(static_cast<std::size_t>(indexDist(engine)), meshes.size() - 1));
				Rendering::Mesh * mesh = meshes[meshIndex].get();
				manager.meshDisplay(mesh);
			}
			manager.trigger();
			displayTimer.stop();
			frameContext.endFrame();
			std::cout << " done (display: " << displayTimer.getSeconds() << " s)" << std::endl;

			if(verbose) {
				outputCacheLevelInformation();
			}
		}
	}

	overallTimer.stop();
	std::cout << "Overall duration: " << overallTimer.getSeconds() << " s" << std::endl;
	
	MinSG::OutOfCore::shutDown();
	
	return EXIT_SUCCESS;
#else /* MINSG_EXT_OUTOFCORE */
	return EXIT_FAILURE;
#endif /* MINSG_EXT_OUTOFCORE */
}
