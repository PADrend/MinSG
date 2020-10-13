/*
	This file is part of the MinSG library.
	Copyright (C) 2009-2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RANDOMCOLORRENDERER_H_
#define RANDOMCOLORRENDERER_H_

#include "../../Core/States/State.h"
#include <map>

#include <Util/Graphics/Color.h>

namespace Rendering {
class Shader;
}
namespace MinSG {
class GeometryNode;

/**
 * Node class to render all GeometryNodes with a different color. The
 * random color is assigned when the node is seen the fist time and
 * stored in an internal cache. It makes usage of programmable shader
 * programs.
 *
 * @author Benjamin Eikel
 * @date 2009-10-09
 * @ingroup states
 */
class RandomColorRenderer : public State {
	PROVIDES_TYPE_NAME(RandomColorRenderer)
	public:
		MINSGAPI RandomColorRenderer();
		MINSGAPI RandomColorRenderer(const RandomColorRenderer & source);
		MINSGAPI virtual ~RandomColorRenderer();

		MINSGAPI RandomColorRenderer * clone() const override;

	private:
		//! Cache for node which was used in the last call.
		Node * lastRoot;

		//! Cache for color mapping.
		typedef std::map<GeometryNode *, Util::Color4f> color_map_t;
		color_map_t colorMapping;

		//! Shader used for rendering the colored geometry.
		MINSGAPI static Rendering::Shader * shader;
		MINSGAPI static Rendering::Shader * getShader();

		MINSGAPI stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;
};

}

#endif /* RANDOMCOLORRENDERER_H_ */
