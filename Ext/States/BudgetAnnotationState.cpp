/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BudgetAnnotationState.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <algorithm>
#include <functional>
#ifdef MINSG_EXT_THESISJONAS
#include "../ThesisJonas/Renderer.h"
#endif

namespace MinSG {

static void setOrUpdateAttribute(Node * node, const Util::StringIdentifier & attributeId, double value) {
	const auto numberAttribute = dynamic_cast<Util::_NumberAttribute<double> *>(node->getAttribute(attributeId));
	if(numberAttribute != nullptr) {
		numberAttribute->set(value);
	} else {
		node->setAttribute(attributeId, Util::GenericAttribute::createNumber(value));
	}
}

//! For a node with \a k child nodes, every child node receives a fraction of 1 / \a k of the budget.
static void distributeEven(double value, const Util::StringIdentifier & attributeId, Node * node, FrameContext & /*context*/) {
	const auto children = getChildNodes(node);
	for(const auto & child : children) {
		setOrUpdateAttribute(child, attributeId, value / children.size());
	}
}

//! Distribute the budget based on the projected size of the child nodes.
static void distributeProjectedSize(double value, const Util::StringIdentifier & attributeId, Node * node, FrameContext & context) {
	const auto children = getChildNodes(node);

	// A pair stores a node and its projected size.
	std::vector<std::pair<Node *, float>> nodeProjSizePairs;
	nodeProjSizePairs.reserve(children.size());
	double projSizeSum = 0.0;

	const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
	for(const auto & child : children) {
		// Clip the projected rect to the screen.
		auto projRect = context.getProjectedRect(child);
		projRect.clipBy(screenRect);

		const auto projSize = projRect.getArea();
		projSizeSum += projSize;
		nodeProjSizePairs.emplace_back(child, projSize);
	}
	for(const auto & nodeProjSizePair : nodeProjSizePairs) {
		const double factor = nodeProjSizePair.second / projSizeSum;
		setOrUpdateAttribute(nodeProjSizePair.first, attributeId, factor * value);
	}
}

//! Distribute the budget based on the projected size and the primitive count of the child nodes.
static void distributeProjectedSizeAndPrimitiveCount(double value, const Util::StringIdentifier & attributeId, Node * node, FrameContext & context) {
	static const Util::StringIdentifier primitiveCountId("PrimitiveCount");
	struct PrimitiveCountAnnotationVisitor : public NodeVisitor {
		const Util::StringIdentifier & m_primitiveCountId;
		PrimitiveCountAnnotationVisitor(const Util::StringIdentifier & p_primitiveCountId) : m_primitiveCountId(p_primitiveCountId) {
		}
		virtual ~PrimitiveCountAnnotationVisitor() {
		}
		NodeVisitor::status leave(Node * _node) override {
			auto geoNode = dynamic_cast<GeometryNode *>(_node);
			uint32_t primitiveCount = 0;
			if(geoNode != nullptr) {
				const auto mesh = geoNode->getMesh();
				primitiveCount = mesh == nullptr ? 0 : mesh->getPrimitiveCount();
			} else {
				const auto children = getChildNodes(_node);
				for(const auto & child : children) {
					primitiveCount += child->getAttribute(m_primitiveCountId)->toUnsignedInt();
				}
			}
			_node->setAttribute(m_primitiveCountId, Util::GenericAttribute::createNumber(primitiveCount));
			return CONTINUE_TRAVERSAL;
		}
	};

	const auto children = getChildNodes(node);

	// A tuple stores the primitive count, the node and its projected size.
	std::vector<std::tuple<uint32_t, Node *, float>> primitiveCountNodeProjSizeTuples;
	primitiveCountNodeProjSizeTuples.reserve(children.size());
	double projSizeSum = 0.0;

	const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
	for(const auto & child : children) {
		if(!child->isAttributeSet(primitiveCountId)) {
			PrimitiveCountAnnotationVisitor visitor(primitiveCountId);
			child->traverse(visitor);
		}
		const auto primitiveCount = child->getAttribute(primitiveCountId)->toUnsignedInt();

		// Clip the projected rect to the screen.
		auto projRect = context.getProjectedRect(child);
		projRect.clipBy(screenRect);

		const auto projSize = projRect.getArea();
		projSizeSum += projSize;
		primitiveCountNodeProjSizeTuples.emplace_back(primitiveCount, child, projSize);
	}

	// Begin with the node with the lowest primitive count
	std::sort(primitiveCountNodeProjSizeTuples.begin(), primitiveCountNodeProjSizeTuples.end());

	for(const auto & primitiveCountNodeProjSizeTuple : primitiveCountNodeProjSizeTuples) {
		const auto primitiveCount = std::get<0>(primitiveCountNodeProjSizeTuple);
		const auto projSize = std::get<2>(primitiveCountNodeProjSizeTuple);
		const auto projSizeFactor = projSize / projSizeSum;
		
		const auto primitiveAssignment = std::min(static_cast<uint32_t>(projSizeFactor * value), primitiveCount);
		setOrUpdateAttribute(std::get<1>(primitiveCountNodeProjSizeTuple), attributeId, primitiveAssignment);
		value -= primitiveAssignment;
		projSizeSum -= projSize;
	}
}

#ifdef MINSG_EXT_THESISJONAS
//! Distribute the budget based on the projected size and the primitive count of the child nodes in an iterative manner for correct distribution.
static void distributeProjectedSizeAndPrimitiveCountIterative(double value, const Util::StringIdentifier & attributeId, Node * node, FrameContext & context) {
	static const Util::StringIdentifier primitiveCountId("PrimitiveCount");
	struct PrimitiveCountAnnotationVisitor : public NodeVisitor {
		const Util::StringIdentifier & m_primitiveCountId;
		PrimitiveCountAnnotationVisitor(const Util::StringIdentifier & p_primitiveCountId) : m_primitiveCountId(p_primitiveCountId) {
		}
		virtual ~PrimitiveCountAnnotationVisitor() {
		}
		NodeVisitor::status leave(Node * _node) override {
			auto geoNode = dynamic_cast<GeometryNode *>(_node);
			uint32_t primitiveCount = 0;
			if(geoNode != nullptr) {
				const auto mesh = geoNode->getMesh();
				primitiveCount = mesh == nullptr ? 0 : mesh->getPrimitiveCount();
			} else {
				const auto children = getChildNodes(_node);
				for(const auto & child : children) {
					primitiveCount += child->getAttribute(m_primitiveCountId)->toUnsignedInt();
				}
			}
			_node->setAttribute(m_primitiveCountId, Util::GenericAttribute::createNumber(primitiveCount));
			return CONTINUE_TRAVERSAL;
		}
	};

	const auto children = getChildNodes(node);

	// A pair stores the primitive count, the node and its projected size.
	std::deque<std::tuple<uint32_t, Node *, float>> primitiveCountNodeProjSizeTuples;
	double projSizeSum = 0.0;

	const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
	for(const auto & child : children) {
		if(!child->isAttributeSet(primitiveCountId)) {
			PrimitiveCountAnnotationVisitor visitor(primitiveCountId);
			child->traverse(visitor);
		}
		const auto primitiveCount = child->getAttribute(primitiveCountId)->toUnsignedInt();

		// Clip the projected rect to the screen.
		auto projRect = context.getProjectedRect(child);
		projRect.clipBy(screenRect);

		const auto projSize = projRect.getArea();
		projSizeSum += projSize;
		primitiveCountNodeProjSizeTuples.emplace_back(primitiveCount, child, projSize);
	}

	// Begin with the node with the lowest primitive count
	std::sort(primitiveCountNodeProjSizeTuples.begin(), primitiveCountNodeProjSizeTuples.end());

	/* Distribute budget until one node gets all budget it asked for.
	 * This means the previous distributions would receive more budget,
	 * thus remove that node from distribution (update value and projSizeSum)
	 * and start again.
	 */
	std::deque<std::tuple<uint32_t, Node *, float>> tmpDeque(primitiveCountNodeProjSizeTuples); // TODO: fails to copy values correctly!!!
	bool nodeRemoved;
	double tmpValue;
	double tmpProjSizeSum;
	double remainingProjSizeSum = projSizeSum;
	do{
		nodeRemoved = false;
		tmpValue = value;
		tmpProjSizeSum = remainingProjSizeSum;
		for(auto it=tmpDeque.begin(); it!=tmpDeque.end(); ++it) {
			std::tuple<uint32_t, Node *, float> element = *it;
			const auto primitiveCount = std::get<0>(element);
			const auto projSize = std::get<2>(element);
			const auto projSizeFactor = projSize / tmpProjSizeSum;

			const auto primitiveAssignment = std::min(static_cast<uint32_t>(projSizeFactor * tmpValue), primitiveCount);
			setOrUpdateAttribute(std::get<1>(element), attributeId, primitiveAssignment);

			if(primitiveAssignment == primitiveCount){
				nodeRemoved = true;
				tmpDeque.erase(it);
				value -= primitiveCount;
				remainingProjSizeSum -= projSize;
				break;
			}
			tmpValue -= primitiveAssignment;
			tmpProjSizeSum -= projSize;
		}
	}while(nodeRemoved);

	// distribute remaining part of value based on projected size only
	if(tmpValue >= 1){
		for(const auto & primitiveCountNodeProjSizeTuple : primitiveCountNodeProjSizeTuples) {
			const auto projSize = std::get<2>(primitiveCountNodeProjSizeTuple);
			const auto projSizeFactor = projSize / projSizeSum;

			const auto attribute = dynamic_cast<Util::_NumberAttribute<double> *>(std::get<1>(primitiveCountNodeProjSizeTuple)->getAttribute(attributeId));
			attribute->set(attribute->get() + projSizeFactor * tmpValue);
		}
	}
}
#endif

//! Delete the attribute from the node and its child nodes.
static void deleteAttribute(double /*value*/, const Util::StringIdentifier & attributeId, Node * node, FrameContext & /*context*/) {
	node->unsetAttribute(attributeId);

	const auto children = getChildNodes(node);
	for(const auto & child : children) {
		child->unsetAttribute(attributeId);
	}
}

BudgetAnnotationState::BudgetAnnotationState() : 
	NodeRendererState(FrameContext::DEFAULT_CHANNEL), annotationAttribute(), budget(0.0), distributionFunction(distributeEven), distributionType(DISTRIBUTE_EVEN) {
}

BudgetAnnotationState::~BudgetAnnotationState() = default;

State::stateResult_t BudgetAnnotationState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	if(annotationAttribute.empty()) {
		return STATE_SKIPPED;
	}
	// Start at the root with the initial budget
	distributionFunction(budget, annotationAttribute, node, context);
	return NodeRendererState::doEnableState(context, node, rp);
}

