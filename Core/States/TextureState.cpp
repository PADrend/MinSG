/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TextureState.h"
#include "../FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/Texture.h>

namespace MinSG {

//! (ctor)
TextureState::TextureState(Rendering::Texture * _texture/*=0*/):
		State(),texture(_texture),textureUnit(0) {
}

//! (ctor)
TextureState::TextureState(const TextureState & source):
		State(source),texture(source.texture),textureUnit(source.textureUnit) {
}

//! ---|> [Node]
TextureState * TextureState::clone() const {
	return new TextureState(*this);
}

//! ---|> [State]
State::stateResult_t TextureState::doEnableState(FrameContext & context,Node *, const RenderParam & /*rp*/) {
	context.getRenderingContext().pushAndSetTexture(getTextureUnit(),getTexture());
	return State::STATE_OK;
}

//! ---|> [State]
void TextureState::doDisableState(FrameContext & context,Node *, const RenderParam & /*rp*/) {
	context.getRenderingContext().popTexture(getTextureUnit());
}

}
