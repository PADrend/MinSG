/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "TreeVisualization.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../SceneManagement/SceneManager.h"
#include <Geometry/Box.h>
#include <Rendering/Draw.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/Color.h>
#include <Util/Macros.h>
#include <deque>
#include <sstream>

namespace MinSG {
namespace TriangleTrees {

TreeVisualization::TreeVisualization() :
	State(), drawDepth(3), showSplittingPlanes(true), showBoundingBoxes(false), showLines(false) {
}

TreeVisualization::~TreeVisualization() = default;

TreeVisualization * TreeVisualization::clone() const {
	return new TreeVisualization(*this);
}

void TreeVisualization::displayBinaryTree(FrameContext & context, Node * root) const {
	// Different colors for the different dimensions.
	const unsigned char colors[3][4] = { { 255, 0, 0, 50 }, { 0, 255, 0, 50 }, { 0, 0, 255, 50 } };
	// Store nodes with their level.
	std::deque<std::pair<Node *, unsigned char> > nodes;
	nodes.emplace_back(root, 0);

	while (!nodes.empty()) {
		Node * current = nodes.front().first;
		unsigned char level = nodes.front().second;
		nodes.pop_front();
		Util::GenericAttribute * dimensionAttribute = current->getAttribute(Util::StringIdentifier("splitDimension"));
		if (level <= drawDepth && dimensionAttribute != nullptr) {
			const Geometry::Box & box = current->getWorldBB();
			const unsigned char dimension = dimensionAttribute->toUnsignedInt();
			const float value = current->getAttribute(Util::StringIdentifier("splitValue"))->toFloat();
			if (showSplittingPlanes) {
				// Render splitting plane.
				context.getRenderingContext().pushAndSetBlending(Rendering::BlendingParameters(Rendering::BlendingParameters::SRC_ALPHA, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA));
				context.getRenderingContext().pushAndSetCullFace(Rendering::CullFaceParameters());
				context.getRenderingContext().applyChanges();

				Geometry::Vec3f lowerLeft;
				Geometry::Vec3f lowerRight;
				Geometry::Vec3f upperRight;
				Geometry::Vec3f upperLeft;
				switch (dimension) {
					case 0:
						lowerLeft.setValue(value, box.getMinY(), box.getMinZ());
						lowerRight.setValue(value, box.getMaxY(), box.getMinZ());
						upperRight.setValue(value, box.getMaxY(), box.getMaxZ());
						upperLeft.setValue(value, box.getMinY(), box.getMaxZ());
						break;
					case 1:
						lowerLeft.setValue(box.getMinX(), value, box.getMinZ());
						lowerRight.setValue(box.getMaxX(), value, box.getMinZ());
						upperRight.setValue(box.getMaxX(), value, box.getMaxZ());
						upperLeft.setValue(box.getMinX(), value, box.getMaxZ());
						break;
					case 2:
						lowerLeft.setValue(box.getMinX(), box.getMinY(), value);
						lowerRight.setValue(box.getMaxX(), box.getMinY(), value);
						upperRight.setValue(box.getMaxX(), box.getMaxY(), value);
						upperLeft.setValue(box.getMinX(), box.getMaxY(), value);
						break;
					default:
						WARN("the roof is on fire");
				}
				const unsigned char * color = colors[dimension];
				Rendering::drawQuad(context.getRenderingContext(), lowerLeft, lowerRight, upperRight, upperLeft, Util::Color4ub(color[0], color[1], color[2], color[3]));

				context.getRenderingContext().popCullFace();
				context.getRenderingContext().popBlending();
			}
			if (showBoundingBoxes && level == drawDepth) {
				// Bounding boxes of current draw depth only.
				const unsigned char * color = colors[dimension];
				Rendering::drawAbsWireframeBox(context.getRenderingContext(), box, Util::Color4ub(color[0], color[1], color[2], color[3]));
			}
			// Add children.
			const auto children = getChildNodes(current);
			for(const auto & child : children) {
				nodes.emplace_back(child, level + 1);
				if (showLines) {
					Rendering::drawVector(context.getRenderingContext(), box.getCenter(), child->getWorldBB().getCenter(), Util::ColorLibrary::BLACK);
				}
			}
		}
	}
}

void TreeVisualization::displayOctree(FrameContext & context, Node * root) const {
	// Different colors for the different dimensions.
	const unsigned char colors[3][4] = { { 255, 0, 0, 50 }, { 0, 255, 0, 50 }, { 0, 0, 255, 50 } };
	// Store nodes with their level.
	std::deque<std::pair<Node *, unsigned char> > nodes;
	nodes.emplace_back(root, 0);

	while (!nodes.empty()) {
		Node * current = nodes.front().first;
		unsigned char level = nodes.front().second;
		nodes.pop_front();
		Util::GenericAttribute * looseFactorAttribute = current->getAttribute(Util::StringIdentifier("looseFactor"));
		if (level <= drawDepth && looseFactorAttribute != nullptr) {
			const Geometry::Box & box = current->getWorldBB();
			const float looseFactor = looseFactorAttribute->toFloat();
			std::istringstream stream(current->getAttribute(Util::StringIdentifier("looseBox"))->toString());
			Geometry::Box looseBox;
			stream >> looseBox;
			if (showSplittingPlanes && level == drawDepth) {
				// Render the inner box.
				context.getRenderingContext().pushAndSetBlending(Rendering::BlendingParameters(Rendering::BlendingParameters::SRC_ALPHA, Rendering::BlendingParameters::ONE_MINUS_SRC_ALPHA));
				context.getRenderingContext().pushAndSetCullFace(Rendering::CullFaceParameters());
				context.getRenderingContext().applyChanges();

				const unsigned char * color = colors[level % 3];
				Geometry::Box tightBox(looseBox);
				tightBox.resizeRel(1.0f / looseFactor);
				Rendering::drawAbsBox(context.getRenderingContext(), tightBox, Util::Color4ub(color[0], color[1], color[2], color[3]));

				context.getRenderingContext().popCullFace();
				context.getRenderingContext().popBlending();

			}
			if (showBoundingBoxes && level == drawDepth) {
				// Render the outer box.
				const unsigned char * color = colors[level % 3];
				Rendering::drawAbsWireframeBox(context.getRenderingContext(), looseBox, Util::Color4ub(color[0], color[1], color[2], color[3]));
			}
			// Add children.
			const auto children = getChildNodes(current);
			for(const auto & child : children) {
				nodes.emplace_back(child, level + 1);
				if (showLines) {
					Rendering::drawVector(context.getRenderingContext(), box.getCenter(), child->getWorldBB().getCenter(), Util::ColorLibrary::BLACK);
				}
			}
		}
	}
}

State::stateResult_t TreeVisualization::doEnableState(FrameContext & context, Node * node, const RenderParam & /*rp*/) {
	// Find root node of a triangle tree in scene graph.
	std::deque<Node *> candidates;
	candidates.push_back(node);
	while (!candidates.empty()) {
		Node * currentNode = candidates.front();
		candidates.pop_front();
		if (currentNode == nullptr) {
			continue;
		}
		// Try to get a MinSG node that was created from an ABTree or a kDTree.
		Util::GenericAttribute * splitDimension = currentNode->getAttribute(Util::StringIdentifier("splitDimension"));
		if (splitDimension != nullptr) {
			displayBinaryTree(context, currentNode);
			return State::STATE_OK;
		}

		// Try to get a MinSG node that was created from an Octree.
		Util::GenericAttribute * looseFactor = currentNode->getAttribute(Util::StringIdentifier("looseFactor"));
		if (looseFactor != nullptr) {
			displayOctree(context, currentNode);
			return State::STATE_OK;
		}

		const auto children = getChildNodes(currentNode);
		candidates.insert(candidates.end(), children.begin(), children.end());
	}

	// Invalid information. => Fall back to standard rendering.
	return State::STATE_SKIPPED;
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
