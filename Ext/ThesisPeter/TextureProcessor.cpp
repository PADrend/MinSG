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
	depthTexture = 0;

	width = 0;
	height = 0;

	fbo = new Rendering::FBO();
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
	width = outputTexture->getWidth();
	height = outputTexture->getHeight();
}

void TextureProcessor::setDepthTexture(Rendering::Texture* texture){
	this->depthTexture = texture;
}

void TextureProcessor::begin(){
	if(outputTexture == 0){
		std::cout << "No output texture defined!" << std::endl;
		return;
	}
	if(renderingContext == 0){
		std::cout << "No rendering context defined!" << std::endl;
		return;
	}

	renderingContext->pushAndSetFBO(fbo.get());
	fbo->attachColorTexture(*renderingContext, outputTexture, 0);
	if(depthTexture != 0) fbo->attachDepthTexture(*renderingContext, depthTexture);
	fbo->setDrawBuffers(1);

	renderingContext->pushAndSetShader(shader);

	renderingContext->pushAndSetScissor(Rendering::ScissorParameters(Geometry::Rect_i(0, 0, width, height)));
	renderingContext->pushViewport();
	renderingContext->setViewport(Geometry::Rect_i(0, 0, width, height));
}

void TextureProcessor::end(){
	if(renderingContext == 0){
		std::cout << "No rendering context defined!" << std::endl;
		return;
	}
	renderingContext->popViewport();
	renderingContext->popScissor();

	if(depthTexture != 0)
		fbo->detachDepthTexture(*renderingContext);
	fbo->detachColorTexture(*renderingContext, 0);

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
