/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION

#include "VisibilityVector.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../SceneManagement/SceneManager.h"
#include <Util/Macros.h>
#include <algorithm>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace MinSG {
namespace VisibilitySubdivision {

// Helper functor
struct NodePtrOnlyCompare {
	bool operator()(const VisibilityVector::node_benefits_pair_t & a, 
					const VisibilityVector::node_benefits_pair_t & b) const {
		return a.first < b.first;
	}
};

bool VisibilityVector::operator==(const VisibilityVector & other) const {
	return visibility == other.visibility;
}

void VisibilityVector::setNode(node_ptr node, benefits_t nodeBenefits) {
	if(node == nullptr) {
		WARN("nullptr pointer given.");
		return;
	}
	if(nodeBenefits == 0) {
		return removeNode(node);
	}
	// Check if the node is already contained.
	NodePtrOnlyCompare comparator;
	const auto testPair = std::make_pair(node, 0);
	const auto lb = std::lower_bound(visibility.begin(), visibility.end(), testPair, comparator);
	if(lb == visibility.end() || comparator(testPair, *lb)) {
		// Insert the node.
		visibility.emplace(lb, node, nodeBenefits);
	} else {
		// Replace the node's value.
		lb->second = nodeBenefits;
	}
}

void VisibilityVector::removeNode(node_ptr node) {
	NodePtrOnlyCompare comparator;
	const auto testPair = std::make_pair(node, 0);
	const auto lb = std::lower_bound(visibility.begin(), visibility.end(), testPair, comparator);
	if(lb != visibility.end() && !comparator(testPair, *lb)) {
		visibility.erase(lb);
	}
}

VisibilityVector::costs_t VisibilityVector::getCosts(node_ptr node) const {
	NodePtrOnlyCompare comparator;
	const auto testPair = std::make_pair(node, 0);
	const auto lb = std::lower_bound(visibility.begin(), visibility.end(), testPair, comparator);
	if(lb == visibility.end() || comparator(testPair, *lb)) {
		return 0;
	} else {
		return node->getTriangleCount();
	}
}

VisibilityVector::benefits_t VisibilityVector::getBenefits(node_ptr node) const {
	NodePtrOnlyCompare comparator;
	const auto testPair = std::make_pair(node, 0);
	const auto lb = std::lower_bound(visibility.begin(), visibility.end(), testPair, comparator);
	if(lb == visibility.end() || comparator(testPair, *lb)) {
		return 0;
	} else {
		return lb->second;
	}
}

VisibilityVector::benefits_t VisibilityVector::increaseBenefits(node_ptr node, 
																benefits_t benefitsIncr) {
	if(node == nullptr) {
		WARN("nullptr pointer given.");
		return 0;
	}
	// Check if the node is contained.
	NodePtrOnlyCompare comparator;
	const auto testPair = std::make_pair(node, 0);
	const auto lb = std::lower_bound(visibility.begin(), visibility.end(), testPair, comparator);
	if(lb == visibility.end() || comparator(testPair, *lb)) {
		// Insert the node.
		visibility.emplace(lb, node, benefitsIncr);
		return 0;
	} else {
		// Increase the node's value.
		const auto oldBenefits = lb->second;
		lb->second += benefitsIncr;
		return oldBenefits;
	}
}

uint32_t VisibilityVector::getIndexCount() const {
	return visibility.size();
}

VisibilityVector::node_ptr VisibilityVector::getNode(uint32_t index) const {
	return visibility[index].first;
}

VisibilityVector::costs_t VisibilityVector::getCosts(uint32_t index) const {
	return visibility[index].first->getTriangleCount();
}

VisibilityVector::benefits_t VisibilityVector::getBenefits(uint32_t index) const {
	return visibility[index].second;
}

VisibilityVector::costs_t VisibilityVector::getTotalCosts() const {
	VisibilityVector::costs_t totalCosts = 0;
	for(const auto & nodeBenefitsPair : visibility) {
		totalCosts += nodeBenefitsPair.first->getTriangleCount();
	}
	return totalCosts;
}

VisibilityVector::benefits_t VisibilityVector::getTotalBenefits() const {	
	VisibilityVector::benefits_t totalBenefits = 0;
	for(const auto & nodeBenefitsPair : visibility) {
		totalBenefits += nodeBenefitsPair.second;
	}
	return totalBenefits;
}

std::size_t VisibilityVector::getVisibleNodeCount() const {
	return visibility.size();
}

VisibilityVector VisibilityVector::makeMin(const VisibilityVector & vv1, const VisibilityVector & vv2) {
	VisibilityVector result;
	result.visibility.reserve(std::min(vv1.visibility.size(), vv2.visibility.size()));
	auto first1 = vv1.visibility.cbegin();
	const auto last1 = vv1.visibility.cend();
	auto first2 = vv2.visibility.cbegin();
	const auto last2 = vv2.visibility.cend();
	while(first1 != last1 && first2 != last2) {
		if(first1->first < first2->first) {
			++first1;
		} else if(first2->first < first1->first) {
			++first2;
		} else {
			result.visibility.emplace_back(first1->first,
										   std::min(first1->second, first2->second));
			++first1;
			++first2;
		}
	}
	return result;
}

VisibilityVector VisibilityVector::makeMax(const VisibilityVector & vv1, const VisibilityVector & vv2) {
	VisibilityVector result;
	result.visibility.reserve(vv1.visibility.size() + vv2.visibility.size());
	auto first1 = vv1.visibility.cbegin();
	const auto last1 = vv1.visibility.cend();
	auto first2 = vv2.visibility.cbegin();
	const auto last2 = vv2.visibility.cend();
	while(first1 != last1 && first2 != last2) {
		if(first1->first < first2->first) {
			result.visibility.emplace_back(*first1);
			++first1;
		} else if(first2->first < first1->first) {
			result.visibility.emplace_back(*first2);
			++first2;
		} else {
			result.visibility.emplace_back(first1->first,
										   std::max(first1->second, first2->second));
			++first1;
			++first2;
		}
	}
	result.visibility.insert(result.visibility.end(), first1, last1);
	result.visibility.insert(result.visibility.end(), first2, last2);
	return result;
}

VisibilityVector VisibilityVector::makeDifference(const VisibilityVector & vv1, const VisibilityVector & vv2) {
	VisibilityVector result;
	std::set_difference(vv1.visibility.begin(), vv1.visibility.end(), vv2.visibility.begin(), vv2.visibility.end(), std::back_inserter(result.visibility), NodePtrOnlyCompare());
	return result;
}

VisibilityVector VisibilityVector::makeSymmetricDifference(const VisibilityVector & vv1, const VisibilityVector & vv2) {
	VisibilityVector result;
	std::set_symmetric_difference(vv1.visibility.begin(), vv1.visibility.end(), vv2.visibility.begin(), vv2.visibility.end(), std::back_inserter(result.visibility), NodePtrOnlyCompare());
	return result;
}

template<typename container_t>
static void makeWeightedOne(float wA,
							std::vector<VisibilityVector::node_benefits_pair_t>::const_iterator & a,
							std::vector<VisibilityVector::node_benefits_pair_t>::const_iterator & aEnd,
							container_t & result) {
	// Add the elements of the single map.
	while(a != aEnd) {
		const VisibilityVector::benefits_t weightedBenefits = (wA * a->second);
		if(weightedBenefits > 0) {
			result.emplace_back(a->first, weightedBenefits);
		}
		++a;
	}
}

template<typename container_t>
static void makeWeightedTwo(float wA,
							std::vector<VisibilityVector::node_benefits_pair_t>::const_iterator & a,
							std::vector<VisibilityVector::node_benefits_pair_t>::const_iterator & aEnd,
							float wB,
							std::vector<VisibilityVector::node_benefits_pair_t>::const_iterator & b,
							std::vector<VisibilityVector::node_benefits_pair_t>::const_iterator & bEnd,
							container_t & result) {
	// Compare elements of the two maps.
	while(a != aEnd && b != bEnd) {
		if(a->first < b->first) {           // Object is only in map of vvA.
			const VisibilityVector::benefits_t weightedBenefits = (wA * a->second);
			if(weightedBenefits > 0) {
				result.emplace_back(a->first, weightedBenefits);
			}
			++a;
		} else if(b->first < a->first) {    // Object is only in map of vvB.
			const VisibilityVector::benefits_t weightedBenefits = (wB * b->second);
			if(weightedBenefits > 0) {
				result.emplace_back(b->first, weightedBenefits);
			}
			++b;
		} else {                            // Object is in both maps.
			const VisibilityVector::benefits_t weightedBenefits = (wA * a->second + wB * b->second);
			if(weightedBenefits > 0) {
				result.emplace_back(a->first, weightedBenefits);
			}
			++a;
			++b;
		}
	}
	if(a == aEnd) {
		makeWeightedOne(wB, b, bEnd, result);
	} else {
		makeWeightedOne(wA, a, aEnd, result);
	}
}

VisibilityVector VisibilityVector::makeWeightedThree(float w1, const VisibilityVector & vv1,
													 float w2, const VisibilityVector & vv2,
													 float w3, const VisibilityVector & vv3) {
	// The visibility entries are always sorted.
	// Make use of this here and compare them in linear time.
	auto it1(vv1.visibility.cbegin());
	auto end1(vv1.visibility.cend());
	auto it2(vv2.visibility.cbegin());
	auto end2(vv2.visibility.cend());
	auto it3(vv3.visibility.cbegin());
	auto end3(vv3.visibility.cend());

	VisibilityVector result;
	result.visibility.reserve(vv1.visibility.size() + vv2.visibility.size() + vv3.visibility.size());

	// Compare elements of the three maps.
	while(it1 != end1 && it2 != end2 && it3 != end3) {
		const auto & a = it1->first;
		const auto & b = it2->first;
		const auto & c = it3->first;
		if(a < b && a < c) {            // a is only in vv1.
			const benefits_t weightedBenefits = (w1 * it1->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it1->first, weightedBenefits);
			}
			++it1;
		} else if(b < a && b < c) {     // b is only in vv2.
			const benefits_t weightedBenefits = (w2 * it2->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it2->first, weightedBenefits);
			}
			++it2;
		} else if(c < a && c < b) {     // c is only in vv3.
			const benefits_t weightedBenefits = (w3 * it3->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it3->first, weightedBenefits);
			}
			++it3;
		} else if(a == b && a < c) {    // a is in vv1 and vv2.
			const benefits_t weightedBenefits = (w1 * it1->second + w2 * it2->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it1->first, weightedBenefits);
			}
			++it1;
			++it2;
		} else if(a == c && a < b) {    // a is in vv1 and vv3.
			const benefits_t weightedBenefits = (w1 * it1->second + w3 * it3->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it1->first, weightedBenefits);
			}
			++it1;
			++it3;
		} else if(b == c && b < a) {    // b is in vv2 and vv3.
			const benefits_t weightedBenefits = (w2 * it2->second + w3 * it3->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it2->first, weightedBenefits);
			}
			++it2;
			++it3;
		} else {                        // a is in vv1, vv2, and vv3.
			const benefits_t weightedBenefits = (w1 * it1->second + w2 * it2->second + w3 * it3->second);
			if(weightedBenefits > 0) {
				result.visibility.emplace_back(it1->first, weightedBenefits);
			}
			++it1;
			++it2;
			++it3;
		}
	}
	// Compare elements of only two maps.
	if(it1 == end1) {
		makeWeightedTwo(w2, it2, end2, w3, it3, end3, result.visibility);
	} else if(it2 == end2) {
		makeWeightedTwo(w1, it1, end1, w3, it3, end3, result.visibility);
	} else {
		makeWeightedTwo(w1, it1, end1, w2, it2, end2, result.visibility);
	}

	return result;
}

