/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "TextAnnotation.h"
#include "../Core/FrameContext.h"
#include "../Core/Nodes/AbstractCameraNode.h"
#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/TextRenderer.h>
#include <Util/Graphics/Color.h>
#include <Util/StringUtils.h>
#include <string>

namespace MinSG {
namespace TextAnnotation {

void displayText(FrameContext & frameContext,
				 const Geometry::Vec3f & worldPos,
				 const Geometry::Vec2i & pinVector,
				 const float pinWidth,
				 const Util::Color4f & backgroundColor,
				 const Rendering::TextRenderer & textRenderer,
				 const std::string & text,
				 const Util::Color4f & textColor) {
	Rendering::RenderingContext & renderingContext = frameContext.getRenderingContext();
	const auto screenPos = frameContext.convertWorldPosToScreenPos(worldPos);
	const auto pinEnd = Geometry::Vec2i(static_cast<int32_t>(screenPos.getX()), static_cast<int32_t>(screenPos.getY())) + pinVector;

	const auto wideText = Util::StringUtils::utf8_to_utf32(text);
	const auto textRect = textRenderer.getTextSize(wideText);

	const auto borderWidth = textRenderer.getWidthOfM() / 3.0f;
	const auto borderHeight = textRenderer.getHeightOfX() / 3.0f;

	const Geometry::Rect bgRect(static_cast<float>(pinEnd.getX()),
								pinEnd.getY() - (textRect.getHeight() / 2.0f) - borderHeight,
								textRect.getWidth() + 2.0f * borderWidth,
								textRect.getHeight() + 2.0f * borderHeight);

	const Geometry::Vec2i textPos(static_cast<int32_t>(bgRect.getX() - textRect.getX() + borderWidth),
								  static_cast<int32_t>(bgRect.getY() - textRect.getY() + borderHeight));

	if(frameContext.hasCamera())
		Rendering::enable2DMode(renderingContext,frameContext.getCamera()->getViewport());
	else
		Rendering::enable2DMode(renderingContext);
	renderingContext.pushAndSetBlending(Rendering::BlendingParameters());
	renderingContext.pushAndSetDepthBuffer(Rendering::DepthBufferParameters(false, false, Rendering::Comparison::LESS));
	renderingContext.pushAndSetLighting(Rendering::LightingParameters());
	renderingContext.pushAndSetLine(Rendering::LineParameters(pinWidth));
	renderingContext.applyChanges(true);

	Rendering::drawRect(renderingContext, bgRect, backgroundColor);
	Rendering::drawVector(renderingContext, screenPos, Geometry::Vec3f(static_cast<float>(pinEnd.getX()), static_cast<float>(pinEnd.getY()), 0.0f), backgroundColor);

	renderingContext.popLine();
	renderingContext.popLighting();
	renderingContext.popDepthBuffer();
	renderingContext.popBlending();

	textRenderer.draw(renderingContext, wideText, textPos, textColor);

	Rendering::disable2DMode(renderingContext);

}

}
}
