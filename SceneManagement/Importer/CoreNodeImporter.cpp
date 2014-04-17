/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CoreNodeImporter.h"

#include "../SceneManager.h"
#include "../SceneDescription.h"
#include "../ImportFunctions.h"
#include "ImporterTools.h"
#include "ImportContext.h"
#include "MeshImportHandler.h"

#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/ListNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Nodes/CameraNodeOrtho.h"

#include "../../Core/States/LightingState.h"

#include <Rendering/Serialization/Serialization.h>
#include <Rendering/MeshUtils/MeshUtils.h>

#include <Util/Macros.h>
#include <Util/Encoding.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <vector>

namespace MinSG {
namespace SceneManagement {

static bool importLightNode(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
	if(nodeType != Consts::NODE_TYPE_LIGHT || parent == nullptr)
		return false;

	auto node = new LightNode;
	const std::string lightType = d.getString(Consts::ATTR_LIGHT_TYPE);
	if(lightType == Consts::LIGHT_TYPE_DIRECTIONAL) {
		node->setLightType(Rendering::LightParameters::DIRECTIONAL);
	} else if(lightType == Consts::LIGHT_TYPE_SPOT) {
		node->setLightType(Rendering::LightParameters::SPOT);
	} else {
		node->setLightType(Rendering::LightParameters::POINT);
	}
	node->setAmbientLightColor(Util::Color4f(Util::StringUtils::toFloats(d.getString(Consts::ATTR_LIGHT_AMBIENT, ""))));
	node->setDiffuseLightColor(Util::Color4f(Util::StringUtils::toFloats(d.getString(Consts::ATTR_LIGHT_DIFFUSE, ""))));
	node->setSpecularLightColor(Util::Color4f(Util::StringUtils::toFloats(d.getString(Consts::ATTR_LIGHT_SPECULAR, ""))));
	node->setConstantAttenuation(Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_LIGHT_CONSTANT_ATTENUATION, "1.0")));
	node->setLinearAttenuation(Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_LIGHT_LINEAR_ATTENUATION, "0.0")));
	node->setQuadraticAttenuation(Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_LIGHT_QUADRATIC_ATTENUATION, "0.0")));
	node->setCutoff(Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_LIGHT_SPOT_CUTOFF, "20.0")));
	node->setExponent(Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_LIGHT_SPOT_EXPONENT, "2.0")));

	// \note if a Light node is created and a scene root node is given, it is enlighted with this light
	// (Important for the import of lights in collada files. When a MinSG file is read, the sceneRoot is just a dummy node and the state is lost afterwards)
	if(ctxt.getRootNode() != nullptr) {
		// Add state referencing the generated light node.
		auto lightState = new LightingState;
		lightState->setLight(dynamic_cast<LightNode *>(node));
		ctxt.getRootNode()->addState(lightState);
	}

	ImporterTools::finalizeNode(ctxt, node, d);
	parent->addChild(node);
	return true;
}

static bool importClone(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
	if(nodeType != Consts::NODE_TYPE_CLONE || parent == nullptr)
		return false;

	Node * node = ctxt.sceneManager.createInstance(d.getString(Consts::ATTR_CLONE_SOURCE));
	if(!node) {
		WARN(std::string("Instance of \"") + d.getString(Consts::ATTR_CLONE_SOURCE) + "\" could not be created.");
		return false;
	}

	ImporterTools::finalizeNode(ctxt, node, d);
	parent->addChild(node);
	return true;
}

static bool importListNode(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
	if(nodeType != Consts::NODE_TYPE_LIST || parent == nullptr)
		return false;

	auto node = new ListNode;

	ImporterTools::finalizeNode(ctxt, node, d);
	parent->addChild(node);
	return true;
}

static bool importCameraNode(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
	if(nodeType != Consts::NODE_TYPE_CAMERA || parent == nullptr)
		return false;

	auto node = new CameraNode();

	float angle = Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_CAM_ANGLE, "80.0"));
	float ratio = Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_CAM_RATIO, "1.33"));
	float nearPlane = Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_CAM_NEAR, "1.0"));
	float farPlane = Util::StringUtils::toNumber<float>(d.getString(Consts::ATTR_CAM_FAR, "1000.0"));

	// FIXME setCorrext viewport
	node->setViewport(Geometry::Rect_i(0,0,ratio, 1));
	// FIXME set left, right, top, bottom angle
	node->applyVerticalAngle(angle);
	node->setNearFar(nearPlane, farPlane);

	ImporterTools::finalizeNode(ctxt, node, d);
	parent->addChild(node);
	return true;
}