void VisibilityVector::diff(const VisibilityVector & vv1, const VisibilityVector & vv2,
						  costs_t & costsDiff, benefits_t & benefitsDiff, std::size_t & sameCount) {
	costsDiff = 0;
	benefitsDiff = 0;
	sameCount = 0;

	// The arrays are always sorted.
	// Make use of this here and compare them in linear time.
	auto a(vv1.visibility.cbegin());
	auto aEnd(vv1.visibility.cend());
	auto b(vv2.visibility.cbegin());
	auto bEnd(vv2.visibility.cend());

	while(a != aEnd && b != bEnd) {
		const costs_t costsA = a->first->getTriangleCount();
		const costs_t costsB = b->first->getTriangleCount();
		if(a->first < b->first) {
			// a->first is only in one list.
			costsDiff += costsA;
			benefitsDiff += a->second;
			++a;
		} else if(b->first < a->first) {
			// b->first is only in one list.
			costsDiff += costsB;
			benefitsDiff += b->second;
			++b;
		} else {
			// Both are the same.
			++sameCount;
			// Compare here first because of unsigned types.
			if(costsA > costsB) {
				costsDiff += costsA - costsB;
			} else {
				costsDiff += costsB - costsA;
			}
			if(a->second > b->second) {
				benefitsDiff += a->second - b->second;
			} else {
				benefitsDiff += b->second - a->second;
			}
			++a;
			++b;
		}
	}
	while(a != aEnd) {
		// a->first is only in one list.
		costsDiff += a->first->getTriangleCount();
		benefitsDiff += a->second;
		++a;
	}
	while(b != bEnd) {
		// b->first is only in one list.
		costsDiff += b->first->getTriangleCount();
		benefitsDiff += b->second;
		++b;
	}
}

