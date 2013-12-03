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

#include "AbstractOnGpuComparator.h"
#include "../../Helper/DataDirectory.h"

#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/Uniform.h>
#include <Rendering/RenderingContext/RenderingParameters.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Texture/TextureUtils.h>
#include <Rendering/Draw.h>
#include <Rendering/FBO.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/IO/FileName.h>
#include <Util/Macros.h>

#include <cassert>
#include <iostream>

namespace MinSG {
namespace ImageCompare {

using Rendering::Texture;
using Rendering::Shader;
using Rendering::Uniform;
using Rendering::FBO;
using Rendering::RenderingContext;
using Util::Reference;
using Geometry::Vec2i;

std::set<Util::Reference<Rendering::Texture> > AbstractOnGpuComparator::usedTextures;
std::map<Geometry::Vec2i, std::vector<Util::Reference<Rendering::Texture> >, AbstractOnGpuComparator::Vec2iComp> AbstractOnGpuComparator::freeTextures;

AbstractOnGpuComparator::AbstractOnGpuComparator(int32_t _filterSize) :
		fbo(new FBO()), texDownSize(64), filterSize(_filterSize), filterType(GAUSS), filterValid(false), initialized(false) {
}

AbstractOnGpuComparator::~AbstractOnGpuComparator() {
	deleteTextures();
}

Util::Reference<Rendering::Texture> AbstractOnGpuComparator::createTexture(const Geometry::Vec2i & size) {

	Util::Reference<Texture> tex;
 
	std::vector<Reference<Texture> > & vec = freeTextures[size];

	if (vec.empty()) {
		tex = Rendering::TextureUtils::createHDRTexture(size.getWidth(), size.getHeight(), false, false);
		//std::cerr << "created HDR Texture: " << size << endl;
	}
	else {

		tex = vec.back();
		vec.pop_back();

	}

	usedTextures.insert(tex);

	return tex;
}

void AbstractOnGpuComparator::releaseTexture(const Util::Reference<Texture> & tex) {
	usedTextures.erase(tex);
	freeTextures[Geometry::Vec2i(tex->getWidth(), tex->getHeight())].push_back(tex);
}

void AbstractOnGpuComparator::deleteTextures() {
	assert(usedTextures.empty());
	freeTextures.clear();
// 	std::cerr << "deleting Textures: " << std::endl;
}

void AbstractOnGpuComparator::copy(Rendering::RenderingContext & context, TexRef_t src, TexRef_t dst) {

	context.pushAndSetShader(shaderCopy.get());
	context.pushAndSetTexture(0, src->get());
	context.pushAndSetScissor(Rendering::ScissorParameters(Geometry::Rect_i(0, 0, src->get()->getWidth(), src->get()->getHeight())));

	fbo->attachColorTexture(context, dst->get());

	Rendering::drawFullScreenRect(context);

	fbo->detachColorTexture(context);

	context.popScissor();
	context.popTexture(0);
	context.popShader();
}

void AbstractOnGpuComparator::filter(Rendering::RenderingContext & context, TexRef_t src, TexRef_t dst) {

	if (!filterValid) {
		shaderFilterH->setUniform(context, Uniform("filterSize", filterSize));
		shaderFilterV->setUniform(context, Uniform("filterSize", filterSize));

// 		std::cerr << getTypeName() << " Filter: " << (filterType == GAUSS ? "Gauss" : "Box") << " (" << filterSize << ") : " << "[";

		std::vector<float> values;
		if (filterType == GAUSS) {
			// old formula was filterSize * 1.5, which was too big.
			// older value used for some measurements in 2013's papers: 0.3
			const double sigma = 0.3 * (filterSize - 1) + 0.8;
			// sqrtOfTwoTimesPi = std::sqrt(2.0 * pi)
			const double sqrtOfTwoTimesPi = 2.506628274631000502415765284811045253006986740609938316629923;
			const double a = 1.0 / (sigma * sqrtOfTwoTimesPi);
			double sum = 0.0;
			for (int i = 0; i <= filterSize; i++) {
				const double v = a * std::exp(-((i * i) / (2.0 * sigma * sigma)));
				values.push_back(v);
				sum += (i == 0 ? v : 2 * v);
			}
			for (int i = 0; i <= filterSize; i++) {
				values[i] /= sum;
			}
		}
		else if (filterType == BOX) {
			for (int i = 0; i <= filterSize; i++) {
				float v = 1.0f / (filterSize * 2.0f + 1.0f);
				values.push_back(v);
			}
		}
		else
			WARN("the roof is on fire");

// 		for(const auto & x : values)
// 			std::cerr << x << " ";
// 		std::cerr << "\b]" << std::endl;

		while (values.size() < 16)
			values.push_back(0.0f);

		shaderFilterH->setUniform(context, Uniform("filterValues", values));
		shaderFilterV->setUniform(context, Uniform("filterValues", values));

		filterValid = true;
	}

	Reference<TexRef> tmp = new TexRef(Vec2i(src->get()->getWidth(), src->get()->getHeight()));
	context.pushAndSetShader(shaderFilterH.get());
	context.pushAndSetTexture(0, src->get());

	fbo->attachColorTexture(context, tmp->get());

	Rendering::drawFullScreenRect(context);

	context.setShader(shaderFilterV.get());
	context.setTexture(0, tmp->get());
	fbo->attachColorTexture(context, dst.isNull() ? src->get() : dst->get());

	Rendering::drawFullScreenRect(context);

	fbo->detachColorTexture(context);

	context.popTexture(0);
	context.popShader();
}

float AbstractOnGpuComparator::average(Rendering::RenderingContext & context, TexRef_t src) {

	// shrink on gpu
	TexRef_t in = src;

	context.pushAndSetShader(shaderShrink.get());
	context.pushTexture(0);
	context.pushViewport();

	uint32_t w = in->get()->getWidth();
	uint32_t h = in->get()->getHeight();
	while (w * h > texDownSize * texDownSize && w % 2 == 0 && h % 2 == 0) {
		w /= 2;
		h /= 2;
		Reference<TexRef> tmp = new TexRef(Geometry::Vec2i(w, h));
		fbo->attachColorTexture(context, tmp->get());
		context.setViewport(Geometry::Rect_i(0, 0, tmp->get()->getWidth(), tmp->get()->getHeight()));
		context.setTexture(0, in->get());
		Rendering::drawFullScreenRect(context);
		in = tmp;
	}

	context.popViewport();
	context.popShader();
	context.popTexture(0);

	// shrink on cpu
	in->get()->downloadGLTexture(context);
	double quality = 0.0;
	auto localBitmap = in->get()->getLocalBitmap();
	const float * tex = reinterpret_cast<const float *>(localBitmap->data());
	const float * end = reinterpret_cast<const float *>(localBitmap->data() + localBitmap->getDataSize());
	const uint32_t numFloats = end - tex;
	while (tex != end) {
		quality += *(tex++);
	}
	return quality /= numFloats;
}

void AbstractOnGpuComparator::setFBO(Util::Reference<Rendering::FBO> _fbo) {
	this->fbo = _fbo;
}

void AbstractOnGpuComparator::checkTextureSize(Geometry::Vec2i size) {
	checkTextureSize(size.x(), size.y());
}

void AbstractOnGpuComparator::checkTextureSize(uint32_t width, uint32_t height) {
	while (width * height > texDownSize * texDownSize) {
		if (width % 2 != 0 || height % 2 != 0) {
			WARN("try to use resolutions where width and height contain more prime factors of two to speed up image quality calculation");
			break;
		}
		width /= 2;
		height /= 2;
	}
}

bool AbstractOnGpuComparator::compare(Rendering::RenderingContext & context, Rendering::Texture * firstTex, Rendering::Texture * secondTex, double & value,
		Rendering::Texture * resultTex) {

	init(context);
	prepare(context);
	
	bool ret = doCompare(context, firstTex, secondTex, value, resultTex);
	finish(context);
	return ret;
}

bool AbstractOnGpuComparator::init(RenderingContext & context) {

	if (!initialized) {

		shaderCopy = Shader::loadShader(Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/ImageCompare.vs"),
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/Copy.fs"), Shader::USE_UNIFORMS);
		shaderCopy->setUniform(context, Rendering::Uniform("A", 0));

		shaderShrink = Shader::loadShader(Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/ImageCompare.vs"),
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/Shrink.fs"), Shader::USE_UNIFORMS);
		shaderShrink->setUniform(context, Rendering::Uniform("A", 0));

		shaderFilterH = Shader::loadShader(Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/ImageCompare.vs"),
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/FilterH.fs"), Shader::USE_UNIFORMS);
		shaderFilterH->setUniform(context, Rendering::Uniform("A", 0));

		shaderFilterV = Shader::loadShader(Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/ImageCompare.vs"),
				Util::FileName(MinSG::DataDirectory::getPath() + "/shader/ImageCompare/FilterV.fs"), Shader::USE_UNIFORMS);
		shaderFilterV->setUniform(context, Rendering::Uniform("A", 0));

		initialized = true;

		return true;
	}
	return false;
}

void AbstractOnGpuComparator::prepare(RenderingContext & context) {

//	timer.resume();

	context.pushViewport();
	context.pushAndSetFBO(fbo.get());

	// in case an external set fbo has a depth texture
	fbo->detachDepthTexture(context);
}

void AbstractOnGpuComparator::finish(RenderingContext & context) {

	context.popFBO();
	context.popViewport();

//	timer.stop();
//	testCounter++;
//	if (timer.getSeconds() > 1) {
//		std::cerr << "tps: " << testCounter / timer.getSeconds() << std::endl;
//		testCounter = 0;
//		timer.reset();
//		timer.stop();
//	}
}

}
}

#endif // MINSG_EXT_IMAGECOMPARE
