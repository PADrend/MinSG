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

#ifndef SSIMCOMPARATOR_H_
#define SSIMCOMPARATOR_H_

#include "AbstractOnGpuComparator.h"

namespace MinSG {
namespace ImageCompare {

/**
 * Comparator using the SSIM index.
 * 
 * Based on the article:
 * Z. Wang; A. Bovik; H. Sheikh & E. Simoncelli: Image quality assessment: from error visibility to structural similarity.
 * IEEE Transactions on Image Processing, vol. 13, no. 4, pp. 600-612, 2004.
 */
class SSIMComparator: public AbstractOnGpuComparator {
	PROVIDES_TYPE_NAME(SSIMComparator)

public:

	MINSGAPI SSIMComparator();

	MINSGAPI virtual ~SSIMComparator();

	MINSGAPI virtual bool doCompare(Rendering::RenderingContext & context, Rendering::Texture * inA, Rendering::Texture * inB, double & quality,
			Rendering::Texture * out) override;

	MINSGAPI virtual bool init(Rendering::RenderingContext & context) override;

private:
	Util::Reference<Rendering::Shader> shaderSSIM;
	Util::Reference<Rendering::Shader> shaderSSIMPrep;
};

}
}

#endif /* SSIMCOMPARATOR_H_ */

#endif // MINSG_EXT_IMAGECOMPARE
