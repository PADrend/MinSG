/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_AGVS_SAMPLE_H
#define MINSG_AGVS_SAMPLE_H

#include <Geometry/Line.h>
#include <cstdint>
#include <limits>

namespace MinSG {
class GeometryNode;
namespace AGVS {

/**
 * Structure to store a single sample. The sample consists of the ray that is
 * cast and the resulting values in the forward and backward direction.
 * 
 * @tparam value_t Either @c float or @c double
 */
template<typename value_t>
class Sample {
	public:
		typedef Geometry::_Vec3<value_t> vec3_t;
		typedef Geometry::_Ray<vec3_t> ray_t;

	private:
		//! Ray in forward direction.
		const ray_t forwardRay;

		/**
		 * Object that has been hit in forward direction. Invalid if it is
		 * @c nullptr.
		 */
		GeometryNode * forwardResult;

		/**
		 * Ray parameter for the forward intersection. Valid only if
		 * @a forwardResult is not @c nullptr.
		 */
		value_t forwardIntersection;

		/**
		 * Object that has been hit in forward direction. Invalid if it is
		 * @c nullptr.
		 */
		GeometryNode * backwardResult;

		/**
		 * Ray parameter for the backward intersection. Valid only if
		 * @a backwardResult is not @c nullptr.
		 */
		value_t backwardIntersection;

		//! Identifier of the distribution that generated this sample
		uint8_t distributionId;

	public:
		/**
		 * Create a new, empty sample by setting only its forward ray.
		 * 
		 * @param ray Forward ray defining the sample
		 */
		Sample(const ray_t & ray) :
			forwardRay(ray),
			forwardResult(nullptr),
			forwardIntersection(std::numeric_limits<value_t>::max()),
			backwardResult(nullptr),
			backwardIntersection(std::numeric_limits<value_t>::max()),
			distributionId(0) {
		}

		GeometryNode * getForwardResult() const {
			return forwardResult;
		}
		GeometryNode * getBackwardResult() const {
			return backwardResult;
		}

		void setForwardResult(GeometryNode * result, value_t intersection) {
			forwardResult = result;
			forwardIntersection = intersection;
		}
		void setBackwardResult(GeometryNode * result, value_t intersection) {
			backwardResult = result;
			backwardIntersection = intersection;
		}

		bool hasForwardResult() const {
			return forwardResult != nullptr;
		}
		bool hasBackwardResult() const {
			return backwardResult != nullptr;
		}

		const ray_t & getForwardRay() const {
			return forwardRay;
		}
		ray_t getBackwardRay() const {
			return ray_t(forwardRay.getOrigin(), -forwardRay.getDirection());
		}

		value_t getForwardIntersection() const {
			return forwardIntersection;
		}
		value_t getBackwardIntersection() const {
			return backwardIntersection;
		}

		vec3_t getOrigin() const {
			return getForwardRay().getOrigin();
		}
		vec3_t getForwardTerminationPoint() const {
			return getForwardRay().getPoint(getForwardIntersection());
		}
		vec3_t getBackwardTerminationPoint() const {
			return getBackwardRay().getPoint(getBackwardIntersection());
		}

		/**
		 * Return the number of objects the sample has hit. It can be zero (no
		 * objects hit), one (object hit in either forward or backward
		 * direction), or two (objects hit in both directions).
		 * 
		 * @return 0, 1, or 2
		 */
		uint8_t getNumHits() const {
			return (hasForwardResult() ? 1 : 0) + (hasBackwardResult() ? 1 : 0);
		}

		/**
		 * Get the distribution that generated this sample.
		 * 
		 * @param id Identifier of the distribution
		 */
		uint8_t getDistributionId() const {
			return distributionId;
		}
		/**
		 * Set the distribution that generated this sample.
		 * 
		 * @param id Identifier of the distribution
		 */
		void setDistributionId(uint8_t id) {
			distributionId = id;
		}
};

}
}

#endif /* MINSG_AGVS_SAMPLE_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
