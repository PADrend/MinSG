/*
	This file is part of the MinSG library.
	Copyright (C) 20014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#define MINSG_EXT_THESISPETER
#ifdef MINSG_EXT_THESISPETER

#ifndef ImageState_H
#define ImageState_H

#include <MinSG/Core/States/State.h>
#include <Rendering/Texture/Texture.h>
#include <algorithm>
#include <map>

namespace Rendering{
class Texture;
}

namespace MinSG {

/**
 *  TextureState ---|> [State]
 */
class ImageState : public State {
		PROVIDES_TYPE_NAME(ImageState)

	private:
		Util::Reference<Rendering::Texture> texture;
		int textureUnit;

		stateResult_t doEnableState(FrameContext & context,Node *, const RenderParam & rp) override;
		void doDisableState(FrameContext & context,Node *, const RenderParam & rp) override;

	public:
		ImageState(Rendering::Texture * _texture=nullptr);
		ImageState(const ImageState & source);
		virtual ~ImageState()							{	}

		Rendering::Texture * getTexture()const 			{	return texture.get();	}
		void setTexture(Rendering::Texture * _texture)	{	texture=_texture;	}
		bool hasTexture()const							{	return texture.isNotNull();	}

		void setTextureUnit(int nr) 					{	textureUnit=nr;	}
		int getTextureUnit()const 						{	return textureUnit;	}

		/// ---|> [State]
		ImageState * clone() const override;
};
}
#endif // ImageState_H

#endif /* MINSG_EXT_THESISPETER */
