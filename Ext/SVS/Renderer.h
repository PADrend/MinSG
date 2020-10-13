/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#ifndef MINSG_SVS_RENDERER_H_
#define MINSG_SVS_RENDERER_H_

#include "../../Core/States/NodeRendererState.h"
#include "Definitions.h"
#include <Util/TypeNameMacro.h>
#include <cstdint>
#include <deque>
#include <memory>
#include <utility>

namespace Rendering {
class OcclusionQuery;
}
namespace MinSG {
class FrameContext;
class GeometryNode;
class GroupNode;
class Node;
class RenderParam;
namespace SVS {

/**
 * Renderer that uses preprocessed visibility information.
 * This information has to be attached to the nodes that are to be rendered.
 * By using this information, occlusion culling is performed.
 *
 * @author Benjamin Eikel
 * @date 2012-01-30
 */
class Renderer : public NodeRendererState {
		PROVIDES_TYPE_NAME(SVS::Renderer)
	private:
		interpolation_type_t interpolationMethod;

		std::deque<std::tuple<std::shared_ptr<Rendering::OcclusionQuery>, std::shared_ptr<Rendering::OcclusionQuery>, GroupNode *>> sphereQueries;

		std::deque<std::pair<std::shared_ptr<Rendering::OcclusionQuery>, GeometryNode *>> geometryQueries;

#ifdef MINSG_PROFILING
		uint32_t numSpheresVisited;
		uint32_t numSpheresEntered;
#endif /* MINSG_PROFILING */

		//! When @c true, perform an occlusion test before displaying a sphere.
		bool performSphereOcclusionTest;

		//! When @c true, perform an occlusion test before displaying geometry stored in the visibility information of a sphere.
		bool performGeometryOcclusionTest;

		MINSGAPI NodeRendererResult displayNode(FrameContext & context, Node * node, const RenderParam & rp) override;

		/**
		 * Display the geometry that is potentially visible for the current camera position.
		 * 
		 * @param context Current frame context used for rendering and to determine the camera position
		 * @param groupNode Group node that contains the sphere
		 * @param rp Current rendering parameters
		 * @param skipGeometryOcclusionTest If @c true, no occlusion tests for geometry are performed, regardless of the value of @a performGeometryOcclusionTest
		 */
		MINSGAPI void displaySphere(FrameContext & context, GroupNode * groupNode, const RenderParam & rp, bool skipGeometryOcclusionTest);

		//! Perform an occlusion test for the sphere stored in @p groupNode
		MINSGAPI void testSphere(FrameContext & context, GroupNode * groupNode);

		/**
		 * Check for available results of occlusion tests for spheres.
		 * 
		 * @return @c true if all pending queries have been processed, 
		 * @c false if the processing stopped leaving untreated queries behind.
		 */
		MINSGAPI bool processPendingSphereQueries(FrameContext & context, const RenderParam & rp);

		//! Perform an occlusion test for the given @p geometryNode
		MINSGAPI void testGeometry(FrameContext & context, GeometryNode * geometryNode);

		/**
		 * Check for available results of occlusion tests for geometry.
		 * 
		 * @return @c true if all pending queries have been processed, 
		 * @c false if the processing stopped leaving untreated queries behind.
		 */
		MINSGAPI bool processPendingGeometryQueries(FrameContext & context, const RenderParam & rp);

	protected:
#ifdef MINSG_PROFILING
		//! Call parent's implementation. Reset counters.
		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
#endif /* MINSG_PROFILING */

		//! Call parent's implementation. Pass counters to statistics.
		MINSGAPI void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	public:
		MINSGAPI Renderer();

		MINSGAPI Renderer * clone() const override;

		interpolation_type_t getInterpolationMethod() const {
			return interpolationMethod;
		}
		void setInterpolationMethod(interpolation_type_t interpolation) {
			interpolationMethod = interpolation;
		}

		bool isSphereOcclusionTestEnabled() const {
			return performSphereOcclusionTest;
		}
		void enableSphereOcclusionTest() {
			performSphereOcclusionTest = true;
		}
		void disableSphereOcclusionTest() {
			performSphereOcclusionTest = false;
		}

		bool isGeometryOcclusionTestEnabled() const {
			return performGeometryOcclusionTest;
		}
		void enableGeometryOcclusionTest() {
			performGeometryOcclusionTest = true;
		}
		void disableGeometryOcclusionTest() {
			performGeometryOcclusionTest = false;
		}
};

}
}

#endif /* MINSG_SVS_RENDERER_H_ */

#endif /* MINSG_EXT_SVS */
