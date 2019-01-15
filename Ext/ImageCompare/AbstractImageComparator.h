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

#ifndef ABSTRACTIMAGECOMPARATOR_H
#define ABSTRACTIMAGECOMPARATOR_H

#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>

namespace Rendering {
class RenderingContext;
class Texture;
}

namespace MinSG {
//! @ingroup ext
namespace ImageCompare {

/**
 * Abstract base class for classes that compare two images to each other.
 * Subclasses implement special algorithms for image comparison.
 *
 * @author Benjamin Eikel
 * @date 2011-06-14
 */
class AbstractImageComparator : public Util::ReferenceCounter<AbstractImageComparator> {
	PROVIDES_TYPE_NAME(AbstractImageComparator)
	public:
		AbstractImageComparator() : Util::ReferenceCounter<AbstractImageComparator>() {
		}
		virtual ~AbstractImageComparator() {
		}

		/**
		 * Compare two images to each other.
		 * Return a value from [0.0, 1.0], where 1.0 means that the two images are the same.
		 *
		 * @param[in] context Current rendering context.
		 * @param[in] firstTex First source image to use for comparison. Must not be @c nullptr.
		 * @param[in] secondTex Second source image to use for comparison. Must not be @c nullptr.
		 * @param[out] value Result of the image comparison. The interpretation of the value depends on the actual implementation.
		 * @param[out] resultTex Resulting texture that contains any kind of difference image specific to the actual implementation. May be @c nullptr.
		 * @return @c true if the comparison was successful, @c false if something went wrong.
		 */
		virtual bool compare(Rendering::RenderingContext & context,
							 Rendering::Texture * firstTex,
							 Rendering::Texture * secondTex,
							 double & value,
							 Rendering::Texture * resultTex) = 0;
};

}
}

#endif /* ABSTRACTIMAGECOMPARATOR_H */

#endif /* MINSG_EXT_IMAGECOMPARE */
