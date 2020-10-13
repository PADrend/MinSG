/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#ifndef RANDOMIZEDSAMPLETREE_H_
#define RANDOMIZEDSAMPLETREE_H_

#include "Octree.h"

#include <functional>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace Rendering {
class Mesh;
}
namespace MinSG {
namespace TriangleTrees {

/**
 * Randomized Sample Tree as presented in
 * Klein, J.; Krokowski, J.; Fischer, M.; Wand, M.; Wanka, R.; Meyer auf der Heide, F.:
 * The randomized sample tree: a data structure for interactive walkthroughs in externally stored virtual environments.
 * In: VRST '02: Proceedings of the ACM symposium on Virtual reality software and technology, ACM, 2002, pages 137-146.
 *
 * @author Benjamin Eikel
 * @date 2011-08-01
 */
class RandomizedSampleTree : public Octree {
	public:
		/**
		 * Create an RandomizedSampleTree root node with TriangleAccessors to all given triangles.
		 *
		 * @param mesh Mesh containing the triangles.
		 */
		explicit RandomizedSampleTree(Rendering::Mesh * mesh) : Octree(mesh, 1, 2.0f) {
		}

		//! Empty destructor.
		virtual ~RandomizedSampleTree() {
		}

		/**
		 * Perform the sampling process.
		 * This function has to be called for the root node of the tree.
		 * Traverse the whole tree and pull up triangles by random sampling.
		 *
		 * @see Figure 3 of the original article
		 */
		MINSGAPI void createSample();
    
        MINSGAPI void createSample(const std::function<double(const TriangleAccessor &)> & calcTriangleWeight);

	protected:
		/**
		 * Create a new RandomizedSampleTree node. This is used to create child
		 * nodes. The creating node has to assign the triangles to the
		 * node.
		 *
		 * @param childBound Axis-aligned bounding box for the child.
		 * @param parent Parent node which is used to copy the parameters from.
		 */
		explicit RandomizedSampleTree(const Geometry::Box & childBound, const Octree & parent) : Octree(childBound, parent) {
		}

		//! Return a child node. Needed for polymorphism.
		RandomizedSampleTree * createChild(const Geometry::Box & childBound, const Octree & parent) const override {
			return new RandomizedSampleTree(childBound, parent);
		}

		/**
		 * Calculate the sample size for this node.
		 *
		 * @see m_u in Theorem 1 of the original article
		 * @param sumTriangleAreas Sum of triangle areas
		 * @return Sample size
		 */
		MINSGAPI double calcSampleSize(double sumTriangleAreas) const;
};

}
}

#endif /* RANDOMIZEDSAMPLETREE_H_ */

#endif /* MINSG_EXT_TRIANGLETREES */
