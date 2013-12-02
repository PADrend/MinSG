/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "NodeRendererRegistrationHolder.h"
#include "../Core/FrameContext.h"
#include <functional>

namespace MinSG {

struct NodeRendererRegistrationHolder::RegistrationState {
	FrameContext::node_renderer_registration_t registrationHandle;
	FrameContext & frameContext;
	Util::StringIdentifier nodeRendererChannel;

	RegistrationState(FrameContext::node_renderer_registration_t handle,
					  FrameContext & context,
					  Util::StringIdentifier channel) : 
		registrationHandle(std::move(handle)), 
		frameContext(context),
		nodeRendererChannel(std::move(channel)) {
	}
	~RegistrationState() {
		frameContext.unregisterNodeRenderer(nodeRendererChannel, 
											std::move(registrationHandle));
	}
};

NodeRendererRegistrationHolder::NodeRendererRegistrationHolder() : impl() {
}

NodeRendererRegistrationHolder::~NodeRendererRegistrationHolder() {
	unregisterNodeRenderer();
}

void NodeRendererRegistrationHolder::registerNodeRenderer(FrameContext & frameContext,
														  Util::StringIdentifier channelName,
														  NodeRenderer renderer) {
	if(impl) {
		return;
	}
	impl.reset(new RegistrationState(frameContext.registerNodeRenderer(channelName, 
																	   std::move(renderer)),
									 frameContext,
									 channelName));
}

void NodeRendererRegistrationHolder::unregisterNodeRenderer() {
	if(!impl) {
		return;
	}
	impl.reset();
}

}
