/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius J�hn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelRenderer.h"

#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/CameraNode.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Mesh/Mesh.h>
//#include "../../../Geometry/Rect.h"
//#include "../../Helper/StdNodeVisitors.h"

#include <Util/StringIdentifier.h>
#include <Util/GenericAttribute.h>
//#include <Util/Graphics/ColorLibrary.h>

//#include <Rendering/Draw.h>
//#include <Rendering/MeshUtils/Simplification.h>

//#include <cassert>
//#include <array>

namespace MinSG{
namespace BlueSurfels {


		
SurfelRenderer::SurfelRenderer() : NodeRendererState(FrameContext::DEFAULT_CHANNEL),
		minSideLength(100.0f),maxSideLength(200.0f),countFactor(2.0f),sizeFactor(2.0f),maxSurfelSize(4.0),projectionScale(1.0){
}
SurfelRenderer::~SurfelRenderer() {}

//node.setNodeAttribute('surfelRelCovering', surfelInfo.relativeCovering);
//
//static Util::GenericAttributeList* getLODs(Node * node){
//	if(geo->isInstance())
//		return dynamic_cast<Util::GenericAttributeList*>(geo->getPrototype()->findAttribute(idMeshes));
//	else
//		return dynamic_cast<Util::GenericAttributeList*>(geo->getAttribute(idMeshes));
//}

State::stateResult_t SurfelRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	State::stateResult_t result = NodeRendererState::doEnableState(context,node,rp);
	if( result == State::STATE_OK ){
		CameraNode* camera = dynamic_cast<CameraNode*>( context.getCamera() );
		if(camera){ // else: an orthographic camera is used; just re-use the previous values.
			cameraOrigin = camera->getWorldOrigin();
			projectionScale = camera->getWidth() / (std::tan((camera->getRightAngle()-camera->getLeftAngle()).rad()*0.5f)*2.0f);
		}
	
	}
	return result;
}

NodeRendererResult SurfelRenderer::displayNode(FrameContext & context, Node * node, const RenderParam & /*rp*/){
	
	static const Util::StringIdentifier SURFEL_ATTRIBUTE("surfels");
	auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));
		
	if( !surfelAttribute || !surfelAttribute->get())
		return NodeRendererResult::PASS_ON;

	Rendering::Mesh& surfelMesh = *surfelAttribute->get();
	
	
	const Geometry::Rect projection = context.getProjectedRect(node);
	const float approxProjectedSideLength =  std::sqrt(projection.getHeight() * projection.getWidth());

//	const auto& worldBB = node->getWorldBB();
//	const float approxProjectedSideLength = projectionScale * worldBB.getDiameter() / (worldBB.getCenter()-cameraOrigin).length();
	
	if(approxProjectedSideLength > maxSideLength)
		return NodeRendererResult::PASS_ON;

	static const Util::StringIdentifier REL_COVERING_ATTRIBUTE("surfelRelCovering");
	auto surfelCoverageAttr = node->findAttribute(REL_COVERING_ATTRIBUTE);
	const float relCovering = surfelCoverageAttr ? surfelCoverageAttr->toFloat() : 0.5;

	const float approxProjectedArea = approxProjectedSideLength * approxProjectedSideLength * relCovering;
	
	uint32_t surfelCount = std::min(	surfelMesh.isUsingIndexData() ?  surfelMesh.getIndexCount() : surfelMesh.getVertexCount(),
										static_cast<uint32_t>(approxProjectedArea * countFactor) + 1);
										
	float surfelSize = std::min(sizeFactor * approxProjectedArea / surfelCount,maxSurfelSize);

	bool handled = true;
	if(approxProjectedSideLength > minSideLength && minSideLength<maxSideLength){
		const float f = 1.0f -(approxProjectedSideLength-minSideLength) / (maxSideLength-minSideLength);
		surfelCount =  std::min(	surfelMesh.isUsingIndexData() ?  surfelMesh.getIndexCount() : surfelMesh.getVertexCount(),
										static_cast<uint32_t>(f * surfelCount) + 1);
		surfelSize *= f;
		handled = false;
//		std::cout << approxProjectedSideLength<<"\t"<<f<<"\n";
		
	}
//	std::cout << surfelSize<<"\t"<<"\n";
//	if( node->getRenderingLayers()&0x02 )
//		std::cout << "pSize"<<approxProjectedSideLength << "\t#:"<<surfelCount<<"\ts:"<<surfelSize<<"\n";
	auto& renderingContext = context.getRenderingContext();
	
	static Rendering::Uniform enableSurfels("renderSurfels", true);
	static Rendering::Uniform disableSurfels("renderSurfels", false);
	
	renderingContext.setGlobalUniform(enableSurfels);
	renderingContext.pushAndSetPointParameters( Rendering::PointParameters(std::min(surfelSize,32.0f) ));
	renderingContext.pushAndSetMatrix_modelToCamera( renderingContext.getMatrix_worldToCamera() );
	renderingContext.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
	context.displayMesh(&surfelMesh,	0, surfelCount );
	renderingContext.popMatrix_modelToCamera();
	renderingContext.popPointParameters();
	renderingContext.setGlobalUniform(disableSurfels);
	
	return handled ? NodeRendererResult::NODE_HANDLED : NodeRendererResult::PASS_ON;
}



}
}
#endif // MINSG_EXT_BLUE_SURFELS
