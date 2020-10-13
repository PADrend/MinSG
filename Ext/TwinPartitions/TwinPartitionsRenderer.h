/*
	This file is part of the MinSG library extension TwinPartitions.
	Copyright (C) 2010-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_TWIN_PARTITIONS

#ifndef TWINPARTITIONSRENDERER_H
#define TWINPARTITIONSRENDERER_H

#include "../../Core/States/State.h"

namespace Rendering {
class Texture;
}

namespace MinSG {
//! @ingroup ext
namespace TwinPartitions {
struct PartitionsData;

/**
 * Renderer that uses an object space and a view space containing preprocessed visibility.
 * It first determines the visibility cell the camera is located in and then displays the objects that are stored in the visible set of that cell.
 *
 * @author Benjamin Eikel
 * @date 2010-09-23
 */
class TwinPartitionsRenderer : public State {
	PROVIDES_TYPE_NAME(TwinPartitionsRenderer)

	public:
		/**
		 * Standard constructor.
		 *
		 * @partitions Valid partition data. The renderer takes ownership of the pointer and deletes it when it is destroyed.
		 */
		MINSGAPI TwinPartitionsRenderer(PartitionsData * partitions);
		MINSGAPI virtual ~TwinPartitionsRenderer();

		uint32_t getMaximumRuntime() const {
			return maxRuntime;
		}

		/**
		 * Set the maximum runtime for the rendering of one frame.
		 *
		 * @param triangles Maximum runtime in number of triangles.
		 */
		void setMaximumRuntime(uint32_t triangles) {
			maxRuntime = triangles;
		}

		bool getDrawTexturedDepthMeshes() const {
			return drawTexturedDepthMeshes;
		}

		void setDrawTexturedDepthMeshes(bool draw) {
			drawTexturedDepthMeshes = draw;
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

		//! Implementation cannot be prevented.
		MINSGAPI State * clone() const override;

	private:
		//! Not implemented.
		MINSGAPI TwinPartitionsRenderer(const TwinPartitionsRenderer & /*source*/);
		//! Not implemented.
		MINSGAPI TwinPartitionsRenderer & operator=(const TwinPartitionsRenderer & /*source*/);

		//! Structure containing the data of the object space and the view space.
		std::unique_ptr<PartitionsData> data;

		//! Array holding the textures for the depth meshes of the current cell.
		std::vector<Util::Reference<Rendering::Texture> > textures;

		//! Index of the current cell.
		uint32_t currentCell;

		//! Stop rendering after this number of triangles.
		uint32_t maxRuntime;

		//! Factor of polygon offset used for rendering the TDMs.
		float polygonOffsetFactor;

		//! Units of polygon offset used for rendering the TDMs.
		float polygonOffsetUnits;

		//! Flag determining if the textured depth meshes are displayed.
		bool drawTexturedDepthMeshes;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node *, const RenderParam & rp) override;
};
}
}

#endif /* TWINPARTITIONSRENDERER_H */
#endif /* MINSG_EXT_TWIN_PARTITIONS */
