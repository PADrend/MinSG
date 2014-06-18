/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_IMAGECOMPARE

#include "AverageComparator.h"

#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>

#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/FBO.h>
#include <Rendering/Draw.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/IO/FileLocator.h>

#include <memory>
#include <cassert>
#include <vector>

namespace MinSG {
namespace ImageCompare {

using Rendering::Texture;
using Rendering::Shader;
using Rendering::Uniform;
using Rendering::FBO;
using Rendering::RenderingContext;
using Geometry::Vec2i;
using Util::Reference;
using std::vector;

AverageComparator::AverageComparator() :
		AbstractOnGpuComparator(1) {

}
AverageComparator::~AverageComparator() {
}

bool AverageComparator::doCompare(Rendering::RenderingContext & context, Rendering::Texture * inA, Rendering::Texture * inB, double & quality,
		Rendering::Texture * out) {

	int32_t w = inA->getWidth();
	int32_t h = inA->getHeight();
	checkTextureSize(w,h);

	context.setViewport(Geometry::Rect_i(0, 0, w, h));

	TexRef_t a = new TexRef(Vec2i(w, h));
	TexRef_t b = new TexRef(Vec2i(w, h));
	TexRef_t tmp = new TexRef(Vec2i(w, h));

	filter(context, new TexRef(inA), a);
	filter(context, new TexRef(inB), b);

	// calc error image
	fbo->attachColorTexture(context,tmp->get());
	context.pushAndSetShader(shaderDist.get());
	context.pushAndSetTexture(0, a->get());
	context.pushAndSetTexture(1, b->get());
	Rendering::drawFullScreenRect(context);
	context.popTexture(0);
	context.popTexture(1);
	context.popShader();
	fbo->detachColorTexture(context);

	quality = average(context, tmp);

	if(out)
		copy(context, tmp, new TexRef(out));

	return true;
}

bool AverageComparator::init(RenderingContext & context) {

	if (AbstractOnGpuComparator::init(context)) {
			
		const auto& locator = getShaderFileLocator();
		shaderDist = Shader::loadShader(
				locator.locateFile(Util::FileName("shader/ImageCompare/ImageCompare.vs")).second, 
				locator.locateFile(Util::FileName("shader/ImageCompare/Dist.fs")).second, Shader::USE_UNIFORMS);
	
		shaderDist->setUniform(context, Rendering::Uniform("A", 0));
		shaderDist->setUniform(context, Rendering::Uniform("B", 1));

		return true;
	}
	return false;
}

}
}
#endif // MINSG_EXT_IMAGECOMPARE
