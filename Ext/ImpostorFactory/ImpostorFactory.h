/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_IMPOSTORFACTORY_H
#define MINSG_IMPOSTORFACTORY_H

namespace MinSG {
class FrameContext;
class GeometryNode;
class Node;
//! @ingroup ext
namespace ImpostorFactory {

MINSGAPI GeometryNode * createReliefBoardForNode(FrameContext & frameContext, Node * node);
MINSGAPI GeometryNode * createTexturedDepthMeshForNode(FrameContext & frameContext, Node * node);

}
}

#endif /* MINSG_IMPOSTORFACTORY_H */
