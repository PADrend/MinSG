/*
	This file is part of the MinSG library.
	Copyright (C) 2014 Peter

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define MINSG_EXT_THESISPETER
#ifdef MINSG_EXT_THESISPETER

#include "ImageState.h"
#include <MinSG/Core/FrameContext.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Texture/Texture.h>

namespace MinSG {

//! (ctor)
ImageState::ImageState(Rendering::Texture * _texture/*=0*/):
		State(),texture(_texture),textureUnit(0) {
}

//! (ctor)
ImageState::ImageState(const ImageState & source):
		State(source),texture(source.texture),textureUnit(source.textureUnit) {
}

//! ---|> [Node]
ImageState * ImageState::clone() const {
	return new ImageState(*this);
}

//! ---|> [State]
State::stateResult_t ImageState::doEnableState(FrameContext & context,Node *, const RenderParam & /*rp*/) {
	context.getRenderingContext().pushAndSetBoundImage(getTextureUnit(), Rendering::ImageBindParameters(getTexture()));
	return State::STATE_OK;
}

//! ---|> [State]
void ImageState::doDisableState(FrameContext & context,Node *, const RenderParam & /*rp*/) {
	context.getRenderingContext().popBoundImage(getTextureUnit());
}

}

#endif /* MINSG_EXT_THESISPETER */
