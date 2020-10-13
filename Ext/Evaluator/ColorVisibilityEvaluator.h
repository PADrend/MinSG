/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_EVALUATORS

#ifndef COLORVISIBILITYEVALUATOR_H_
#define COLORVISIBILITYEVALUATOR_H_

#include "Evaluator.h"
#include <Util/References.h>

namespace Rendering {
class FBO;
class Texture;
}

namespace MinSG {
class GeometryNode;

namespace Evaluators {
/**
 * Evaluator to determine the number of visible triangles.
 * The triangles are colored with distinct colors and the colors are searched in the resulting image after rendering.
 * The result is the overall number of visible triangles.
 *
 * @author Benjamin Eikel
 * @date 2010-09-14
 */
class ColorVisibilityEvaluator : public Evaluator {
	PROVIDES_TYPE_NAME(ColorVisibilityEvaluator)
	public:
		//! This uses always Evaluator::SINGLE_VALUE mode.
		MINSGAPI ColorVisibilityEvaluator(DirectionMode dirMode);
		MINSGAPI virtual ~ColorVisibilityEvaluator();

		// ---|> Evaluator
		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & rect) override;
		MINSGAPI virtual void endMeasure(FrameContext & context) override;

	private:
		//! Stores the number of triangles which were detected visible.
		size_t numTrianglesVisible;

		//! Handle for the color texture of the framebuffer object.
		Util::Reference<Rendering::Texture> colorTexture;

		//! Handle for the depth texture of the framebuffer object.
		Util::Reference<Rendering::Texture> depthTexture;

		//! Handle to frame buffer object used for rendering.
		Util::Reference<Rendering::FBO> fbo;
};
}
}

#endif /* COLORVISIBILITYEVALUATOR_H_ */
#endif /* MINSG_EXT_EVALUATORS */
