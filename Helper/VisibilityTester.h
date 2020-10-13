/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_VISIBILITYTESTER_H_
#define MINSG_VISIBILITYTESTER_H_

#include <cstdint>
#include <deque>
#include <utility>

namespace MinSG {
class FrameContext;
class GeometryNode;

/**
 * @brief Execution of multiple visibility tests
 *
 * Helper for performing visibility tests.
 * OpenGL occlusion queries are used for the visibility tests.
 *
 * @see OpenGL constant GL_SAMPLES_PASSED and function glBeginQuery
 * @author Benjamin Eikel
 * @date 2012-01-16
 * @ingroup helper
 */
namespace VisibilityTester {

/**
 * Test the given nodes for visibility.
 * At first, the bounding box of a node is tested.
 * Afterwards, a node is tested only if its bounding box was determined visible before.
 *
 * @param context Frame context that is used for testing
 * @param nodes Array of nodes that will be tested
 * @return Array of visible nodes.
 * The second entry of each pair contains the number of passed samples for the node in the first entry.
 * @note The depth buffer has to be filled already.
 * The visibility tests are performned using the current content of the depth buffer.
 */
MINSGAPI std::deque<std::pair<GeometryNode *, uint32_t>> testNodes(FrameContext & context, const std::deque<GeometryNode *> & nodes);

}

}

#endif /* MINSG_VISIBILITYTESTER_H_ */
