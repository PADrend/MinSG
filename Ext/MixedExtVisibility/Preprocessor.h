/*
	This file is part of the MinSG library extension MixedExtVisibility.
	Copyright (C) 2015 Claudius Jähn <claudius@uni-paderborn.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_MIXED_EXTERN_VISIBILITY

#ifndef E_EXTVISIBILITY_PREPROCESSOR_H
#define E_EXTVISIBILITY_PREPROCESSOR_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include <Util/References.h>

namespace MinSG {
class Node;
class FrameContext;
class AbstractCameraNode;
//typedef uint8_t renderingLayerMask_t;	//! \see RenderingLayer.h

//! @ingroup ext
namespace MixedExtVisibility {

MINSGAPI std::vector<Util::Reference<Node>> filterAndSortNodesByExtVisibility(FrameContext & context,
																	const std::vector<Util::Reference<MinSG::Node>>& nodes,
																	const std::vector<Util::Reference<AbstractCameraNode>>& cameras,
																	size_t polygonLimit=0);

}
}
#endif /* E_EXTVISIBILITY_PREPROCESSOR_H */

#endif /* MINSG_EXT_MIXED_EXTERN_VISIBILITY */
