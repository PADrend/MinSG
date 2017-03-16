/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2017 Sascha Brandt <myeti@mail.uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#include "SurfelRenderer_Budget.h"
#include "SurfelAnalysis.h"

#include "../../Core/Statistics.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/CameraNode.h"
#include "../../Core/Transformations.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../../Core/NodeAttributeModifier.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Mesh/Mesh.h>

#include <Geometry/Vec3.h>
#include <Geometry/Frustum.h>

#include <Util/StringIdentifier.h>
#include <Util/GenericAttribute.h>

#include <queue>
#include <cmath>

namespace MinSG{
namespace BlueSurfels {

static const Rendering::Uniform::UniformName uniform_renderSurfels("renderSurfels");
static const Rendering::Uniform::UniformName uniform_surfelRadius("surfelRadius");
static const Util::StringIdentifier SURFEL_ATTRIBUTE("surfels");
static const Util::StringIdentifier SURFEL_MEDIAN_ATTRIBUTE("surfelMedianDist");
static const Util::StringIdentifier SURFEL_BUDGET_ATTRIBUTE("SurfelBudget");
static const Util::StringIdentifier PASS_CHILDREN_ATTRIBUTE(NodeAttributeModifier::create("PassChildren", NodeAttributeModifier::PRIVATE_ATTRIBUTE));

static const Util::StringIdentifier MAX_DEPTH_ATTRIBUTE("MaxDepth");
static const Util::StringIdentifier SURFEL_COUNT_ATTRIBUTE("SurfelCount");
static const Util::StringIdentifier PRIMITIVE_COUNT_ATTRIBUTE("PrimitiveCount");

#define SURFEL_MEDIAN_COUNT 1000

static inline double sech2(double x) {
    double sh = 1.0 / std::cosh(x);   // sech(x) == 1/cosh(x)
    return sh*sh;                     // sech^2(x)
}

template<typename T>
static void setOrUpdateAttribute(Node * node, const Util::StringIdentifier & attributeId, T value) {
	const auto numberAttribute = dynamic_cast<Util::_NumberAttribute<T> *>(node->getAttribute(attributeId));
	if(numberAttribute != nullptr) {
		numberAttribute->set(value);
	} else {
		node->setAttribute(attributeId, Util::GenericAttribute::createNumber(value));
	}
}

static void setOrUpdateAttribute(Node * node, const Util::StringIdentifier & attributeId, bool value) {
	const auto attr = dynamic_cast<Util::BoolAttribute *>(node->getAttribute(attributeId));
	if(attr != nullptr) {
		attr->set(value);
	} else {
		node->setAttribute(attributeId, Util::GenericAttribute::createBool(value));
	}
}

static uint32_t getUIntAttr(Node* node, const Util::StringIdentifier& id) {
	auto attr = node->findAttribute(id);	
	return attr ? attr->toUnsignedInt() : 0;
}

static double getDoubleAttr(Node* node, const Util::StringIdentifier& id) {
	auto attr = node->findAttribute(id);	
	return attr ? attr->toDouble() : -1;
}

static bool getBoolAttr(Node* node, const Util::StringIdentifier& id) {
	auto attr = node->findAttribute(id);	
	return attr ? attr->toBool() : false;
}

static Rendering::Mesh* getSurfelMesh(Node * node) {
	auto surfelAttribute = dynamic_cast<Util::ReferenceAttribute<Rendering::Mesh>*>(node->findAttribute( SURFEL_ATTRIBUTE ));
	return surfelAttribute ? surfelAttribute->get() : nullptr;
}

static Rendering::Mesh* getGeometryMesh(Node * node) {	
	auto geoNode = dynamic_cast<GeometryNode*>(node);
	return geoNode ? geoNode->getMesh() : nullptr;
}

static float getProjectedSize(FrameContext & context, Node* node) {
	const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
	// Clip the projected rect to the screen.
	auto projRect = context.getProjectedRect(node);
	projRect.clipBy(screenRect);
	return projRect.getArea();
}

/*
static uint32_t getMaxDepth(Node* node) {	
	struct DepthVisitor : public NodeVisitor {
		uint32_t depth;
		uint32_t maxDepth;
		DepthVisitor() : depth(0), maxDepth(0) {	}
		virtual ~DepthVisitor() { }
		NodeVisitor::status enter(Node * _node) override {
			++depth;
			maxDepth = std::max(depth, maxDepth);
			return CONTINUE_TRAVERSAL;
		}
		NodeVisitor::status leave(Node * _node) override {
			--depth;
			return CONTINUE_TRAVERSAL;
		}
	};
	
	if(!node->isAttributeSet(MAX_DEPTH_ATTRIBUTE)) {
		DepthVisitor visitor;
		node->traverse(visitor);
		setOrUpdateAttribute(node, MAX_DEPTH_ATTRIBUTE, visitor.maxDepth);
	}
	return getUIntAttr(node, MAX_DEPTH_ATTRIBUTE);
}
*/

//! Distribute the budget based on the projected size of the child nodes.
static void distributeBudget(double value, Node * node, FrameContext & context) {
	
	struct PrimitiveSurfelCountAnnotationVisitor : public NodeVisitor {
		PrimitiveSurfelCountAnnotationVisitor() {	}
		virtual ~PrimitiveSurfelCountAnnotationVisitor() { }
		NodeVisitor::status leave(Node * _node) override {
			auto mesh = getGeometryMesh(_node);
			auto surfels = getSurfelMesh(_node);
			uint32_t primitiveCount = 0;
			uint32_t surfelCount = 0;
			if(mesh) 
				primitiveCount += mesh->getPrimitiveCount();
			if(surfels)
				surfelCount += surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount();			
			const auto children = getChildNodes(_node);
			for(const auto & child : children) {
				primitiveCount += child->getAttribute(PRIMITIVE_COUNT_ATTRIBUTE)->toUnsignedInt();
				surfelCount += child->getAttribute(SURFEL_COUNT_ATTRIBUTE)->toUnsignedInt();
			}
			setOrUpdateAttribute(_node, PRIMITIVE_COUNT_ATTRIBUTE, primitiveCount);
			setOrUpdateAttribute(_node, SURFEL_COUNT_ATTRIBUTE, surfelCount);
			return CONTINUE_TRAVERSAL;
		}
	};
	
	const auto children = getChildNodes(node);

	// A tuple stores the benefit/cost ratio and the node.
	std::vector<std::tuple<float, Node *, uint32_t>> benefitCostTuples;
	benefitCostTuples.reserve(children.size());
	double benefitCostSum = 0.0;
	
	for(const auto & child : children) {		
		// Compute primitive count and surfel count in subtree (only once)
		if(!child->isAttributeSet(PRIMITIVE_COUNT_ATTRIBUTE) || !child->isAttributeSet(SURFEL_COUNT_ATTRIBUTE)) {
			PrimitiveSurfelCountAnnotationVisitor visitor;
			node->traverse(visitor);
		}
		const auto primitiveCount = child->getAttribute(PRIMITIVE_COUNT_ATTRIBUTE)->toUnsignedInt();
		const auto projSize = getProjectedSize(context, child);
		const auto benefitCost = projSize / static_cast<double>(primitiveCount);
		benefitCostSum += benefitCost;
		benefitCostTuples.emplace_back(benefitCost, child, primitiveCount);
	}

	for(const auto & benefitCostTuple : benefitCostTuples) {
		const auto benefitCost = std::get<0>(benefitCostTuple);
		auto child = std::get<1>(benefitCostTuple);
		auto primitiveCount = std::get<2>(benefitCostTuple);
		const auto benefitCostFactor = benefitCost / benefitCostSum;		
		const auto assignment = std::min(benefitCostFactor * value, static_cast<double>(primitiveCount));
		setOrUpdateAttribute(child, SURFEL_BUDGET_ATTRIBUTE, assignment);
		value -= assignment;
		benefitCostSum -= benefitCost;
	}
}

static void passChildren(Node * node, bool value) {
	const auto children = getChildNodes(node);
	for(const auto & child : children) {
		setOrUpdateAttribute(child, PASS_CHILDREN_ATTRIBUTE, value);
	}
}

SurfelRendererBudget::SurfelRendererBudget() : NodeRendererState(FrameContext::DEFAULT_CHANNEL),
		maxSurfelSize(32.0), surfelCostFactor(1.0), geoCostFactor(1.0), benefitGrowRate(2.0), 
		debugHideSurfels(false), debugCameraEnabled(false), deferredSurfels(true), 
    budget(1e+6), budgetRemainder(0), usedBudget(0), maxTime(100) {}
SurfelRendererBudget::~SurfelRendererBudget() {}


float SurfelRendererBudget::getMedianDist(Node * node, Rendering::Mesh* mesh) {
	auto surfelMedianAttr = node->findAttribute(SURFEL_MEDIAN_ATTRIBUTE);
	float surfelMedianDist = 0;
	if(!surfelMedianAttr) {
		std::cout << "Computing surfel median distance...";
		surfelMedianDist = getMedianOfNthClosestNeighbours(*mesh, SURFEL_MEDIAN_COUNT, 2);
		node->setAttribute(SURFEL_MEDIAN_ATTRIBUTE, Util::GenericAttribute::createNumber(surfelMedianDist));
		std::cout << "done!" << std::endl;
	} else {
		surfelMedianDist = surfelMedianAttr->toFloat();
	}
	return surfelMedianDist;
};

NodeRendererResult SurfelRendererBudget::displayNode(FrameContext & context, Node * node, const RenderParam & /*rp*/){
	if(!node->isActive())
		return NodeRendererResult::NODE_HANDLED;	
	
	
	double nodeBudget = getDoubleAttr(node, SURFEL_BUDGET_ATTRIBUTE);
		
	if(nodeBudget < 0) 
		return NodeRendererResult::PASS_ON;
	
	if(getBoolAttr(node, PASS_CHILDREN_ATTRIBUTE)) {
		passChildren(node, true);
		return NodeRendererResult::PASS_ON;
	} 
	
	uint32_t primitiveCount = getUIntAttr(node, PRIMITIVE_COUNT_ATTRIBUTE);
	
	const auto projSize = getProjectedSize(context, node);
	const auto benefitCost = std::min(projSize / static_cast<double>(primitiveCount), 1.0);
	
	if(primitiveCount > nodeBudget && primitiveCount <= nodeBudget + budgetRemainder*benefitCost) {
		nodeBudget += budgetRemainder*benefitCost;
		budgetRemainder -= budgetRemainder*benefitCost;	
	}	
	
	if(primitiveCount <= nodeBudget) {
		passChildren(node, true);
		budgetRemainder += nodeBudget-primitiveCount;
		usedBudget += primitiveCount;
		return NodeRendererResult::PASS_ON;
	} else {
		passChildren(node, false);
	}
	distributeBudget(nodeBudget, node, context);
	
	/*
	auto mesh = getGeometryMesh(node);
	if(mesh && primitiveCount > nodeBudget) {
		budgetRemainder += nodeBudget;
		return NodeRendererResult::NODE_HANDLED;
	}*/
			
	auto surfels = getSurfelMesh(node);
	
	// get max. surfel count
	uint32_t surfelCount = surfels ? (surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount()) : 0;	
	uint32_t subtreeSurfelCount = getUIntAttr(node, SURFEL_COUNT_ATTRIBUTE) - surfelCount;
	
	// subtree cannot be displayed with current budget -> surfels need to be rendered
	bool surfelsRequired = subtreeSurfelCount == 0 && primitiveCount > nodeBudget;
	
	if(!surfels && surfelsRequired) {
		budgetRemainder += nodeBudget;
		return NodeRendererResult::NODE_HANDLED;
	}
	
	if(!surfels)
		return NodeRendererResult::PASS_ON;


	// get median distance between surfels at a fixed prefix length 
	float medianDist = getMedianDist(node, surfels);		
	// calculate the projected distance between two adjacent pixels in screen space
	float meterPerPixel = getMeterPerPixel(context, node);
	
	// compute saturation factor (number of surfels when they cover the object with surfel size 1)
	float minRadius = meterPerPixel / 2.0f; 
	double saturation = SURFEL_MEDIAN_COUNT * medianDist * medianDist / (minRadius * minRadius);
	saturation /= surfelCount;
	
	//double surfelBenefit = getSurfelBenefit(float x, projSize, saturation);	
	uint32_t maxPrefix = std::min(static_cast<uint32_t>(saturation*surfelCount), surfelCount);	
	
	// compute min. prefix, i.e. number of surfels that cover the object with max. surfel size
	float maxRadius = maxSurfelSize * minRadius;
	uint32_t minPrefix = SURFEL_MEDIAN_COUNT * medianDist * medianDist / (maxRadius * maxRadius);
	
	if(saturation <= 1.0 && nodeBudget >= maxPrefix) {
		// we can render surfels with size 1 within budget
		surfelAssignments.push_back({node, maxPrefix, minRadius, minPrefix, maxPrefix, projSize, meterPerPixel, medianDist});
		return NodeRendererResult::NODE_HANDLED;
	}
	
	if(!surfelsRequired && (minPrefix > maxPrefix || minPrefix > nodeBudget)) {
		// cannot draw surfels without exceeding max. surfel radius 
		return NodeRendererResult::PASS_ON;
	}
	minPrefix = std::min(minPrefix, maxPrefix);
	
	uint32_t budgetCount = std::min(maxPrefix, static_cast<uint32_t>(nodeBudget));
	float budgetRadius = medianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(budgetCount));
	
