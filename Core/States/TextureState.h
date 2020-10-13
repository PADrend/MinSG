/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TextureState_H
#define TextureState_H

#include "State.h"
#include <Rendering/Texture/Texture.h>
#include <algorithm>
#include <map>

namespace Rendering{
class Texture;
}

namespace MinSG {

/**
 *  TextureState ---|> [State]
 * @ingroup states
 */
class TextureState : public State {
		PROVIDES_TYPE_NAME(TextureState)

	private:
		Util::Reference<Rendering::Texture> texture;
		int textureUnit;

		MINSGAPI stateResult_t doEnableState(FrameContext & context,Node *, const RenderParam & rp) override;
		MINSGAPI void doDisableState(FrameContext & context,Node *, const RenderParam & rp) override;

	public:
		MINSGAPI TextureState(Rendering::Texture * _texture=nullptr);
		MINSGAPI TextureState(const TextureState & source);
		virtual ~TextureState()							{	}

		Rendering::Texture * getTexture()const 			{	return texture.get();	}
		void setTexture(Rendering::Texture * _texture)	{	texture=_texture;	}
		bool hasTexture()const							{	return texture.isNotNull();	}

		void setTextureUnit(int nr) 					{	textureUnit=nr;	}
		int getTextureUnit()const 						{	return textureUnit;	}

		/// ---|> [State]
		MINSGAPI TextureState * clone() const override;
};
}
#endif // TextureState_H
