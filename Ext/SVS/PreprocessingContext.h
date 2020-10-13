/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#include <cstdint>
#include <memory>
#include <vector>

#ifndef MINSG_SVS_PREPROCESSINGCONTEXT_H_
#define MINSG_SVS_PREPROCESSINGCONTEXT_H_

#ifdef MINSG_EXT_SVS_PROFILING
#include <Util/StringIdentifier.h>
#endif /* MINSG_EXT_SVS_PROFILING */

namespace Geometry {
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3f;
}
namespace MinSG {
class FrameContext;
class GroupNode;
#ifdef MINSG_EXT_SVS_PROFILING
namespace Profiling {
class Profiler;
}
#endif /* MINSG_EXT_SVS_PROFILING */
namespace SceneManagement {
class SceneManager;
}
namespace SVS {

/**
 * @brief State storage for Spherical Visibility Sampling preprocessing
 * 
 * This helper class stores the state of the Spherical Visibility Sampling
 * preprocessing and thereby enables to perform the preprocessing step by step
 * with intermediate interruptions.
 * 
 * @author Benjamin Eikel
 * @date 2013-01-29
 */
class PreprocessingContext {
	private:
		// Use Pimpl idiom
		struct Implementation;
		std::unique_ptr<Implementation> impl;

	public:
		/**
		 * Create a context for the Spherical Visibility Sampling
		 * preprocessing. The context is directly initialized with the
		 * required objects and settings. To execute the preprocessing,
		 * @a preprocessSingleNode() has to be called until @a isFinished()
		 * returns @c true.
		 *
		 * @param sceneManager SceneManager used by the visibility vectors
		 * @param frameContext Frame context required for rendering
		 * @param rootNode Root node of the tree to do the preprocessing for
		 * @param positions Positions on the unit sphere specifying the
		 * viewing directions
		 * @param resolution Image resolution in pixels
		 * @param useExistingVisibilityResults If @c true, visibility results
		 * from child nodes that have been computed earlier will be used for
		 * visibility tests in inner nodes
		 * @param computeTightInnerBoundingSpheres If @c true, the bounding
		 * sphere for an inner nodes will be computed based on all meshes'
		 * vertices in the subtree. If @c false, the bounding spheres of the
		 * child nodes will be combined to compute the bounding sphere of the
		 * inner node.
		 */
		MINSGAPI PreprocessingContext(SceneManagement::SceneManager & sceneManager,
							 FrameContext & frameContext,
							 GroupNode * rootNode,
							 const std::vector<Geometry::Vec3f> & positions,
							 uint32_t resolution,
							 bool useExistingVisibilityResults,
							 bool computeTightInnerBoundingSpheres);

		//! Destroy the preprocessing context
		MINSGAPI ~PreprocessingContext();

		/**
		 * Preprocess a single node. If the preprocessing is already finished,
		 * do nothing.
		 */
		MINSGAPI void preprocessSingleNode();

		/**
		 * Check the state of the preprocessing.
		 *
		 * @return @c true if and only if the preprocessing is finished
		 */
		MINSGAPI bool isFinished() const;

		/**
		 * Return the number of nodes that wait to be preprocessed.
		 * 
		 * @return Current number of nodes
		 */
		MINSGAPI std::size_t getNumRemainingNodes() const;

		//! Access the scene manager.
		MINSGAPI SceneManagement::SceneManager & getSceneManager();

		//! Access the frame context.
		MINSGAPI FrameContext & getFrameContext();

		//! Read the sample positions.
		MINSGAPI const std::vector<Geometry::Vec3f> & getPositions() const;

		//! Read the image resolution.
		MINSGAPI uint32_t getResolution() const;

		//! Read if existing results are to be used.
		MINSGAPI bool getUseExistingVisibilityResults() const;

		//! Read if tight bounding spheres for inner nodes are to be computed.
		MINSGAPI bool getComputeTightInnerBoundingSpheres() const;

#ifdef MINSG_EXT_SVS_PROFILING
		//! Access the profiler.
		MINSGAPI Profiling::Profiler & getProfiler();

		MINSGAPI static const Util::StringIdentifier ATTR_numDescendantsGeometryNode;
		MINSGAPI static const Util::StringIdentifier ATTR_numDescendantsGroupNode;
		MINSGAPI static const Util::StringIdentifier ATTR_numDescendantsTriangles;
		MINSGAPI static const Util::StringIdentifier ATTR_numGeometryNodesVisible;
		MINSGAPI static const Util::StringIdentifier ATTR_numTrianglesVisible;
		MINSGAPI static const Util::StringIdentifier ATTR_numPixelsVisible;
		MINSGAPI static const Util::StringIdentifier ATTR_numChildrenGeometryNode;
		MINSGAPI static const Util::StringIdentifier ATTR_numChildrenGroupNode;
		MINSGAPI static const Util::StringIdentifier ATTR_numVertices;
		MINSGAPI static const Util::StringIdentifier ATTR_sphereRadius;
#endif /* MINSG_EXT_SVS_PROFILING */
};

}
}

#endif /* MINSG_SVS_PREPROCESSINGCONTEXT_H_ */

#endif /* MINSG_EXT_SVS */
