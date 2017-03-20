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
#include <Rendering/Helper.h>

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

static const Util::StringIdentifier BUDGET_ASSIGNMENT_ATTRIBUTE(NodeAttributeModifier::create("BudgetAssignment", NodeAttributeModifier::PRIVATE_ATTRIBUTE));

static const Util::StringIdentifier SURFEL_COUNT_ATTRIBUTE(NodeAttributeModifier::create("SurfelCount", NodeAttributeModifier::PRIVATE_ATTRIBUTE));
static const Util::StringIdentifier PRIMITIVE_COUNT_ATTRIBUTE(NodeAttributeModifier::create("PrimitiveCount", NodeAttributeModifier::PRIVATE_ATTRIBUTE));

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

static uint32_t getUIntAttr(Node* node, const Util::StringIdentifier& id) {
	auto attr = node->findAttribute(id);	
	return attr ? attr->toUnsignedInt() : 0;
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

static void updatePrimitiveCount(Node * node, FrameContext & context) {	
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

	// Compute primitive count and surfel count in subtree (only once)
	if(!node->isAttributeSet(PRIMITIVE_COUNT_ATTRIBUTE) || !node->isAttributeSet(SURFEL_COUNT_ATTRIBUTE)) {
		PrimitiveSurfelCountAnnotationVisitor visitor;
		node->traverse(visitor);
	}
}

SurfelRendererBudget::SurfelRendererBudget() : NodeRendererState(FrameContext::DEFAULT_CHANNEL) {}
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
  
  updatePrimitiveCount(node, context);
	
	BudgetAssignment& assignment = getAssignment(node);
  
	auto surfels = getSurfelMesh(node);
	
	// get max. surfel count
	uint32_t surfelCount = surfels ? (surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount()) : 0;	
	uint32_t subtreeSurfelCount = getUIntAttr(node, SURFEL_COUNT_ATTRIBUTE) - surfelCount;
	uint32_t primitiveCount = getUIntAttr(node, PRIMITIVE_COUNT_ATTRIBUTE);
  
  auto mesh = getGeometryMesh(node);
  if(mesh && (assignment.expanded || surfelCount == 0)) {
    usedBudget += mesh->getPrimitiveCount() * geoCostFactor;
    return NodeRendererResult::PASS_ON;
  }
  
  if(subtreeSurfelCount == 0 && surfelCount == 0) {
    return NodeRendererResult::PASS_ON;
  }
  
  float minSurfelSize = 0;
  float projSize = getProjectedSize(context, node); 
    
  if(surfels) {
  	// get median distance between surfels at a fixed prefix length 
  	float medianDist = getMedianDist(node, surfels);		
  	// calculate the projected distance between two adjacent pixels in screen space
  	float meterPerPixel = getMeterPerPixel(context, node);
  	
  	// compute saturation factor (number of surfels when they cover the object with surfel size 1)
  	float minRadius = meterPerPixel / 2.0f; 
  	float saturation = (SURFEL_MEDIAN_COUNT * medianDist * medianDist / (minRadius * minRadius)) / static_cast<float>(surfelCount);
  	
    // compute max. prefix
  	assignment.maxPrefix = std::min(static_cast<uint32_t>(saturation*surfelCount), surfelCount);
  	
  	// compute min. prefix, i.e. number of surfels that cover the object with max. surfel size
  	float maxRadius = maxSurfelSize * minRadius;
  	assignment.minPrefix = SURFEL_MEDIAN_COUNT * medianDist * medianDist / (maxRadius * maxRadius);
    
    // update min. surfel size
  	minSurfelSize = 2.0f * medianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(assignment.maxPrefix)) / meterPerPixel;
    
    float a = static_cast<float>(assignment.prefix) / static_cast<float>(assignment.maxPrefix);
    assignment.gradient = getSurfelBenefitDerivative(a, projSize, saturation);
		assignment.gradient = a < 1 ? std::max(0.0f, assignment.gradient-0.0001f/(1.0f - a)) : 0.0f; 
    gradientSum += assignment.gradient;
    
    // compute current size
    if(assignment.prefix > 0)
      assignment.size = std::min(2.0f * medianDist * std::sqrt(SURFEL_MEDIAN_COUNT / static_cast<float>(assignment.prefix)) / meterPerPixel, maxSurfelSize);
  }
  
  // compute expansion benefit
  assignment.expansionBenefit = 0;
  auto children = getChildNodes(node);
  if(children.size() == 0) {
    assignment.expansionBenefit = projSize;
  } else {
    for(auto child : children) {
      float ps = getProjectedSize(context, child);        
      assignment.expansionBenefit += ps;
    }
  }
  
  // compute expansion cost (only once)
  if(!assignment.expanded) { 
    assignment.expansionCost = 0;
    if(subtreeSurfelCount == 0) {
      assignment.expansionCost = primitiveCount * geoCostFactor;
    } else {
      // compute min. cost of subtree
      for(auto child : children) {
        auto s = getSurfelMesh(child);
        if(s) {
        	uint32_t sc = surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount();	
          float md = getMedianDist(child, s);		
        	float mpp = getMeterPerPixel(context, child);
          float msd = minSurfelSize * mpp / 2.0f;
          uint32_t minPrefix = std::min<uint32_t>(SURFEL_MEDIAN_COUNT * md * md / (msd * msd), sc);
          assignment.expansionCost += minPrefix * surfelCostFactor; // TODO: incororate surfel size
        } else {
          // TODO: check subtree surfel count
          assignment.expansionCost += getUIntAttr(child, PRIMITIVE_COUNT_ATTRIBUTE) * geoCostFactor;
        }
      }
    }
  }
  
  if(!assignment.expanded || (assignment.prefix > 0)) {
    assignments.insert(&assignment);
    usedBudget += assignment.prefix;
  }
  
  if(!deferredSurfels && !debugHideSurfels && assignment.prefix > 0) {
    assert(assignment.prefix <= surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount());
  	/*auto& rc = context.getRenderingContext();	
    rc.setGlobalUniform({uniform_renderSurfels, true});
    rc.pushAndSetPointParameters( Rendering::PointParameters(assignment.size));
    rc.pushAndSetMatrix_modelToCamera( rc.getMatrix_worldToCamera() );
    rc.multMatrix_modelToCamera(node->getWorldTransformationMatrix());
    context.displayMesh(surfels,	0, assignment.prefix );
    rc.popMatrix_modelToCamera();
    rc.popPointParameters();
    rc.setGlobalUniform({uniform_renderSurfels, false});*/
  }
    
  return assignment.expanded ? NodeRendererResult::PASS_ON : NodeRendererResult::NODE_HANDLED;
}

