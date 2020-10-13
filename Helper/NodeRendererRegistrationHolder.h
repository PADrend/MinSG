/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_HELPER_NODERENDERERREGISTRATIONHOLDER_H
#define MINSG_HELPER_NODERENDERERREGISTRATIONHOLDER_H

#include "../Core/NodeRenderer.h"
#include <memory>

namespace Util {
class StringIdentifier;
}
namespace MinSG {
class FrameContext;

/**
 * @brief Holder of a NodeRenderer registration
 * 
 * Class storing the registration of a NodeRenderer at a FrameContext.
 * 
 * @author Benjamin Eikel
 * @date 2013-12-02
 * @ingroup helper
 */
class NodeRendererRegistrationHolder {
	private:
		// Use Pimpl idiom
		struct RegistrationState;
		std::unique_ptr<RegistrationState> impl;

	public:
		MINSGAPI NodeRendererRegistrationHolder();
		MINSGAPI ~NodeRendererRegistrationHolder();
		NodeRendererRegistrationHolder(NodeRendererRegistrationHolder &&) = default;
		NodeRendererRegistrationHolder(const NodeRendererRegistrationHolder &) = delete;
		NodeRendererRegistrationHolder & operator=(NodeRendererRegistrationHolder &&) = default;
		NodeRendererRegistrationHolder & operator=(const NodeRendererRegistrationHolder &) = delete;

		/**
		 * Register the given @p renderer at the rendering channel with name
		 * @p channelName inside the @p frameContext. If there is a previous
		 * registration by this instance, nothing happens.
		 * 
		 * @param frameContext Frame context holding the rendering channels
		 * @param channelName Name of the rendering channel used for
		 * registration
		 * @param renderer NodeRenderer function that will be registered at the
		 * rendering channel
		 */
		MINSGAPI void registerNodeRenderer(FrameContext & frameContext,
								  Util::StringIdentifier channelName,
								  NodeRenderer renderer);

		/**
		 * Cancel the registration of a NodeRenderer that has been registered
		 * before by this instance. If there is no previous registration,
		 * nothing happens.
		 */
		MINSGAPI void unregisterNodeRenderer();
};

}

#endif /* MINSG_HELPER_NODERENDERERREGISTRATIONHOLDER_H */
