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
namespace BlueSurfels {
class AbstractSurfelStrategy;

/**
 *  [SurfelRenderer] ---|> [NodeRendererState]
 */
class SurfelRenderer : public MinSG::NodeRendererState {
	PROVIDES_TYPE_NAME(SurfelRenderer)
	public:

		SurfelRenderer();
		SurfelRenderer(const SurfelRenderer&) = delete;
		virtual ~SurfelRenderer();

		/// ---|> [State]
		SurfelRenderer* clone() const override;
	protected:
		stateResult_t doEnableState(MinSG::FrameContext& context, MinSG::Node* node, const MinSG::RenderParam& rp) override;
		void doDisableState(MinSG::FrameContext& context, MinSG::Node* node, const MinSG::RenderParam& rp) override;
		MinSG::NodeRendererResult displayNode(MinSG::FrameContext& context, MinSG::Node* node, const MinSG::RenderParam& rp) override;
	public:

	void addSurfelStrategy(AbstractSurfelStrategy* strategy);
	void removeSurfelStrategy(AbstractSurfelStrategy* strategy);
	void clearSurfelStrategies();
	std::vector<AbstractSurfelStrategy*> getSurfelStrategies() const;
	
	void drawSurfels(MinSG::FrameContext& context);
  private:
		struct Data;
		std::unique_ptr<Data> data;
};

}
}

#endif // SurfelRenderer_H
#endif // MINSG_EXT_BLUE_SURFELS