/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#include "SphereVisualizationRenderer.h"
#include "Helper.h"
#include "VisibilitySphere.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/States/BlendingState.h"
#include "../../Core/States/CullFaceState.h"
#include "../../Core/States/MaterialState.h"
#include "../../Core/States/ShaderState.h"
#include "../../Core/FrameContext.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../../Helper/DataDirectory.h"
#include "../../Helper/Helper.h"
#include <Geometry/Sphere.h>
#include <Geometry/Vec3.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Draw.h>
#include <Util/IO/FileLocator.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <Util/References.h>

namespace MinSG {
namespace SVS {

SphereVisualizationRenderer::SphereVisualizationRenderer() : 
	NodeRendererState(FrameContext::DEFAULT_CHANNEL) {
}

SphereVisualizationRenderer * SphereVisualizationRenderer::clone() const {
	return new SphereVisualizationRenderer(*this);
}

NodeRendererResult SphereVisualizationRenderer::displayNode(FrameContext & context, Node * node, const RenderParam & rp) {
	try {
		if(!node->isActive()) {
			return NodeRendererResult::NODE_HANDLED;
		}

		// Only handle GroupNodes.
		GroupNode * groupNode = dynamic_cast<GroupNode *>(node);
		if(groupNode == nullptr) {
			return NodeRendererResult::PASS_ON;
		}

		// Simply ignore nodes without a visibility sphere.
		if(!hasVisibilitySphere(groupNode)) {
			return NodeRendererResult::PASS_ON;
		}

		const VisibilitySphere & visibilitySphere = retrieveVisibilitySphere(groupNode);
		const auto & sphere = visibilitySphere.getSphere();

		static const Util::StringIdentifier attributeId(NodeAttributeModifier::create( "SVS::SphereVisualizationRenderer::MetaObject", NodeAttributeModifier::PRIVATE_ATTRIBUTE));
		typedef Util::ReferenceAttribute<GeometryNode> MetaObjectAttribute;

		MetaObjectAttribute * attribute = dynamic_cast<MetaObjectAttribute *>(node->getAttribute(attributeId));
		if(attribute == nullptr) {
			Rendering::VertexDescription vertexDescription;
			vertexDescription.appendPosition3D();
			vertexDescription.appendNormalFloat();
			vertexDescription.appendColorRGBFloat();
			vertexDescription.appendTexCoord();
			Util::Reference<Rendering::Mesh> sphereMesh = Rendering::MeshUtils::MeshBuilder::createSphere(vertexDescription, 64, 64);
			Util::Reference<GeometryNode> sphereNode = new GeometryNode(sphereMesh);
			sphereNode->setWorldPosition(sphere.getCenter());
			sphereNode->setScale(sphere.getRadius());

			Rendering::MaterialParameters materialParams;
			materialParams.setAmbient(Util::Color4f(0.0f, 1.0f, 0.0f, 1.0f));
			materialParams.setDiffuse(Util::Color4f(0.0f, 1.0f, 0.0f, 1.0f));
			materialParams.setSpecular(Util::Color4f(1.0f, 1.0f, 1.0f, 1.0f));
			materialParams.setShininess(32.0f);
			sphereNode->addState(new MaterialState(materialParams));

			auto blendingState = new BlendingState;
			blendingState->changeParameters().setBlendFunc(Rendering::BlendingParameters::SRC_ALPHA, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA);
			blendingState->setBlendDepthMask(false);
			sphereNode->addState(blendingState);

			auto cullFaceState = new CullFaceState;
			cullFaceState->changeParameters().disable();
			sphereNode->addState(cullFaceState);

			static Util::Reference<ShaderState> sphereShader;
			if(sphereShader.isNull()) {
				sphereShader = new ShaderState;
				std::vector<std::string> vsFiles;
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/universal.vs");
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/sgHelpers.sfn");
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/shading_phong.sfn");
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/texture_disabled.sfn");
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/shadow_disabled.sfn");
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/effect_normalToAlpha.sfn");
				vsFiles.push_back(DataDirectory::getPath() + "/shader/universal2/color_standard.sfn");
				std::vector<std::string> gsFiles;
				std::vector<std::string> fsFiles(vsFiles);
				fsFiles[0] = DataDirectory::getPath() + "/shader/universal2/universal.fs";
				initShaderState(sphereShader.get(), vsFiles, gsFiles, fsFiles, Rendering::Shader::USE_UNIFORMS,Util::FileLocator());
			}
			if(sphereShader.isNotNull()) {
				sphereNode->addState(sphereShader.get());
			}

			attribute = new MetaObjectAttribute(sphereNode.get());
			node->setAttribute(attributeId, attribute);
		}
		context.getRenderingContext().pushMatrix();
		context.getRenderingContext().resetMatrix();
		context.getRenderingContext().multMatrix(groupNode->getWorldMatrix());
		attribute->get()->display(context, FRUSTUM_CULLING);
		context.getRenderingContext().popMatrix();
	} catch(const std::exception & e) {
		WARN(std::string("Exception during rendering: ") + e.what());
		deactivate();

		// Do this here because doDisableState is not called for inactive states.
		doDisableState(context, node, rp);
	}

	return NodeRendererResult::PASS_ON;
}

}
}

#endif /* MINSG_EXT_SVS */
