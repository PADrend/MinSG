/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef HOMRENDERER_H
#define HOMRENDERER_H

#include "../../Core/States/State.h"

#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>

#include <Util/References.h>
#include <Util/TypeNameMacro.h>

#include <deque>
#include <vector>

namespace Geometry {
template<typename _T> class _Matrix4x4;
typedef _Matrix4x4<float> Matrix4x4f;
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3f;
}
namespace Rendering {
class RenderingContext;
class Shader;
}

namespace MinSG {
class AbstractCameraNode;
class FrameContext;
class GeometryNode;
class GroupNode;
class Node;

	/**
	 * Occlusion culling renderer which implements
	 * Hierarchical Occlusion Maps (HOM) presented in the article mentioned
	 * below.
	 *
	 * @author Benjamin Eikel
	 * @date 2008-09-07 (original as AbstractOctreeRenderer), 2009-07-24
	 * (porting to State)
	 * @see http://portal.acm.org/citation.cfm?doid=258734.258781
	 * @ingroup states
	 */
	class HOMRenderer : public State {
		PROVIDES_TYPE_NAME(HOMRenderer)
		public:


			/**
			 * Default constructor. Reservers memory for the HOM pyramid and
			 * sets default values for members.
			 *
			 * @param pyramidSize Side length of the HOM pyramid in pixels. It
			 * has to be greater or equal four and a power of two.
			 */
			MINSGAPI HOMRenderer(unsigned int pyramidSize = 512);

			//! Copy constructor
			MINSGAPI HOMRenderer(const HOMRenderer & source);

			//! Move constructor
			HOMRenderer(HOMRenderer &&) = default;

			//! Destructor. Frees the HOM pyramid memory.
			MINSGAPI virtual ~HOMRenderer();

			//! Collect possible occluders into the occluder-database.
			MINSGAPI void initOccluderDatabase();

			/**
			 * Change the pyramid size if the parameter is valid.
			 *
			 * @param pyramidSize Side length of the HOM pyramid in pixels. It
			 * has to be greater or equal four and a power of two.
			 * @return @c true if the side length was changed, @c false
			 * otherwise.
			 */
			MINSGAPI bool setSideLength(unsigned int pyramidSize);
			unsigned int getSideLength() const {
				return sideLength;
			}

			/**
			 * Set the maximum z value for occluders. Occluders which are
			 * further away should not be rendered anymore.
			 */
			void setMaxOccluderDepth(float maxDepth) {
				maxOccluderDepth = maxDepth;
			}
			float getMaxOccluderDepth() const {
				return maxOccluderDepth;
			}


			/**
			 * Specify if instead of rendering the scene only all occluders in
			 * the database should be shown.
			 */
			void setShowOnlyOccluders(bool onlyOccluders) {
				showOnlyOccluders = onlyOccluders;
			}
			bool getShowOnlyOccluders() const {
				return showOnlyOccluders;
			}


			/**
			 * Specify if the current HOM pyramid should be shown.
			 */
			void setShowHOMPyramid(bool showPyramid) {
				showHOMPyramid = showPyramid;
			}
			bool getShowHOMPyramid() const {
				return showHOMPyramid;
			}


			/**
			 * Specify if the geometry which was culled by the occlusion culling
			 * algorithm should be shown.
			 */
			void setShowCulledGeometry(bool showCulled) {
				showCulledGeometry = showCulled;
			}
			bool getShowCulledGeometry() const {
				return showCulledGeometry;
			}


			/**
			 * Set the minimum projected size of an object to get selected as an
			 * occluder (size of bounding sphere).
			 */
			void setMinOccluderSize(float minSize) {
				minOccluderSize = minSize;
			}
			float getMinOccluderSize() const {
				return minOccluderSize;
			}


			/**
			 * Set the maximum number of triangles of an object to get selected
			 * as an occluder.
			 */
			void setMaxOccluderComplexity(unsigned int maxComplexity) {
				maxOccluderComplexity = maxComplexity;
			}
			unsigned int getMaxOccluderComplexity() const {
				return maxOccluderComplexity;
			}


			/**
			 * Set the maximum number of triangles rendered in one frame for
			 * occlusion map generation.
			 */
			void setTriangleLimit(unsigned long limit) {
				triangleLimit = limit;
			}
			unsigned long getTriangleLimit() const {
				return triangleLimit;
			}

			MINSGAPI HOMRenderer * clone() const override;

		protected:
			/**
			 * Root node of the scene graph which should be rendered.
			 */
			GroupNode * rootNode;

			/**
			 * Side length in pixels of the highest resolution HOM. The value
			 * has to be a power of two and greater or equal four.
			 */
			unsigned int sideLength;

			/**
			 * Number of the levels in the HOM pyramid. Calculated from the
			 * @a sideLength.
			 */
			unsigned int numLevels;

			/**
			 * Array of HOMs. At level 0 (index 0) there is the highest
			 * resolution HOM. The size of the HOMs is halved with each level
			 * until it reaches size 4 x 4 pixels.
			 */
			std::vector<Util::Reference<Rendering::Texture> > homPyramid;

