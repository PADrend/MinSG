/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2017 Sascha Brandt <myeti@mail.uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelRenderer_FixedSize.h"
#include "SurfelAnalysis.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/CameraNode.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Mesh/Mesh.h>

#include <Geometry/Vec3.h>

#include <Util/StringIdentifier.h>
#include <Util/GenericAttribute.h>

namespace MinSG{
namespace BlueSurfels {

static const Rendering::Uniform::UniformName uniform_renderSurfels("renderSurfels");
static const Rendering::Uniform::UniformName uniform_surfelRadius("surfelRadius");
static const Util::StringIdentifier SURFEL_ATTRIBUTE("surfels");
static const Util::StringIdentifier SURFEL_MEDIAN_ATTRIBUTE("surfelMedianDist");

#define SURFEL_MEDIAN_COUNT 1000

float SurfelRendererFixedSize::getMedianDist(Node * node, Rendering::Mesh& mesh) {
	auto surfelMedianAttr = node->findAttribute(SURFEL_MEDIAN_ATTRIBUTE);
	float surfelMedianDist = 0;
	if(!surfelMedianAttr) {
		std::cout << "Computing surfel median distance...";
		surfelMedianDist = getMedianOfNthClosestNeighbours(mesh, SURFEL_MEDIAN_COUNT, 2);
		node->setAttribute(SURFEL_MEDIAN_ATTRIBUTE, Util::GenericAttribute::createNumber(surfelMedianDist));
		std::cout << "done!" << std::endl;
	} else {
		surfelMedianDist = surfelMedianAttr->toFloat();
	}
	return surfelMedianDist;
};

SurfelRendererFixedSize::SurfelRendererFixedSize() : NodeRendererState(FrameContext::DEFAULT_CHANNEL),
		countFactor(1.0f),sizeFactor(2.0f),maxSurfelSize(32.0), debugHideSurfels(false), debugCameraEnabled(false) {
}
SurfelRendererFixedSize::~SurfelRendererFixedSize() {}

NodeRendererResult SurfelRendererFixedSize::displayNode(FrameContext & context, Node * node, const RenderParam & /*rp*/){
	if(!node->isActive())
		return NodeRendererResult::NODE_HANDLED;
				
	auto& renderingContext = context.getRenderingContext();
		
	// debugging
	if(debugCameraEnabled && debugCamera.isNull()) {
		debugCamera = static_cast<CameraNode*>(context.getCamera()->clone());
		debugCamera->setWorldTransformation(context.getCamera()->getWorldTransformationSRT());
	}
	
	// get surfel mesh
	auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));
	if( !surfelAttribute || !surfelAttribute->get())
		return NodeRendererResult::PASS_ON;
	Rendering::Mesh& surfelMesh = *surfelAttribute->get();
	
	// get median distance between surfels at a fixed prefix length 
	float surfelMedianDist = getMedianDist(node, surfelMesh);

	// calculate the projected distance between two adjacent pixels in screen space
	float meterPerPixelOriginal = getMeterPerPixel(context, node);
	float meterPerPixel;
	
	// debugging
	if(debugCameraEnabled) {
		renderingContext.pushMatrix_modelToCamera();
		context.pushAndSetCamera(debugCamera.get());		
		meterPerPixel = getMeterPerPixel(context, node);		
		context.popCamera();
		renderingContext.popMatrix_modelToCamera();		
	} else {
		meterPerPixel = meterPerPixelOriginal;
	}	
	
	float surfelSize = this->sizeFactor;
	float surfelRadius = std::min(surfelSize,this->maxSurfelSize) * meterPerPixel / 2.0f;
	uint32_t maxSurfelCount = surfelMesh.isUsingIndexData() ?  surfelMesh.getIndexCount() : surfelMesh.getVertexCount();
	float minSurfelDistance = surfelMedianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(maxSurfelCount));

	// Calculate the surfel prefix length based on the estimated median distance between surfels and the coverage of one surfel
	uint32_t surfelPrefixLength = (SURFEL_MEDIAN_COUNT * surfelMedianDist * surfelMedianDist / (surfelRadius * surfelRadius)) * this->countFactor;

	bool renderOriginal = surfelPrefixLength > maxSurfelCount && minSurfelDistance > surfelRadius;
	surfelPrefixLength = std::min(std::max<uint32_t>(renderOriginal ? (2*maxSurfelCount-std::min(2*maxSurfelCount,surfelPrefixLength)) : surfelPrefixLength,0),maxSurfelCount);

	if(surfelPrefixLength > 0 && !debugHideSurfels) {
		if(debugCameraEnabled)
			surfelSize *= meterPerPixel/meterPerPixelOriginal;
			
		float nodeScale = node->getWorldTransformationSRT().getScale();
		renderingContext.setGlobalUniform({uniform_surfelRadius, surfelRadius*nodeScale});
		renderingContext.setGlobalUniform({uniform_renderSurfels, true});
		renderingContext.pushAndSetPointParameters( Rendering::PointParameters(std::min(surfelSize,this->maxSurfelSize) ));
		renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
		renderingContext.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
		context.displayMesh(&surfelMesh,	0, surfelPrefixLength );
		renderingContext.popMatrix_modelToCamera();
		renderingContext.popPointParameters();
		renderingContext.setGlobalUniform({uniform_renderSurfels, false});

		return renderOriginal ? NodeRendererResult::PASS_ON : NodeRendererResult::NODE_HANDLED;
	}
	return (debugHideSurfels && !renderOriginal) ? NodeRendererResult::NODE_HANDLED : NodeRendererResult::PASS_ON;
}

void SurfelRendererFixedSize::setDebugCameraEnabled(bool b) {
	debugCamera = nullptr;
	debugCameraEnabled = b;
}

}
}
#endif // MINSG_EXT_BLUE_SURFELS
