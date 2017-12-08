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
#include <Rendering/Draw.h>

#include <Geometry/Vec3.h>
#include <Geometry/Vec2.h>

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
}

static Geometry::Vec2 closestRect(const Geometry::Rect& rect, const Geometry::Vec2& pos) {
	Geometry::Vec2 out(pos);
	if(pos.x() < rect.getMinX())
		out.x(rect.getMinX());
	else if(pos.x() > rect.getMaxX())
		out.x(rect.getMaxX());
	if(pos.y() < rect.getMinY())
		out.y(rect.getMinY());
	else if(pos.y() > rect.getMaxY())
		out.y(rect.getMaxY());
	return out;
}

SurfelRendererFixedSize::SurfelRendererFixedSize() : NodeRendererState(FrameContext::DEFAULT_CHANNEL),
		countFactor(1.0f),sizeFactor(1.0f),surfelSize(1.0f),maxSurfelSize(32.0), maxFrameTime(16.0f), 
		debugHideSurfels(false), debugCameraEnabled(false), deferredSurfels(false), adaptive(false), foveated(false) {
		foveatZones.push_back({0.0f, 0.0f});
		foveatZones.push_back({0.25f, 1.0f});
		foveatZones.push_back({1.0f, 0.5f});
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
	
	Rendering::Mesh* surfelMesh;
	uint32_t maxSurfelCount;
	float surfelMedianDist;
	// get surfels
	std::tie(surfelMesh, maxSurfelCount, surfelMedianDist) = getSurfelsForNode(context, node);
	
	if( !surfelMesh )
		return NodeRendererResult::PASS_ON;
	

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
	float cFactor = this->countFactor;
	if(foveated) {
		const auto viewport = context.getCamera()->getViewport();
		Geometry::Vec2 vpCenter(viewport.getCenter());
		auto projRect = context.getProjectedRect(node);
		
		auto it = foveatZones.begin();
		if(it != foveatZones.end()) 
			vpCenter += {it->first * viewport.getWidth(),it->second * viewport.getHeight()}; // offset
		float dist = closestRect(projRect, vpCenter).distance(vpCenter);
					
		if(dist > 0) {
			float maxDist = std::sqrt(viewport.getWidth()*viewport.getWidth() + viewport.getHeight()*viewport.getHeight()) * 0.5f;
			float z1 = 0;
			float z2 = maxDist;
			float c1 = pointSize;
			float c2 = maxSurfelSize;
			for(++it; it != foveatZones.end(); ++it) {
				if(dist < it->first * maxDist) {
					z2 = it->first * maxDist;
					c2 = it->second * pointSize;
					break;
				} else {
					z1 = it->first * maxDist;
					c1 = it->second * pointSize;
				}
			}
			float a = std::min((dist-z1) / (z2-z1), 1.0f);
			//cFactor = (1.0f - a) * c1 + a * c2;
			pointSize = (1.0f - a) * c1 + a * c2;
			//std::cout << "\ra= " << a << ", z1=(" << z1 << "," << c1 << "), z2=(" << z2 << "," << c2 << "), d=" << dist << ", f=" << cFactor << "             " << std::flush;
		}
	}
	
	float surfelRadius = pointSize * meterPerPixel / 2.0f;
	float minSurfelDistance = surfelMedianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(maxSurfelCount));

	// Calculate the surfel prefix length based on the estimated median distance between surfels and the coverage of one surfel
	uint32_t surfelPrefixLength = (SURFEL_MEDIAN_COUNT * surfelMedianDist * surfelMedianDist / (surfelRadius * surfelRadius)) * cFactor;

	bool renderOriginal = surfelPrefixLength > maxSurfelCount && minSurfelDistance > surfelRadius;
	if(renderOriginal) {
		uint32_t diff = std::min(maxSurfelCount, surfelPrefixLength - maxSurfelCount);
		surfelPrefixLength = maxSurfelCount - diff;
	}
	surfelPrefixLength = std::min(surfelPrefixLength,maxSurfelCount);
	
	if(debugHideSurfels && !renderOriginal)
		return NodeRendererResult::NODE_HANDLED;

	if(surfelPrefixLength > 0 && !debugHideSurfels) {
		if(debugCameraEnabled)
			pointSize *= meterPerPixel/meterPerPixelOriginal;
		//pointSize = std::min(pointSize*sizeFactor,this->maxSurfelSize);
		pointSize *= sizeFactor;
		
		if(deferredSurfels) {
			float camDistSqr = node->getWorldBB().getDistanceSquared(context.getCamera()->getWorldOrigin());
			deferredSurfelQueue.emplace(camDistSqr, node, surfelPrefixLength, pointSize);
			//deferredSurfelQueue.emplace_back(camDistSqr, node, surfelPrefixLength, pointSize);
		} else {
			renderingContext.setGlobalUniform({uniform_renderSurfels, true});
			renderingContext.pushAndSetPointParameters( Rendering::PointParameters(pointSize));
			renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
			renderingContext.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
			context.displayMesh(surfelMesh,	0, surfelPrefixLength );
			renderingContext.popMatrix_modelToCamera();
			renderingContext.popPointParameters();
			renderingContext.setGlobalUniform({uniform_renderSurfels, false});
		}
	}
	return renderOriginal ? NodeRendererResult::PASS_ON : NodeRendererResult::NODE_HANDLED;
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
		
	if(foveated && debugFoveated) {
		const auto viewport = context.getCamera()->getViewport();
		Geometry::Vec2 vpCenter(viewport.getCenter());
		
		auto it = foveatZones.begin();
		if(it != foveatZones.end()) 
			vpCenter += {it->first * viewport.getWidth(),-it->second * viewport.getHeight()}; // offset
		
		auto& rc = context.getRenderingContext(); 
		Rendering::enable2DMode(rc, viewport);
		rc.pushAndSetLighting(Rendering::LightingParameters());
		
		float rad = std::sqrt(viewport.getWidth()*viewport.getWidth() + viewport.getHeight()*viewport.getHeight()) * 0.5f;
		for(++it; it != foveatZones.end(); ++it) {
			//Geometry::Rect rect(vpCenter.x() - it->first*rad, viewport.getHeight() - vpCenter.y() - it->first*rad, it->first*2*rad, it->first*2*rad);
			//Rendering::drawWireframeRect(context.getRenderingContext(), rect, Util::Color4f(0,0,0.5,1));
			Rendering::drawWireframeCircle(context.getRenderingContext(), vpCenter, it->first*rad, Util::Color4f(0,0,0.5,1));
		}
		
		rc.popLighting();
		Rendering::disable2DMode(rc);
	}
}

SurfelRendererFixedSize::Surfels_t SurfelRendererFixedSize::getSurfelsForNode(FrameContext & context, Node * node) {
	// get surfel mesh
	auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));		
	if(surfelAttribute && surfelAttribute->get()) {
		Rendering::Mesh* surfelMesh = surfelAttribute->get();		
		
		uint32_t maxSurfelCount = surfelMesh->isUsingIndexData() ? surfelMesh->getIndexCount() : surfelMesh->getVertexCount();
		
		// get median distance between surfels at a fixed prefix length 
		float surfelMedianDist = getMedianDist(node, *surfelMesh);
		
		return {surfelMesh, maxSurfelCount, surfelMedianDist};
	}
	return {nullptr, 0, 0};
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
