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

#include "SSIMComparator.h"
#include "../../Helper/DataDirectory.h"

#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>

#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/FBO.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>

#include <memory>
#include <cassert>
#include <vector>

namespace MinSG {
namespace ImageCompare {

using Rendering::TextureUtils;
using Geometry::Vec2i;
using Rendering::Texture;
using Rendering::RenderingContext;
using Rendering::Shader;
using Util::Reference;

SSIMComparator::SSIMComparator() :
		AbstractOnGpuComparator(5) {
}

SSIMComparator::~SSIMComparator() {
}

bool SSIMComparator::doCompare(RenderingContext & context, Texture * inA, Texture * inB, double & quality, Texture * out) {

	Vec2i size(inA->getWidth(), inA->getHeight());
	checkTextureSize(size);
	context.setViewport(Geometry::Rect_i(0, 0, size.getWidth(), size.getHeight()));

	TexRef_t a = new TexRef(size);
	TexRef_t b = new TexRef(size);
	TexRef_t aa = new TexRef(size);
	TexRef_t bb = new TexRef(size);
	TexRef_t ab = new TexRef(size);
	TexRef_t tmp = new TexRef(size);

	// ssim prepare
	fbo->attachColorTexture(context,aa->get(), 0);
	fbo->attachColorTexture(context,bb->get(), 1);
	fbo->attachColorTexture(context,ab->get(), 2);
	fbo->setDrawBuffers(3);
	context.pushAndSetShader(shaderSSIMPrep.get());
	context.pushAndSetTexture(0,inA);
	context.pushAndSetTexture(1,inB);
	Rendering::drawFullScreenRect(context);
	context.popTexture(0);
	context.popTexture(1);
	context.popShader();
	fbo->setDrawBuffers(1);
	fbo->detachColorTexture(context,0);
	fbo->detachColorTexture(context,1);
	fbo->detachColorTexture(context,2);

	// filtering
	filter(context, new TexRef(inA), a);
	filter(context, new TexRef(inB), b);
	filter(context, aa, aa);
	filter(context, bb, bb);
	filter(context, ab, ab);

	// ssim main
	fbo->attachColorTexture(context,tmp->get());
	context.pushAndSetShader(shaderSSIM.get());
	context.pushAndSetTexture(0,a->get());
	context.pushAndSetTexture(1,b->get());
	context.pushAndSetTexture(2,aa->get());
	context.pushAndSetTexture(3,bb->get());
	context.pushAndSetTexture(4,ab->get());
	Rendering::drawFullScreenRect(context);
	context.popTexture(0);
	context.popTexture(1);
	context.popTexture(2);
	context.popTexture(3);
	context.popTexture(4);
	context.popShader();

	// shrink
	quality = average(context, tmp);

	if(out)
		copy(context, tmp, new TexRef(out));

	return true;
}

bool SSIMComparator::init(RenderingContext & context) {

	if (AbstractOnGpuComparator::init(context)) {

		shaderSSIM = Shader::loadShader(
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/ImageCompare.vs"),
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/SSIM.fs"),
				Shader::USE_UNIFORMS);
		shaderSSIM->setUniform(context, Rendering::Uniform("A", 0));
		shaderSSIM->setUniform(context, Rendering::Uniform("B", 1));
		shaderSSIM->setUniform(context, Rendering::Uniform("AA", 2));
		shaderSSIM->setUniform(context, Rendering::Uniform("BB", 3));
		shaderSSIM->setUniform(context, Rendering::Uniform("AB", 4));

		shaderSSIMPrep = Shader::loadShader(
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/ImageCompare.vs"),
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/SSIMPrep.fs"),
				Shader::USE_UNIFORMS);
		shaderSSIMPrep->setUniform(context, Rendering::Uniform("A", 0));
		shaderSSIMPrep->setUniform(context, Rendering::Uniform("B", 1));

		return true;
	}
	return false;
}

}
}

#endif // MINSG_EXT_IMAGECOMPARE
