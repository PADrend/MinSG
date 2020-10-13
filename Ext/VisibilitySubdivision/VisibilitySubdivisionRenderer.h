/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION

#ifndef VISIBILITYSUBDIVISIONRENDERER_H
#define VISIBILITYSUBDIVISIONRENDERER_H

#include "../../Core/States/State.h"
#include <Geometry/Matrix4x4.h>
#include <Util/References.h>
#include <vector>

namespace Rendering {
class Mesh;
class Texture;
}

namespace MinSG {
class GeometryNode;
class ValuatedRegionNode;
//! @ingroup ext
namespace VisibilitySubdivision {
/**
 * Node class to render the scene using pre-calculated visibility
 * information.
 *
 * @author Benjamin Eikel
 * @date 2009-02-20, extended for Master's thesis 2009-09-26, added sky box rendering 2010-04-20
 */
class VisibilitySubdivisionRenderer : public State {
	PROVIDES_TYPE_NAME(VisibilitySubdivisionRenderer)

	public:
		typedef GeometryNode * object_ptr;
		typedef ValuatedRegionNode * cell_ptr;

		/**
		 * Standard constructor.
		 */
		MINSGAPI VisibilitySubdivisionRenderer();
		MINSGAPI VisibilitySubdivisionRenderer(const VisibilitySubdivisionRenderer & source);
		MINSGAPI virtual ~VisibilitySubdivisionRenderer();

		/**
		 * Assign a new visibility subdivision.
		 *
		 * @param root Root of a visibility subdivision.
		 */
		MINSGAPI void setViSu(cell_ptr root);

		/**
		 * Set the maximum runtime for the rendering of one frame.
		 *
		 * @param triangles Maximum runtime in number of triangles.
		 */
		void setMaximumRuntime(uint32_t triangles) {
			maxRuntime = triangles;
		}
		uint32_t getMaximumRuntime() const {
			return maxRuntime;
		}

		/**
		 * Set if the current data should be used for the next frames.
		 * This can be used to move around the scene inspecting the
		 * data used for an initial position.
		 *
		 * @param doHold If @c true, current data will not be updated.
		 */
		void setHold(bool doHold) {
			hold = doHold;
		}
		bool getHold() const {
			return hold;
		}



		/**
		 * If the debug output is activated, the objects from different
		 * triangle budgets are rendered in different colors instead of culling
		 * them.
		 *
		 * @param debug Enable or disable debug output.
		 */
		void setDebugOutput(bool debug) {
			debugOutput = debug;
		}
		bool getDebugOutput() const {
			return debugOutput;
		}

		MINSGAPI VisibilitySubdivisionRenderer * clone() const override;

		/**
		 * Display a given subset of the potentially visible objects inside the given cell.
		 *
		 * @param context Context that is used for rendering.
		 * @param cell Cell to retrieve the potentially visible set from.
		 * @param budgetBegin Number of triangles where the rendering of objects should start. If zero then the rendering starts at the beginning.
		 * @param budgetEnd Number of triangles where the rendering of objects should stop. If zero then the rendering is not stopped.
		 * @return @c true if the objects were display and @c false if an error occurred.
		 */
		MINSGAPI static bool renderCellSubset(FrameContext & context, cell_ptr cell, uint32_t budgetBegin, uint32_t budgetEnd);

	private:
		//! Hold current data and do not update for new frames.
		bool hold;

		//! If @c true, display different budgets in different colors.
		bool debugOutput;

		//! Stop rendering after this number of triangles.
		uint32_t maxRuntime;

		cell_ptr viSu;

		//! Cache the cell which was used in the last frame.
		cell_ptr currentCell;

		//! Cache for objects from the last frame.
		std::vector<std::pair<float, object_ptr>> objects;

		//! Storage for objects in hold mode.
		std::vector<object_ptr> holdObjects;

		//! Render objects from different budgets in different colors.
		MINSGAPI void debugDisplay(uint32_t & renderedTriangles, object_ptr object, FrameContext & context, const RenderParam & rp);

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;

		// -----

		/*! @name AccumRendering
			If the observer does not move, complete the frame iterativly */
		// @{
	private:
		//! Start rendering after this number of triangles (when the observer does not move).
		uint32_t startRuntime;
		Geometry::Matrix4x4 lastCamMatrix;
		bool accumRenderingEnabled;
	public:
		void setAccumRendering(bool b) 	{	accumRenderingEnabled= b;	}
		bool getAccumRendering()const 	{	return accumRenderingEnabled;	}
		// @}

		/**
		 * @name TexturedDepthMeshes
		 * The renderer is able to use textured depth meshes stored in a visibility cell. This may improve the visual quality with approximative rendering.
		 */
		// @{
	private:
		//! Flag that determines of the usage of textured depth meshes.
		bool displayTexturedDepthMeshes;

		//! Array of depth meshes for the current cell.
		std::vector<Util::Reference<Rendering::Mesh>> depthMeshes;

		//! Array of textures for the current cell.
		std::vector<Util::Reference<Rendering::Texture>> textures;

		//! Factor of polygon offset used for rendering the TDMs.
		float polygonOffsetFactor;

		//! Units of polygon offset used for rendering the TDMs.
		float polygonOffsetUnits;

	public:
		/**
		 * If the textured depth meshes usage is activated, the textured depth meshes stored inside a visibility cell will be used as background.
		 *
		 * @param debug Enable or disable textured depth meshes rendering.
		 */
		void setUseTexturedDepthMeshes(bool status) {
			displayTexturedDepthMeshes = status;
		}
		bool getUseTexturedDepthMeshes() const {
			return displayTexturedDepthMeshes;
		}

		float getPolygonOffsetFactor() const {
			return polygonOffsetFactor;
		}

		void setPolygonOffsetFactor(float factor) {
			polygonOffsetFactor = factor;
		}

		float getPolygonOffsetUnits() const {
			return polygonOffsetUnits;
		}

		void setPolygonOffsetUnits(float units) {
			polygonOffsetUnits = units;
		}
		// @}
};
}
}

#endif // VISIBILITYSUBDIVISIONRENDERER_H
#endif // MINSG_EXT_VISIBILITY_SUBDIVISION
