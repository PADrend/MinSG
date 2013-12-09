/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ExportFunctions.h"
#include "Exporter/ExporterTools.h"
#include "Exporter/WriterMinSG.h"

#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Timer.h>
#include <Util/Utils.h>

#include "../Core/Nodes/GeometryNode.h"
#include "../Helper/StdNodeVisitors.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Serialization/GenericAttributeSerialization.h>

namespace MinSG{

void SceneManagement::saveMeshesInSubtreeAsPLY(Node * rootNode, const std::string & s_dirName, bool saveRegisteredNodes/*=false*/) {
	float start = Util::Timer::now();
	Util::FileName dirName = Util::FileName::createDirName(s_dirName);
	const auto geoNodes = collectNodes<GeometryNode>(rootNode);
	const size_t numMeshes = geoNodes.size();

	if(!geoNodes.empty() && !Util::FileUtils::isDir(dirName)) {
		Util::FileUtils::createDir(dirName);
	}
	size_t counter = 0;
	size_t successCounter = 0;
	for(const auto & geoNode : geoNodes) {
		counter++;
		Rendering::Mesh * mesh = geoNode->getMesh();

		// skip node, if node has no mesh or mesh already has a corresponding file.
		if(mesh == nullptr || (!(mesh->getFileName().empty()) && !saveRegisteredNodes)) {
			continue;
		}

		Util::FileName fileName = Util::FileUtils::generateNewRandFilename(dirName, "mesh_", ".ply", 8);

		Util::info << "\rExporting " << counter << "/" << numMeshes << "     ";
		if(!Util::FileUtils::isFile(fileName)) {
			bool successful = Rendering::Serialization::saveMesh(mesh, fileName);
			if(successful) {
				successCounter++;
				mesh->setFileName(fileName);
			}
		}
	}
	Util::info << "\nExported " << successCounter << "/" << numMeshes << " in " << (Util::Timer::now() - start) << " sec.\n";
}

void SceneManagement::saveMeshesInSubtreeAsMMF(Node * rootNode, const std::string & s_dirName, bool saveRegisteredNodes/*=false*/) {
	float start = Util::Timer::now();
	Util::FileName dirName = Util::FileName::createDirName(s_dirName);
	const auto geoNodes = collectNodes<GeometryNode>(rootNode);
	const size_t numMeshes = geoNodes.size();

	if(!geoNodes.empty() && !Util::FileUtils::isDir(dirName)) {
		Util::FileUtils::createDir(dirName);
	}
	size_t counter = 0;
	size_t successCounter = 0;
	for(const auto & geoNode : geoNodes) {
		counter++;
		Rendering::Mesh * mesh = geoNode->getMesh();

		// skip node, if node has no mesh or mesh already has a corresponding file.
		if(mesh == nullptr || (!(mesh->getFileName().empty()) && !saveRegisteredNodes))
			continue;

		Util::FileName fileName = Util::FileUtils::generateNewRandFilename(dirName, "mesh_", ".mmf", 8);

		Util::info << "\rExporting " << counter << "/" << numMeshes << "     ";
		if(!Util::FileUtils::isFile(fileName)) {
			bool successful = Rendering::Serialization::saveMesh(mesh, fileName);
			if(successful) {
				successCounter++;
				mesh->setFileName(fileName);
			}
		}
	}
	Util::info << "\nExported " << successCounter << "/" << numMeshes << " in " << (Util::Timer::now() - start) << " sec.\n";
}


void SceneManagement::saveMinSGFile(SceneManager & sm, const Util::FileName & fileName, const std::deque<Node *> & nodes) {
	// generate output
	auto out = Util::FileUtils::openForWriting(fileName);
	if(!out)
		throw std::runtime_error("Cannot write to file " + fileName.toString());

	ExporterContext ctxt(sm);
	ctxt.sceneFile = fileName;
	std::unique_ptr<NodeDescription> description(ExporterTools::createDescriptionForScene(ctxt, nodes));
	if(!WriterMinSG::save(*(out.get()), *(description.get())))
		throw std::runtime_error("Could not export scene to file " + fileName.toString());
}

void SceneManagement::saveMinSGStream(SceneManager & sm, std::ostream & out, const std::deque<Node *> & nodes) {
	if(!out.good())
		throw std::runtime_error("Cannot save MinSG nodes to the given stream.");

	ExporterContext ctxt(sm);
	std::unique_ptr<NodeDescription> description(ExporterTools::createDescriptionForScene(ctxt, nodes));
	if(!WriterMinSG::save(out, *(description.get())))
		throw std::runtime_error("Could not serialize MinSG nodes.");
}
}