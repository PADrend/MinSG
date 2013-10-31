/*
	This file is part of the MinSG library.
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "RandomColorRenderer.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Geometry/Box.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <cstdlib>

namespace MinSG {

Rendering::Shader * RandomColorRenderer::shader = nullptr;

Rendering::Shader * RandomColorRenderer::getShader() {
	const std::string vertexProgram("void main() {gl_Position = ftransform();}");
	const std::string fragmentProgram("uniform vec4 colorOverride;\
												void main() {\
												gl_FragColor = colorOverride;\
												}");
	if (shader == nullptr) {
		shader = Rendering::Shader::createShader(vertexProgram, fragmentProgram);
	}
	return shader;
}

RandomColorRenderer::RandomColorRenderer() :
	State(), lastRoot(nullptr) {
}

RandomColorRenderer::RandomColorRenderer(const RandomColorRenderer & source) :
	State(source), lastRoot(source.lastRoot), colorMapping(source.colorMapping) {
}

RandomColorRenderer::~RandomColorRenderer() {
}
struct stack_element_t {
		stack_element_t(Node * _node, unsigned char _level, float * _color) :
			node(_node), level(_level), color(_color) {
		}
		Node * node;
		unsigned char level;
		float * color;
};
State::stateResult_t RandomColorRenderer::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	if (rp.getFlag(SKIP_RENDERER)) {
		return State::STATE_SKIPPED;
	}

	if (node != lastRoot) {
		lastRoot = node;
		colorMapping.clear();


		// Get nodes under the root node.
		const auto nodes = collectNodes<GeometryNode>(node);

		// Maps sorted by coordinates pointing to color values.
		std::multimap<float, Util::Color4f *> xSorted;
		std::multimap<float, Util::Color4f *> ySorted;
		std::multimap<float, Util::Color4f *> zSorted;
		for(const auto & geoNode : nodes) {
			// Insert all nodes with empty colors into mapping.
			auto insertIt = colorMapping.insert(std::make_pair(geoNode, Util::Color4f())).first;
			// Store pointers to color values.
			const Geometry::Box & bb = geoNode->getWorldBB();
			const Geometry::Vec3f center = bb.getCenter();
			xSorted.insert(std::make_pair(center.getX(), &(insertIt->second)));
			ySorted.insert(std::make_pair(center.getY(), &(insertIt->second)));
			zSorted.insert(std::make_pair(center.getZ(), &(insertIt->second)));
		}

		// Rainbow colors
		const size_t size = colorMapping.size();
		size_t count = 0;
		for (auto it = xSorted.rbegin(); it != xSorted.rend(); ++it) {
			it->second->setR(static_cast<float> (count) / static_cast<float> (size));
			++count;
		}
		count = 0;
		for (auto it = ySorted.rbegin(); it != ySorted.rend(); ++it) {
			it->second->setG(static_cast<float> (count) / static_cast<float> (size));
			++count;
		}
		count = 0;
		for (auto it = zSorted.rbegin(); it != zSorted.rend(); ++it) {
			it->second->setB(static_cast<float> (count) / static_cast<float> (size));
			++count;
		}
		// Checkerboard
		bool darker = true;
		for (std::multimap<float, Util::Color4f *>::const_iterator it = xSorted.begin(); it != xSorted.end(); ++it) {
			if (darker) {
				*(it->second) *= 0.8f;
			}
			darker = !darker;
		}
	}

	context.getRenderingContext().pushAndSetShader(getShader());
	for (color_map_t::const_iterator it = colorMapping.begin(); it != colorMapping.end(); ++it) {
		getShader()->setUniform(context.getRenderingContext(), Rendering::Uniform("colorOverride", it->second));
		context.displayNode(it->first, rp);
	}
	context.getRenderingContext().popShader();
	return State::STATE_SKIP_RENDERING;
}

RandomColorRenderer * RandomColorRenderer::clone() const {
	return new RandomColorRenderer(*this);
}

}
