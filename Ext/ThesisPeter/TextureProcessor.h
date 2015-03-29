/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_THESISPETER

#ifndef MINSG_THESISPETER_TEXTUREPROCESSOR_H_
#define MINSG_THESISPETER_TEXTUREPROCESSOR_H_

#include <Rendering/Shader/Shader.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/FBO.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Macros.h>

namespace MinSG {
namespace ThesisPeter {

class TextureProcessor {
public:
	TextureProcessor();
	virtual ~TextureProcessor();

	void setRenderingContext(Rendering::RenderingContext* renderingContext);
	void setShader(Rendering::Shader* shader);
	void setInputTexture(Rendering::Texture* texture);
	void setOutputTexture(Rendering::Texture* texture);
	void setDepthTexture(Rendering::Texture* texture);
	void begin();
	void end();
	void execute();

private:
	Rendering::Shader* shader;
	Rendering::Texture* inputTexture;
	Rendering::Texture* outputTexture;
	Rendering::Texture* depthTexture;
	Util::Reference<Rendering::FBO> fbo;
	Rendering::RenderingContext* renderingContext;

	unsigned int width, height;
};

}
}

#endif /* MINSG_THESISPETER_TEXTUREPROCESSOR_H_ */

#endif /* MINSG_EXT_THESISPETER */
