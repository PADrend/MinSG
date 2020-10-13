/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_BUDGETRENDERER_H
#define MINSG_SVS_BUDGETRENDERER_H

#include "../../Core/States/NodeRendererState.h"
#include <Util/TypeNameMacro.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace Rendering {
class OcclusionQuery;
}
namespace Util {
class StringIdentifier;
}
namespace MinSG {
class FrameContext;
class GeometryNode;
class Node;
class RenderParam;
namespace SVS {

/**
 * @brief Budget rendering
 * 
 * State to perform budget rendering. It collects the GeometryNodes that are
 * going through a rendering channel. When it is deactivated, it displays the
 * most important objects while keeping a budget.
 * To perform the budget rendering, an instance of the 0-1 knapsack problem has
 * to be solved. A greedy approximation is used to achieve this. The objects
 * are sorted by the ratio of their primitive count divided by their projected
 * size. Beginning with the lowest ratio, the objects are displayed iteratively
 * until the budget has been reached.
 * If an object has an attribute containing the projected size (see
 * #projectedSizeId), the minimum of the calculated projected size and the
 * attribute's value is used for the ratio calculation. The attribute can be
 * set by other renderers to influence the importance of objects.
 *
 * @author Benjamin Eikel
 * @date 2013-07-23
 */
class BudgetRenderer : public NodeRendererState {
		PROVIDES_TYPE_NAME(SVS::BudgetRenderer)
	private:
		uint32_t budget;

		std::vector<std::pair<double, GeometryNode *>> collectedNodes;

		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

	protected:
		//! Clear the set of collected nodes.
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

		//! Display a subset of the collected nodes.
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	public:
		MINSGAPI BudgetRenderer();

		MINSGAPI BudgetRenderer * clone() const override;

		uint32_t getBudget() const {
			return budget;
		}
		//! Set the budget as number of primitives to render at most.
		void setBudget(uint32_t newBudget) {
			budget = newBudget;
		}

		//! Identifier of a node attribute containing the projected size.
		MINSGAPI static const Util::StringIdentifier projectedSizeId;
};

}
}

#endif /* MINSG_SVS_BUDGETRENDERER_H */

#endif /* MINSG_EXT_SVS */