std::string VisibilityVector::toString() const {
	std::ostringstream out;
	out << "VisibilityVector {";
	for(const auto & nodeBenefitsPair : visibility) {
		out << '\n' << '\t' << nodeBenefitsPair.first << "\t|--->\t(" << nodeBenefitsPair.first->getTriangleCount() << ", " << nodeBenefitsPair.second << ')';
	}
	out << "}\n";
	return out.str();
}

size_t VisibilityVector::getMemoryUsage() const {
	return	sizeof(VisibilityVector) +
			visibility.size() * sizeof(node_benefits_pair_t);
}

void VisibilityVector::serialize(std::ostream & out,
								 const SceneManagement::SceneManager & sceneManager) const {
	out << visibility.size();
	for(const auto & nodeBenefitsPair : visibility) {
		const std::string nodeName = sceneManager.getNameOfRegisteredNode(nodeBenefitsPair.first);
		if(nodeName == std::string()) {
			WARN("Could not retrieve the name of a node: Possibly the node has not been registered at the scene manager.");
		}
		out << ' ' << nodeName << ' ' << nodeBenefitsPair.second;
	}
}

VisibilityVector VisibilityVector::unserialize(std::istream & in,
											   const SceneManagement::SceneManager & sceneManager) {
	uint32_t numEntries;
	in >> numEntries;
	VisibilityVector vv;
	vv.visibility.reserve(numEntries);
	for(uint_fast32_t entry = 0; entry < numEntries; ++entry) {
		std::string name;
		VisibilityVector::benefits_t benefits;
		in >> name >> benefits;
		if(benefits > 0) {
			VisibilityVector::node_ptr node = dynamic_cast<VisibilityVector::node_ptr>(sceneManager.getRegisteredNode(name));
			if(node == nullptr) {
				WARN("Could not retrieve the node with a given name: Possibly the node has not been registered at the scene manager.");
			}
			vv.visibility.emplace_back(node, benefits);
		}
	}
	std::sort(vv.visibility.begin(), vv.visibility.end());
	return vv;
}

}
}

#endif // MINSG_EXT_VISIBILITY_SUBDIVISION
