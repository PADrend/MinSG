/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_GEOMETRYNODECOLLECTOR_H
#define MINSG_SVS_GEOMETRYNODECOLLECTOR_H

#include "../../Core/States/NodeRendererState.h"
#include "Definitions.h"
#include <Util/TypeNameMacro.h>
#include <unordered_set>

namespace Rendering {
class OcclusionQuery;
}
namespace MinSG {
class FrameContext;
class GeometryNode;
class Node;
class RenderParam;
namespace SVS {

/**
 * State to collect the GeometryNodes that are going through a rendering
 * channel. The set of collected nodes can be retrieved at any time. It is
 * cleared when the state is activated. If combined with a culling renderer,
 * this state can be used to collect the potentially visible set (PVS) that is
 * created by the culling renderer.
 *
 * @author Benjamin Eikel
 * @date 2013-06-26
 */
class GeometryNodeCollector : public NodeRendererState {
		PROVIDES_TYPE_NAME(SVS::GeometryNodeCollector)
	private:
		std::unordered_set<GeometryNode *> collectedNodes;

		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

	protected:
		//! Clear the set of collected nodes.
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	public:
		MINSGAPI GeometryNodeCollector();

		MINSGAPI GeometryNodeCollector * clone() const override;

		//! Read the set of collected nodes.
		const std::unordered_set<GeometryNode *> & getCollectedNodes() const {
			return collectedNodes;
		}
};

}
}

#endif /* MINSG_SVS_GEOMETRYNODECOLLECTOR_H */

#endif /* MINSG_EXT_SVS */