void SurfelRendererBudget::assignmentStep() {  
  double remainingBudget = budget - usedBudget;
  float increment = std::min<double>(maxIncrement, remainingBudget);
  float slope_d = 0; // max. discrete slope
  double slope_c = 0; // max. continuous slope
  BudgetAssignment* maxExpand = nullptr;
  double benefitSum = 0;
  double costSum = 0;
  
  std::cout << "Budget " << remainingBudget << " Increment " << increment << std::endl;
  for(auto a : assignments) {
    if(!a->expanded && a->expansionCost <= remainingBudget && a->prefix >= a->maxPrefix) {
      float s = a->expansionBenefit / a->expansionCost; 
      if(s > slope_d) {
        slope_d = s;
        maxExpand = a;
      }
    }
    std::cout << "(" << a->expansionCost << " cost)";
    benefitSum += a->gradient;
    costSum += a->maxPrefix * surfelCostFactor; // TODO: incorporate surfel size
    
    /*
    if(a->prefix < a->maxPrefix && a->expanded) {
      a->expanded = false;
      std::cout << "(col " << a->node << ")";
    }
    
    if(a->prefix > a->maxPrefix) {
      uint32_t dec = a->prefix - a->maxPrefix;
      std::cout << "(" << a->prefix << "-" << dec << ")";
      a->prefix -= dec;
      increment += dec;
    }
    
    if(a->expanded && a->prefix > 0) {
      uint32_t dec = a->prefix * 0.1;
      std::cout << "(" << a->prefix << "-" << dec << ")";
      a->prefix -= dec;
      increment += dec;
    }*/
  }
  
  slope_c = benefitSum / costSum;
  if(slope_d > slope_c && maxExpand) {
    // expand node
    maxExpand->expanded = true;
    for(auto n : getChildNodes(maxExpand->node)) {
      BudgetAssignment& a = getAssignment(n);
      a.prefix = 0;
      a.expanded = false;
    }
    std::cout << "(exp " << maxExpand->node << ")";
  } else {
    for(auto a : assignments) {
      if(gradientSum <= 0)
        break;
      float maxInc = a->maxPrefix - a->prefix;
      float factor = a->gradient / gradientSum;
      uint32_t inc = std::min(maxInc, factor * increment);
      std::cout << "(" << a->prefix << "+" << inc << " " << factor << ")";
      a->prefix += inc;
      increment -= inc;
      //gradientSum -= a->gradient;
    }
  }
  std::cout << std::endl;
  std::cout << "Slopes " << slope_d << "/" << slope_c << std::endl << std::endl;
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
  if(doClearAssignment) {
    forEachNodeTopDown(node, [&](Node * n) {
      BudgetAssignment& a = getAssignment(n);
      a.prefix = 0;
      a.expanded = false;
    });
    doClearAssignment = false;
  }
  
	assignments.clear();
  gradientSum = 0;
  usedBudget = 0;
	return NodeRendererState::doEnableState(context, node, rp);
}

void SurfelRendererBudget::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);
  
  if(!debugAssignment)
    assignmentStep();
	
	if(deferredSurfels && !debugHideSurfels) 
		drawSurfels(context);
}

void SurfelRendererBudget::drawSurfels(FrameContext & context, float minSize, float maxSize) const {
	auto& rc = context.getRenderingContext();	
	rc.setGlobalUniform({uniform_renderSurfels, true});	
	for(auto s : assignments) {
    auto surfels = getSurfelMesh(s->node);
    if(!surfels || s->prefix == 0 || s->size < minSize || s->size >= maxSize)
      continue;
		rc.pushAndSetPointParameters( Rendering::PointParameters(std::max(1.0f, s->size)));
		rc.pushAndSetMatrix_modelToCamera( rc.getMatrix_worldToCamera() );
		rc.multMatrix_modelToCamera(s->node->getWorldTransformationMatrix());
		context.displayMesh(surfels, 0, s->prefix );
		rc.popMatrix_modelToCamera();
		rc.popPointParameters();
	}
	rc.setGlobalUniform({uniform_renderSurfels, false});
}

SurfelRendererBudget::BudgetAssignment& SurfelRendererBudget::getAssignment(Node* node) {
	auto attr = dynamic_cast<Util::WrapperAttribute<BudgetAssignment> *>(node->getAttribute(BUDGET_ASSIGNMENT_ATTRIBUTE));
  if(!attr) {
    BudgetAssignment assignment;
    assignment.node = node;
    attr = new Util::WrapperAttribute<BudgetAssignment>(assignment);
    node->setAttribute(BUDGET_ASSIGNMENT_ATTRIBUTE, attr);
  }
  return attr->ref();
}

}
}
#endif // MINSG_EXT_BLUE_SURFELS
