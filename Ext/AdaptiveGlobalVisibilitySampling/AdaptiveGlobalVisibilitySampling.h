/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_AGVS_ADAPTIVEGLOBALVISIBILITYSAMPLING_H
#define MINSG_AGVS_ADAPTIVEGLOBALVISIBILITYSAMPLING_H

#include <cstdint>
#include <memory>

namespace Rendering {
class Mesh;
}
namespace MinSG {
class GroupNode;
class ValuatedRegionNode;
namespace AGVS {

/**
 * Implementation of Adaptive Global Visibility Sampling.
 * 
 * Based on the article:
 * Jiri Bittner, Oliver Mattausch, Peter Wonka, Vlastimil Havran, Michael
 * Wimmer: Adaptive global visibility sampling.
 * ACM Transactions on Graphics, 28, 3, article 94, 2009.
 * 
 * @note This implementation is based on the article only. No existing code was
 * used for this implementation.
 * @author Benjamin Eikel
 * @date 2013-02-22
 */
class AdaptiveGlobalVisibilitySampling {
	private:
		// Use Pimpl idiom
		template<typename value_t> struct Implementation;
		std::unique_ptr<Implementation<float>> impl;

	public:
		/**
		 * Create a new preprocessing instance for the given scene
		 * 
		 * @param scene Scene that will be used to perform the global visibility
		 * sampling in.
		 * @param viewSpaceSubdivision Root node of the view cell hierarchy
		 */
		MINSGAPI AdaptiveGlobalVisibilitySampling(GroupNode * scene,
										 ValuatedRegionNode * viewSpaceSubdivision);

		//! Standard destructor: Free resources
		MINSGAPI ~AdaptiveGlobalVisibilitySampling();

		/**
		 * Perform the sampling by creating the given number of samples. For
		 * every samples, two rays will be cast.
		 * 
		 * @param numSamples Number of samples to evaluate
		 * @return If @c true, the pixel error is small and the sampling can be
		 * terminated. Otherwise, the sampling should be continued.
		 */
		MINSGAPI bool performSampling(uint32_t numSamples);

		/**
		 * Create a mesh from the samples that can be used as visualization of
		 * the casted rays. For every sample that contributes at least one
		 * visible object, a line is created. Samples with contribution exactly
		 * one will be colored blue, samples with contribution two are colored
		 * cyan.
		 * 
		 * @return New mesh. The caller should store it in a Util::Reference.
		 */
		MINSGAPI Rendering::Mesh * createMeshFromSamples() const;

		//! Return the root of the view cell hierarchy.
		MINSGAPI ValuatedRegionNode * getViewCellHierarchy() const;
};

}
}

#endif /* MINSG_AGVS_ADAPTIVEGLOBALVISIBILITYSAMPLING_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
