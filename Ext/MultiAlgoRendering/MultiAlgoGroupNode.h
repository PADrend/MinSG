/*
	This file is part of the MinSG library extension MultiAlgoRendering.
	Copyright (C) 2009-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_MULTIALGORENDERING
#include "Dependencies.h"

#ifndef MAR_MULTIALGOGROUPNODE_H
#define MAR_MULTIALGOGROUPNODE_H

#include "../../Core/Nodes/GroupNode.h"
#include "../../Core/States/GroupState.h"
#include "../../Core/States/ShaderState.h"
#include "../../Core/FrameContext.h"

#include <map>
#include <deque>
#include <limits>

namespace MinSG {

class ShaderUniformState;

//! @ingroup ext
namespace MAR {

class MultiAlgoGroupNode : public GroupNode {
	PROVIDES_TYPE_NAME(MultiAlgoGroupNode)

public:

	typedef Util::Reference<MultiAlgoGroupNode> ref_t;

	typedef int32_t AlgoId_t ;
	enum AlgoId : AlgoId_t {Auto = 0, SkipRendering = 1, ColorCubes = 2, CHCppAggressive = 3, CHCpp = 4, BruteForce = 5, BlueSurfels = 6, SphericalSampling = 7, ClassicLOD = 8, ForceSurfels = 9};
	MINSGAPI static std::string algoIdToString(AlgoId id);
	
	MINSGAPI static const uint32_t INVALID_NODE_ID;

	MINSGAPI MultiAlgoGroupNode();
	MINSGAPI MultiAlgoGroupNode(const MultiAlgoGroupNode & source);
	MINSGAPI virtual ~MultiAlgoGroupNode();

	MINSGAPI void setAlgorithm(AlgoId);
	inline AlgoId getAlgorithm() const {
		return algoId;
	}

	MINSGAPI static void initAlgorithm(AlgoId, GroupState * state = nullptr);

	static void setHighlightIntensity(float f) {
		highlightIntensity = f;
	}
	
	static float getHighlightIntensity() {
		return highlightIntensity;
	}

	void setNodeId(uint32_t id){
		nodeId = id;
	}
	uint32_t getNodeId() const {
		return nodeId;
	}
	Node * getNodeForExport(){
		return node.get();
	}
	
	MINSGAPI void initNode();

	MINSGAPI std::string toString() const;

	/// ---|> [GroupNode]
	MINSGAPI size_t countChildren() const override;

	/// ---|> [Node]
	MINSGAPI NodeVisitor::status traverse(NodeVisitor & visitor) override;
	MINSGAPI void doDisplay(FrameContext & context, const RenderParam & rp) override;

private:
	/// ---|> [Node]
	MINSGAPI const Geometry::Box& doGetBB() const override;

	/// ---|> [GroupNode]
	void invalidateCompoundBB() override{}
	MINSGAPI void doAddChild(Util::Reference<Node> child) override;
	MINSGAPI bool doRemoveChild(Util::Reference<Node> child) override;

		
	MINSGAPI Node * doClone() const override;

	uint32_t nodeId;

	Util::Reference<ShaderUniformState> uniformState;

	typedef std::map<AlgoId,Util::Reference<GroupState> > states_t;
	typedef states_t::const_iterator states_c_it;
	typedef states_t::iterator states_it;
	MINSGAPI static states_t states;

	Util::Reference<GroupNode> node;

	AlgoId algoId;

	MINSGAPI static float highlightIntensity;
};
}

}
#endif // MULTIALGOGROUPNODE_H

#endif // MINSG_EXT_MULTIALGORENDERING