NodeRendererResult BudgetAnnotationState::displayNode(FrameContext & context, Node * node, const RenderParam & /*rp*/) {
	// Retrieve the value to annotate from the current node
	const auto attribute = node->getAttribute(annotationAttribute);
	if(attribute == nullptr) {
		return NodeRendererResult::PASS_ON;
	}
	distributionFunction(attribute->toDouble(), annotationAttribute, node, context);
	return NodeRendererResult::PASS_ON;
}

BudgetAnnotationState * BudgetAnnotationState::clone() const {
	return new BudgetAnnotationState(*this);
}

void BudgetAnnotationState::setDistributionType(distribution_type_t type) {
	distributionType = type;
	switch(type) {
		case DISTRIBUTE_EVEN:
			distributionFunction = distributeEven;
			return;
		case DISTRIBUTE_PROJECTED_SIZE:
			distributionFunction = distributeProjectedSize;
			return;
		case DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT:
			distributionFunction = distributeProjectedSizeAndPrimitiveCount;
			return;
#ifdef MINSG_EXT_THESISJONAS
		case DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE:
			distributionFunction = distributeProjectedSizeAndPrimitiveCountIterative;
			return;
#endif
		case DISTRIBUTE_DELETE:
		default:
			distributionFunction = deleteAttribute;
			break;
	}
}

