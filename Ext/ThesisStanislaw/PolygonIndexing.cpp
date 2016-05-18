#ifdef MINSG_EXT_THESISSTANISLAW

#include <MinSG/Ext/ThesisStanislaw/PolygonIndexing.h>

#include "../../Core/NodeRenderer.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/GeometryNode.h"
#include <iostream>

namespace MinSG{
namespace ThesisStanislaw{
  
State::stateResult_t PolygonIndexingState::doEnableState(FrameContext & context, Node * node, const RenderParam & rp){
  if(updatePolygonIDs){
    if(outputDebug){ std::cout << "Start Indexing Polygons" << std::endl; }
    node->traverse(visitor);
    updatePolygonIDs = false;
    if(outputDebug){ std::cout << "End Indexing Polygons"  << std::endl; }
  }
  return State::stateResult_t::STATE_OK;
}

NodeRendererResult PolygonIndexingState::displayNode(FrameContext & context, Node * node, const RenderParam & rp){
  return NodeRendererResult::PASS_ON;
}

PolygonIndexingState * PolygonIndexingState::clone() const {
	return new PolygonIndexingState(*this);
}

PolygonIndexingState::PolygonIndexingState() : NodeRendererState(FrameContext::DEFAULT_CHANNEL), updatePolygonIDs(true) {}

PolygonIndexingState::~PolygonIndexingState(){}

}}

#endif // MINSG_EXT_THESISSTANISLAW
