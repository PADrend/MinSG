/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius J�hn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelRenderer2.h"
#include "SurfelGenerator.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Transformations.h"
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

#define SURFEL_MEDIAN_COUNT 1000
		
SurfelRenderer2::SurfelRenderer2() : NodeRendererState(FrameContext::DEFAULT_CHANNEL),
		countFactor(1.0f),sizeFactor(2.0f),maxSurfelSize(32.0) {
}
SurfelRenderer2::~SurfelRenderer2() {}

NodeRendererResult SurfelRenderer2::displayNode(FrameContext & context, Node * node, const RenderParam & /*rp*/){
	if(!node->isActive())
		return NodeRendererResult::NODE_HANDLED;
	
	static const Util::StringIdentifier SURFEL_ATTRIBUTE("surfels");
	auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));
		
	if( !surfelAttribute || !surfelAttribute->get())
		return NodeRendererResult::PASS_ON;

	Rendering::Mesh& surfelMesh = *surfelAttribute->get();
		
	static const Util::StringIdentifier SURFEL_MEDIAN_ATTRIBUTE("surfelMedianDist");
	auto surfelMedianAttr = node->findAttribute(SURFEL_MEDIAN_ATTRIBUTE);
	float surfelMedianDist = 0;
	if(!surfelMedianAttr) {
		surfelMedianDist = SurfelGenerator::getMedianOfNthClosestNeighbours(surfelMesh, SURFEL_MEDIAN_COUNT, 2);
		node->setAttribute(SURFEL_MEDIAN_ATTRIBUTE, Util::GenericAttribute::createNumber(surfelMedianDist));
	} else {
		surfelMedianDist = surfelMedianAttr->toFloat();
	}
	
	float meterPerPixel;
	{
		static Geometry::Vec3 X_AXIS(1,0,0);
		auto centerWorld = Transformations::localPosToWorldPos(*node, node->getBB().getCenter() );
		auto oneMeterVector = Transformations::localDirToWorldDir(*context.getCamera(), X_AXIS ).normalize()*0.1;
		auto screenPos1 = context.convertWorldPosToScreenPos(centerWorld);
		auto screenPos2 = context.convertWorldPosToScreenPos(centerWorld+oneMeterVector);
		float d = screenPos1.distance(screenPos2)*10;
		float nodeScale = node->getWorldTransformationSRT().getScale();
		meterPerPixel = 1/(d!=0?d:1) / nodeScale;
	}
	
	float surfelSize = this->sizeFactor;
	float meterPerSurfel = std::min(surfelSize,this->maxSurfelSize) * meterPerPixel;
	
	uint32_t maxCount = surfelMesh.isUsingIndexData() ?  surfelMesh.getIndexCount() : surfelMesh.getVertexCount();	
	uint32_t surfelCount = (SURFEL_MEDIAN_COUNT * surfelMedianDist * surfelMedianDist / (meterPerSurfel*meterPerSurfel/4)) * this->countFactor;

	float minSurfelDistance = surfelMedianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(maxCount));
	bool renderOriginal = surfelCount > maxCount && minSurfelDistance > meterPerSurfel/2;
	surfelCount = std::min(std::max<uint32_t>(renderOriginal ? (2*maxCount-std::min(2*maxCount,surfelCount)) : surfelCount,0),maxCount);

	if(surfelCount > 0) {
		auto& renderingContext = context.getRenderingContext();
		
		static Rendering::Uniform enableSurfels("renderSurfels", true);
		static Rendering::Uniform disableSurfels("renderSurfels", false);
		
		renderingContext.setGlobalUniform(enableSurfels);
		renderingContext.pushAndSetPointParameters( Rendering::PointParameters(std::min(surfelSize,this->maxSurfelSize) ));
		renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
		renderingContext.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
		context.displayMesh(&surfelMesh,	0, surfelCount );
		renderingContext.popMatrix_modelToCamera();
		renderingContext.popPointParameters();
		renderingContext.setGlobalUniform(disableSurfels);
		
		return renderOriginal ? NodeRendererResult::PASS_ON : NodeRendererResult::NODE_HANDLED;
	}
	return NodeRendererResult::PASS_ON;
}



}
}
#endif // MINSG_EXT_BLUE_SURFELS
