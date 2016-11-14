/*
	This file is part of the MinSG library.
	Copyright (C) 20016 Stanislaw Eppinger

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

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
    IndexingVisitor visitor;
    if(outputDebug) visitor.setDebug(true);
    node->traverse(visitor);
    updatePolygonIDs = false;
    if(outputDebug){ std::cout << "End Indexing Polygons"  << std::endl; }
  }
  return State::stateResult_t::STATE_OK;
}

PolygonIndexingState * PolygonIndexingState::clone() const {
	return new PolygonIndexingState(*this);
}

void PolygonIndexingState::reupdatePolygonIDs(){
  updatePolygonIDs = true;
}

PolygonIndexingState::PolygonIndexingState() : State(), updatePolygonIDs(true), outputDebug(false) {}

PolygonIndexingState::~PolygonIndexingState(){}

}}

#endif // MINSG_EXT_THESISSTANISLAW
