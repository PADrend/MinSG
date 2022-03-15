/*
	This file is part of the MinSG library extension MixedExtVisibility.
	Copyright (C) 2015 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MIXED_EXTERN_VISIBILITY

#include "Preprocessor.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/Graphics/Color.h>

#include <algorithm>

 
struct KeyHash {
 size_t operator()(const Util::Reference<MinSG::Node>& a) const
 {
 	std::hash<MinSG::Node*> hash_fn;
    return hash_fn(a.get());
 }
};

std::vector<Util::Reference<MinSG::Node>> MinSG::MixedExtVisibility::
		filterAndSortNodesByExtVisibility(FrameContext & context,
											const std::vector<Util::Reference<Node>>& nodes,
											const std::vector<Util::Reference<AbstractCameraNode>>& cameras,
											size_t polygonLimit){
	context.pushCamera();
	std::unordered_map<Util::Reference<MinSG::Node>,uint32_t,KeyHash> nodeVisibilities; // node -> max visible pixels
	
	for(const auto& node: nodes)
		nodeVisibilities[node] = 0;
	
	RenderParam param(USE_WORLD_MATRIX);
	param.setRenderingLayers(1);

	for(const auto& camera : cameras){
		context.setCamera(camera.get());


		// first pass
		context.getRenderingContext().clearScreen(Util::Color4f(0,0,0,0));
		for(const auto & node : nodes) 
			context.displayNode(node.get(), param);

		context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::EQUAL));

		std::vector<Rendering::OcclusionQuery> queries(nodeVisibilities.size());
		size_t i = 0;
		Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
		for(const auto & node : nodes) {
			queries[i].begin(context.getRenderingContext());
			context.displayNode(node.get(), param);
			queries[i].end(context.getRenderingContext());
			++i;
		}
		Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
		i=0;
		for(const auto & node : nodes){
			nodeVisibilities[node] = std::max(queries[i].getResult(),nodeVisibilities[node]);
			++i;
		}
		context.getRenderingContext().popDepthBuffer();
	}
	context.popCamera();
	
	std::vector<std::pair<Util::Reference<Node>,uint32_t>> sortedNodes;
	for(const auto& nodeVis : nodeVisibilities)
		sortedNodes.emplace_back(nodeVis.first,nodeVis.second);
	std::sort(sortedNodes.begin(),sortedNodes.end(),
			[](const std::pair<Util::Reference<Node>,uint32_t>&a,const std::pair<Util::Reference<Node>,uint32_t>&b){return a.second>b.second;});

	for(const auto& nodeVis : sortedNodes)
		std::cout << " "<<nodeVis.second<<" ";
	
	std::vector<Util::Reference<Node>> collectedNodes;
	size_t pCounter = 0;
	for(auto& entry : sortedNodes){
		if(entry.second==0)
			break;
		pCounter += countTriangles(entry.first.get());
		if(polygonLimit>0 && pCounter>polygonLimit)
			break;
		collectedNodes.emplace_back(std::move(entry.first));
	}
	return collectedNodes;
}


#endif /* MINSG_EXT_MIXED_EXTERN_VISIBILITY */
