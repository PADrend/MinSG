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

#include "PyramidComparator.h"
#include "SSIMComparator.h"
#include "AverageComparator.h"

#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>

#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/Draw.h>
#include <Rendering/FBO.h>
#include <Rendering/RenderingContext/RenderingContext.h>

namespace MinSG {
namespace ImageCompare {

using Rendering::TextureUtils;
using Rendering::RenderingContext;
using Geometry::Vec2i;
using Rendering::Texture;
using Rendering::Uniform;
using Util::Reference;

PyramidComparator::PyramidComparator() :
		AbstractOnGpuComparator(5) {
	minTestSize = 64;
	setInternalComparator(new SSIMComparator());
}

PyramidComparator::~PyramidComparator() {
}

void PyramidComparator::setInternalComparator(ImageCompare::AbstractOnGpuComparator * comp) {
	comparator = comp;
	comparator->setFBO(fbo);
// 	comparator->setFilterSize(getFilterSize());
// 	comparator->setFilterType(filterType);
	comparator->setTextureDownloadSize(getTextureDownloadSize());
}

void PyramidComparator::setFBO(Util::Reference<Rendering::FBO> _fbo) {
	AbstractOnGpuComparator::setFBO(_fbo);
	comparator->setFBO(_fbo);
}

bool PyramidComparator::doCompare(RenderingContext & context, Texture * inA, Texture * inB, double & quality, Texture * out) {

	comparator->init(context);

	uint32_t w = inA->getWidth();
	uint32_t h = inA->getHeight();
	checkTextureSize(w, h);
	context.setViewport(Geometry::Rect_i(0, 0, w, h));

	TexRef_t a = new TexRef(Geometry::Vec2i(w, h));
	TexRef_t b = new TexRef(Geometry::Vec2i(w, h));

	std::vector<double> results;
	comparator->doCompare(context, inA, inB, quality, out);
	results.push_back(quality);

	bool first = true;

	while (w % 2 == 0 && h % 2 == 0 && w / 2 * h / 2 > minTestSize * minTestSize) {

		if (first) {
			first = false;
			filter(context, new TexRef(inA), a);
			filter(context, new TexRef(inB), b);
		}
		else {
			filter(context, a, a);
			filter(context, b, b);
		}

		w /= 2;
		h /= 2;
		context.setViewport(Geometry::Rect_i(0, 0, w, h));
		Reference<TexRef> a2 = new TexRef(Geometry::Vec2i(w, h));
		Reference<TexRef> b2 = new TexRef(Geometry::Vec2i(w, h));

		// shrink images
		context.pushAndSetShader(shaderShrink.get());
		context.pushAndSetTexture(0, a->get());
		fbo->attachColorTexture(context, a2->get());
		Rendering::drawFullScreenRect(context);
		context.setTexture(0, b->get());
		fbo->attachColorTexture(context, b2->get());
		Rendering::drawFullScreenRect(context);
		context.popShader();
		context.popTexture(0);
		fbo->detachColorTexture(context);

		a = a2;
		b = b2;

		comparator->doCompare(context, a->get(), b->get(), quality, out);
		results.push_back(quality);
	}

	quality = 0;

	for (auto & result : results) {
		quality += result;
	}

	quality /= results.size();

	return true;
}
}
}
#endif // MINSG_EXT_IMAGECOMPARE
