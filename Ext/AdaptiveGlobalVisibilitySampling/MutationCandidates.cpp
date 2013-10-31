/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#include "MutationCandidates.h"
#include "Definitions.h"
#include "MutationCandidate.h"
#include "Sample.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../../Core/Nodes/GeometryNode.h"
#include <Geometry/Box.h>
#include <Geometry/Line.h>
#include <Geometry/Vec3.h>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <stdexcept>

namespace MinSG {
namespace AGVS {

template<typename value_t>
struct MutationCandidates::Implementation {
	std::deque<MutationCandidate<value_t>> mutationCandidates;

	Implementation() :
		mutationCandidates() {
	}

	void addMutationCandidate(const Sample<value_t> & sample,
							  const contribution_t & contribution,
							  ValuatedRegionNode * viewCell) {
		/*
		 * Insert new mutation candidates at the front because their mutation
		 * count is zero. Therefore, the array stays sorted.
		 */
		const bool useOrigin = (sample.getNumHits() < 2);
		if(std::get<0>(contribution) > 0) {
			const auto originPoint = useOrigin ? sample.getOrigin() :
												 sample.getBackwardTerminationPoint();
			Node * originObject = useOrigin ? static_cast<Node *>(viewCell) :
											  sample.getBackwardResult();
			const auto terminationPoint = sample.getForwardTerminationPoint();
			if(originPoint.distanceSquared(terminationPoint) > 1.0e-3) {
				mutationCandidates.emplace_front(originPoint,
												 originObject,
												 terminationPoint,
												 sample.getForwardResult());
			}
		}
		if(std::get<1>(contribution) > 0) {
			const auto originPoint = useOrigin ? sample.getOrigin() :
												 sample.getForwardTerminationPoint();
			Node * originObject = useOrigin ? static_cast<Node *>(viewCell) :
											  sample.getForwardResult();
			const auto terminationPoint = sample.getBackwardTerminationPoint();
			if(originPoint.distanceSquared(terminationPoint) > 1.0e-3) {
				mutationCandidates.emplace_front(originPoint,
												 originObject,
												 terminationPoint,
												 sample.getBackwardResult());
			}
		}
		// Constant suggested in the article
		const uint32_t maxNumMutationCandidates = 2000000;
		while(mutationCandidates.size() > maxNumMutationCandidates) {
			mutationCandidates.pop_back();
		}
	}

	static bool mutationCountLess(const MutationCandidate<value_t> & candidate,
								  uint32_t compareMutationCount) {
		return candidate.mutationCount < compareMutationCount;
	}

	const MutationCandidate<value_t> & getMutationCandidate() {
		if(mutationCandidates.empty()) {
			throw std::logic_error("No mutation candidates available.");
		}
		uint32_t & mutationCount = mutationCandidates.front().mutationCount;
		++mutationCount;

		// Find the first element with a value not less the new mutationCount
		auto firstElem = std::lower_bound(mutationCandidates.begin(),
										  mutationCandidates.end(),
										  mutationCount,
										  mutationCountLess);
		if(firstElem == mutationCandidates.begin()) {
			// If the only entry is the first one itself, nothing has to be done
			return mutationCandidates.front();
		}
		auto swapPosition = std::prev(firstElem);
		std::iter_swap(swapPosition, mutationCandidates.begin());
		return *swapPosition;
	}
};

MutationCandidates::MutationCandidates() :
	impl(new Implementation<float>) {
}

MutationCandidates::~MutationCandidates() = default;

void MutationCandidates::addMutationCandidate(const Sample<float> & sample,
											  const contribution_t & contribution,
											  ValuatedRegionNode * viewCell) {
	impl->addMutationCandidate(sample, contribution, viewCell);
}

bool MutationCandidates::isEmpty() const {
	return impl->mutationCandidates.empty();
}

const MutationCandidate<float> & MutationCandidates::getMutationCandidate() {
	return impl->getMutationCandidate();
}

}
}

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
