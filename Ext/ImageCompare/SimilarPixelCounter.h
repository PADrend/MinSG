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

#ifndef SIMILARPIXELCOUNTER_H
#define SIMILARPIXELCOUNTER_H

#include "AbstractImageComparator.h"

namespace Rendering {
class RenderingContext;
class Texture;
}

namespace MinSG {
namespace ImageCompare {

/**
 * Count the quantum of pixels which are the similar in their color between the two images.
 *
 * @author Benjamin Eikel
 * @date 2011-06-14
 */
class SimilarPixelCounter : public AbstractImageComparator {
	PROVIDES_TYPE_NAME(SimilarPixelCounter)
	public:
		SimilarPixelCounter() : AbstractImageComparator() {
		}
		SimilarPixelCounter(const SimilarPixelCounter & other) : AbstractImageComparator(other) {
		}
		virtual ~SimilarPixelCounter() {
		}
		SimilarPixelCounter & operator=(const SimilarPixelCounter & /*other*/) {
			return *this;
		}

		/**
		 * @param[out] value Share of correct pixels on the whole image. 1.0 means there are no different pixels, 0.0 means that all pixels differ.
		 * @param[out] resultTex Texture containing the absolute difference values between the two input textures.
		 */
		MINSGAPI virtual bool compare(Rendering::RenderingContext & context,
							   Rendering::Texture * firstTex,
							 Rendering::Texture * secondTex,
							 double & value,
							 Rendering::Texture * resultTex) override;
};

}
}

#endif /* SIMILARPIXELCOUNTER_H */

#endif /* MINSG_EXT_IMAGECOMPARE */
