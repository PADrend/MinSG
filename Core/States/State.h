/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_STATE_H
#define MINSG_STATE_H

#include "../RenderParam.h"

#include <Util/AttributeProvider.h>
#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>
#include <Util/Macros.h>
#include <string>

namespace MinSG {
class Node;
class FrameContext;

/**
* Representation of a state, that can be bound to a node.
*/
class State :
			public Util::ReferenceCounter<State>,
			public Util::AttributeProvider {
		PROVIDES_TYPE_NAME(State)
	public:

		State() : ReferenceCounter_t(), Util::AttributeProvider(), statusFlags(STATUS_ACTIVE), renderingLayers(1<<0 /*RENDERING_LAYER_DEFAULT*/) {}
		State(const State &) = default;
		State(State &&) = default;
		State & operator=(const State &) = default;
		State & operator=(State &&) = default;
		virtual ~State() {}

		//! Return type of enableState().
		enum stateResult_t {
			/**
			 * The state was enabled.
			 * It has to be disabled for the node using disableState().
			 */
			STATE_OK = 0,
			/**
			 * The state is not enabled (e.g. due to an error).
			 * disableState() must not be called for the node.
			 */
			STATE_SKIPPED = 1,
			/**
			 * The state was enabled.
			 * No further states of the node should be enabled.
			 * The state has to be disabled for this node using disableState().
			 */
			STATE_SKIP_OTHER_STATES = 2,
			/**
			 * Rendering of the node should be skipped (e.g. if the rendering has been handled by this state because it is a renderer).
			 * After the call, the state is not active for that node, so disableState() must not be called for that node.
			 */
			STATE_SKIP_RENDERING = 3
		};

		/**
		 * Enable this state for the given node (=subtree).
		 *
		 * @param context FrameContext to be used by this state.
		 * @param node Node that this state should be enabled for.
		 * @param rp Rendering options.
		 * @return Special result enumerator
		 * @see stateResult_t for description of return type
		 */
		stateResult_t enableState(FrameContext & context, Node * node, const RenderParam & rp) {
			return (isActive() && testRenderingLayer(rp.getRenderingLayers()))  ? 
						doEnableState(context, node, rp) :
						STATE_SKIPPED;
		}

		/**
		 * Disable this state for the given node (=subtree).
		 *
		 * @param context FrameContext to be used by this state.
		 * @param node Node that this state should be enabled for.
		 * @param rp Rendering options.
		 */
		void disableState(FrameContext & context, Node * node, const RenderParam & rp) {
			FAIL_IF(!isActive());
			doDisableState(context, node, rp);
		}

		//! Create a duplicate of this State object.
		virtual State * clone() const = 0;

		bool isActive() const							{	return statusFlags&STATUS_ACTIVE;	}
		void activate()									{	statusFlags |= STATUS_ACTIVE;	}
		void deactivate()								{	statusFlags &= ~STATUS_ACTIVE;	}

		//! If a State is marked as tempState, it is ignored when saving the scene.
		bool isTempState()const							{	return statusFlags&STATUS_TEMPORARY;	}
		void setTempState(bool b)						{	statusFlags = b ? statusFlags|STATUS_TEMPORARY : statusFlags&~STATUS_TEMPORARY;	}

	private:
		static const uint8_t STATUS_ACTIVE = 1<<0;
		static const uint8_t STATUS_TEMPORARY = 1<<1;
		uint8_t statusFlags;

		/**
		 * Enable this state for the given node (=subtree).
		 * This function has to be implemented by derived classes.
		 * It is called by @a enableState.
		 *
		 * @param context FrameContext to be used by this state.
		 * @param node Node that this state should be enabled for.
		 * @param rp Rendering options.
		 * @return Special result enumerator
		 * @see stateResult_t for description of return type
		 */
		virtual stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) = 0;

		/**
		 * Disable this state for the given node (=subtree).
		 * It is called by @a disableState.
		 *
		 * @param context FrameContext to be used by this state.
		 * @param node Node that this state should be enabled for.
		 * @param rp Rendering options.
		 */
		virtual void doDisableState(FrameContext & /*context*/, Node * /*node*/, const RenderParam & /*rp*/) {}
		
	// -------
	/**
	 * @name Rendering layers
	 */
	//@{
	private:
		renderingLayerMask_t renderingLayers;

	public:
		renderingLayerMask_t getRenderingLayers()const					{	return renderingLayers;	}
		void setRenderingLayers(renderingLayerMask_t l)					{	renderingLayers = l;	}
		bool testRenderingLayer(renderingLayerMask_t l)const			{	return (renderingLayers&l)>0; }
	//@}
};
}

#endif // MINSG_STATE_H
