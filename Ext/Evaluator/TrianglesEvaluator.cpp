/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_EVALUATORS

/*
 * This file is part of Benjamin Eikel's Master's thesis.
 * Copyright 2009 Benjamin Eikel
 */

#include "TrianglesEvaluator.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/VertexDescription.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Draw.h>
#include <Rendering/OcclusionQuery.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>

#include <list>
#include <memory>

using namespace Rendering;

namespace MinSG {
namespace Evaluators {

static Rendering::Shader * getShader() {
	static std::unique_ptr<Rendering::Shader> shader(Shader::createShader("void main(void){gl_Position=ftransform();}",
			"void main(void){gl_FragColor=vec4(1.0,0.0,0.0,1.0);}"));
	return shader.get();
}

TrianglesEvaluator::TrianglesEvaluator() :
	Evaluator(SINGLE_VALUE), numTrianglesRendered(0), numTrianglesVisible(0) {
}

TrianglesEvaluator::~TrianglesEvaluator() {
}

void TrianglesEvaluator::beginMeasure() {
	numTrianglesRendered = 0;
	numTrianglesVisible = 0;
}

void TrianglesEvaluator::measure(FrameContext & context, Node & node,
									const Geometry::Rect & /*r*/) {
	context.getRenderingContext().clearScreen(Util::Color4f(0.9f, 0.9f, 0.9f, 1.0f));
	context.getRenderingContext().pushAndSetShader(getShader());

	// Fill the buffers for occlusion culling.
	node.display(context, USE_WORLD_MATRIX | FRUSTUM_CULLING);

	context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, true, Rendering::Comparison::EQUAL));

	// Collect and render visible geometry nodes.
	const auto geoNodes = collectVisibleNodes(&node, context);
	for(const auto & geoNode : geoNodes) {
		numTrianglesRendered += geoNode->getTriangleCount();
		// First rendering pass: Fill depth buffer.
		context.displayNode(geoNode, USE_WORLD_MATRIX);
	}

	for(const auto & geoNode : geoNodes) {
		// Second rendering pass: Determine visibility.
		numTrianglesVisible += getNumTrianglesVisible(context, geoNode);
	}

	context.getRenderingContext().popDepthBuffer();

	context.getRenderingContext().popShader();
}

void TrianglesEvaluator::endMeasure(FrameContext & /*context*/) {
}

size_t TrianglesEvaluator::getNumTrianglesVisible(FrameContext & context, GeometryNode * node) {
	if (node == nullptr) {
		WARN("GeometryNode is nullptr.");
		return 0;
	}

	Mesh * mesh = node->getMesh();
	if (mesh == nullptr) {
		WARN("Mesh of GeometryNode is nullptr.");
		return 0;
	}

	size_t numIndices = mesh->getIndexCount();
	size_t visibleTriangles = 0;


	// Setup occlusion queries.
	size_t numQueries = numIndices / 3;
	if (numQueries == 0) {
		return 0;
	}

	context.getRenderingContext().pushMatrix_modelToCamera();
	context.getRenderingContext().multMatrix_modelToCamera(node->getWorldMatrix());

	const VertexDescription & desc = mesh->getVertexDescription();
	const VertexAttribute & posAttr=desc.getAttribute(VertexAttributeIds::POSITION);
	if (posAttr.getNumValues() != 3 || mesh->getDrawMode() != Rendering::Mesh::DRAW_TRIANGLES) {
		WARN( "TrianglesEvaluator::getNumTrianglesVisible: Unsupported mesh format.");
		return 0;
	}
	const uint16_t vertexOffset = posAttr.getOffset();
	const std::size_t stride = desc.getVertexSize();

	const MeshVertexData & vd = mesh->openVertexData();
	const MeshIndexData & id = mesh->openIndexData();

	const uint8_t * data = vd.data() + vertexOffset;
	const uint32_t * index = id.data();

	// Start queries.
	std::vector<Rendering::OcclusionQuery> queries(numQueries);
	Rendering::OcclusionQuery::enableTestMode(context.getRenderingContext());
	for (auto & query : queries) {
		query.begin();
		Rendering::drawTriangle(context.getRenderingContext(),
								Geometry::Vec3f(reinterpret_cast<const float *>(data + (index[0] * stride))),
								Geometry::Vec3f(reinterpret_cast<const float *>(data + (index[1] * stride))),
								Geometry::Vec3f(reinterpret_cast<const float *>(data + (index[2] * stride)))
							   );
		query.end();
		index += 3;
	}
	Rendering::OcclusionQuery::disableTestMode(context.getRenderingContext());
	// Get results.
	for (const auto & query : queries) {
		if (query.getResult() > 0) {
			++visibleTriangles;
		}
	}

	// Clean up.
	context.getRenderingContext().popMatrix_modelToCamera();

	return visibleTriangles;
}

const Util::GenericAttributeList * TrianglesEvaluator::getResults() {
	values->clear();
	values->push_back(Util::GenericAttribute::createNumber(numTrianglesVisible));
	return values.get();
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
