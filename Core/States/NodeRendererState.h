/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_NODERENDERERSTATE_H
#define MINSG_NODERENDERERSTATE_H

#include "State.h"
#include "../../Helper/NodeRendererRegistrationHolder.h"
#include <Util/StringIdentifier.h>

namespace MinSG {
class FrameContext;
class Node;
enum class NodeRendererResult : bool;
class RenderParam;

/**
 * Abstract class for a node renderer
 * that registers itself at the FrameContext when activated,
 * and unregisters itself at the FrameContext when deactivated.
 * 
 * @author Benjamin Eikel
 * @date 2012-04-18
 * @ingroup states
 */
class NodeRendererState : public State {
		PROVIDES_TYPE_NAME(NodeRendererState)
	private:
		//! Identifier of the channel the node renderer is registered to.
		Util::StringIdentifier nodeRendererChannel;

		//! Handle storing the registration of this state as NodeRenderer
		NodeRendererRegistrationHolder registrationHolder;

		/**
		 * Node renderer function.
		 * This function is registered at the configured channel when the state is activated.
		 * This function has to be implemented by subclasses.
		 */
		virtual NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) = 0;

	protected:
		//! Register the node renderer at the configured channel.
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

		//! Remove the node renderer from the configured channel.
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;

	public:
		/**
		 * Create a new node renderer that treats the given channel.
		 * 
		 * @param newChannel Rendering channel identifier
		 */
		MINSGAPI NodeRendererState(Util::StringIdentifier newChannel);

		MINSGAPI NodeRendererState(const NodeRendererState & other);
		MINSGAPI ~NodeRendererState();

		/**
		 * Return the channel that is treated by the node renderer.
		 * 
		 * @return Rendering channel identifier
		 */
		Util::StringIdentifier getSourceChannel() const {
			return nodeRendererChannel;
		}

		/**
		 * Configure the channel that will be treated by the node renderer.
		 *
		 * @note The channel must not be changed between @a doEnableState and @a doDisableState
		 * @param newChannel Rendering channel identifier
		 */
		void setSourceChannel(Util::StringIdentifier newChannel) {
			nodeRendererChannel = newChannel;
		}
};

}

#endif /* MINSG_NODERENDERERSTATE_H */
