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
static const Util::StringIdentifier IN_FRUSTUM_ATTRIBUTE(NodeAttributeModifier::create("InFrustum", NodeAttributeModifier::PRIVATE_ATTRIBUTE));

#define SURFEL_MEDIAN_COUNT 1000

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

static float getBenefit(FrameContext & context, Node* node) {
	const Geometry::Rect_f screenRect(context.getRenderingContext().getWindowClientArea());
	auto projRect = context.getProjectedRect(node);
	Geometry::Rect_f clippedRect(projRect);
  clippedRect.clipBy(screenRect);
  
	return clippedRect.getArea() > 0 ? std::sqrt(projRect.getArea() / screenRect.getArea()) : 0;
  /*float dist = node->getWorldBB().getDistanceSquared(context.getCamera()->getWorldOrigin());
  float far = context.getCamera()->getFarPlane();
  float near = context.getCamera()->getNearPlane();
  return std::min(1.0f, std::max(0.0f, (far*far-dist)/(far*far-near*near)));*/
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
	if(!node->isActive() || !isActive())
		return NodeRendererResult::NODE_HANDLED;	
  
  updatePrimitiveCount(node, context);
	
	BudgetAssignment& assignment = getAssignment(node);
  
	auto surfels = getSurfelMesh(node);
  auto mesh = getGeometryMesh(node);
	
	// get max. surfel count
	assignment.surfelCount = surfels ? (surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount()) : 0;	
	uint32_t subtreeSurfelCount = getUIntAttr(node, SURFEL_COUNT_ATTRIBUTE) - assignment.surfelCount;
	uint32_t primitiveCount = getUIntAttr(node, PRIMITIVE_COUNT_ATTRIBUTE);
  
  if(!mesh && subtreeSurfelCount+assignment.surfelCount == 0) {
    // no surfels in subtree & no surfels on node -> pass to leaf nodes
    return NodeRendererResult::PASS_ON;
  }
  
  if(debugAssignment && !stepAssignment) {
    return assignment.expanded ? NodeRendererResult::PASS_ON : NodeRendererResult::NODE_HANDLED;  
  }
  
  // compute benefit
  float benefit = getBenefit(context, node); 
  auto children = getChildNodes(node);
  
  // Update surfel parameters
  if(surfels) {
  	// get median distance between surfels at a fixed prefix length 
  	assignment.medianDist = getMedianDist(node, surfels);		
  	// calculate the projected distance between two adjacent pixels in screen space
  	assignment.mpp = getMeterPerPixel(context, node);
      	
    // compute max. prefix, i.e. number of surfels that cover the object with size 1
  	float minRadius = sizeToRadius(1, assignment.mpp); 
  	assignment.maxPrefix = getPrefixForRadius(minRadius, assignment.medianDist, SURFEL_MEDIAN_COUNT, assignment.surfelCount);
    assignment.minSize = radiusToSize(getRadiusForPrefix(assignment.maxPrefix, assignment.medianDist, SURFEL_MEDIAN_COUNT), assignment.mpp);
  	
  	// compute min. prefix, i.e. number of surfels that cover the object with max. surfel size
  	float maxRadius = sizeToRadius(maxSurfelSize, assignment.mpp);
  	assignment.minPrefix = getPrefixForRadius(maxRadius, assignment.medianDist, SURFEL_MEDIAN_COUNT, assignment.surfelCount);
    
    // decrement overhead (surfel count over max. prefix length)
    if(assignment.prefix > assignment.maxPrefix) {
      uint32_t dec = assignment.prefix - assignment.maxPrefix;
      if(debugAssignment) std::cout << "decrement " << assignment.node << " " << assignment.prefix << "->" << (assignment.prefix-dec)  << std::endl;
      assignment.prefix -= dec;
    }
    
    /*if(assignment.minPrefix >= assignment.maxPrefix) {
      assignment.prefix = 0;
    }*/
    
    if(assignment.expanded && assignment.prefix > 0) {
      // expanded & prefix > 0 -> gradually decrease prefix without changing size
      float factor = 0.9f;//std::max(0.0f, std::min(1.0f, (childCost - assignment.expansionCost)/assignment.expansionCost));
      if(debugAssignment) std::cout << "shrink " << assignment.node << " " << assignment.prefix << "->" << (factor * assignment.maxPrefix)  << std::endl;
      assignment.prefix = assignment.prefix < assignment.minPrefix ? 0 : factor * assignment.prefix;
      assignment.surfelBenefit = 0;
      
    } else if(!assignment.expanded) {  
      
      //float a = static_cast<float>(assignment.prefix) / static_cast<float>(assignment.maxPrefix);
      //assignment.surfelBenefit = getSurfelBenefitDerivative(a, benefit, saturation);
  		//assignment.surfelBenefit = a < 1 ? std::max(0.0, assignment.surfelBenefit) : 0.0; 
      assignment.surfelBenefit = benefit;      
            
      if(assignment.prefix > 0 && assignment.prefix < assignment.minPrefix) {
        if(debugAssignment) std::cout << "needs expansion " << node << " "<< assignment.prefix << "<" << assignment.minPrefix << std::endl;
        assignment.prefix = assignment.minPrefix;
      } 
      
      
      if(assignment.minPrefix >= assignment.maxPrefix) {
        assignment.prefix = assignment.minPrefix;
        //assignment.expanded = true;
        assignment.surfelBenefit = 1;
      }
      
      surfelBenefitSum += assignment.surfelBenefit;
    }
    
  }
  
  // compute expansion benefit (proj. size of leaf node or sum of proj size of children)
  assignment.expansionBenefit = 0;
  if(children.size() == 0) {
    assignment.expansionBenefit = benefit;
  } else {
    for(auto child : children) {
      float ps = getBenefit(context, child);        
      assignment.expansionBenefit += ps;
    }
  }
  
  // compute expansion cost 
  if(!assignment.expanded) { 
    assignment.expansionCost = 0;
    if(subtreeSurfelCount == 0) {
      assignment.expansionCost = primitiveCount * geoCostFactor;
    } else {
      // compute min. cost of subtree
      for(auto child : children) {
        auto s = getSurfelMesh(child);
        if(s) {
          BudgetAssignment& a = getAssignment(child);   
        	a.surfelCount = s->isUsingIndexData() ? s->getIndexCount() : s->getVertexCount();	
          a.medianDist = getMedianDist(child, s);		
        	a.mpp = getMeterPerPixel(context, child);
          uint32_t minPrefix = getPrefixForRadius(sizeToRadius(assignment.minSize, a.mpp), a.medianDist, SURFEL_MEDIAN_COUNT, a.surfelCount);
          double cost = static_cast<double>(minPrefix) * surfelCostFactor; // TODO: incororate surfel size
          assignment.expansionCost += cost; 
        } else {
          // TODO: check subtree surfel count
          assignment.expansionCost += getUIntAttr(child, PRIMITIVE_COUNT_ATTRIBUTE) * geoCostFactor;
        }
      }
    }
  }
  
  
  if(assignment.frame < frameCount-1 || (assignment.expanded && 2*assignment.expansionBenefit < assignment.expansionThreshold)) {
    // benefit/cost-ratio below threshold -> collapse node
    assignment.expanded = false;
    uint32_t newPrefix = std::max(std::min<uint32_t>(assignment.expansionCost/surfelCostFactor, assignment.maxPrefix), assignment.minPrefix);
    if(debugAssignment) std::cout << "collapse " << node << " " << assignment.prefix <<  "->" << newPrefix << std::endl;
    assignment.prefix = newPrefix;
  }
  assignment.frame = frameCount;
  
  if(assignment.prefix > 0) {
    // compute current size
    assignment.size = std::min(radiusToSize(getRadiusForPrefix(assignment.prefix, assignment.medianDist, SURFEL_MEDIAN_COUNT), assignment.mpp), maxSurfelSize);
    if(debugAssignment) std::cout << "udpate " << node << " p " << assignment.prefix << " s " << assignment.size << " min " << assignment.minPrefix << " max " << assignment.maxPrefix << std::endl;
  }
  
  if(!assignment.expanded || assignment.prefix > 0) {
    assignments.insert(&assignment);
    double cost = static_cast<double>(assignment.prefix) * surfelCostFactor;
    if(debugAssignment) std::cout << "draw surfels " << node << " for " << cost << std::endl;
    usedBudget += cost;
    debugSurfelUsed += cost;
  }
  
  if(mesh && assignment.expanded) {
    // leaf node -> draw mesh & update budget
    double cost = static_cast<double>(mesh->getPrimitiveCount()) * geoCostFactor;
    if(debugAssignment) std::cout << "draw " << node << " for " << cost << std::endl;
    usedBudget += cost;
    debugGeoUsed += cost;
    return NodeRendererResult::PASS_ON;
  }
  
  if(!deferredSurfels && !debugHideSurfels && assignment.prefix > 0) {
    FAIL_IF(assignment.prefix > surfels->isUsingIndexData() ? surfels->getIndexCount() : surfels->getVertexCount());
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

void SurfelRendererBudget::assignBudget() {  
  double remainingBudget = budget - usedBudget;
  
  
  // find best candidates for expansion
  for(auto ass : assignments) {
    // only expand when it fits the budget & the surfel count is not exhausted
    if(!ass->expanded && ass->expansionCost <= remainingBudget && ass->prefix >= ass->surfelCount) {
      /*float s = ass->expansionBenefit / ass->expansionCost; 
      if(s > maxBenefitCost) {
        maxBenefitCost = s;
        maxExpand = ass;
      }*/
      
      // expand node
      ass->expanded = true;
      float expansionCost = 0;
      if(debugAssignment) std::cout << "expanding " << ass->node << " for " << ass->expansionCost << std::endl;
      for(auto child : getChildNodes(ass->node)) {
        BudgetAssignment& a = getAssignment(child);      
        auto s = getSurfelMesh(child);
        if(s) {
          uint32_t sc = s->isUsingIndexData() ? s->getIndexCount() : s->getVertexCount();	
          a.size = ass->minSize;
          a.prefix = getPrefixForRadius(sizeToRadius(ass->minSize, a.mpp), a.medianDist, SURFEL_MEDIAN_COUNT, sc);
          expansionCost += static_cast<float>(a.prefix) * surfelCostFactor;
          if(debugAssignment) std::cout << "  " << a.node << " for " << (a.prefix * surfelCostFactor) << std::endl;
        }
        a.expanded = false;
      }
      ass->expansionThreshold = ass->expansionBenefit;
      remainingBudget -= ass->expansionCost;
    }  
    //if(!a->expanded && debugAssignment) std::cout << "e " << a->node << " " << a->expansionCost << "/" << remainingBudget << " " << a->prefix << "/" << a->surfelCount << std::endl;
  }
  
  if(remainingBudget < 0) {
    // overshot budget -> decrease surfels
    double decrement = -remainingBudget;
    //std::cout << "Budget " << remainingBudget << "->";
    
    for(auto it = assignments.rbegin(); it != assignments.rend(); ++it) {
      auto a = *it;
      float factor = 1.0f - a->surfelBenefit / surfelBenefitSum;
      uint32_t maxDec = a->prefix > a->minPrefix ? a->prefix - a->minPrefix : 0;
      uint32_t dec = std::min<uint32_t>(maxDec, std::ceil(factor * decrement));
      if(debugAssignment) std::cout << "decrement " << a->node << " " << a->prefix << "->" << (a->prefix-dec) << std::endl;
      a->prefix -= dec;
      decrement -= dec;
      remainingBudget += static_cast<double>(dec) * surfelCostFactor;
      
      // update size
      a->size = std::min(radiusToSize(getRadiusForPrefix(a->prefix, a->medianDist, SURFEL_MEDIAN_COUNT), a->mpp), maxSurfelSize);
    }
    
    //std::cout << remainingBudget << std::endl;
  } 
    
  double increment = std::min<double>(maxIncrement, remainingBudget/surfelCostFactor);
  
  // distribute remaining budget
  if(surfelBenefitSum > 0) {
    for(auto a : assignments) {
      float factor = a->surfelBenefit / surfelBenefitSum;
      if(factor <= 0)
        break;
      uint32_t maxInc = a->maxPrefix - a->prefix;
      uint32_t inc = std::min<uint32_t>(maxInc, std::floor(factor * increment));
      if(debugAssignment) std::cout << "increment " << a->node << " " << a->prefix << "->" << (a->prefix+inc) << std::endl;
      a->prefix += inc;
      increment -= inc;
      remainingBudget -= static_cast<double>(inc) * surfelCostFactor;
      
      // update size
      a->size = std::min(radiusToSize(getRadiusForPrefix(a->prefix, a->medianDist, SURFEL_MEDIAN_COUNT), a->mpp), maxSurfelSize);
    }
  }
  if(debugAssignment) {
    std::cout << "Budget " << (budget-remainingBudget) << "/" << budget << " (s " << debugSurfelUsed << " g " << debugGeoUsed << ") " << std::endl;
    std::cout << std::endl;
  }
  if(remainingBudget < 0) {
    //WARN("Used budged greater than budget!");
    //deactivate();
    //doClearAssignment = true;
    return;
  }
}

// surfel benefit: benefit * tanh(a*x / 2*b)
// hyperbolic function tanh(ax/2b) <-> logistic function (2 / (1+e^-(ax/b)) - 1)
/*double SurfelRendererBudget::getSurfelBenefit(float x, float ps, float sat) const {
	if(sat <= 0.0)
		return 0.0;
	return ps * std::tanh(benefitGrowRate * x / (2.0f*sat));
}*/


/*
static inline double sech2(double x) {
    double sh = 1.0 / std::cosh(x);   // sech(x) == 1/cosh(x)
    return sh*sh;                     // sech^2(x)
}*/

// derivative of surfel benefit function: p * a/2b * sech^2(ax/2b)
/*double SurfelRendererBudget::getSurfelBenefitDerivative(float x, float ps, float sat) const {
	if(sat <= 0.0)
		return 0.0;
	float a2b = benefitGrowRate/(2.0f*sat); 
	return ps * a2b * sech2(a2b*x); 
}*/

SurfelRendererBudget::stateResult_t SurfelRendererBudget::doEnableState(FrameContext & context, Node * node, const RenderParam & rp) {
  if(doClearAssignment) {
    forEachNodeTopDown(node, [&](Node * n) {
      BudgetAssignment& a = getAssignment(n);
      a.prefix = 0;
      a.expanded = false;
    });
    doClearAssignment = false;
  }
  
  if(!debugAssignment || stepAssignment) {
	  assignments.clear();
    ++frameCount;
  }
    
  surfelBenefitSum = 0;
  usedBudget = 0;
  debugSurfelUsed = 0;
  debugGeoUsed = 0;
	return NodeRendererState::doEnableState(context, node, rp);
}

void SurfelRendererBudget::doDisableState(FrameContext & context, Node * node, const RenderParam & rp) {
	NodeRendererState::doDisableState(context, node, rp);
  
  if(!debugAssignment || stepAssignment)
    assignBudget();
  stepAssignment = false;
	
	if(deferredSurfels && !debugHideSurfels) 
		drawSurfels(context);
}

void SurfelRendererBudget::drawSurfels(FrameContext & context, float minSize, float maxSize) const {
	auto& rc = context.getRenderingContext();	
	rc.setGlobalUniform({uniform_renderSurfels, true});	
	for(auto s : assignments) {
    auto surfels = getSurfelMesh(s->node);
    if(!surfels || s->prefix == 0 || s->prefix > s->surfelCount || s->size < minSize || s->size >= maxSize)
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
