/*
	This file is part of the MinSG library extension ThesisPeter.
	Copyright (C) 2014 Peter Stilow

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#define MINSG_EXT_THESISPETER
#ifdef MINSG_EXT_THESISPETER

#include "TextureProcessor.h"

#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/Texture/TextureUtils.h>

namespace MinSG {
namespace ThesisPeter {

TextureProcessor::TextureProcessor(){
	shader = 0;
	inputTexture = 0;
	outputTexture = 0;

	width = 0;
	height = 0;
}

TextureProcessor::~TextureProcessor(){

}

void TextureProcessor::setRenderingContext(Rendering::RenderingContext* renderingContext){
	this->renderingContext = renderingContext;
}

void TextureProcessor::setShader(Rendering::Shader* shader){
	this->shader = shader;
}

void TextureProcessor::setInputTexture(Rendering::Texture* texture){
	this->inputTexture = texture;
}

void TextureProcessor::setOutputTexture(Rendering::Texture* texture){
	this->outputTexture = texture;
}

void TextureProcessor::begin(){
	if(outputTexture == 0){
		DEBUG("No output texture defined!");
		return;
	}
	if(renderingContext == 0){
		DEBUG("No rendering context defined!");
		return;
	}

	renderingContext->pushAndSetFBO(&fbo);
	fbo.attachColorTexture(*renderingContext, outputTexture, 0);

	fbo.setDrawBuffers(1);
// 	out(fbo.getStatusMessage(renderingContext),"\n");

	renderingContext->pushAndSetShader(shader);

	width = outputTexture->getWidth();
	height = outputTexture->getHeight();
	renderingContext->pushAndSetScissor(Rendering::ScissorParameters(Geometry::Rect_i(0, 0, width, height)));
	renderingContext->pushViewport();
	renderingContext->setViewport(Geometry::Rect_i(0, 0, width, height));
}

void TextureProcessor::end(){
	renderingContext->popScissor();
	renderingContext->popViewport();

	fbo.detachColorTexture(*renderingContext, 0);

	renderingContext->popShader();

	renderingContext->popFBO();
}

void TextureProcessor::execute(){
	begin();
	Rendering::TextureUtils::drawTextureToScreen(*renderingContext, Geometry::Rect_i(0, 0, width, height), *inputTexture, Geometry::Rect_f(0, 0, 1, 1));
	end();
}

}
}

#endif /* MINSG_EXT_THESISPETER */
