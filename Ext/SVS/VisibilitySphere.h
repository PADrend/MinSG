/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_VISIBILITYSPHERE_H_
#define MINSG_SVS_VISIBILITYSPHERE_H_

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
namespace SVS {
struct SampleEntry;

/**
 * Sphere containing sample points on its surface.
 * The sample points store visibility information.
 *
 * @author Benjamin Eikel
 * @date 2012-01-17
 */
class VisibilitySphere {
	private:
		//! Geometric representation of the sphere surface.
		Geometry::Sphere_f sphere;

		//! Array holding the sample points.
		std::vector<SamplePoint> samples;

		//! 3D delaunay triangulation that is built from the sample points.
		std::shared_ptr<Triangulation::Delaunay3d<SampleEntry>> triangulation;

		//! Factor that is used to scale the query vector to find a point inside the triangulation.
		mutable float minimumScaleFactor;

		VisibilitySphere & operator=(const VisibilitySphere &) = delete;
		VisibilitySphere & operator=(VisibilitySphere &&) = delete;

	public:
		//! Build a new triangulation from the given samples.
		MINSGAPI VisibilitySphere(Geometry::Sphere_f _sphere, const std::vector<SamplePoint> & _samples);

		//! Move the given sphere and the given samples into the visibility sphere.
		MINSGAPI VisibilitySphere(Geometry::Sphere_f && _sphere, std::vector<SamplePoint> && _samples);

		/**
		 * Build a new visibility sphere.
		 * Firstly, the given set of visibility spheres is checked for consistency (same sample positions).
		 * Secondly, a new visibility sphere is created for the given sphere and sample positions.
		 *
		 * @param newSphere Geometric representation of the sphere surface of the new visibility sphere
		 * @param visibilitySpheres Sampling spheres that are used to create the new visibility sphere
		 */
		MINSGAPI VisibilitySphere(Geometry::Sphere_f newSphere,
					   const std::deque<const VisibilitySphere *> & visibilitySpheres);

		VisibilitySphere(const VisibilitySphere &) = default;
		VisibilitySphere(VisibilitySphere &&) = default;
		~VisibilitySphere() = default;

		//! Equality comparison
		MINSGAPI bool operator==(const VisibilitySphere & other) const;

		const Geometry::Sphere_f & getSphere() const {
			return sphere;
		}

		void setSphere(const Geometry::Sphere_f & newSphere) {
			sphere = newSphere;
		}

		const std::vector<SamplePoint> & getSamples() const {
			return samples;
		}

		MINSGAPI ListNode * getTriangulationMinSGNodes() const;

		/**
		 * Calculate the amount of memory that is required to store the visibility sphere.
		 * 
		 * @return Overall amount of memory in bytes
		 */
		MINSGAPI size_t getMemoryUsage() const;

		/**
		 * Iterate over all sample points on the sphere and perform an evaluation for each point.
		 * The given evaluator is used for the evaluation.
		 *
		 * @param frameContext Frame context used for rendering
		 * @param evaluator Evaluator that is used to generate values
		 * @param camera Orthographic camera that is used for rendering
		 * @param node Root node of the scene that is given to the evaluator
		 */
		MINSGAPI void evaluateAllSamples(FrameContext & frameContext,
								Evaluators::Evaluator & evaluator,
								CameraNodeOrtho * camera,
								Node * node);

		/**
		 * Iterate over all sample points on the sphere and perform an evaluation for each point.
		 * The given evaluator is used for the evaluation.
		 * When evaluating a sample point, only those nodes are taken into account, which are visible
		 * for at least one of the corresponding sample points of the given visibility spheres, or which
		 * are given explicitly.
		 *
		 * @param frameContext Frame context used for rendering
		 * @param evaluator Evaluator that is used to generate values
		 * @param camera Orthographic camera that is used for rendering
		 * @param node Root node of the scene that is given to the evaluator
		 * @param visibilitySpheres Visibility spheres that define the visible nodes for the sampling
		 * @param explicitNodes Additional nodes that are explicitly taken into account.
		 * The range has to be sorted.
		 */
		MINSGAPI void evaluateAllSamples(FrameContext & frameContext,
								Evaluators::Evaluator & evaluator,
								CameraNodeOrtho * camera,
								Node * node,
								const std::deque<const VisibilitySphere *> & visibilitySpheres,
								const std::deque<GeometryNode *> & explicitNodes);

		/**
		 * Return a value for the given @a query.
		 * The result depends on the @a interpolationMethod.
		 *
		 * @param query Unit vector
		 * @param interpolationMethod See documentation of @a interpolation_type_t
		 * @return Visibility information for the queried position
		 */
		MINSGAPI VisibilitySubdivision::VisibilityVector queryValue(const Geometry::Vec3f & query, interpolation_type_t interpolationMethod) const;
};

}
}

#endif /* MINSG_SVS_VISIBILITYSPHERE_H_ */

#endif /* MINSG_EXT_SVS */
