/*
	This file is part of the MinSG library extension VisibilityMerge.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITYMERGE

#include "VisibilityMerge.h"
#include "Helper.h"
#include "Statistics.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../ValuatedRegion/ValuatedRegionNode.h"
#include "../VisibilitySubdivision/VisibilityVector.h"
#include "../../SceneManagement/SceneManager.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/MeshUtils/MeshUtils.h>
#include <Util/GenericAttribute.h>
#include <Util/Timer.h>
#include <Util/Macros.h>
#include <Util/Utils.h>
#include <limits>
#include <map>
#include <unordered_set>

using namespace Rendering;

namespace MinSG {
using namespace VisibilitySubdivision;
namespace VisibilityMerge {

std::pair<ValuatedRegionNode *, ListNode *> VisibilityMerge::run(SceneManagement::SceneManager * mgr, cell_ptr root, const size_t wsSize, const costs_t b_D,
																	const costs_t b_SPO, const costs_t b_SPZ) {
	// Sorted set which will be filled with all objects in the scene.
	sorted_object_set_t objects;
	/*
	 * Mapping from an object to a list of visibility cells that the
	 * object is visible from.
	 */
	reverse_map_t reverseVis;

	// Set which will be filled with all visibility cells.
	auto cells = Helper::collectVisibilityCells(root);
	createReverseMapping(cells, objects, reverseVis);


	// Remove objects from the scene graph.
	// This is done solely to use less memory overall.
	for(const auto & object : objects) {
		// Add reference because the object is kept in the set now.
		Node::addReference(object);

		GroupNode * parent = object->getParent();
		if(parent != nullptr) {
			parent->removeChild(object);
		}
		mgr->unregisterNode(mgr->getNameOfRegisteredNode(object));
	}

	visibility_sharer_map_t lists;
	for(const auto & cell : cells) {
		list_ptr list = Helper::getVVList(cell);
		cell_set_t cellSet;
		cellSet.insert(cell);
		lists.insert(std::make_pair(list, cellSet));
	}

#ifdef MINSG_PROFILING
	const std::string timeStamp = Util::Utils::createTimeStamp();
	std::string prefix("visibility_merge_" + timeStamp + '_');
	Statistics listOutput(prefix + "list.tsv");
	Statistics cellOutput(prefix + "cell.tsv");
	Statistics objectOutput(prefix + "object.tsv");
	Statistics passOutput(prefix + "pass.tsv");

	Util::Timer timer;
#endif
	size_t numMerges;
	do {
#ifdef MINSG_PROFILING
		listOutput.perList(lists);
		cellOutput.perCell(cells);
		objectOutput.perObject(objects);
		timer.reset();
#endif
		numMerges = objectSpaceReduce(objects, reverseVis, wsSize, b_D, b_SPO);
#ifdef MINSG_PROFILING
		timer.stop();
		passOutput.perPass(timer.getMicroseconds(), numMerges);
#endif
	} while (numMerges > 0);

#ifdef MINSG_PROFILING
	Statistics::perRun(prefix + "run.tsv", timeStamp, "", wsSize, b_D, b_SPO, b_SPZ, passOutput.getPass());
#endif

	do {
#ifdef MINSG_PROFILING
		listOutput.perList(lists);
		cellOutput.perCell(cells);
		objectOutput.perObject(objects);
		timer.reset();
#endif
		numMerges = viewSpaceReduceGlobal(lists, wsSize, b_SPZ);
#ifdef MINSG_PROFILING
		timer.stop();
		passOutput.perPass(timer.getMicroseconds(), numMerges);
#endif
	} while (numMerges > 0);

	// Create new visibility subdivision root node.
	Geometry::Box bound;
	for(const auto & cell : cells) {
		bound.include(cell->getBB());
	}
	auto newVSRoot = new ValuatedRegionNode(bound, Geometry::Vec3i(1, 1, 1));
	for(const auto & cell : cells) {
		newVSRoot->addChild(cell->clone());
	}
	// Create new scene graph root node.
	auto newSGRoot = new ListNode;
	for(const auto & object : objects) {
		newSGRoot->addChild(object);
		Node::removeReference(object);
	}
	return std::make_pair(newVSRoot, newSGRoot);
}

