/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef RANDOMIZEDSAMPLETREEBUILDER_H_
#define RANDOMIZEDSAMPLETREEBUILDER_H_

#include "TriangleTreeBuilder.h"

namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {
class TriangleTree;

/**
 * Class that creates a randomized sample tree.
 *
 * @author Benjamin Eikel
 * @date 2011-08-01
 */
class RandomizedSampleTreeBuilder : public Builder {
	public:
		explicit RandomizedSampleTreeBuilder() : Builder() {
		}

		/**
		 * Create a randomized sample tree root by extracting geometry from @a mesh.
		 *
		 * @param mesh Mesh containing geometry.
		 * @return Root node of constructed randomized sample tree.
		 * @see RandomizedSampleTree::RandomizedSampleTree()
		 */
		MINSGAPI TriangleTree * buildTriangleTree(Rendering::Mesh * mesh) override;
};

}
}

#endif /* RANDOMIZEDSAMPLETREEBUILDER_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
