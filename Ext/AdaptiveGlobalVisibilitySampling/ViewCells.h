/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_AGVS_VIEWCELLS_H
#define MINSG_AGVS_VIEWCELLS_H

#include "Definitions.h"

namespace MinSG {
class ValuatedRegionNode;
namespace AGVS {
template<typename value_t> class Sample;

//! Split the view cell recursively until no further split is possible.
MINSGAPI void splitViewCell(ValuatedRegionNode * viewCell);

/**
 * Update the view cell hierarchy with a sample. The contribution of
 * the given sample will be returned.
 * 
 * @param rootViewCell The root node of the view space subdivision
 * @param sample New sample used to update the view cells
 * @param originCell View cell containing the origin of the sample
 * @return Pair of forward and backward contribution of the sample
 */
MINSGAPI contribution_t updateWithSample(ValuatedRegionNode * rootViewCell,
								const Sample<float> & sample,
								const ValuatedRegionNode * originCell);

}
}

#endif /* MINSG_AGVS_VIEWCELLS_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