void VisibilityMerge::createReverseMapping(const cell_set_t & cells, sorted_object_set_t & objects, reverse_map_t & reverseMap) {
	for(const auto & cell : cells) {
		list_ptr gal = Helper::getVVList(cell);
		// Go over list containing elements for different directions.
		for(const auto & vvAttrib : *gal) {
			const auto & vv = Helper::getVV(vvAttrib.get());
			const uint32_t maxIndex = vv.getIndexCount();
			for(uint_fast32_t index = 0; index < maxIndex; ++index) {
				if(vv.getBenefits(index) == 0) {
					continue;
				}
				object_ptr geoNode = vv.getNode(index);
				// Add node to set of objects.
				objects.insert(geoNode);

				// Check if object exists.
				auto reverseIt = reverseMap.find(geoNode);
				if(reverseIt != reverseMap.end()) {
					// Append the visibility region.
					reverseIt->second.insert(cell);
				} else {
					// Insert new object with an empty list.
					cell_set_t newSet;
					newSet.insert(cell);
					reverseMap.insert(std::make_pair(geoNode, newSet));
				}
			}
		}
	}
}

costs_volume_t VisibilityMerge::getMergeCosts(object_ptr o_i, object_ptr o_j, reverse_map_t & reverseMap) {
	const auto reverse_i = reverseMap.find(o_i);
	auto it_i = reverse_i->second.cbegin();
	const auto end_i = reverse_i->second.cend();
	const auto reverse_j = reverseMap.find(o_j);
	auto it_j = reverse_j->second.cbegin();
	const auto end_j = reverse_j->second.cend();

	const auto costs_i = o_i->getTriangleCount();
	const auto costs_j = o_j->getTriangleCount();
	costs_volume_t costs = 0.0f;

	while (it_i != end_i && it_j != end_j) {
		if(*it_i < *it_j) {
			// *it_i only sees o_i
			// *it_i will get additional o_j
			costs += costs_j * (*it_i)->getBB().getVolume();
			++it_i;
		} else if(*it_j < *it_i) {
			// *it_j only sees o_j
			// *it_j will get additional o_i
			costs += costs_i * (*it_j)->getBB().getVolume();
			++it_j;
		} else {
			// *it_i == *it_j sees both o_i and o_j
			++it_i;
			++it_j;
		}
	}
	while (it_i != end_i) {
		// All cells only see o_i
		// and will get additional o_j
		costs += costs_j * (*it_i)->getBB().getVolume();
		++it_i;
	}
	while (it_j != end_j) {
		// All cells only see o_j
		// and will get additional o_j
		costs += costs_i * (*it_j)->getBB().getVolume();
		++it_j;
	}

	return costs;
}

size_t VisibilityMerge::objectSpaceReduce(sorted_object_set_t & objectSpace, reverse_map_t & reverseMap, const size_t wsSize, const costs_t b_D,
											const costs_t b_SPO) {
	// Check if objects are big enough already
	// and object space is small enough.
	if((*objectSpace.begin())->getTriangleCount() >= b_D && objectSpace.size() <= b_SPO) {
		return 0;
	}
	// Get end of working set.
	auto wsEnd = objectSpace.begin();
	size_t wsCount = 0;
	while (wsEnd != objectSpace.end()) {
		// Check if the working set size is reached.
		if(wsSize == wsCount) {
			break;
		}
		++wsCount;
		++wsEnd;
	}
	// We need two objects to merge.
	if(wsCount < 2) {
		return 0;
	}

	// Extract working set.
	sorted_object_set_t ws(objectSpace.begin(), wsEnd);
	objectSpace.erase(objectSpace.begin(), wsEnd);

	std::cout << "WS[" << ws.size() << "](" << (*(ws.begin()))->getTriangleCount() << " -- " << (*(ws.rbegin()))->getTriangleCount() << ")";
	if(objectSpace.empty()) {
		std::cout << "()[0]OS" << std::endl;
	} else {
		std::cout << "(" << (*(objectSpace.begin()))->getTriangleCount() << " -- " << (*(objectSpace.rbegin()))->getTriangleCount() << ")["
				<< objectSpace.size() << "]OS" << std::endl;
	}

	// Map of additional costs induced when merging the two objects.
	std::multimap<costs_volume_t, std::pair<object_ptr, object_ptr> > mergeCosts;
	// Go over all object pairs in working set.
	for(auto first = ws.cbegin(); first != ws.cend(); ++first) {
		auto second = first;
		++second;
		for(; second != ws.cend(); ++second) {
			costs_volume_t costs = getMergeCosts(*first, *second, reverseMap);
			mergeCosts.insert(std::make_pair(costs, std::make_pair(*first, *second)));
		}
	}

	std::cout << "Number of possible merges: " << mergeCosts.size() << std::endl;
	std::cout << "Best merge: " << mergeCosts.begin()->first << std::endl;
	std::cout << "Worst merge: " << mergeCosts.rbegin()->first << std::endl;


	// Set of objects which have not been merged yet.
	std::unordered_set<object_ptr> notMerged;
	notMerged.insert(ws.begin(), ws.end());

	size_t mergesDone = 0;
	for(const auto & mergeCostsPair : mergeCosts) {
		object_ptr o_i = mergeCostsPair.second.first;
		object_ptr o_j = mergeCostsPair.second.second;

		auto notMerged_i = notMerged.find(o_i);
		auto notMerged_j = notMerged.find(o_j);
		if(notMerged_i != notMerged.end() && notMerged_j != notMerged.end()) {
			object_ptr o_z = VisibilityMerge::mergeObjects(o_i, o_j, reverseMap);
			Node::addReference(o_z);
			objectSpace.insert(o_z);

			notMerged.erase(notMerged_i);
			notMerged.erase(notMerged_j);

			Node::removeReference(o_i);
			Node::removeReference(o_j);

			++mergesDone;
		}
	}

	// Add all objects which were not merged.
	objectSpace.insert(notMerged.cbegin(), notMerged.cend());
	return mergesDone;
}

