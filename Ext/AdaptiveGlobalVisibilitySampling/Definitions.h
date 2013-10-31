/*
	This file is part of the MinSG library extension
	AdaptiveGlobalVisibilitySampling.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING

#ifndef MINSG_EXT_RAYCASTING
#error "MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING requires MINSG_EXT_RAYCASTING."
#endif /* MINSG_EXT_TRIANGLETREES */

#ifndef MINSG_AGVS_DEFINITIONS_H
#define MINSG_AGVS_DEFINITIONS_H

#include <cstdint>
#include <utility>

namespace MinSG {
namespace AGVS {

/**
 * Pair used to store the contribution of a sample.
 * - First entry: Number of times that the sample's @a forwardResult has been
 *   added to view cells
 * - Second entry: Number of times that the sample's @a backwardResult has been
 *   added to view cells
 * - Third entry: Number of entries that have been added to the view cell
 *   containing the sample's origin.
 */
typedef std::tuple<uint16_t, uint16_t, uint16_t> contribution_t;

}
}

#endif /* MINSG_AGVS_DEFINITIONS_H */

#endif /* MINSG_EXT_ADAPTIVEGLOBALVISIBILITYSAMPLING */
