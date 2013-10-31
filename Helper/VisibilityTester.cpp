/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VisibilityTester.h"
#include "../Core/Nodes/GeometryNode.h"
#include "../Core/FrameContext.h"
#include <Geometry/Box.h>
#include <Rendering/RenderingContext/ParameterStructs.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <cstdint>
#include <deque>
#include <vector>

namespace MinSG {
namespace VisibilityTester {

std::deque<std::pair<GeometryNode *, uint32_t>> testNodes(FrameContext & context, const std::deque<GeometryNode *> & nodes) {
	std::deque<std::pair<GeometryNode *, uint32_t>> result;

	// First pass (done before calling this function): Render scene to fill depth buffer.
	if(nodes.empty()) {
		return result;
	}

	const std::size_t numQueries = nodes.size();
	std::vector<bool> bbVisible(numQueries, false);
	std::vector<Rendering::OcclusionQuery> queries(numQueries);

	Rendering::RenderingContext & renderingContext = context.getRenderingContext();

	// Second pass: Determine the visible bounding boxes first.
	Rendering::OcclusionQuery::enableTestMode(renderingContext);

	renderingContext.pushAndSetCullFace(Rendering::CullFaceParameters());
	// Use polygon offset when rendering bounding boxes to prevent errors due to z fighting.
	renderingContext.pushAndSetPolygonOffset(Rendering::PolygonOffsetParameters(0.0f, -1.0f));
	renderingContext.applyChanges();

	for(std::size_t i = 0; i < numQueries; ++i) {
		queries[i].begin();
		Rendering::drawAbsBox(renderingContext, nodes[i]->getWorldBB());
		queries[i].end();
	}

	renderingContext.popPolygonOffset();
	renderingContext.applyChanges();

	// Third pass: Determine the visible objects by rendering their geometry.
	for(std::size_t i = 0; i < numQueries; ++i) {
		if(queries[i].getResult() > 0) {
			bbVisible[i] = true;
			queries[i].begin();
			// Bounding box is visible: Render geometry.
			nodes[i]->display(context, USE_WORLD_MATRIX | NO_STATES);
			queries[i].end();
		}
	}

	renderingContext.popCullFace();
	Rendering::OcclusionQuery::disableTestMode(renderingContext);

	// Check query result.
	for(std::size_t i = 0; i < numQueries; ++i) {
		if(bbVisible[i]) {
			const uint32_t sampleCount = queries[i].getResult();
			if(sampleCount > 0) {
				result.emplace_back(nodes[i], sampleCount);
			}
		}
	}
	return result;
}

}
}
