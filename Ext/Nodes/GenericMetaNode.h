/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef MINSG_EXT_GENERICMETANODE_H
#define MINSG_EXT_GENERICMETANODE_H

#include "../../Core/Nodes/Node.h"
#include <Geometry/Box.h>

namespace MinSG {

/**
 *  GenericMetaNode ---|> Node
 * An invisible Node, which can be seen when SHOW_META_OBJECTS is enabled.
 * Useful as generic target for various Behaviours (e.g. SoundSources) or as a particle emitter.
 * @ingroup nodes
 */
class GenericMetaNode : public Node {
	PROVIDES_TYPE_NAME(GenericMetaNode)
	public:
		GenericMetaNode() : Node(), bb(Geometry::Vec3(0, 0, 0), 1) {
		}
		MINSGAPI virtual ~GenericMetaNode();

		void setBB(const Geometry::Box & newBB) {
			bb = newBB;
			worldBBChanged();
		}

		// ---|> Node
		MINSGAPI void doDisplay(FrameContext & context, const RenderParam & rp) override;
	private:
		/// ---|> [Node]
		GenericMetaNode * doClone()const override		{	return new GenericMetaNode(*this);	}		
		const Geometry::Box& doGetBB() const override	{	return bb;	}
		Geometry::Box bb;
};

}

#endif /* MINSG_EXT_GENERICMETANODE_H */
