#ifdef MINSG_EXT_THESISSTANISLAW

#include <MinSG/Ext/ThesisStanislaw/PolygonIndexing.h>

#include "../../Core/NodeRenderer.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Nodes/GeometryNode.h"
#include <iostream>

namespace MinSG{
namespace ThesisStanislaw{

NodeRendererResult PolygonIndexingState::displayNode(FrameContext & context, Node * node, const RenderParam & rp){
  if(updatePolygonIDs){
    if(outputDebug){ std::cout << "Start Indexing Polygons" << std::endl; }
    node->traverse(visitor);
    updatePolygonIDs = false;
    if(outputDebug){ std::cout << "End Indexing Polygons"  << std::endl; }
  }
  return NodeRendererResult::PASS_ON;
}

PolygonIndexingState * PolygonIndexingState::clone() const {
	return new PolygonIndexingState(*this);
}

PolygonIndexingState::PolygonIndexingState() : NodeRendererState(FrameContext::DEFAULT_CHANNEL), updatePolygonIDs(true) {}

PolygonIndexingState::~PolygonIndexingState(){}

}}

#endif // MINSG_EXT_THESISSTANISLAW
