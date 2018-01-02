/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_FRUSTUMTEST_H
#define MINSG_FRUSTUMTEST_H

#include "../Core/RenderParam.h"
#include <Geometry/Box.h>
#include <Geometry/Frustum.h>

namespace MinSG {

/**
 * Perform a test if a box intersects a frustum, if frustum culling is enabled.
 * 
 * @param frustum Frustum that the box is tested against
 * @param box Box to be tested
 * @param rp %Rendering parameters that decide if frustum culling is enabled
 * @retval true if frustum culling is disabled or the box intersects the
 * frustum
 * @retval false if frustum culling is enabled and the box does not intersect
 * the frustum
 */
inline bool conditionalFrustumTest(const Geometry::Frustum & frustum,
								   const Geometry::Box & box,
								   const RenderParam & rp) {
	return !rp.getFlag(FRUSTUM_CULLING) 
				|| (frustum.isBoxInFrustum(box) != Geometry::Frustum::intersection_t::OUTSIDE);
}

}

#endif /* MINSG_FRUSTUMTEST_H */