	if(surfelsRequired || maxPrefix > nodeBudget) {
		// we fill the budget with this node -> it is not worthwhile to traverse any further
		budgetRemainder += nodeBudget - budgetCount;
		surfelAssignments.push_back({node, budgetCount, budgetRadius, minPrefix, maxPrefix, projSize, meterPerPixel, medianDist});
		return NodeRendererResult::NODE_HANDLED;
	}
		
	return NodeRendererResult::PASS_ON;
}


// surfel benefit: projSize * tanh(a*x / 2*b)
// hyperbolic function tanh(ax/2b) <-> logistic function (2 / (1+e^-(ax/b)) - 1)
double SurfelRendererBudget::getSurfelBenefit(float x, float ps, float sat) const {
	if(sat <= 0.0)
		return 0.0;
	return ps * std::tanh(benefitGrowRate * x / (2.0f*sat));
}

// derivative of surfel benefit function: p * a/2b * sech^2(ax/2b)
double SurfelRendererBudget::getSurfelBenefitDerivative(float x, float ps, float sat) const {
	if(sat <= 0.0)
		return 0.0;
	float a2b = benefitGrowRate/(2.0f*sat); 
	return ps * a2b * sech2(a2b*x); 
}

SurfelRendererBudget::stateResult_t SurfelRendererBudget::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
	usedBudget = 0;
	budgetRemainder = 0;
	surfelAssignments.clear();
	setOrUpdateAttribute(node, SURFEL_BUDGET_ATTRIBUTE, budget);
	setOrUpdateAttribute(node, PASS_CHILDREN_ATTRIBUTE, false);
	distributeBudget(budget, node, context);
	passChildren(node, false);
	return NodeRendererState::doEnableState(context, node, rp);
}

void SurfelRendererBudget::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);
	
	if(!debugHideSurfels) 
		drawSurfels(context);
}

void SurfelRendererBudget::drawSurfels(FrameContext & context) const {
	auto& rc = context.getRenderingContext();	
	rc.setGlobalUniform({uniform_renderSurfels, true});	
	for(auto& s : surfelAssignments) {
		float nodeScale = s.node->getWorldTransformationSRT().getScale();
		float surfelSize = std::min(2.0f * s.radius / s.mpp, maxSurfelSize);
		auto surfels = getSurfelMesh(s.node);
		rc.setGlobalUniform({uniform_surfelRadius, s.radius*nodeScale});
		rc.pushAndSetPointParameters( Rendering::PointParameters(surfelSize));
		rc.pushAndSetMatrix_modelToCamera( rc.getMatrix_worldToCamera() );
		rc.multMatrix_modelToCamera(s.node->getWorldTransformationMatrix());
		context.displayMesh(surfels,	0, s.prefix );
		rc.popMatrix_modelToCamera();
		rc.popPointParameters();
	}
	rc.setGlobalUniform({uniform_renderSurfels, false});
}

}
}
#endif // MINSG_EXT_BLUE_SURFELS