			/**
			 * Set the maximum z value for occluders. Occluders which are
			 * further away should not be rendered anymore.
			 */
			float maxOccluderDepth;

			/**
			 * Instead of rendering the scene only show all occluders in the
			 * database.
			 */
			bool showOnlyOccluders;

			/**
			 * Render the current HOM pyramid.
			 */
			bool showHOMPyramid;

			/**
			 * Render the geometry which was culled by the occlusion culling
			 * algorithm.
			 */
			bool showCulledGeometry;

			/**
			 * Minimum projected size of an object selected as an occluder (size
			 * of bounding sphere).
			 */
			float minOccluderSize;

			/**
			 * Maximum number of triangles of an object selected as an occluder.
			 */
			unsigned int maxOccluderComplexity;

			/**
			 * Maximum number of triangles rendered in one frame for occlusion
			 * map generation.
			 */
			unsigned long triangleLimit;

			//! OpenGL frame buffer object for off-screen rendering.
			Util::Reference<Rendering::FBO> fbo;

			/**
			 * List containing only occluders.
			 */
			std::deque<GeometryNode *> occluderDatabase;

			/**
			 * Shader used for rendering the occluders for the HOM.
			 */
			Util::Reference<Rendering::Shader> occluderShader;

			/**
			 * Storage for statistics.
			 */
			unsigned int pyramidTests;

			/**
			 * Storage for statistics.
			 */
			unsigned int pyramidTestsVisible;

			/**
			 * Storage for statistics.
			 */
			unsigned int pyramidTestsInvisible;

			/**
			 * Storage for statistics.
			 */
			unsigned int culledGeometry;

			/**
			 * Reserves the memory for the HOM pyramid.
			 */
			MINSGAPI void setupHOMPyramid(Rendering::RenderingContext & context);

			/**
			 * Creates the Shader.
			 */
			void setupShader();

			/**
			 * Clear the occluder-database.
			 */
			void clearOccluderDatabase();

			/**
			 * Frees the memory for the HOM pyramid.
			 */
			void cleanupHOMPyramid();

			//! Forward declaration for internal class.
			struct SelectedOccluder;

			//! Forward declaration for internal class.
			struct DepthSorter;

			/**
			 * Select occluders in frustum.
			 */
			MINSGAPI void selectOccluders(std::deque<SelectedOccluder> & occluders, AbstractCameraNode * camera) const;

			/**
			 * Draw selected occluders.
			 */
			MINSGAPI double drawOccluders(const std::deque<SelectedOccluder> & occluders, FrameContext & context) const;

			/**
			 * Checks the given area inside the HOM pyramid and determines if
			 * the area is visible.
			 *
			 * @param level Level to do the area check in.
			 * @param minX Left border of the area in the given level.
			 * @param maxX Right border of the area in the given level.
			 * @param minY Bottom border of the area in the given level.
			 * @param maxY Top border of the area in the given level.
			 * @param bMinX Left border of the area in the bottom level.
			 * @param bMaxX Right border of the area in the bottom level.
			 * @param bMinY Bottom border of the area in the bottom level.
			 * @param bMaxY Top border of the area in the bottom level.
			 * @return @c true if the area is visible and @c false if the area
			 * is hidden.
			 */
			MINSGAPI bool isAreaVisible(unsigned int level, unsigned int minX,
								unsigned int maxX, unsigned int minY,
								unsigned int maxY, unsigned int bMinX,
								unsigned int bMaxX, unsigned int bMinY,
								unsigned int bMaxY) const;

			/**
			 * Used when displaying the scene with the occlusion culling
			 * algorithm. It checks if a node is visible. If it is, it draws it
			 * and traverses the children. Otherwise the traversal is stopped at
			 * that node.
			 *
			 * @param node Node currently visited.
			 * @param cameraPos Absolute position of the camera.
			 * @param cameraDir Normalized viewing direction of the camera.
			 * @param zPlane Maximum z coordinate of occluders that are used in
			 * the current frame.
			 * @param rendContext Current rendering context which should be used
			 * for displaying.
			 * @param rendFlags Flags which should be used for displaying.
			 * @param cameraMatrix Camera matrix that was used to render the occluders.
			 * @param projectionMatrix Projection matrix that was used to render the occluders.
			 * @return Status code indicating if the traversal should be
			 * continued.
			 */
			MINSGAPI int process(Node * node, const Geometry::Vec3f & cameraPos, const Geometry::Vec3f & cameraDir, float zPlane,
						FrameContext & rendContext, const RenderParam & rp,
							const Geometry::Matrix4x4f & cameraMatrix,
							const Geometry::Matrix4x4f & projectionMatrix);

		private:
			/**
			 * Render the given @a node with this renderer using the
			 * FrameContext @a context and the @a flags given.
			 */
			MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	};
}

#endif // HOMRENDERER_H
