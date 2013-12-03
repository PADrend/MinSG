/*
	This file is part of the MinSG library extension ColorCubes.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2010-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2010-2011 Paul Justus <paul.justus@gmx.net>
	Copyright (C) 2010-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_COLORCUBES

#include "ColorCubeRenderer.h"

#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"

#include <Geometry/BoxHelper.h>

#include <Rendering/Mesh/Mesh.h>
#include <Rendering/Mesh/MeshDataStrategy.h>
#include <Rendering/Mesh/VertexAttributeIds.h>
#include <Rendering/MeshUtils/MeshBuilder.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>

#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>

#include <cassert>
#include <functional>
#include <sstream>

using namespace Util;
using namespace Rendering;

namespace MinSG {

ColorCubeRenderer::ColorCubeRenderer() :
		NodeRendererState(FrameContext::APPROXIMATION_CHANNEL), colorcubeNodes(), highlight(false) {
}

ColorCubeRenderer::ColorCubeRenderer(Util::StringIdentifier _channel) :
		NodeRendererState(_channel), colorcubeNodes(), highlight(false) {
}

ColorCubeRenderer * ColorCubeRenderer::clone() const {
	return new ColorCubeRenderer(*this);
}

State::stateResult_t ColorCubeRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	if ( !isActive() || (rp.getFlag(NO_GEOMETRY)) || (rp.getFlag(SKIP_RENDERER)) )
		return State::STATE_SKIPPED;

	// initialize distance queue for color cube nodes
	colorcubeNodes.clear();

	return NodeRendererState::doEnableState(context, node, rp);
}

NodeRendererResult ColorCubeRenderer::displayNode(FrameContext & /*context*/, Node * node, const RenderParam & /*rp*/) {
	colorcubeNodes.push_back(node);
	return NodeRendererResult::NODE_HANDLED;
}

void ColorCubeRenderer::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	if (rp.getFlag(NO_GEOMETRY) || rp.getFlag(SKIP_RENDERER))
		return;

	NodeRendererState::doDisableState(context, node, rp);

	renderColorCubes(context, colorcubeNodes);

	colorcubeNodes.clear();
}

//! (internal) used to render the color cubes stored in the priority queue
void ColorCubeRenderer::renderColorCubes(FrameContext& context,std::deque<Node*> & nodes) const{
	context.getRenderingContext().pushMatrix();
	context.getRenderingContext().resetMatrix();
// 	context.getRenderingContext().pushAndSetShader(nullptr);
	context.getRenderingContext().pushAndSetBlending(BlendingParameters(BlendingParameters::SRC_ALPHA, BlendingParameters::ONE_MINUS_SRC_ALPHA));
	context.getRenderingContext().pushAndSetCullFace( CullFaceParameters::CULL_BACK );
	context.getRenderingContext().applyChanges();

	if(isHighlightEnabled()){
		context.getRenderingContext().pushAndSetColorMaterial(Util::ColorLibrary::RED);
	}

	std::sort(
		nodes.begin(),
		nodes.end(),
		_DistanceCompare<Node, std::greater<volatile float>, std::greater<const Node *> >(context.getCamera()->getWorldPosition())
	);


	uint32_t meshIndex = largestDisplayMeshExp;
	uint32_t meshSize  = largestDisplayMeshSize;

	while (!nodes.empty()) {
		while (meshSize > nodes.size()) {
			meshSize /= 2;
			meshIndex--;
		}

		context.displayMesh(ColorCubeRenderer::getColorCubesMesh(context, nodes, meshSize, meshIndex));
	}

	if(isHighlightEnabled())
		context.getRenderingContext().popMaterial();

	context.getRenderingContext().popCullFace();
	context.getRenderingContext().popBlending();
// 	context.getRenderingContext().popShader();
	context.getRenderingContext().popMatrix();
}

//! (static,internal) used by renderColorCubes(...)
Rendering::Mesh * ColorCubeRenderer::getColorCubesMesh(FrameContext& context, std::deque<Node*> & nodes, uint32_t meshsize, uint32_t meshindex){
	static Util::Reference<Mesh> smeshes[largestDisplayMeshExp+1];

	if (smeshes[meshindex].isNull())
		smeshes[meshindex] = ColorCubeRenderer::createMesh(meshsize);
	Mesh * mesh = smeshes[meshindex].get();

	MeshVertexData & vd = mesh->openVertexData();
	const VertexDescription & desc = vd.getVertexDescription();

	uint8_t * dataCursor = vd.data();
	const uint8_t vertexOffset = desc.getAttribute(VertexAttributeIds::POSITION).getOffset();
	const uint8_t colorOffset  = desc.getAttribute(VertexAttributeIds::COLOR).getOffset();

	for (uint32_t index=0; index<meshsize; ++index) {

		Node * node = nodes.front();
		nodes.pop_front();

		const Geometry::Box& box = node->getWorldBB();
		const bool hasColorCube = ColorCube::hasColorCube(node);
		if(!hasColorCube) {
			ColorCube::buildColorCubes(context, node);
		}
		const ColorCube & cc = hasColorCube ? ColorCube::getColorCube(node) : ColorCube();

		for (uint_fast8_t s = 0; s < 6; ++s) {
			const Geometry::side_t side = static_cast<Geometry::side_t> (s);
			const Geometry::corner_t * corners = Geometry::Helper::getCornerIndices(side);
			for (uint_fast8_t v = 0; v < 4; ++v) {
				// Position
				const Geometry::Vec3 & corner = box.getCorner(corners[v]);
				float * vertex=reinterpret_cast<float*>(dataCursor+vertexOffset);
				*vertex++ = corner.getX();
				*vertex++ = corner.getY();
				*vertex   = corner.getZ();

				// Color
				const Util::Color4ub & c = cc.getColor(side);

				(dataCursor+colorOffset)[0] = c.getR();
				(dataCursor+colorOffset)[1] = c.getG();
				(dataCursor+colorOffset)[2] = c.getB();
				(dataCursor+colorOffset)[3] = c.getA();

				dataCursor+=desc.getVertexSize();
			}
		}
	}
	vd.updateBoundingBox();
	vd.markAsChanged();

	return mesh;
}

//! (static,internal) used by getColorCubesMesh(...)
Rendering::Mesh * ColorCubeRenderer::createMesh(uint32_t size) {
	using namespace Rendering;

	Rendering::VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalByte();
	vertexDescription.appendColorRGBAByte();
	MeshUtils::MeshBuilder meshBuilder(vertexDescription);

	for (uint32_t index=0; index<size; ++index) {
		// add vertices
		const Geometry::Box box(Geometry::Vec3(-0.5f, -0.5f, -0.5f), Geometry::Vec3(0.5f, 0.5f, 0.5f));
		meshBuilder.color(Util::ColorLibrary::RED);

		for (uint_fast8_t s = 0; s < 6; ++s) {
			const Geometry::side_t side = static_cast<Geometry::side_t> (s);
			const Geometry::corner_t * corners = Geometry::Helper::getCornerIndices(side);

			meshBuilder.normal( Geometry::Helper::getNormal(side) );
			for (uint_fast8_t v = 0; v < 4; ++v) {
				meshBuilder.position(box.getCorner(corners[v]));
				meshBuilder.addVertex();
			}
		}
		// add indices
		uint32_t begin = index*6;
		uint32_t end = begin + 6;
		for (uint32_t i = begin; i < end; ++i) {
			meshBuilder.addQuad( i*4, i*4+1, i*4+2, i*4+3 );
		}
	}

	Mesh * m = meshBuilder.buildMesh();
	m->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());

	return m;
}

}

#endif // MINSG_EXT_COLORCUBES
