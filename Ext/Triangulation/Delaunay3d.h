/*
	This file is part of the MinSG library extension Triangulation.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TRIANGULATION

#ifndef MINSG_TRIANGULATION_DELAUNAY3D_H
#define MINSG_TRIANGULATION_DELAUNAY3D_H

#include "TetrahedronWrapper.h"
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>
#include <Geometry/Vec3.h>

namespace MinSG {
namespace Triangulation {

MINSGAPI std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> doTetrahedralization(const std::vector<Geometry::Vec3f> & positions);

template<typename Point_t>
class Delaunay3d {
	private:
		static_assert(std::is_convertible<typename Point_t::vector_t, Geometry::Vec3f>::value, "Vector has to be three-dimensional.");
	public:

		typedef std::vector<TetrahedronWrapper<Point_t>> tetrahedronArray_t;

	private:
		std::deque<Point_t> points;
		tetrahedronArray_t tetrahedra;

		//! Cache for the tetrahedron that contained the last position in @a findTetrahedron.
		const TetrahedronWrapper<Point_t> * lastResult;

		bool valid;

		Delaunay3d(const Delaunay3d &) = delete;
		Delaunay3d(Delaunay3d &&) = delete;
	public:

		Delaunay3d() : lastResult(nullptr), valid(true){}

		inline void addPoint(const Point_t & point);

		/**
		 * Find the tetrahedron that contains the given position.
		 *
		 * @param[in] pos Query position
		 * @param[in] epsilon Safety margin around the tetrahedron that is still considered to be inside
		 * @return Tetrahedron containing @a pos, or @c nullptr if @a pos is outside of the triangulation.
		 */
		inline const TetrahedronWrapper<Point_t> * findTetrahedron(const Geometry::Vec3 & pos, float epsilon = 0.0f);

		/**
		 * Find the tetrahedron that contains, or one of the tetrahedrons that has the minimum distance to the given position.
		 *
		 * @param[in] pos Query position
		 * @return Tetrahedron that contains @a pos, or has the minimum distance to @a pos.
		 */
		inline const TetrahedronWrapper<Point_t> * findNearestTetrahedron(const Geometry::Vec3 & pos);

		/*! Recreates the tetrahedra.
			\note This needs only be called explicitly if an existing point	is moved after inserted. If
				new points are added, the function is automatically called on first access.	*/
		inline void updateTetrahedra();

		//! Returns true iff no points have been added since the last updateTethradra() call.
		bool isValid()const			{	return valid;	}
		void validate()				{	if(!valid) updateTetrahedra();	}

		//! Function expecting a single tetrahedron
		typedef std::function<void (const TetrahedronWrapper<Point_t> &)> generatorFunction_t;

		/**
		 * Call the given functor for every tetrahedron.
		 * 
		 * @param generator Functor that will be called for every tetrahedron
		 */
		void generate(generatorFunction_t generator) {
			validate();
			std::for_each(tetrahedra.begin(), tetrahedra.end(), generator);
		}

		/**
		 * Calculate the amount of memory that is required to store the triangulation.
		 * 
		 * @return Overall amount of memory in bytes
		 */
		size_t getMemoryUsage() const {
			return sizeof(Delaunay3d) +
					points.size() * sizeof(Point_t) +
					tetrahedra.size() * sizeof(TetrahedronWrapper<Point_t>);
		}
};


template<typename Point_t>
void Delaunay3d<Point_t>::addPoint(const Point_t & point){
	points.push_back(point);
	valid = false;
}

template<typename Point_t>
const TetrahedronWrapper<Point_t> * Delaunay3d<Point_t>::findTetrahedron(const Geometry::Vec3 & pos, float epsilon) {
	validate();

	// Check the cache.
	if(lastResult != nullptr && lastResult->getTetrahedron().containsPoint(pos, epsilon)) {
		return lastResult;
	}

	for(const auto & wrapper : tetrahedra) {
		if(wrapper.getTetrahedron().containsPoint(pos, epsilon)) {
			// Fill the cache.
			lastResult = &wrapper;
			return lastResult;
		}
	}
	return nullptr;
}

template<typename Point_t>
const TetrahedronWrapper<Point_t> * Delaunay3d<Point_t>::findNearestTetrahedron(const Geometry::Vec3 & pos) {
	validate();

	const TetrahedronWrapper<Point_t> * result = nullptr;
	float minimumDistance = std::numeric_limits<float>::max();

	for(const auto & wrapper : tetrahedra) {
		const float distance = wrapper.getTetrahedron().distanceSquared(pos);
		if(distance < minimumDistance) {
			minimumDistance = distance;
			result = &wrapper;
		}

		// Stop if the tetrahedron containing the position has been found.
		if(minimumDistance == 0.0f) {
			break;
		}
	}
	return result;
}

template<typename Point_t>
void Delaunay3d<Point_t>::updateTetrahedra(){
	// Clear the cache.
	lastResult = nullptr;

	tetrahedra.clear();

	if(points.size() == 4) {
		tetrahedra.emplace_back(&points[0], &points[1], &points[2], &points[3]);
	} else if(points.size() > 4) {
		std::vector<Geometry::Vec3f> positions;
		positions.reserve(points.size());
		for(const auto & point : points) {
			positions.emplace_back(point.getPosition());
		}
		const auto results = doTetrahedralization(positions);
		tetrahedra.reserve(results.size());
		for(const auto & result : results) {
			tetrahedra.emplace_back(&points[std::get<0>(result)], &points[std::get<1>(result)], &points[std::get<2>(result)], &points[std::get<3>(result)]);
		}
	}

	valid = true;
}

}
}

#endif /* MINSG_TRIANGULATION_DELAUNAY3D_H */
#endif /* MINSG_EXT_TRIANGULATION */
