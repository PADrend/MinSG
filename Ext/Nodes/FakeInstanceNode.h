/*
	This file is part of the MinSG library.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_EXT_FAKEINSTANCENODE_H
#define MINSG_EXT_FAKEINSTANCENODE_H

#include "../../Core/Nodes/Node.h"
#include <Util/References.h>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace MinSG {
class FrameContext;
class RenderParam;

/**
 * @brief Node to display a fake instance of another node
 * 
 * A FakeInstanceNode object refers to another node inside the scene graph.
 * When the FakeInstanceNode object is to be displayed, it changes the camera
 * and displays the other node. The other node is displayed with the
 * transformation of the FakeInstanceNode object applied. Therefore, the
 * FakeInstanceNode can be used to display a node multiple times with
 * different transformations, but without the need to have the node multiple
 * times in memory.
 * 
 * @note The FakeInstanceNode is no replacement for a real instance and might
 * lead to problems when used. Only use it if there is no better alternative.
 * @author Benjamin Eikel
 * @date 2012-10-22 
 * @ingroup nodes
 */
class FakeInstanceNode : public Node {
	PROVIDES_TYPE_NAME(FakeInstanceNode)
	private:
		Util::Reference<Node> fakePrototype;
	public:
		FakeInstanceNode() : Node(), fakePrototype() {
		}
		MINSGAPI virtual ~FakeInstanceNode();

		MINSGAPI void doDisplay(FrameContext & frameContext, const RenderParam & rp) override;

		Node * getFakePrototype() const {
			return fakePrototype.get();
		}
		void setFakePrototype(Node * prototype) {
			fakePrototype = prototype;
		}
	private:
		/// ---|> [Node]
		FakeInstanceNode * doClone()const override	{	return new FakeInstanceNode(*this);	}
		MINSGAPI const Geometry::Box& doGetBB() const override;		
};

}

#endif /* MINSG_EXT_FAKEINSTANCENODE_H */
