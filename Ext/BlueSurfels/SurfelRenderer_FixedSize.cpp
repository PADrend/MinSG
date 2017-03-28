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
#include "../../Core/Statistics.h"
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
		countFactor(1.0f),sizeFactor(1.0f),surfelSize(1.0f),maxSurfelSize(32.0), maxFrameTime(16.0f), 
		debugHideSurfels(false), debugCameraEnabled(false), deferredSurfels(false), adaptive(false), foveated(false) {
		foveatZones.push_back({0.5f, 2.0f});
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
	
	float pointSize = this->surfelSize;
	if(foveated) {
		const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
		auto screenCenter = screenRect.getCenter();
		auto projRect = context.getProjectedRect(node);
		if(!projRect.contains(screenCenter)) {
			float distSqr = projRect.getCorner(Geometry::CORNER_xy).distanceSquared(screenCenter);
			distSqr = std::min(distSqr, projRect.getCorner(Geometry::CORNER_Xy).distanceSquared(screenCenter));
			distSqr = std::min(distSqr, projRect.getCorner(Geometry::CORNER_xY).distanceSquared(screenCenter));
			distSqr = std::min(distSqr, projRect.getCorner(Geometry::CORNER_XY).distanceSquared(screenCenter));
			float maxDistSqr = std::min(screenRect.getWidth(), screenRect.getHeight()) * 0.5f;
			maxDistSqr *= maxDistSqr;
			for(const auto& zone : foveatZones) {
				if(distSqr > zone.first*zone.first*maxDistSqr)
					pointSize = surfelSize * zone.second;
			}
		}
	}
	
	float surfelRadius = std::min(pointSize,this->maxSurfelSize) * meterPerPixel / 2.0f;
	uint32_t maxSurfelCount = surfelMesh.isUsingIndexData() ?  surfelMesh.getIndexCount() : surfelMesh.getVertexCount();
	float minSurfelDistance = surfelMedianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(maxSurfelCount));

	// Calculate the surfel prefix length based on the estimated median distance between surfels and the coverage of one surfel
	uint32_t surfelPrefixLength = (SURFEL_MEDIAN_COUNT * surfelMedianDist * surfelMedianDist / (surfelRadius * surfelRadius)) * this->countFactor;

	bool renderOriginal = surfelPrefixLength > maxSurfelCount && minSurfelDistance > surfelRadius;
	if(renderOriginal) {
		uint32_t diff = std::min(maxSurfelCount, surfelPrefixLength - maxSurfelCount);
		surfelPrefixLength = maxSurfelCount - diff;
	}
	surfelPrefixLength = std::min(surfelPrefixLength,maxSurfelCount);

	if(surfelPrefixLength > 0 && (!debugHideSurfels || deferredSurfels)) {
		if(debugCameraEnabled)
			pointSize *= meterPerPixel/meterPerPixelOriginal;
		pointSize = std::min(pointSize*sizeFactor,this->maxSurfelSize);
		
		if(deferredSurfels) {
			float camDistSqr = node->getWorldBB().getDistanceSquared(context.getCamera()->getWorldOrigin());
			deferredSurfelQueue.emplace_back(camDistSqr, node, surfelPrefixLength, pointSize);
		} else {
			renderingContext.setGlobalUniform({uniform_renderSurfels, true});
			renderingContext.pushAndSetPointParameters( Rendering::PointParameters(pointSize));
			renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
			renderingContext.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
			context.displayMesh(&surfelMesh,	0, surfelPrefixLength );
			renderingContext.popMatrix_modelToCamera();
			renderingContext.popPointParameters();
			renderingContext.setGlobalUniform({uniform_renderSurfels, false});
		}

		return renderOriginal ? NodeRendererResult::PASS_ON : NodeRendererResult::NODE_HANDLED;
	}
	return (debugHideSurfels && !renderOriginal) ? NodeRendererResult::NODE_HANDLED : NodeRendererResult::PASS_ON;
}

SurfelRendererFixedSize::stateResult_t SurfelRendererFixedSize::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	deferredSurfelQueue.clear();
		
	if(adaptive) {
		float factor = frameTimer.getMilliseconds() / maxFrameTime;
		if(factor > 1.1f || factor < 0.9f) {
			surfelSize = std::min(std::max((3.0f*surfelSize + surfelSize*factor)/4.0f, 1.0f), maxSurfelSize);
		}
		//std::cout << "\r" << avgFrameTime << " " << sizeFactor << "                 " << std::flush;
		frameTimer.reset();
	}
	
	return NodeRendererState::doEnableState(context, node, rp);
}

void SurfelRendererFixedSize::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);	
	if(deferredSurfels && !debugHideSurfels) 
		drawSurfels(context);
}

void SurfelRendererFixedSize::drawSurfels(FrameContext & context, float minSize, float maxSize) const {
	auto& rc = context.getRenderingContext();	
	rc.setGlobalUniform({uniform_renderSurfels, true});	
	Node* node; uint32_t prefix; float size, distance;
	for(auto& s : deferredSurfelQueue) {
		std::tie(distance, node, prefix, size) = s;
    if(size < minSize || size >= maxSize)
      continue;			
		auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));
		auto surfels = surfelAttribute ? surfelAttribute->get() : nullptr;
		rc.pushAndSetPointParameters( Rendering::PointParameters(size));
		rc.pushAndSetMatrix_modelToCamera( rc.getMatrix_worldToCamera() );
		rc.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
		context.displayMesh(surfels,	0, prefix );
		rc.popMatrix_modelToCamera();
		rc.popPointParameters();
	}
	rc.setGlobalUniform({uniform_renderSurfels, false});
}

}
}
#endif // MINSG_EXT_BLUE_SURFELS
