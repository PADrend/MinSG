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

#include "SimilarPixelCounter.h"
#include <Rendering/Texture/Texture.h>
#include <Util/Macros.h>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace MinSG {
namespace ImageCompare {

bool SimilarPixelCounter::compare(     Rendering::RenderingContext & context,
											  Rendering::Texture * firstTex,
									   Rendering::Texture * secondTex,
									   double & value,
									   Rendering::Texture * resultTex) {
	if(firstTex == nullptr) {
		WARN("SimilarPixelCounter::compare: first texture was null");
		return false;
	}
	if(secondTex == nullptr) {
		WARN("SimilarPixelCounter::compare: second texture was null");
		return false;
	}
	const uint32_t width = static_cast<const uint32_t>(firstTex->getWidth());
	const uint32_t height = static_cast<const uint32_t>(firstTex->getHeight());
	if(width == 0 || height == 0) {
		WARN("SimilarPixelCounter::compare: texture size was zero");
		return false;
	}
	if(width != static_cast<const uint32_t>(secondTex->getWidth()) || height != static_cast<const uint32_t>(secondTex->getHeight())) {
		WARN("SimilarPixelCounter::compare: size of input textures differ");
		return false;
	}
	if(resultTex != nullptr && (width != static_cast<const uint32_t>(resultTex->getWidth()) || height != static_cast<const uint32_t>(resultTex->getHeight()))) {
		WARN("SimilarPixelCounter::compare: size of input textures and output texture differ");
		return false;
	}
	if(firstTex->isGLTextureValid())
		firstTex->downloadGLTexture(context);
	const uint32_t * firstData = reinterpret_cast<const uint32_t *>(firstTex->openLocalData(context));
	if(secondTex->isGLTextureValid())
		secondTex->downloadGLTexture(context);
	const uint32_t * secondData = reinterpret_cast<const uint32_t *>(secondTex->openLocalData(context));

	uint32_t * resultData = nullptr;
	if(resultTex != nullptr) {
		resultTex->removeGLData();
		resultData = reinterpret_cast<uint32_t *>(resultTex->openLocalData(context));
	}

	const uint32_t numPixels = width * height;
	uint_fast32_t numSame = 0;
	for(uint_fast32_t p = 0; p < numPixels; ++p) {
		const bool samePixels = (firstData[p] == secondData[p]);
		if(samePixels) {
			++numSame;
		}
		if(resultData != nullptr) {
			// Make sure that "no difference" is white.
			if(samePixels) {
				resultData[p] = 0xFFFFFFFF;
			} else {
				resultData[p] = 0xFF000000;
			}
		}
	}

	value = static_cast<double>(numSame) / static_cast<double>(numPixels);

	if(resultTex != nullptr) {
		resultTex->dataChanged();
	}

	return true;
}

}
}

#endif /* MINSG_EXT_IMAGECOMPARE */
