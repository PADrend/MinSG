/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#include "Helper.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Util/GenericAttribute.h>
#include <Util/Macros.h>
#include <Util/Numeric.h>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace MinSG {
using namespace VisibilitySubdivision;
namespace VisibilityMerge {
std::unordered_map<list_ptr, costs_t> Helper::runtimeCache;

list_ptr Helper::getVVList(const cell_ptr node) {
	if(node->getValue() == nullptr) {
		WARN("Given node has no value.");
		return nullptr;
	}
	list_ptr gal = dynamic_cast<list_ptr> (node->getValue());
	if(gal == nullptr) {
		WARN("Given node does not have a GenericAttributeList.");
	}
	return gal;
}

const VisibilityVector & Helper::getVV(const Util::GenericAttribute * attribute) {
	const auto * vva = dynamic_cast<const VisibilitySubdivision::VisibilityVectorAttribute *> (attribute);
	if(vva == nullptr) {
		throw std::logic_error("List entry is no VisibilityVectorAttribute.");
	}
	return vva->ref();
}

VisibilityVector & Helper::getVV(Util::GenericAttribute * attribute) {
	auto * vva = dynamic_cast<VisibilitySubdivision::VisibilityVectorAttribute *> (attribute);
	if(vva == nullptr) {
		throw std::logic_error("List entry is no VisibilityVectorAttribute.");
	}
	return vva->ref();
}

cell_set_t Helper::collectVisibilityCells(cell_ptr root) {
	cell_set_t result;
	if(root == nullptr) {
		WARN("nullptr pointer given.");
		return result;
	}
	std::deque<cell_ptr> cells;
	cells.push_back(root);
	while(!cells.empty()) {
		auto cell = cells.front();
		cells.pop_front();
		if(cell->isLeaf()) {
			result.insert(cell);
		} else {
			// Collect children.
			const auto children = getChildNodes(cell);
			for(const auto & child : children) {
				cells.push_back(static_cast<cell_ptr>(child));
			}
		}
	}
	return result;
}

costs_t Helper::getRuntime(const list_ptr list) {
	// Check if value is cached.
	const auto cacheIt = runtimeCache.find(list);
	if(cacheIt != runtimeCache.end()) {
		// Return cached value.
		return cacheIt->second;
	}

	costs_t totalCosts = 0;

	if(list->size() == 1) {
		const auto & vv = getVV(list->front());
		totalCosts = vv.getTotalCosts();
	} else {
		std::vector<object_ptr> visibleObjects;

		// Go over list containing elements for different directions.
		for(const auto & attrib : *list) {
			const auto & vv = getVV(attrib.get());
			const uint32_t maxIndex = vv.getIndexCount();
			for(uint_fast32_t index = 0; index < maxIndex; ++index) {
				if(vv.getBenefits(index) > 0) {
					visibleObjects.push_back(vv.getNode(index));
				}
			}
		}
		// Make sure we use every object only once.
		std::sort(visibleObjects.begin(), visibleObjects.end());
		visibleObjects.erase(std::unique(visibleObjects.begin(), visibleObjects.end()),
							 visibleObjects.end());

		for(const auto & object : visibleObjects) {
			totalCosts += object->getTriangleCount();
		}
	}

	// Insert value into cache.
	runtimeCache.insert(std::make_pair(list, totalCosts));

	return totalCosts;
}

void Helper::clearRuntime(const list_ptr list) {
	runtimeCache.erase(list);
}

VisibilityVector Helper::getMaximumVisibility(const list_ptr list) {
	if(list->size() == 1) {
		return getVV(list->front());
	} else {
		VisibilityVector vv(getVV(list->front()));
		// Go over list containing elements for different directions.
		for(Util::GenericAttributeList::const_iterator it = std::next(list->begin()); it != list->end(); ++it) {
			vv = VisibilityVector::makeMax(vv, getVV(it->get()));
		}
		return vv;
	}
}

VisibilityVector Helper::getMaximumVisibility(const cell_ptr cell) {
	Util::GenericAttributeList * list = getVVList(cell);
	return getMaximumVisibility(list);
}

}
}

#endif /* MINSG_EXT_VISIBILITYMERGE */