object_ptr VisibilityMerge::mergeObjects(object_ptr o_i, object_ptr o_j, reverse_map_t & reverseMap) {
	// Join meshes.
	std::deque<Mesh *> meshes;
	meshes.push_back(o_i->getMesh());
	meshes.push_back(o_j->getMesh());
	Mesh * newMesh = Rendering::MeshUtils::combineMeshes(meshes);

	auto o_z = new GeometryNode;
	o_z->setMesh(newMesh);
	//			o_z->setVBOWrapper(new VBOWrapper(newMesh));
	// Collect visibility vectors referencing the two objects.
	std::set<VisibilityVector *> vecs;
	// Search visibility vectors in cells that see o_i.
	const auto reverse_i = reverseMap.find(o_i);
	FAIL_IF(reverse_i == reverseMap.cend());
	for(const auto & cell : reverse_i->second) {
		list_ptr gal = Helper::getVVList(cell);
		// Go over list containing elements for different directions.
		for(const auto & element : *gal) {
			vecs.insert(&Helper::getVV(element.get()));
		}
	}
	// Search visibility vectors in cells that see o_j.
	const auto reverse_j = reverseMap.find(o_j);
	FAIL_IF (reverse_j == reverseMap.cend());
	for(const auto & cell : reverse_j->second) {
		list_ptr gal = Helper::getVVList(cell);
		// Go over list containing elements for different directions.
		for(const auto & element : *gal) {
			vecs.insert(&Helper::getVV(element.get()));
		}
	}
	// Really update references here.
	for(const auto & vv : vecs) {
		vv->setNode(o_z, vv->getBenefits(o_i) + vv->getBenefits(o_j));
		vv->removeNode(o_i);
		vv->removeNode(o_j);
	}
	// Update reverse references.
	cell_set_t cells;
	std::set_union(reverse_i->second.cbegin(), reverse_i->second.cend(), reverse_j->second.cbegin(), reverse_j->second.cend(), std::inserter(cells, cells.begin()));
	// Invalidate cached runtime values.
	for(const auto & cell : cells) {
		Helper::clearRuntime(cell);
	}
	reverseMap.insert(std::make_pair(o_z, cells));
	reverseMap.erase(reverse_i);
	reverseMap.erase(reverse_j);
	return o_z;
}

costs_volume_t VisibilityMerge::getMergeScoreLists(const visibility_sharer_map_t::const_iterator & l_i, const visibility_sharer_map_t::const_iterator & l_j) {
	const auto vv_i = Helper::getMaximumVisibility(l_i->first);
	const auto vv_j = Helper::getMaximumVisibility(l_j->first);

	VisibilityVector::costs_t additionalRuntime;
	VisibilityVector::benefits_t benefits;
	std::size_t sameObjects;
	VisibilityVector::diff(vv_i, vv_j, additionalRuntime, benefits, sameObjects);

	float volume_i = 0.0f;
	for(const auto & cell : l_i->second) {
		volume_i += cell->getBB().getVolume();
	}
	float volume_j = 0.0f;
	for(const auto & cell : l_j->second) {
		volume_j += cell->getBB().getVolume();
	}

	costs_volume_t score = additionalRuntime * (volume_i + volume_j);

	if(sameObjects > 1) {
		// Prefer merges where the number of references decreases more.
		score /= static_cast<costs_volume_t>(sameObjects);
	}

	return score;
}

