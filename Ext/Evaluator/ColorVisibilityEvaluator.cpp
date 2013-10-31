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

#include "ColorVisibilityEvaluator.h"
#include "../../Core/Nodes/GeometryNode.h"
#include "../../Core/FrameContext.h"
#include "../../Helper/StdNodeVisitors.h"
#include <Rendering/Mesh/Mesh.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/FBO.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>

#include <list>
#include <memory>

namespace MinSG {
namespace Evaluators {

static Rendering::Shader * getShader() {
	static std::unique_ptr<Rendering::Shader>
			shader(
					Rendering::Shader::createShader(
							"#version 150 compatibility\nvoid main(void){gl_Position=ftransform();}",
							"#version 150\nout vec4 fragColor; in int gl_PrimitiveID; uniform int colorOffset; void main(void){int color = gl_PrimitiveID + colorOffset; fragColor=vec4((color & 255) / 255.0, ((color >> 8) & 255) / 255.0, ((color >> 16) & 255) / 255.0, ((color >> 24) & 255) / 255.0);}"));
	return shader.get();
}

ColorVisibilityEvaluator::ColorVisibilityEvaluator(DirectionMode dirMode) :
	Evaluator(dirMode), numTrianglesVisible(0), colorTexture(nullptr), depthTexture(nullptr), fbo(new Rendering::FBO) {

}

ColorVisibilityEvaluator::~ColorVisibilityEvaluator() {
}

static inline void increaseColorUsage(uint32_t color, uint32_t usageCount, std::map<uint32_t, uint32_t> & colorUsage) {
	auto lb = colorUsage.lower_bound(color);
	if (lb != colorUsage.end() && !(colorUsage.key_comp()(color, lb->first))) {
		// Color already in map.
		lb->second += usageCount;
	} else {
		// Insert new color into map.
		colorUsage.insert(lb, std::make_pair(color, usageCount));
	}
}

void ColorVisibilityEvaluator::beginMeasure() {
	values->clear();
	numTrianglesVisible = 0;
}

void ColorVisibilityEvaluator::measure(FrameContext & context, Node & node, const Geometry::Rect & rect) {
	Rendering::RenderingContext & rCtxt = context.getRenderingContext();

	// ### Set up textures for the framebuffer object. ###
	const uint32_t width = static_cast<uint32_t> (rect.getWidth());
	const uint32_t height = static_cast<uint32_t> (rect.getHeight());
	if (colorTexture == nullptr || width != static_cast<uint32_t> (colorTexture->getWidth()) || height != static_cast<uint32_t> (colorTexture->getHeight())) {
		// (Re-)Create textures with correct dimension.
		rCtxt.pushAndSetFBO(fbo.get());
		fbo->detachColorTexture(context.getRenderingContext());
		fbo->detachDepthTexture(context.getRenderingContext());
		rCtxt.popFBO();

		colorTexture = Rendering::TextureUtils::createStdTexture(width, height, true, false);
		depthTexture = Rendering::TextureUtils::createDepthTexture(width, height);

		// Bind textures to FBO.
		rCtxt.pushAndSetFBO(fbo.get());
		fbo->attachColorTexture(context.getRenderingContext(),colorTexture.get());
		fbo->attachDepthTexture(context.getRenderingContext(),depthTexture.get());
		rCtxt.popFBO();
	}

	// ### Render the scene into a framebuffer object. ###
	rCtxt.pushAndSetFBO(fbo.get());

	rCtxt.pushAndSetShader(getShader());
	rCtxt.clearScreen(Util::Color4f(0.0f, 0.0f, 0.0f, 0.0f));


	// Color counter for assignment of unique colors to triangles.
	// Background is black (color 0).
	int32_t currentColor = 1;


	// Collect potentially visible geometry nodes.
	const auto geoNodes = collectNodesInFrustum<GeometryNode>(&node, context.getCamera()->getFrustum());
	for(const auto & geoNode : geoNodes) {
		getShader()->setUniform(rCtxt, Rendering::Uniform("colorOffset", currentColor));
		context.displayNode(geoNode, USE_WORLD_MATRIX | NO_STATES);
		currentColor += geoNode->getTriangleCount();
	}

	rCtxt.popShader();
	rCtxt.popFBO();


	// ### Read texture and analyze colors. ###

	// Map from colors to number of occurrences.
	std::map<uint32_t, uint32_t> colorUsage;


	// Current color is compared with the previous color. If they match, the local count variable is used as cache before insertion into map is performed.
	uint32_t lastColor = 0;
	uint32_t lastCount = 0;

	colorTexture->downloadGLTexture(rCtxt);
	const uint32_t * texData = reinterpret_cast<const uint32_t *> (colorTexture->openLocalData(rCtxt));
	const uint32_t numPixels = width * height;
	for (uint_fast32_t p = 0; p < numPixels; ++p) {
		const uint32_t & color = texData[p];

		if (color == 0) {
			// Ignore background.
			continue;
		}

		if (color == lastColor) {
			++lastCount;
			continue;
		} else {
			// Insert previous color.
			if (lastColor != 0) {
				increaseColorUsage(lastColor, lastCount, colorUsage);
			}

			lastColor = color;
			lastCount = 1;
		}
	}
	if (lastColor != 0) {
		increaseColorUsage(lastColor, lastCount, colorUsage);
	}

	if (mode == SINGLE_VALUE) {
		numTrianglesVisible += colorUsage.size();
	} else {
		values->push_back(Util::GenericAttribute::createNumber(colorUsage.size()));
	}
}

void ColorVisibilityEvaluator::endMeasure(FrameContext & /*context*/) {
	if (mode == SINGLE_VALUE) {
		values->push_back(Util::GenericAttribute::createNumber(numTrianglesVisible));
	}
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
