/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "CoreNodeExporter.h"

#include "ExporterContext.h"
#include "ExporterTools.h"

#include "../SceneManager.h"
#include "../SceneDescription.h"

#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/ListNode.h"
#include "../../Core/Nodes/LightNode.h"
#include "../../Core/Nodes/GeometryNode.h"

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Serialization/Serialization.h>

#include <Util/Serialization/Serialization.h>
#include <Util/IO/FileUtils.h>
#include <Util/Encoding.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <vector>

namespace MinSG {
namespace SceneManagement {


static void describeGeometryNode(ExporterContext & /*ctxt*/,NodeDescription & desc, Node * node) {
	desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_GEOMETRY);

	NodeDescription dataDesc;

	GeometryNode * gn = dynamic_cast<GeometryNode*>(node);

	// annotate with bounding box
	const Geometry::Box bb = gn->getBB();
	const Geometry::Vec3 center = bb.getCenter();
	std::stringstream s;
	s << center.getX() << " " << center.getY() << " " << center.getZ() << " " << bb.getExtentX() << " " << bb.getExtentY() << " " << bb.getExtentZ();
	dataDesc.setString(Consts::ATTR_MESH_BB, s.str());

	Rendering::Mesh * m = gn->getMesh();
	if(m!=nullptr) { // mesh present?
		// no filename -> store data in .minsg
		if(m->getFileName().empty()) {
			std::ostringstream meshStream;
			if(Rendering::Serialization::saveMesh(gn->getMesh(), "mmf", meshStream)) {
				dataDesc.setString(Consts::ATTR_DATA_TYPE,"mesh");
				dataDesc.setString(Consts::ATTR_DATA_ENCODING,"base64");
				const std::string streamString = meshStream.str();
				const std::string meshString = Util::encodeBase64(std::vector<uint8_t>(streamString.begin(), streamString.end()));
				dataDesc.setString(Consts::DATA_BLOCK,meshString);
			}
		} else { // filename given?
			Util::FileName meshFilename(m->getFileName());

//			// make path to mesh relative to scene (if mesh lies below the scene)
//			Util::FileUtils::makeRelativeIfPossible(ctxt.sceneFile, meshFilename);

			dataDesc.setString(Consts::ATTR_DATA_TYPE,"mesh");
			dataDesc.setString(Consts::ATTR_MESH_FILENAME,meshFilename.toShortString());
		}
		ExporterTools::addDataEntry(desc, std::move(dataDesc));
	}

}

static void describeListNode(ExporterContext & ctxt,NodeDescription & desc, Node * node) {
	desc.setValue(Consts::ATTR_NODE_TYPE,Util::GenericAttribute::createString(Consts::NODE_TYPE_LIST));
	ExporterTools::addChildNodesToDescription(ctxt,desc,node);
}

static void describeLightNode(ExporterContext &,NodeDescription & desc, Node * node) {
	LightNode * ln = dynamic_cast<LightNode*>(node);
	switch(ln->getType()) {
		case Rendering::LightParameters::DIRECTIONAL:
			desc.setString(Consts::ATTR_LIGHT_TYPE, Consts::LIGHT_TYPE_DIRECTIONAL);
			break;
		case Rendering::LightParameters::SPOT:
			desc.setString(Consts::ATTR_LIGHT_TYPE, Consts::LIGHT_TYPE_SPOT);
			break;
		case Rendering::LightParameters::POINT:
			desc.setString(Consts::ATTR_LIGHT_TYPE, Consts::LIGHT_TYPE_POINT);
			break;
		default:
			WARN("unexpected case in switch statement");
	}
	desc.setString(Consts::ATTR_NODE_TYPE, Consts::NODE_TYPE_LIGHT);
	desc.setString(Consts::ATTR_LIGHT_CONSTANT_ATTENUATION, Util::StringUtils::toString(ln->getConstantAttenuation()));
	desc.setString(Consts::ATTR_LIGHT_LINEAR_ATTENUATION, Util::StringUtils::toString(ln->getLinearAttenuation()));
	desc.setString(Consts::ATTR_LIGHT_QUADRATIC_ATTENUATION, Util::StringUtils::toString(ln->getQuadraticAttenuation()));
	desc.setString(Consts::ATTR_LIGHT_SPOT_CUTOFF, Util::StringUtils::toString(ln->getCutoff()));
	desc.setString(Consts::ATTR_LIGHT_SPOT_EXPONENT, Util::StringUtils::toString(ln->getExponent()));

	desc.setString(Consts::ATTR_LIGHT_AMBIENT, Util::StringUtils::implode(ln->getAmbientLightColor().data(), ln->getAmbientLightColor().data() + 4, " "));
	desc.setString(Consts::ATTR_LIGHT_DIFFUSE, Util::StringUtils::implode(ln->getDiffuseLightColor().data(), ln->getDiffuseLightColor().data() + 4, " "));
	desc.setString(Consts::ATTR_LIGHT_SPECULAR, Util::StringUtils::implode(ln->getSpecularLightColor().data(), ln->getSpecularLightColor().data() + 4, " "));

	
}

void initCoreNodeExporter() {
	ExporterTools::registerNodeExporter(GeometryNode::getClassId(),&describeGeometryNode);
	ExporterTools::registerNodeExporter(ListNode::getClassId(),&describeListNode);
	ExporterTools::registerNodeExporter(LightNode::getClassId(),&describeLightNode);
}

}
}
