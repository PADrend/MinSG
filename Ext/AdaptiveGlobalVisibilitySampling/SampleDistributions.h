/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_AGVS_SAMPLEDISTRIBUTIONS_H
#define MINSG_AGVS_SAMPLEDISTRIBUTIONS_H

#include "Definitions.h"
#include <memory>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace MinSG {
class GroupNode;
class ValuatedRegionNode;
namespace AGVS {
template<typename value_t> class Sample;

//! Class for generating samples based on different distributions.
class SampleDistributions {
	private:
		// Use Pimpl idiom
		template<typename value_t> struct Implementation;
		std::unique_ptr<Implementation<float>> impl;

	public:
		MINSGAPI SampleDistributions(const Geometry::Box & viewSpaceBounds,
							const GroupNode * scene);
		MINSGAPI ~SampleDistributions();
		
		/**
		 * Generate a sample from a random sample distribution.
		 * 
		 * @see section 4.2 "Adaptive Mixture Distribution"
		 */
		MINSGAPI Sample<float> generateSample() const;

		/**
		 * Use the contribution of a sample to update the contribution of the
		 * sample distributions.
		 * 
		 * @param sample New sample used to update the view cells
		 * @param contribution Pair of forward and backward contribution of the
		 * sample
		 * @param viewCell View cell containing the origin of the sample
		 */
		MINSGAPI void updateWithSample(const Sample<float> & sample,
							  const contribution_t & contribution,
							  ValuatedRegionNode * viewCell);

		/**
		 * Update the probabilities used to select the sample distribution.
		 * 
		 * @see section 4.2 "Adaptive Mixture Distribution"
		 */
		MINSGAPI void calculateDistributionProbabilities();

		/**
		 * Estimate the pixel error. Check if the estimated pixel error is below
		 * a treshold and the probability that it is similar to the real error
		 * is high.
		 * 
		 * @see section 4.5 "Termination of the computation"
		 * @return @c true if the sampling is finished, @c false otherwise
		 */
		MINSGAPI bool terminate() const;
};

}
}

#endif /* MINSG_AGVS_SAMPLEDISTRIBUTIONS_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
