/*
	This file is part of the MinSG library extension TwinPartitions.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TWIN_PARTITIONS

#include "Loader.h"
#include "PartitionsData.h"
#include "TwinPartitionsRenderer.h"
#include "../../Core/Nodes/Node.h"

#ifdef MINSG_EXT_OUTOFCORE
#include "../OutOfCore/CacheManager.h"
#include "../OutOfCore/DataStrategy.h"
#include "../OutOfCore/OutOfCore.h"
#else /* MINSG_EXT_OUTOFCORE */
#include <Rendering/Serialization/Serialization.h>
#endif /* MINSG_EXT_OUTOFCORE */

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Macros.h>
#include <algorithm>
#include <cstdlib>
#include <istream>
#include <map>
#include <memory>
#include <string>

namespace MinSG {
namespace TwinPartitions {

Node * Loader::importPartitions(const Util::FileName & fileName) {
	auto file = Util::FileUtils::openForReading(fileName);
	if(!file) {
		WARN("Failed to open file.");
		return nullptr;
	}

	std::unique_ptr<PartitionsData> data(new PartitionsData);
	std::string buffer;
	// ##### Load Objects #####
	// Temporary mapping to resolve identifiers and get indices inside the objects vector.
	std::map<std::string, uint32_t> objectIdToIndex;
	{
		std::size_t numObjects;
		std::getline(*file, buffer, '\n');
		if (buffer.compare(0, 8, "objects ") == 0) {
			numObjects = strtoul(buffer.c_str() + 8, nullptr, 10);
		} else {
			WARN("\"objects\" not found.");
			return nullptr;
		}
		data->objects.reserve(numObjects);

		uint32_t objectCount = 0;
		while (objectCount < numObjects && file->good()) {
			std::string id;
			std::getline(*file, id, ';');
			std::string meshFile;
			std::getline(*file, meshFile, ';');
			std::string meshBB;
			std::getline(*file, meshBB, '\n');

			const Util::FileName meshFileName(meshFile);
#ifdef MINSG_EXT_OUTOFCORE
			std::vector<float> bbValues = Util::StringUtils::toFloats(meshBB);
			FAIL_IF(bbValues.size() != 6);
			Rendering::Mesh * mesh = OutOfCore::addMesh(meshFileName, Geometry::Box(Geometry::Vec3(bbValues[0], bbValues[1], bbValues[2]), bbValues[3], bbValues[4], bbValues[5]));
#else /* MINSG_EXT_OUTOFCORE */
			Rendering::Mesh * mesh = Rendering::Serialization::loadMesh(meshFileName);
#endif /* MINSG_EXT_OUTOFCORE */
			if (mesh == nullptr) {
				WARN("Cannot load model.");
			} else {
				data->objects.push_back(mesh);
				objectIdToIndex.insert(std::pair<std::string, uint32_t>(id, data->objects.size() - 1));
			}

			++objectCount;
		}
		if (objectCount != numObjects) {
			WARN("Number of objects that were announced and number of objects that were found differ.");
		}
	}
	// ##### Load Visible Sets #####
	// Temporary mapping to resolve identifiers and get indices inside the visible sets vector.
	std::map<std::string, uint32_t> visibleSetIdToIndex;
	{
		std::size_t numVisibleSets;
		std::getline(*file, buffer, '\n');
		if (buffer.compare(0, 13, "visible_sets ") == 0) {
			numVisibleSets = strtoul(buffer.c_str() + 13, nullptr, 10);
		} else {
			WARN("\"visible_sets\" not found.");
			return nullptr;
		}
		data->visibleSets.reserve(numVisibleSets);

		uint32_t visibleSetCount = 0;
		while (visibleSetCount < numVisibleSets && file->good()) {
			std::string id;
			std::getline(*file, id, ';');
			std::string elements;
			std::getline(*file, elements, '\n');

			PartitionsData::visible_set_t visibleSet;

			std::size_t cursor = 0;
			const std::size_t length = elements.size();

			while (cursor < length) {
				size_t oldCursor = cursor;
				cursor = elements.find_first_of(',', cursor);
				if (cursor == std::string::npos) {
					WARN("Error parsing visible set: Invalid object identifier.");
					break;
				}
				const std::string objectId(elements, oldCursor, cursor - oldCursor);
				++cursor;


				// Resolve object identifier to get location in objects vector.
				std::map<std::string, uint32_t>::const_iterator it = objectIdToIndex.find(objectId);
				if (it == objectIdToIndex.end()) {
					WARN("Error parsing visible set: Cannot resolve object identifier.");
					break;
				}
				const uint32_t objectIndex = it->second;

				oldCursor = cursor;
				cursor = elements.find_first_of(',', cursor);
				if (cursor == std::string::npos) {
					cursor = length;
				}
				const uint32_t triangles = strtoul(elements.c_str() + oldCursor, nullptr, 10);
				++cursor;

				oldCursor = cursor;
				cursor = elements.find_first_of(' ', cursor);
				if (cursor == std::string::npos) {
					cursor = length;
				}
				const uint32_t pixels = strtoul(elements.c_str() + oldCursor, nullptr, 10);
				++cursor;

				const float ratio = static_cast<float>(triangles) / static_cast<float>(pixels);
				visibleSet.emplace_back(ratio, objectIndex);
			}

			std::sort(visibleSet.begin(), visibleSet.end());
			data->visibleSets.push_back(visibleSet);
			visibleSetIdToIndex.insert(std::pair<std::string, uint32_t>(id, data->visibleSets.size() - 1));
			++visibleSetCount;
		}
		if (visibleSetCount != numVisibleSets) {
			WARN("Number of visible sets that were announced and number of visible sets that were found differ.");
		}
	}
	// ##### Load Cells #####
	Geometry::Box cellBounds;
	{
		std::size_t numCells;
		std::getline(*file, buffer, '\n');
		if (buffer.compare(0, 6, "cells ") == 0) {
			numCells = strtoul(buffer.c_str() + 6, nullptr, 10);
		} else {
			WARN("\"cells\" not found.");
			return nullptr;
		}
		data->cells.reserve(numCells);

		uint32_t cellCount = 0;
		while (cellCount < numCells && file->good()) {
			std::string boxString;
			std::getline(*file, boxString, ';');
			std::string visibleSetId;
			std::getline(*file, visibleSetId, ';');
			std::string depthMesh[6];
			std::string depthMeshBounds[6];
			std::string textures[6];
			std::getline(*file, depthMesh[0], ';');
			std::getline(*file, depthMeshBounds[0], ';');
			std::getline(*file, textures[0], ';');
			std::getline(*file, depthMesh[1], ';');
			std::getline(*file, depthMeshBounds[1], ';');
			std::getline(*file, textures[1], ';');
			std::getline(*file, depthMesh[2], ';');
			std::getline(*file, depthMeshBounds[2], ';');
			std::getline(*file, textures[2], ';');
			std::getline(*file, depthMesh[3], ';');
			std::getline(*file, depthMeshBounds[3], ';');
			std::getline(*file, textures[3], ';');
			std::getline(*file, depthMesh[4], ';');
			std::getline(*file, depthMeshBounds[4], ';');
			std::getline(*file, textures[4], ';');
			std::getline(*file, depthMesh[5], ';');
			std::getline(*file, depthMeshBounds[5], ';');
			std::getline(*file, textures[5], '\n');

			std::vector<float> bbValues = Util::StringUtils::toFloats(boxString);
			FAIL_IF(bbValues.size() != 6);
			const Geometry::Box box(Geometry::Box(Geometry::Vec3(bbValues[0], bbValues[1], bbValues[2]), bbValues[3], bbValues[4], bbValues[5]));
			cellBounds.include(box);

			std::vector<uint32_t> surroundingIds;
			for (uint_fast8_t i = 0; i < 6; ++i) {
				if(!depthMesh[i].empty()) {
					const Util::FileName depthMeshFileName(depthMesh[i]);
#ifdef MINSG_EXT_OUTOFCORE
					std::vector<float> dmBBValues = Util::StringUtils::toFloats(depthMeshBounds[i]);
					FAIL_IF(dmBBValues.size() != 6);
					const Geometry::Box depthMeshBB(Geometry::Box(Geometry::Vec3(dmBBValues[0], dmBBValues[1], dmBBValues[2]), dmBBValues[3], dmBBValues[4], dmBBValues[5]));
					Rendering::Mesh * mesh = OutOfCore::addMesh(depthMeshFileName, depthMeshBB);
#else /* MINSG_EXT_OUTOFCORE */
					Rendering::Mesh * mesh = Rendering::Serialization::loadMesh(depthMeshFileName);
#endif /* MINSG_EXT_OUTOFCORE */
					if(mesh == nullptr) {
						WARN("Cannot load depth mesh.");
					} else {
						surroundingIds.push_back(data->depthMeshes.size());
						data->depthMeshes.push_back(mesh);
					}
					if(textures[i].empty() || !Util::FileUtils::isFile(Util::FileName(textures[i]))) {
						WARN("Invalid texture for depth mesh." + textures[i]);
					} else {
						data->textureFiles.push_back(textures[i]);
					}
				}
			}

			// Resolve visible set identifier to get location in visible sets vector.
			std::map<std::string, uint32_t>::const_iterator it = visibleSetIdToIndex.find(visibleSetId);
			if (it == visibleSetIdToIndex.end()) {
				WARN("Error parsing cells: Cannot resolve visible set identifier.");
				break;
			}
			const uint32_t visibleSetIndex = it->second;

			data->cells.emplace_back(box, visibleSetIndex, surroundingIds);
			++cellCount;
		}
		if (cellCount != numCells) {
			WARN("Number of cells that were announced and number of cells that were found differ.");
		}
	}

	struct TwinPartitionsNode : public Node {
		Geometry::Box bb;
		TwinPartitionsNode(Geometry::Box bounds) : Node(), bb(std::move(bounds)) {
		}
		Node * doClone() const override {
			return nullptr;
		}
		const Geometry::Box& doGetBB() const override	{	return bb;	}

	};
	Node * node = new TwinPartitionsNode(cellBounds);
	node->addState(new TwinPartitionsRenderer(data.release()));
	return node;
}

}
}

#endif /* MINSG_EXT_TWIN_PARTITIONS */
