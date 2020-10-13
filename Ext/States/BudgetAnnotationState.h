/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_BUDGETANNOTATIONSTATE_H_
#define MINSG_BUDGETANNOTATIONSTATE_H_

#include "../../Core/FrameContext.h"
#include "../../Core/States/NodeRendererState.h"
#include <Util/StringIdentifier.h>
#include <Util/TypeNameMacro.h>
#include <string>

namespace MinSG {
class Node;
class RenderParam;

/**
 * @brief Distribute a budget value among nodes of a tree
 *
 * When the state is enabled, it takes the initial budget value and starts to
 * distribute the value to the node it is attached to and fractions of it to
 * the child nodes. The function to calculate the fractions can be configured.
 * The values are stored as attributes at the nodes. The name of the attribute
 * can be configured, too.
 *
 * @author Benjamin Eikel
 * @date 2012-11-16
 * @ingroup states
 */
class BudgetAnnotationState : public NodeRendererState {
	PROVIDES_TYPE_NAME(BudgetAnnotationState)
	public:
		enum distribution_type_t {
			//! For a node with \a k child nodes, every child node receives a fraction of 1 / \a k of the budget.
			DISTRIBUTE_EVEN,
			//! Distribute the budget based on the projected size of the child nodes.
			DISTRIBUTE_PROJECTED_SIZE,
			//! Distribute the budget based on the projected size and the primitive count of the child nodes.
			DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT,
#ifdef MINSG_EXT_THESISJONAS
			//! Distribute the budget based on the projected size and the primitive count of the child nodes in an iterative manner for correct distribution.
			DISTRIBUTE_PROJECTED_SIZE_AND_PRIMITIVE_COUNT_ITERATIVE,
#endif
			//! Delete the attribute from all nodes.
			DISTRIBUTE_DELETE
		};

		/**
		 * Convert the value of an enumerator to a string.
		 *
		 * @param type Value that will be converted to a string
		 * @throw std::invalid_argument If the conversion of an unknown value is detected
		 * @return Human-readable string
		 */
		MINSGAPI static std::string distributionTypeToString(distribution_type_t type);

		/**
		 * Convert the value of a string to an enumerator.
		 *
		 * @param str Value that can be converted to an enumerator
		 * @throw std::invalid_argument If the conversion of an unknown value is detected
		 * @return Distribution type enumerator
		 */
		MINSGAPI static distribution_type_t distributionTypeFromString(const std::string & str);
	private:
		//! The attribute that is written to the nodes
		Util::StringIdentifier annotationAttribute;

		//! Overall budget for the whole tree
		double budget;

		/**
		 * Distribute the given value to the child nodes of the given node.
		 * The value is distributed by writing the attribute to the child
		 * nodes.
		 *
		 * @param value Value that is to be distributed
		 * @param attributeId Identifier of the attribute to distribute
		 * @param node Root node of subtree that is to be annotated
		 * @param context Frame context that may deliver additional
		 * information
		 */
		typedef std::function<void (double, const Util::StringIdentifier &, Node *, FrameContext &)> distribution_function_t;

		distribution_function_t distributionFunction;

		distribution_type_t distributionType;
	protected:
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	public:
		MINSGAPI BudgetAnnotationState();

		MINSGAPI virtual ~BudgetAnnotationState();

		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		MINSGAPI BudgetAnnotationState * clone() const override;

		const Util::StringIdentifier & getAnnotationAttribute() const {
			return annotationAttribute;
		}
		void setAnnotationAttribute(const Util::StringIdentifier & attribute) {
			annotationAttribute = attribute;
		}

		double getBudget() const {
			return budget;
		}
		void setBudget(double newBudget) {
			budget = newBudget;
		}

		distribution_type_t getDistributionType() const {
			return distributionType;
		}
		MINSGAPI void setDistributionType(distribution_type_t type);
};

}

#endif /* MINSG_BUDGETANNOTATIONSTATE_H_ */
