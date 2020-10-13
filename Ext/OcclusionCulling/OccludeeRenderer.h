/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef OCCLUDEERENDERER_H
#define OCCLUDEERENDERER_H

#include "../../Core/States/State.h"
#include <memory>

namespace MinSG {
class OccRenderer;

/**
 * Renderer that displays the scene using conservative occlusion culling first.
 * After that the occluded objects are additionally displayed in front.
 * The real work is not done by this class, but by using an occlusion culling renderer.
 *
 * @author Benjamin Eikel
 * @date 2011-07-06
 * @ingroup states
 */
class OccludeeRenderer : public State {
		PROVIDES_TYPE_NAME(OccludeeRenderer)
	public:
		MINSGAPI OccludeeRenderer();
		MINSGAPI OccludeeRenderer(const OccludeeRenderer & other);
		OccludeeRenderer(OccludeeRenderer &&) = default;
		MINSGAPI virtual ~OccludeeRenderer();

		MINSGAPI OccludeeRenderer * clone() const override;
		
		void setUseWireframe(bool b) { useWireframe = b; }
		bool getUseWireframe() const { return useWireframe; }
		
		void setShowOriginal(bool b) { showOriginal = b; }
		bool getShowOriginal() const { return showOriginal; }
		
	private:
		std::unique_ptr<OccRenderer> occlusionCullingRenderer;
		bool useWireframe, showOriginal;

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * rootNode, const RenderParam & rp) override;
};

}

#endif /* OCCLUDEERENDERER_H */