size_t VisibilityMerge::viewSpaceReduceGlobal(visibility_sharer_map_t & sharer, const size_t wsSize, const costs_t b_SPZ) {
	if(sharer.size() <= b_SPZ) {
		return 0;
	}

	// Sort the working set by costs.
	std::vector<std::pair<costs_t, list_ptr>> ws;
	ws.reserve(sharer.size());
	for(const auto & listCellsPair : sharer) {
		ws.emplace_back(Helper::getRuntime(listCellsPair.first), listCellsPair.first);
	}
	std::sort(ws.begin(), ws.end());
	// Reduce the working set to its desired size.
	if(ws.size() > wsSize) {
		ws.resize(wsSize);
	}

	// Map of scores for merging two lists.
	std::multimap<costs_volume_t, std::pair<list_ptr, list_ptr> > mergeScores;
	// Go over all lists and search the possible merge candidates.
	for(auto a = ws.cbegin(); a != ws.cend(); ++a) {
		const auto it_i = sharer.find(a->second);
		auto b = std::next(a);
		for(; b != ws.cend(); ++b) {
			const auto it_j = sharer.find(b->second);
			costs_volume_t score = getMergeScoreLists(it_i, it_j);
			mergeScores.insert(std::make_pair(score, std::make_pair(it_i->first, it_j->first)));
		}
	}

	std::cout << "Number of possible merges: " << mergeScores.size() << std::endl;
	if(!mergeScores.empty()) {
		std::cout << "Best score: " << mergeScores.begin()->first << std::endl;
		std::cout << "Worst score: " << mergeScores.rbegin()->first << std::endl;
	}

	size_t lowerQuartile = mergeScores.size() / 4;
	size_t mergesDone = 0;
	for(const auto & mergeScore : mergeScores) {
		// Stop if score value is not good enough anymore.
		if(mergesDone > 0 && lowerQuartile == 0) {
			std::cout << "Stop score: " << mergeScore.first << std::endl;
			break;
		}
		--lowerQuartile;

		list_ptr l_i = mergeScore.second.first;
		if(sharer.find(l_i) == sharer.end()) {
			continue;
		}
		list_ptr l_j = mergeScore.second.second;
		if(sharer.find(l_j) == sharer.end()) {
			continue;
		}

		mergeVisibility(sharer, l_i, l_j);
		++mergesDone;

		if(sharer.size() <= b_SPZ) {
			return mergesDone;
		}
	}
	return mergesDone;
}

void VisibilityMerge::mergeVisibility(visibility_sharer_map_t & sharer, list_ptr l_i, list_ptr l_j) {
	const auto it_i = sharer.find(l_i);
	FAIL_IF(it_i == sharer.cend());
	const auto it_j = sharer.find(l_j);
	FAIL_IF(it_j == sharer.cend());

	// First remove lists from all nodes using them.
	std::for_each(it_i->second.begin(), it_i->second.end(), std::mem_fn(&cell_t::clearValue));
	std::for_each(it_j->second.begin(), it_j->second.end(), std::mem_fn(&cell_t::clearValue));


	// Create the new list.
	auto l_z = new Util::GenericAttributeList;
	// Go over lists containing elements for different directions.
	auto listIt_i = l_i->begin();
	auto listIt_j = l_j->begin();
	FAIL_IF(l_i->size() != l_j->size());
	while (listIt_i != l_i->end()) {
		const auto & vv_i = Helper::getVV(listIt_i->get());
		const auto & vv_j = Helper::getVV(listIt_j->get());

		auto vv_z = VisibilityVector::makeMax(vv_i, vv_j);

		l_z->push_back(new VisibilityVectorAttribute(vv_z));

		++listIt_i;
		++listIt_j;
	}

	// Create a new entry for the new list.
	const auto it_z = sharer.insert(std::make_pair(l_z, cell_set_t())).first;
	// Fill the set with values from the old entries.
	it_z->second.insert(it_i->second.begin(), it_i->second.end());
	it_z->second.insert(it_j->second.begin(), it_j->second.end());


	// Destroy the old lists.
	delete l_i;
	delete l_j;
	sharer.erase(it_i);
	sharer.erase(it_j);


	// Update the cells.
	for(const auto & cell : it_z->second) {
		cell->setValue(l_z);
		Helper::clearRuntime(cell);
	}
}
}
}

#endif /* MINSG_EXT_VISIBILITYMERGE */
