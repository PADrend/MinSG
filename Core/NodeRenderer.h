/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_CORE_NODERENDERER_H
#define MINSG_CORE_NODERENDERER_H

#include <functional>

namespace MinSG {
class FrameContext;
class Node;
class RenderParam;

//! Return type used by node renderer functions.
enum class NodeRendererResult : bool {
	NODE_HANDLED,
	PASS_ON
};

//! Type of a node renderer function used in rendering channels.
typedef std::function<NodeRendererResult (FrameContext &, Node *, const RenderParam &)> NodeRenderer;

}

#endif /* MINSG_CORE_NODERENDERER_H */