static bool importGeometryNode(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
	if(nodeType != Consts::NODE_TYPE_GEOMETRY || parent == nullptr)
		return false;

	const auto dataDescList = ImporterTools::filterElements(Consts::TYPE_DATA,
															dynamic_cast<const NodeDescriptionList *>(d.getValue(Consts::CHILDREN)));
	if(dataDescList.empty()) {// No data block given -> GeometryNode intentionally without mesh
		auto gn = new GeometryNode;
		ImporterTools::finalizeNode(ctxt, gn, d);
		parent->addChild(gn);
		return true;
	}
	if(dataDescList.size() != 1)
		WARN(std::string("GeometryNode needs one data description. Got ") + Util::StringUtils::toString(dataDescList.size()));
	const NodeDescription * dataDesc = dataDescList.front();

	/// GeometryNode
	std::string dataType = dataDesc->getString(Consts::ATTR_DATA_TYPE);
	// Check for a correct data type. In new version this should always be "mesh". Check for strings beginning with "mesh" for backwards compatibility.
	if(dataType.compare(0, 4, "mesh") != 0) {
		WARN("Unknown data type \"" + dataType + "\" for GeometryNode.");
		return false;
	}

	Node * node = nullptr;

	const std::string fileNameString = dataDesc->getString(Consts::ATTR_MESH_FILENAME);

	// A filename is given -> create the Mesh using the MeshImportHandler (or look for a duplicate depending on the import options)
	if(!fileNameString.empty()) {
		// if the mesh registry is used; look if the mesh is already available in the registry
		const bool useMeshRegistry = (ctxt.importOptions & IMPORT_OPTION_USE_MESH_REGISTRY)>0 ;
		if(useMeshRegistry) {
			Rendering::Mesh * mesh = ctxt.getRegisteredMesh(fileNameString);
			if(mesh!=nullptr) {
				node = new GeometryNode(mesh);
				//std::cout << "Mesh reused: "<<fileName.toString()<<"\n";
			}
		}

		if(node==nullptr) {
			node = ImporterTools::getMeshImportHandler()->handleImport(ctxt.fileLocator, fileNameString, dataDesc);

			if(node == nullptr) {
				WARN("Loading the mesh failed.");
				return nullptr;
			}

			// if the mesh registry is used and the node is an ordanary GeometryNode, store the mesh in the registry
			if(useMeshRegistry) {
				GeometryNode * gn = dynamic_cast<GeometryNode *>(node);
				if(gn!=nullptr) {
					ctxt.registerMesh(fileNameString,gn->getMesh());
				}
			}
		}

	} // Load MMF data from a Base64 encoded block.
	else if(dataDesc->getValue(Consts::DATA_BLOCK) != nullptr) {
		const std::string dataBlock = dataDesc->getString(Consts::DATA_BLOCK);
		if(dataDesc->getString(Consts::ATTR_DATA_ENCODING) != Consts::DATA_ENCODING_BASE64) {
			WARN("Unknown data block encoding.");
			return false;
		}
		const std::vector<uint8_t> meshData = Util::decodeBase64(dataBlock);

		Rendering::Mesh * mesh = Rendering::Serialization::loadMesh("mmf", std::string(meshData.begin(), meshData.end()));
		if(mesh == nullptr) {
			WARN("Loading the mesh failed.");
			return false;
		}
		node = new GeometryNode(mesh);
	} //  A Mesh-Object is already contained in the description.
	else if(dataType == "mesh") {
		Rendering::Serialization::MeshWrapper_t * meshWrapper = dynamic_cast<Rendering::Serialization::MeshWrapper_t *>(dataDesc->getValue(Consts::ATTR_MESH_DATA));
		if(meshWrapper == nullptr) {
			WARN("No mesh found.");
			return false;
		}
		Rendering::Mesh * mesh = meshWrapper->get();
		node = new GeometryNode(mesh);
	} else {
		WARN("Unknown Data for GeometryNode.");
		return false;
	}

	// if the mesh hashing registry is used; look if the mesh is already available in the registry by its hash
	const bool useMeshHashingRegistry = (ctxt.importOptions & IMPORT_OPTION_USE_MESH_HASHING_REGISTRY)>0 ;
	if(useMeshHashingRegistry) {
		GeometryNode * gn = dynamic_cast<GeometryNode *>(node);
		if(gn!=nullptr && !gn->getMesh()->empty()) {
			Util::Reference<Rendering::Mesh> mesh = gn->getMesh();
			const uint32_t hash = Rendering::MeshUtils::calculateHash(mesh.get());
			Rendering::Mesh * mesh2 = ctxt.getRegisteredMesh(hash,mesh.get());
			if(mesh2==nullptr) {
				ctxt.registerMesh(hash,mesh.get());
			} else if(mesh!=mesh2) {
				gn->setMesh(mesh2);
				//std::cout << "Mesh reused: "<<(void*)hash<<"\n";
			}
		}

	}

	ImporterTools::finalizeNode(ctxt, node, d);
	parent->addChild(node);
	return true;
}

//! template for new importers
// static bool importXY(ImportContext & ctxt, const std::string & nodeType, const NodeDescription & d, GroupNode * parent) {
//  if(nodeType != Consts::NODE_TYPE_XY) // check parent != nullptr is done by SceneManager
//      return false;
//
//  XY * node = new XY;
//
//  //TODO
//
//
//  ImporterTools::finalizeNode(ctxt, node, d);
//  parent->addChild(node);
//  return true;
// }

void initCoreNodeImporter() {

	ImporterTools::registerNodeImporter(&importLightNode);
	ImporterTools::registerNodeImporter(&importClone);
	ImporterTools::registerNodeImporter(&importCameraNode);
	ImporterTools::registerNodeImporter(&importListNode);
	ImporterTools::registerNodeImporter(&importGeometryNode);
}

}
}
