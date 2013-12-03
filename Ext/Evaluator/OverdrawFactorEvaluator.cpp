/*
	This file is part of the MinSG library extension Evaluator.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_EVALUATORS

#include "OverdrawFactorEvaluator.h"
#include "../../Core/FrameContext.h"
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/GenericAttribute.h>
#include <algorithm>

// // Needed for writing a debugging image
// #include <Rendering/Serialization/Serialization.h>
// #include <array>

namespace MinSG {
namespace Evaluators {

OverdrawFactorEvaluator::OverdrawFactorEvaluator(DirectionMode _mode) :
	Evaluator(_mode), resultQuantile(0.5), resultRemoveZeroValues(true) {
	setMaxValue_i(0u);
}

OverdrawFactorEvaluator::~OverdrawFactorEvaluator() = default;

void OverdrawFactorEvaluator::beginMeasure() {
	values->clear();
	setMaxValue_i(0u);
}

void OverdrawFactorEvaluator::measure(FrameContext & frameContext, Node & node, const Geometry::Rect & rect) {
	Rendering::RenderingContext & renderingContext = frameContext.getRenderingContext();

	// Set up FBO and texture
	Util::Reference<Rendering::FBO> fbo = new Rendering::FBO;
	renderingContext.pushAndSetFBO(fbo.get());
	Util::Reference<Rendering::Texture> depthStencilTexture = Rendering::TextureUtils::createDepthStencilTexture(rect.getWidth(), rect.getHeight());
	fbo->attachDepthStencilTexture(renderingContext, depthStencilTexture.get());

	// Disable color and depth writes
	renderingContext.pushAndSetColorBuffer(Rendering::ColorBufferParameters(false, false, false, false));
	renderingContext.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LESS));

	// Increase the stencil value for every rendered pixel
	Rendering::StencilParameters stencilParams;
	stencilParams.enable();
	stencilParams.setFunction(Rendering::Comparison::ALWAYS);
	stencilParams.setReferenceValue(0);
	stencilParams.setBitMask(0);
	stencilParams.setFailAction(Rendering::StencilParameters::INCR);
	stencilParams.setDepthTestFailAction(Rendering::StencilParameters::INCR);
	stencilParams.setDepthTestPassAction(Rendering::StencilParameters::INCR);
	renderingContext.pushAndSetStencil(stencilParams);

	// Render the node
	renderingContext.clearStencil(0);
	frameContext.displayNode(&node, 0);

	// Reset GL state
	renderingContext.popStencil();
	renderingContext.popDepthBuffer();
	renderingContext.popColorBuffer();

	// Fetch the texture.
	depthStencilTexture->downloadGLTexture(renderingContext);
	renderingContext.popFBO();

	Util::Reference<Util::PixelAccessor> stencilAccessor = Rendering::TextureUtils::createStencilPixelAccessor(renderingContext, depthStencilTexture.get());

	std::vector<uint8_t> stencilValues;
	stencilValues.reserve(rect.getWidth() * rect.getHeight());
	for(uint_fast32_t y = 0; y < rect.getHeight(); ++y) {
		for(uint_fast32_t x = 0; x < rect.getWidth(); ++x) {
			const uint8_t stencilValue = stencilAccessor->readSingleValueByte(x, y);
			stencilValues.push_back(stencilValue);
		}
	}

// 	// Create and write debug image
// 	{
// 		Util::Reference<Rendering::Texture> colorTexture = Rendering::TextureUtils::createStdTexture(rect.getWidth(), rect.getHeight(), true);
// 		Util::Reference<Util::PixelAccessor> colorAccessor = Rendering::TextureUtils::createColorPixelAccessor(renderingContext, colorTexture.get());
// 		const auto stencilMinMax = std::minmax_element(stencilValues.cbegin(), stencilValues.cend());
// 		const auto stencilMin = *stencilMinMax.first;
// 		const auto stencilMax = *stencilMinMax.second;
// 		const double stencilRange = stencilMax - stencilMin;
// 		const double factor = 1.0 / stencilRange;
// 		// Color scheme RdYlBu with 5 of 8 colors from www.colorbrewer2.org
// 		const std::array<Util::Color4f, 5> gradient =	{
// 															Util::Color4ub(44, 123, 182, 255),
// 															Util::Color4ub(171, 217, 233, 255),
// 															Util::Color4ub(255, 255, 191, 255),
// 															Util::Color4ub(253, 174, 97, 255),
// 															Util::Color4ub(215, 25, 28, 255)
// 														};
// 		for(uint_fast32_t y = 0; y < rect.getHeight(); ++y) {
// 			for(uint_fast32_t x = 0; x < rect.getWidth(); ++x) {
// 				const uint8_t stencilValue = stencilValues[y * rect.getWidth() + x];
// 				if(stencilValue == 0) {
// 					colorAccessor->writeColor(x, y, Util::Color4f(1.0, 1.0, 1.0, 0.0));
// 				} else {
// 					const double normalizedValue = static_cast<double>(stencilValue - stencilMin) * factor;
// 					const double gradientPos = normalizedValue * (gradient.size() - 1);
// 					const size_t gradientIndex = std::floor(gradientPos);
// 					colorAccessor->writeColor(x, y, Util::Color4f(gradient[gradientIndex], gradient[gradientIndex + 1], gradientPos - gradientIndex));
// 				}
// 			}
// 		}
// 		Rendering::Serialization::saveTexture(renderingContext, colorTexture.get(), Util::FileName("stencil.png"));
// 	}

	if(resultRemoveZeroValues) {
		stencilValues.erase(std::remove(stencilValues.begin(), stencilValues.end(), 0),
							stencilValues.end());
	}
	uint8_t result = 0;
	if(!stencilValues.empty()) {
		if(resultQuantile >= 1.0) {
			result = *std::max_element(stencilValues.cbegin(), stencilValues.cend());
		} else if(resultQuantile <= 0.0) {
			result = *std::min_element(stencilValues.cbegin(), stencilValues.cend());
		} else {
			const std::size_t quantilePos = resultQuantile * stencilValues.size();
			std::nth_element(stencilValues.begin(),
							 std::next(stencilValues.begin(), static_cast<std::ptrdiff_t>(quantilePos)),
							 stencilValues.end());
			result = stencilValues[quantilePos];
		}
	}

	values->push_back(Util::GenericAttribute::createNumber(result));
	setMaxValue_i(result);
}

void OverdrawFactorEvaluator::endMeasure(FrameContext &) {
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
