/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef __GroupState_H
#define __GroupState_H

#include "State.h"
#include <stack>
#include <vector>

namespace MinSG {

/**
 *  [GroupState] ---|> [State]
 *  A GroupState can contain multiple other States. The States attached to a Node's GroupState
 *  behave like as if they were directly attached to the Node.
 *
 *  \note GroupStates support to be enabled in a nested way (e.g. on an inner Node and on a leaf Node on the same subtree),
 *		though this behvior is strongly not encouraged, as the contained States are not guaranteed to work in a nested way.
 *  \note the interface is intentionally designed as close as possible to the state related functions of Node.
 *		If the corresponding interface of Node is altered, these changes should also be performed to GroupState.
 * @ingroup states
 */
class GroupState : public State {
		PROVIDES_TYPE_NAME(GroupState)

	public:
		typedef std::vector<Util::Reference<State>> stateArray_t;

		GroupState() : State() {}
		MINSGAPI virtual ~GroupState();

		MINSGAPI void addState(State * s);
		/// ---|> [State]
		GroupState * clone() const override				{	return new GroupState(*this);	}
		const stateArray_t & getStates() const			{	return states;	}
		bool hasStates() const							{	return !states.empty(); }
		bool isEnabled() const							{	return !enabledStates.empty();	}
		MINSGAPI void removeState(State * s);
		MINSGAPI void removeStates();

	private:
		static State * const NO_STATE;
		stateArray_t states;
		std::stack<State *> enabledStates;

		/// ---|> [State]
		MINSGAPI void doDisableState(FrameContext & context, Node *, const RenderParam & rp) override;
		/// ---|> [State]
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;

};
}
#endif // GroupState_H
