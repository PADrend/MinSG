/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "SkyboxState.h"

#include "../../Core/Nodes/AbstractCameraNode.h"
#include "../../Core/FrameContext.h"
#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Serialization/Serialization.h>
#include <Rendering/Draw.h>
#include <Util/IO/FileName.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/StringUtils.h>

using namespace Rendering;
using namespace Geometry;

namespace MinSG {

SkyboxState * SkyboxState::createSkybox(const std::string & filename) {
	std::string s = filename;

	auto sb = new SkyboxState;

	s = Util::StringUtils::replaceAll(filename, "?", "TOP");
	sb->setTexture(SIDE_Y_POS, Serialization::loadTexture(Util::FileName(s)).detachAndDecrease());
	s = Util::StringUtils::replaceAll(filename, "?", "BOTTOM");
	sb->setTexture(SIDE_Y_NEG, Serialization::loadTexture(Util::FileName(s)).detachAndDecrease());
	s = Util::StringUtils::replaceAll(filename, "?", "LEFT");
	sb->setTexture(SIDE_X_NEG, Serialization::loadTexture(Util::FileName(s)).detachAndDecrease());
	s = Util::StringUtils::replaceAll(filename, "?", "RIGHT");
	sb->setTexture(SIDE_X_POS, Serialization::loadTexture(Util::FileName(s)).detachAndDecrease());
	// FIXME: Texture file names and Box side names are not the same.
	s = Util::StringUtils::replaceAll(filename, "?", "BACK");
	sb->setTexture(SIDE_Z_POS, Serialization::loadTexture(Util::FileName(s)).detachAndDecrease());
	s = Util::StringUtils::replaceAll(filename, "?", "FRONT");
	sb->setTexture(SIDE_Z_NEG, Serialization::loadTexture(Util::FileName(s)).detachAndDecrease());

	return sb;
}

SkyboxState::SkyboxState() :
	State(), sideLength(5.0f) {
	for (auto & elem : textures) {
		elem = nullptr;
	}
}

SkyboxState::SkyboxState(const SkyboxState & source) :
	State(source), sideLength(source.sideLength) {
	for (uint_fast8_t side = 0; side < 6; ++side) {
		textures[side] = nullptr;
		setTexture(static_cast<Geometry::side_t> (side), source.textures[side].get());
	}
}

void SkyboxState::setTexture(Geometry::side_t side, Rendering::Texture * texture) {
	textures[side] = texture;
}

SkyboxState * SkyboxState::clone() const {
	return new SkyboxState(*this);
}

State::stateResult_t SkyboxState::doEnableState(FrameContext & context, Node *, const RenderParam & rp) {
	if (rp.getFlag(NO_GEOMETRY)) {
		return State::STATE_SKIPPED;
	}

	Vec3 pos = context.getCamera()->getWorldPosition();
	Geometry::Matrix4x4 matrix;
	matrix.translate(pos);
	context.getRenderingContext().pushMatrix();
	context.getRenderingContext().multMatrix(matrix);
	context.getRenderingContext().pushTexture(0);

	context.getRenderingContext().pushAndSetDepthBuffer(Rendering::DepthBufferParameters(true, false, Rendering::Comparison::LESS));
	context.getRenderingContext().pushAndSetLighting(Rendering::LightingParameters(false));

	Geometry::Box box(Geometry::Vec3(0.0f, 0.0f, 0.0f), sideLength);
	for (uint_fast8_t side = 0; side < 6; ++side) {
		if (!textures[side].isNull()) {
			context.getRenderingContext().setTexture(0,textures[side].get());
			const Geometry::corner_t * corners = Geometry::Helper::getCornerIndices(static_cast<Geometry::side_t> (side));
			// Reverse the corner order because we need the inner surface.
			Rendering::drawQuad(context.getRenderingContext(), box.getCorner(corners[1]), box.getCorner(corners[0]), box.getCorner(corners[3]), box.getCorner(corners[2]),Util::ColorLibrary::WHITE);

		}
	}
	context.getRenderingContext().popLighting();
	context.getRenderingContext().popDepthBuffer();
	context.getRenderingContext().popTexture(0);

	context.getRenderingContext().popMatrix();
	return State::STATE_OK;
}

}
