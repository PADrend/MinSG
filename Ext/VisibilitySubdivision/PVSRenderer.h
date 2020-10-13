/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_VISIBILITY_SUBDIVISION

#ifndef MINSG_VISIBILITYSUBDIVISION_PVSRENDERER_H
#define MINSG_VISIBILITYSUBDIVISION_PVSRENDERER_H

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
namespace VisibilitySubdivision {

/**
 * Renderer that uses the potentially visible set (PVS) stored in a view cell
 * hierarchy for rendering.
 *
 * @author Benjamin Eikel
 * @date 2013-09-23
 */
class PVSRenderer : public State {
	PROVIDES_TYPE_NAME(PVSRenderer)

	public:
		typedef GeometryNode * object_ptr;
		typedef ValuatedRegionNode * cell_ptr;

		/**
		 * Standard constructor.
		 */
		MINSGAPI PVSRenderer();
		MINSGAPI PVSRenderer(const PVSRenderer &);
		PVSRenderer(PVSRenderer &&) = delete;
		MINSGAPI virtual ~PVSRenderer();

		/**
		 * Assign a new view cell hierarchy.
		 *
		 * @param root Root cell of a view cell hierarchy.
		 */
		void setViewCells(cell_ptr root) {
			viewCells = root;
		}

		MINSGAPI PVSRenderer * clone() const override;

	private:
		//! Root of a view cell hierarchy.
		cell_ptr viewCells;

		//! Cache for the view cell that was used for the last frame.
		cell_ptr lastViewCell;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;
};

}
}

#endif /* MINSG_VISIBILITYSUBDIVISION_PVSRENDERER_H */
#endif /* MINSG_EXT_VISIBILITY_SUBDIVISION */
