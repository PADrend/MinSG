/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING

#include "MultiAlgoGroupNode.h"
#include "AlgoSelector.h"
#include "SurfelRenderer.h"

#include "../../Core/Nodes/ListNode.h"
#include "../../Core/FrameContext.h"
#include "../../Core/NodeAttributeModifier.h"
#include "../OcclusionCulling/OccRenderer.h"
#include "../OcclusionCulling/CHCppRenderer.h"
#include "../../Ext/ColorCubes/ColorCubeRenderer.h"
#include "../../Ext/ColorCubes/ColorCubeGenerator.h"
#include "../../Helper/StdNodeVisitors.h"
#include "../States/ProjSizeFilterState.h"
#include "../States/LODRenderer.h"
#include "../../Core/States/TransparencyRenderer.h"
#include "../../Core/States/ShaderUniformState.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../TreeBuilder/TreeBuilder.h"
#include "../SVS/Renderer.h"

#include <Rendering/Draw.h>
#include <Rendering/Mesh/Mesh.h>

#include <Util/Graphics/ColorLibrary.h>
#include <Util/Macros.h>
#include <Util/GenericAttribute.h>

#include <cassert>

namespace MinSG {
	
	class SkipRenderer : public State {
		virtual State* clone()const override{return new SkipRenderer();}
		virtual stateResult_t doEnableState(FrameContext & /*context*/, Node * /*node*/, const RenderParam & /*rp*/) override{return State::STATE_SKIP_RENDERING;}
	};
	
namespace MAR {
	
MultiAlgoGroupNode::states_t MultiAlgoGroupNode::states;

float MultiAlgoGroupNode::highlightIntensity = 0.5;
const uint32_t MultiAlgoGroupNode::INVALID_NODE_ID = std::numeric_limits<uint32_t>::max();

std::string MultiAlgoGroupNode::algoIdToString(AlgoId algoId) {
	switch(algoId) {
		case Auto:
            return "Auto";
		case SkipRendering:
            return "Skip";
		case BruteForce:
            return "BruF";
		case CHCpp:
            return "CHCp";
		case CHCppAggressive:
            return "CHCa";
		case ColorCubes:
            return "ColC";
		case BlueSurfels:
            return "BluS";
		case ForceSurfels:
            return "ForS";
		case SphericalSampling:
            return "SVS ";
		case ClassicLOD:
            return "LOD ";
		default:
			WARN("please implement missing id");
			return "Algo UNKNOWN";
	}
}

MultiAlgoGroupNode::MultiAlgoGroupNode() :
GroupNode(), nodeId(INVALID_NODE_ID), uniformState(new ShaderUniformState()), node(new ListNode()) {
	this->setClosed(true);
	node->_setParent(this);
	setAlgorithm(CHCpp);
}

MultiAlgoGroupNode::MultiAlgoGroupNode(const MultiAlgoGroupNode & source) :
GroupNode(source), nodeId(source.nodeId), uniformState(new ShaderUniformState()), node(new ListNode()) {
	WARN("MultiAlgoGroupNode::copy_constructor: not implemented correct ...TODO...");
	setAlgorithm(source.algoId);
}

MultiAlgoGroupNode::~MultiAlgoGroupNode() {
}

void MultiAlgoGroupNode::initAlgorithm(AlgoId _algoId, GroupState * state) {

	if(states[_algoId].isNotNull())
		return;

	if(state) {
		states[_algoId] = state;
		return;
	}

	switch(_algoId) {
		case BruteForce: {
			states[_algoId] = new GroupState();
		}
		break;
		case CHCpp: {
			states[_algoId] = new GroupState();
			states[_algoId]->addState(new CHCppRenderer());
		}
		break;
		case CHCppAggressive: {
			states[_algoId] = new GroupState();
			states[_algoId]->addState(new CHCppRenderer(100));
		}
		break;
		case ColorCubes: {
			states[_algoId] = new GroupState();
			states[_algoId]->addState(new TransparencyRenderer());
			states[_algoId]->addState(new ColorCubeRenderer());
			auto psfs = new ProjSizeFilterState();
			psfs->setMaximumProjSize(100);
			psfs->setForceClosed(false);
			states[_algoId]->addState(psfs);
		}
		break;
		case BlueSurfels: {
			states[_algoId] = new GroupState();
			auto sr = new MAR::SurfelRenderer();
			sr->setSurfelCountFactor(8);
            sr->setSurfelSizeFactor(2);
			sr->setMaxAutoSurfelSize(10);
			sr->setForceSurfels(false);
			states[_algoId]->addState(sr);
		}
		break;
		case ForceSurfels: {
			states[_algoId] = new GroupState();
			auto sr = new MAR::SurfelRenderer();
			sr->setSurfelCountFactor(8);
			sr->setSurfelSizeFactor(1.5);
			sr->setMaxAutoSurfelSize(10);
			sr->setForceSurfels(true);
			states[_algoId]->addState(sr);
		}
		break;
		case SphericalSampling: {
			states[_algoId] = new GroupState();
			auto svsr = new SVS::Renderer();
			svsr->setInterpolationMethod(SVS::INTERPOLATION_MAX3);
			states[_algoId]->addState(svsr);
		}
		break;
		case ClassicLOD: {
			states[_algoId] = new GroupState();
			states[_algoId]->addState(new LODRenderer());
		}
		break;
		case SkipRendering:{
			states[_algoId] = new GroupState();
			states[_algoId]->addState(new SkipRenderer());
		}
		break;
		case Auto:
		default:
			throw std::logic_error("MultiAlgoGroupNode: calling initAlgorithm with unknown or invalid algo id");
	}
}

void MultiAlgoGroupNode::initNode() {

	moveStatesIntoClosedNodes(node.get());
	moveTransformationsIntoClosedNodes(node.get());
	TreeBuilder::buildOcTree(node.get());

}

static bool isToComplex(Node * r){
//	static const Util::StringIdentifier comp("_mar:complexity_");
	static const Util::StringIdentifier comp( NodeAttributeModifier::create("mar:complexity", NodeAttributeModifier::PRIVATE_ATTRIBUTE | NodeAttributeModifier::COPY_TO_CLONES) );
	uint32_t toComp = 10000000;
	if(!r->isAttributeSet(comp) || r->getAttribute(comp)->toUnsignedInt() == 0){
		uint32_t c = 0;
		auto geos = MinSG::collectNodes<MinSG::GeometryNode>(r);
		for(const auto & g : geos){
			if(g->getMesh()){
				c = std::max(c, g->getMesh()->getPrimitiveCount());
			}
		}
		r->setAttribute(comp, Util::GenericAttribute::createNumber<float>(c));
	}
	return r->getAttribute(comp)->toUnsignedInt() > toComp;
}

void MultiAlgoGroupNode::setAlgorithm(AlgoId newAlgoId) {
	
//  CODE FOR FAKING VIDEOS WITH LUCY
// 	if(newAlgoId != ForceSurfels && newAlgoId != BlueSurfels && newAlgoId != ColorCubes && newAlgoId != ClassicLOD && MinSG::MAR::isToComplex(node.get())){
// 		std::cerr << "changing algo for region with complex geo node: " << algoIdToString(newAlgoId) << " --> " << algoIdToString(ClassicLOD) << std::endl;
// 		newAlgoId = ClassicLOD;
// 	}

	if(states[newAlgoId].isNull()) {
		this->initAlgorithm(newAlgoId);
	}
	
	Util::Color4f color;
	switch(newAlgoId) {
		case BruteForce:
            color = Util::Color4ub(222, 222, 222);
			break;
		case CHCpp:
			color = Util::Color4ub(255, 255, 51);
			break;
		case CHCppAggressive:
			color = Util::ColorLibrary::BLACK;
			break;
		case ColorCubes:
			color = Util::Color4ub(255, 127, 0);
			break;
		case BlueSurfels:
			color = Util::Color4ub(128, 177, 211);
			break;
		case ForceSurfels:
			color = Util::ColorLibrary::BLUE;
			break;
		case SphericalSampling:
			color = Util::Color4ub(77, 175, 74);
			break;
		case ClassicLOD:
			color = Util::Color4ub(152, 78, 163);
			break;
		case Auto:
		case SkipRendering:
		default:
			color = Util::ColorLibrary::RED;
	}
	color.setA(getHighlightIntensity());
	uniformState->setUniform(Rendering::Uniform("color", color));
	
	this->removeStates();
	this->addState(uniformState.get()); // first
	this->addState(states[newAlgoId].get()); // last
	algoId = newAlgoId;

	
}

/// ---|> [GroupNode]
size_t MultiAlgoGroupNode::countChildren() const {
	return 1;
}

void MultiAlgoGroupNode::doAddChild(Util::Reference<Node> child) {
	node->addChild(child);
}

bool MultiAlgoGroupNode::doRemoveChild(Util::Reference<Node> child) {
	if(node.get() == child.get()) {
		WARN("MultiAlgoGroupNode::doRemoveChild: don't remove direct child of MultiAlgoGroupNode");
		return false;
	}
	return node->removeChild(child);
}

/// ---|> [Node]
const Geometry::Box& MultiAlgoGroupNode::doGetBB() const {
	return node->getBB();
}

Node * MultiAlgoGroupNode::doClone() const {
	WARN("MultiAlgoGroupNode::doClone : don't do that");
	return nullptr;
}

std::string MultiAlgoGroupNode::toString() const {
	std::stringstream ss;
	ss << "MAGN[" << this << "]";
	return ss.str();
}

NodeVisitor::status MultiAlgoGroupNode::traverse(NodeVisitor & visitor) {
	NodeVisitor::status status = visitor.enter(this);
	if(status == NodeVisitor::EXIT_TRAVERSAL) {
		return NodeVisitor::EXIT_TRAVERSAL;
	} else if(status == NodeVisitor::CONTINUE_TRAVERSAL) {

		if(node->traverse(visitor) == NodeVisitor::EXIT_TRAVERSAL) {
			return NodeVisitor::EXIT_TRAVERSAL;
		}

	}
	return visitor.leave(this);
}

void MultiAlgoGroupNode::doDisplay(FrameContext & context, const RenderParam & rp) {		
	context.displayNode(node.get(), rp);
}

}
}

#endif // MINSG_EXT_MULTIALGORENDERING
