/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_AGVS_MUTATIONCANDIDATES_H
#define MINSG_AGVS_MUTATIONCANDIDATES_H

#include "Definitions.h"
#include <memory>
#include <random>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
}
namespace MinSG {
class GroupNode;
class ValuatedRegionNode;
namespace AGVS {
template<typename value_t> class MutationCandidate;
template<typename value_t> class Sample;

//! Manager for mutation candidates needed by mutation-based distributions.
class MutationCandidates {
	private:
		// Use Pimpl idiom
		template<typename value_t> struct Implementation;
		std::unique_ptr<Implementation<float>> impl;

	public:
		MINSGAPI MutationCandidates();
		MINSGAPI ~MutationCandidates();

		/**
		 * Add a sample that has contributed at least one object to view cells.
		 * Depending on its contribution, one or two mutation candidates are
		 * created.
		 * 
		 * @see section 4.4 "Mutation-Based Distributions"
		 * @param sample Sample that is the source for a new mutation candidate
		 * @param contribution The contribution of the sample
		 * @param viewCell View cell containing the origin of the sample
		 */
		MINSGAPI void addMutationCandidate(const Sample<float> & sample,
								  const contribution_t & contribution,
								  ValuatedRegionNode * viewCell);

		//! Return @c true if there are no mutation candidates available.
		MINSGAPI bool isEmpty() const;

		//! Return a mutation candidate with minimum number of mutations.
		MINSGAPI const MutationCandidate<float> & getMutationCandidate();
};

}
}

#endif /* MINSG_AGVS_MUTATIONCANDIDATES_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
