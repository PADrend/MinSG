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

#ifndef PYRAMIDCOMPARATOR_H_
#define PYRAMIDCOMPARATOR_H_

#include "AbstractOnGpuComparator.h"

#include <memory>
#include <cassert>
#include <vector>

namespace MinSG {
namespace ImageCompare {

class PyramidComparator: public AbstractOnGpuComparator {
	PROVIDES_TYPE_NAME(PyramidComparator)

public:

	MINSGAPI PyramidComparator();

	MINSGAPI virtual ~PyramidComparator();

	MINSGAPI virtual bool doCompare(Rendering::RenderingContext & context, Rendering::Texture * inA, Rendering::Texture * inB, double & quality, Rendering::Texture * out) override;

	void setTextureDownloadSize(uint32_t sideLength) override {
		AbstractOnGpuComparator::setTextureDownloadSize(sideLength);
		comparator->setTextureDownloadSize(sideLength);
	}

	void setFilterSize(int32_t size) override {
		AbstractOnGpuComparator::setFilterSize(size);
// 		comparator->setFilterSize(size);
	}

	void setFilterType(FilterType type) override {
		AbstractOnGpuComparator::setFilterType(type);
// 		comparator->setFilterType(type);
	}

	MINSGAPI void setFBO(Util::Reference<Rendering::FBO> _fbo) override;

	AbstractOnGpuComparator * getInternalComparator() {
		return comparator.get();
	}
	MINSGAPI void setInternalComparator(AbstractOnGpuComparator * comp);

	uint32_t getMinimalTestSize() {
		return minTestSize;
	}
	void setMinimalTestSize(uint32_t sideLength) {
		minTestSize = sideLength;
	}

private:

	uint32_t minTestSize;
	Util::Reference<AbstractOnGpuComparator> comparator;
};

}
}

#endif /* PYRAMIDCOMPARATOR_H_ */

#endif // MINSG_EXT_IMAGECOMPARE
