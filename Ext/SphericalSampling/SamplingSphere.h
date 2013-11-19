/*
	This file is part of the MinSG library extension SphericalSampling.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SPHERICALSAMPLING

#ifndef MINSG_SPHERICALSAMPLING_SAMPLINGSPHERE_H_
#define MINSG_SPHERICALSAMPLING_SAMPLINGSPHERE_H_

#include "Definitions.h"
#include "SamplePoint.h"
#include <Geometry/Sphere.h>
#include <deque>
#include <memory>
#include <vector>

namespace MinSG {
class CameraNodeOrtho;
class FrameContext;
class GeometryNode;
class ListNode;
class Node;
namespace Evaluators {
class Evaluator;
}
namespace Triangulation {
template<typename Point_t> class Delaunay3d;
}
namespace VisibilitySubdivision {
class VisibilityVector;
}
namespace SphericalSampling {
struct SampleEntry;

/**
 * Sphere containing sample points on its surface.
 * The sample points store visibility information.
 *
 * @author Benjamin Eikel
 * @date 2012-01-17
 */
class SamplingSphere {
	private:
		//! Geometric representation of the sphere surface.
		Geometry::Sphere_f sphere;

		//! Array holding the sample points.
		std::vector<SamplePoint> samples;

		//! 3D delaunay triangulation that is built from the sample points.
		std::shared_ptr<Triangulation::Delaunay3d<SampleEntry>> triangulation;

		//! Factor that is used to scale the query vector to find a point inside the triangulation.
		mutable float minimumScaleFactor;

		SamplingSphere & operator=(const SamplingSphere &) = delete;
		SamplingSphere & operator=(SamplingSphere &&) = delete;

	public:
		//! Build a new triangulation from the given samples.
		SamplingSphere(Geometry::Sphere_f _sphere, const std::vector<SamplePoint> & _samples);

		//! Move the given sphere and the given samples into the sampling sphere.
		SamplingSphere(Geometry::Sphere_f && _sphere, std::vector<SamplePoint> && _samples);

		/**
		 * Build a new sampling sphere.
		 * Firstly, the given set of sampling spheres is checked for consistency (same sample positions).
		 * Secondly, a new sampling sphere is created for the given sphere and sample positions.
		 *
		 * @param newSphere Geometric representation of the sphere surface of the new sampling sphere
		 * @param samplingSpheres Sampling spheres that are used to create the new sampling sphere
		 */
		SamplingSphere(Geometry::Sphere_f newSphere,
					   const std::deque<const SamplingSphere *> & samplingSpheres);

		SamplingSphere(const SamplingSphere &) = default;
		SamplingSphere(SamplingSphere &&) = default;
		~SamplingSphere() = default;

		//! Equality comparison
		bool operator==(const SamplingSphere & other) const;

		const Geometry::Sphere_f & getSphere() const {
			return sphere;
		}

		void setSphere(const Geometry::Sphere_f & newSphere) {
			sphere = newSphere;
		}

		const std::vector<SamplePoint> & getSamples() const {
			return samples;
		}

		ListNode * getTriangulationMinSGNodes() const;

		/**
		 * Calculate the amount of memory that is required to store the sampling sphere.
		 * 
		 * @return Overall amount of memory in bytes
		 */
		size_t getMemoryUsage() const;

		/**
		 * Iterate over all sample points on the sphere and perform an evaluation for each point.
		 * The given evaluator is used for the evaluation.
		 *
		 * @param frameContext Frame context used for rendering
		 * @param evaluator Evaluator that is used to generate values
		 * @param camera Orthographic camera that is used for rendering
		 * @param node Root node of the scene that is given to the evaluator
		 */
		void evaluateAllSamples(FrameContext & frameContext,
								Evaluators::Evaluator & evaluator,
								CameraNodeOrtho * camera,
								Node * node);

		/**
		 * Iterate over all sample points on the sphere and perform an evaluation for each point.
		 * The given evaluator is used for the evaluation.
		 * When evaluating a sample point, only those nodes are taken into account, which are visible
		 * for at least one of the corresponding sample points of the given sampling spheres, or which
		 * are given explicitly.
		 *
		 * @param frameContext Frame context used for rendering
		 * @param evaluator Evaluator that is used to generate values
		 * @param camera Orthographic camera that is used for rendering
		 * @param node Root node of the scene that is given to the evaluator
		 * @param samplingSpheres Sampling spheres that define the visible nodes for the sampling
		 * @param explicitNodes Additional nodes that are explicitly taken into account.
		 * The range has to be sorted.
		 */
		void evaluateAllSamples(FrameContext & frameContext,
								Evaluators::Evaluator & evaluator,
								CameraNodeOrtho * camera,
								Node * node,
								const std::deque<const SamplingSphere *> & samplingSpheres,
								const std::deque<GeometryNode *> & explicitNodes);

		/**
		 * Return a value for the given @a query.
		 * The result depends on the @a interpolationMethod.
		 *
		 * @param query Unit vector
		 * @param interpolationMethod See documentation of @a interpolation_type_t
		 * @return Visibility information for the queried position
		 */
		VisibilitySubdivision::VisibilityVector queryValue(const Geometry::Vec3f & query, interpolation_type_t interpolationMethod) const;
};

}
}

#endif /* MINSG_SPHERICALSAMPLING_SAMPLINGSPHERE_H_ */

#endif /* MINSG_EXT_SPHERICALSAMPLING */
