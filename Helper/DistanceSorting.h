/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef DISTANCESORTING_H_
#define DISTANCESORTING_H_

#include "../Core/Nodes/Node.h"
#include <Geometry/Box.h>

#include <set>
#include <queue>

namespace MinSG {

/**
 * @file
 *
 * Comparators, sets, queues to sort objects by distance to some reference point.
 *
 * comparison is done by distance, if distances are equal comparison is done by pointers.
 *
 * @author Ralf Petring
 * @date 2011-02-14
 */

/** @addtogroup helper
 * @{
 */
	
// ------------------------
// Distance calculators
 
namespace DistanceCalculators{

struct NodeDistanceCalculator{
	template<class NodePtrType_t>
	static float getDistance(const Geometry::Vec3 & pos,const NodePtrType_t & node){	return node->getWorldBB().getDistanceSquared(pos);	}
};

}

// ------------------------
// Priority queue

//! Simple queue for sorting Elements by their distance to a given point.
template<typename ElementType_t,typename distanceCalulator,template<typename> class Comparator_t>
struct _GenericDistancePriorityQueue {
	const Geometry::Vec3 referencePos;
	_GenericDistancePriorityQueue(Geometry::Vec3 _referencePos) : referencePos(std::move(_referencePos)){}

	bool empty()const					{	return data.empty();	}
	void pop()							{	data.erase(data.begin());	}
	bool push(const ElementType_t & e)	{	return data.insert( std::make_pair(distanceCalulator::getDistance(referencePos,e),e )).second;	} //! \todo use 'emplace' when available
	size_t size()						{	return data.size();	}
	ElementType_t top()					{	return data.begin()->second;	}

private:
	typedef std::pair<float,ElementType_t> distanceElementPair_t;
	std::set< distanceElementPair_t,Comparator_t<distanceElementPair_t>> data;
};

typedef _GenericDistancePriorityQueue<Node*,DistanceCalculators::NodeDistanceCalculator,std::less> NodeDistancePriorityQueue_F2B;
typedef _GenericDistancePriorityQueue<Node*,DistanceCalculators::NodeDistanceCalculator,std::greater> NodeDistancePriorityQueue_B2F;

// -----------------------
// Set

template<typename T, typename floatCompare, typename pointerCompare>
class _DistanceCompare {

	public:

		//! (ctor)
		_DistanceCompare(Geometry::Vec3 _referencePosition) :
			referencePosition(std::move(_referencePosition)) {
		}

		//! (ctor)
		_DistanceCompare(const _DistanceCompare & other) :
			referencePosition(other.referencePosition) {
		}

	public:
		/**
		 *
		 * @param 	a first object
		 * @param 	b second object
		 * @return 	true if the first object is closer (if order == BACK_TO_FRONT) to the reference position than the second
		 * 			if the objects have equal distance to the reference position the pointers of the objects are compared
		 *
		 * @see		float Geometry::Box::getDistanceSquared(position)
		 */
		bool operator()(const T * a, const T * b) const {
			volatile const float d1 = getDistance(a);
			volatile const float d2 = getDistance(b);
			return fltCmp(d1, d2) || ( !fltCmp(d2, d1) && ptrCmp(a, b));
		}

	private:

		Geometry::Vec3 referencePosition;

		floatCompare fltCmp;
		pointerCompare ptrCmp;

		float getDistance(const Geometry::Vec3 * a) const {
			return a->distanceSquared(referencePosition);
		}

		float getDistance(const Node * a) const {
			return a->getWorldBB().getDistanceSquared(referencePosition);
		}

		float getDistance(const Geometry::Box * a) const {
			return a->getDistanceSquared(referencePosition);
		}

		//! Unimplemented, because the sort order must not be changed for an existing data structure.
#if defined(_MSC_VER)
	// TODO find the actual problem here
    public:
        _DistanceCompare & operator=(const _DistanceCompare & other) { __debugbreak(); return *this; }
#else
		_DistanceCompare & operator=(const _DistanceCompare & other);
#endif
};

// First element of std::set with default comparison std::less is the smallest element.

template<typename T>
struct DistanceSetB2F : public std::set<T *, _DistanceCompare<T, std::greater<volatile float>, std::greater<const T *> > > {
		DistanceSetB2F(const Geometry::Vec3 & pos) :
			std::set<T *, _DistanceCompare<T, std::greater<volatile float>, std::greater<const T *> > >(pos) {
		}
};
template<typename T>
struct DistanceSetF2B : public std::set<T *, _DistanceCompare<T, std::less<volatile float>, std::less<const T *> > > {
		DistanceSetF2B(const Geometry::Vec3 & pos) :
			std::set<T *, _DistanceCompare<T, std::less<volatile float>, std::less<const T *> > >(pos) {
		}
};

//! @}

}

#endif /* DISTANCESORTING_H_ */