std::string BudgetAnnotationState::distributionTypeToString(distribution_type_t type) {
	switch(type) {
		case DISTRIBUTE_EVEN:
			return "EVEN";
		case DISTRIBUTE_PROJECTED_SIZE:
			return "PROJECTED_SIZE";
		case DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT:
			return "PROJECTED_SIZE_AND_PRIMITIVE_COUNT";
#ifdef MINSG_EXT_THESISJONAS
		case DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE:
			return "DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE";
#endif
		case DISTRIBUTE_DELETE:
			return "DELETE";
		default:
			break;
	}
	throw std::invalid_argument("Invalid distribution type method.");
}

BudgetAnnotationState::distribution_type_t BudgetAnnotationState::distributionTypeFromString(const std::string & str) {
	static const Util::StringIdentifier idEVEN("EVEN");
	static const Util::StringIdentifier idPROJECTED_SIZE("PROJECTED_SIZE");
	static const Util::StringIdentifier idPROJECTED_SIZE_AND_PRIMITIVE_COUNT("PROJECTED_SIZE_AND_PRIMITIVE_COUNT");
#ifdef MINSG_EXT_THESISJONAS
	static const Util::StringIdentifier idPROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE("PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE");
#endif
	static const Util::StringIdentifier idDELETE("DELETE");

	const Util::StringIdentifier idStr(str);
	if(idStr == idEVEN) {
		return DISTRIBUTE_EVEN;
	} else if(idStr == idPROJECTED_SIZE) {
		return DISTRIBUTE_PROJECTED_SIZE;
	} else if(idStr == idPROJECTED_SIZE_AND_PRIMITIVE_COUNT) {
		return DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT;
#ifdef MINSG_EXT_THESISJONAS
	} else if(idStr == idPROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE) {
		return DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE;
#endif
	} else if(idStr == idDELETE) {
		return DISTRIBUTE_DELETE;
	}
	throw std::invalid_argument("Invalid distribution type method.");
}

}
