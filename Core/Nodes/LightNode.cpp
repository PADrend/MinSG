/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "LightNode.h"
#include "../FrameContext.h"
#include "../Transformations.h"
#include "../States/MaterialState.h"
#include <Geometry/Convert.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Rendering/MeshUtils/PlatonicSolids.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>
#include <cassert>

namespace MinSG {

//! (static)
LightNode * LightNode::createPointLight() {
	return new LightNode(Rendering::LightParameters::POINT);
}

void LightNode::setCutoff(float cutoff) {
	parameters.cutoff = std::min(std::max(cutoff, 0.0f), 90.0f);
	parameters.cosCutoff = cos(Geometry::Convert::degToRad(cutoff));
	if(parameters.type == Rendering::LightParameters::SPOT) {
		removeMetaMesh();
	}
}

//! (static)
LightNode * LightNode::createDirectionalLight() {
	return new LightNode(Rendering::LightParameters::DIRECTIONAL);
}

//! (static)
LightNode * LightNode::createSpotLight() {
	return new LightNode(Rendering::LightParameters::SPOT);
}

LightNode::LightNode(Rendering::LightParameters::lightType_t type/*=POINT*/) :
	Node(), parameters(), lightNumber(INVALID_LIGHT_NUMBER) {
	parameters.type = type;
}

LightNode::LightNode(const LightNode & source) :
	Node(), parameters(source.parameters), lightNumber(INVALID_LIGHT_NUMBER) {
}

LightNode::~LightNode() {

}

void LightNode::validateParameters()const {
	parameters.direction = Transformations::localDirToWorldDir(*this, Geometry::Vec3(0, 0, -1)).normalize();
	parameters.position = getWorldOrigin();
}

//! ---|> Node
void LightNode::doDisplay(FrameContext & context, const RenderParam & rp) {
	if(!(rp.getFlag(SHOW_META_OBJECTS))) {
		return;
	}
	static Util::Reference<MaterialState> material;
	if( !material ) {
		material = new MaterialState();
		material->changeParameters().setShininess(128.0f);
	}
	if( !metaMesh )
		metaMesh = createMetaMesh();

	material->changeParameters().setAmbient(parameters.ambient);
	material->changeParameters().setDiffuse(parameters.diffuse);
	material->changeParameters().setSpecular(parameters.specular);

	material->enableState(context, this, rp);
	context.displayMesh(metaMesh.get());
	material->disableState(context, this, rp);
}

void LightNode::removeMetaMesh() {
	metaMesh = nullptr;
}

void LightNode::switchOn(FrameContext & context) {
	validateParameters();
	lightNumber = context.getRenderingContext().enableLight(parameters);
}

void LightNode::switchOff(FrameContext & context) {
	context.getRenderingContext().disableLight(lightNumber);
	lightNumber = INVALID_LIGHT_NUMBER;
}

const Geometry::Box& LightNode::doGetBB() const {
	static const Geometry::Box bb(-0.1,0.1,-0.1,0.1,-0.1,0.1);
	return bb;
}

Rendering::Mesh * LightNode::createMetaMesh() {
	switch (parameters.type) {
		case Rendering::LightParameters::SPOT: {
			const float height = std::cos(Geometry::Convert::degToRad(parameters.cutoff));
			const float radius = std::sin(Geometry::Convert::degToRad(parameters.cutoff));
			std::deque<Rendering::Mesh *> meshes;
			std::deque<Geometry::Matrix4x4> transformations;
			Geometry::Matrix4x4 transform;
			transform.rotate_deg(-90.0f, 0.0f, 1.0f, 0.0f);
			transform.translate(-height, 0.0f, 0.0f);
			meshes.push_back(Rendering::MeshUtils::MeshBuilder::createDiscSector(radius, 64));
			transformations.push_back(transform);
			meshes.push_back(Rendering::MeshUtils::MeshBuilder::createCone(radius, height, 64));
			transformations.push_back(transform);
			Rendering::Mesh * mesh = Rendering::MeshUtils::combineMeshes(meshes, transformations);
			while (!meshes.empty()) {
				delete meshes.back();
				meshes.pop_back();
			}
			return mesh;
		}
		case Rendering::LightParameters::POINT: {
			Util::Reference<Rendering::Mesh> icosahedron = Rendering::MeshUtils::PlatonicSolids::createIcosahedron();
			return Rendering::MeshUtils::PlatonicSolids::createEdgeSubdivisionSphere(icosahedron.get(), 3);
		}
		case Rendering::LightParameters::DIRECTIONAL: {
			const float radiusBottom = 0.75f;
			const float radiusTop = 0.5f;
			std::deque<Rendering::Mesh *> meshes;
			std::deque<Geometry::Matrix4x4> transformations;
			Geometry::Matrix4x4 transform;
			transform.rotate_deg(-90.0f, 0.0f, 1.0f, 0.0f);
			transform.translate(-0.1f, 0.0f, 0.0f);
			meshes.push_back(Rendering::MeshUtils::MeshBuilder::createDiscSector(radiusBottom, 64));
			transformations.push_back(transform);
			meshes.push_back(Rendering::MeshUtils::MeshBuilder::createConicalFrustum(radiusBottom, radiusTop, 0.2f, 64));
			transformations.push_back(transform);
			transform.setIdentity();
			transform.rotate_deg(-270.0f, 0.0f, 1.0f, 0.0f);
			transform.translate(-0.1f, 0.0f, 0.0f);
			meshes.push_back(Rendering::MeshUtils::MeshBuilder::createDiscSector(radiusTop, 64));
			transformations.push_back(transform);
			Rendering::Mesh * mesh = Rendering::MeshUtils::combineMeshes(meshes, transformations);
			while (!meshes.empty()) {
				delete meshes.back();
				meshes.pop_back();
			}
			return mesh;
		}
		default:
			WARN("createMetaMesh");
			FAIL();
	}
}

}
