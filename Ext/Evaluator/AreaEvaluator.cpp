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

#include "AreaEvaluator.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Rect.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>
#include <Util/References.h>

#include <algorithm>

namespace MinSG {
namespace Evaluators {

Util::Reference<Rendering::Shader> AreaEvaluator::whiteShader;

//! [ctor]
AreaEvaluator::AreaEvaluator(DirectionMode _mode/*=SINGLE_VALUE*/): Evaluator(_mode) {
	if(whiteShader.isNull()) {
		whiteShader = Rendering::Shader::createShader(
						  "void main(void){gl_Position=ftransform();}",
						  "void main(void){gl_FragColor=vec4(1.0,1.0,1.0,1.0);}"
					  );
	}
	setMaxValue_f(1.0f);
}

//! [dtor]
AreaEvaluator::~AreaEvaluator() {
}

//! ---|> Evaluator
void AreaEvaluator::beginMeasure() {
	whitePixel=0;
	allPixel=0;
	values->clear();
}

//! ---|> Evaluator
void AreaEvaluator::measure(FrameContext & context, Node & node, const Geometry::Rect & r) {
	context.getRenderingContext().clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 1.0f));
	if(!whiteShader->isActive(context.getRenderingContext())) {
		context.getRenderingContext().pushAndSetShader(whiteShader.get());
	}
	node.display(context, 0);

	context.getRenderingContext().flush();
	unsigned int width = static_cast<unsigned int>(r.getWidth());
	unsigned int height = static_cast<unsigned int>(r.getHeight());
	unsigned int pixelCounter = width * height;
	allPixel += pixelCounter;

	Util::Reference<Rendering::Texture> framebuffer = Rendering::TextureUtils::createRedTexture(width, height,true);

	uint8_t * data = framebuffer->openLocalData(context.getRenderingContext());
	int currentWhitePixel = 0;
	for (unsigned int i = 0; i < pixelCounter; ++i) {
		if (data[i] > 0) {
			++currentWhitePixel;
		}
	}

	if(mode == DIRECTION_VALUES) {
		values->push_back(new Util::_NumberAttribute<float>(currentWhitePixel / static_cast<float>(pixelCounter)));
	} else {
		whitePixel += currentWhitePixel;
	}

}

//! ---|> Evaluator
void AreaEvaluator::endMeasure(FrameContext & context) {
	context.getRenderingContext().popShader();
	if(mode==SINGLE_VALUE){
		values->push_back(new Util::_NumberAttribute<float>(allPixel>0? whitePixel/static_cast<float>(allPixel):0));
	}
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
