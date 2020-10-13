/*
	This file is part of the MinSG library extension BlueSurfels.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2016-2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_BLUE_SURFELS

#ifndef MINSG_EXT_BLUESURFELS_SURFEL_RENDERER_H_
#define MINSG_EXT_BLUESURFELS_SURFEL_RENDERER_H_

#include "../../Core/States/NodeRendererState.h"

#include <Geometry/Matrix4x4.h>

#include <memory>
#include <functional>

namespace MinSG {
//! @ingroup ext
namespace BlueSurfels {
class AbstractSurfelStrategy;

/**
 *  [SurfelRenderer] ---|> [NodeRendererState]
 */
class SurfelRenderer : public MinSG::NodeRendererState {
	PROVIDES_TYPE_NAME(SurfelRenderer)
	public:

		MINSGAPI SurfelRenderer();
		SurfelRenderer(const SurfelRenderer&) = delete;
		MINSGAPI virtual ~SurfelRenderer();

		/// ---|> [State]
		MINSGAPI SurfelRenderer* clone() const override;
	protected:
		MINSGAPI stateResult_t doEnableState(MinSG::FrameContext& context, MinSG::Node* node, const MinSG::RenderParam& rp) override;
		MINSGAPI void doDisableState(MinSG::FrameContext& context, MinSG::Node* node, const MinSG::RenderParam& rp) override;
		MINSGAPI MinSG::NodeRendererResult displayNode(MinSG::FrameContext& context, MinSG::Node* node, const MinSG::RenderParam& rp) override;
	public:

	MINSGAPI void addSurfelStrategy(AbstractSurfelStrategy* strategy);
	MINSGAPI void removeSurfelStrategy(AbstractSurfelStrategy* strategy);
	MINSGAPI void clearSurfelStrategies();
	MINSGAPI std::vector<AbstractSurfelStrategy*> getSurfelStrategies() const;
	
	MINSGAPI void drawSurfels(MinSG::FrameContext& context);
  private:
		struct Data;
		std::unique_ptr<Data> data;
};

}
}

#endif // SurfelRenderer_H
#endif // MINSG_EXT_BLUE_SURFELS