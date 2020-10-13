/*
	This file is part of the MinSG library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef MINSG_HELPER_GRAPHVIZOUTPUT_H
#define MINSG_HELPER_GRAPHVIZOUTPUT_H

namespace Util {
class FileName;
}
namespace MinSG {
class Node;
namespace SceneManagement {
class SceneManager;
}
//! @ingroup helper
namespace GraphVizOutput {

/**
 * Save a given MinSG tree in GraphViz DOT format to the given file.
 *
 * @param rootNode Root of a MinSG subgraph which shall be stored.
 * @param sceneManager Optional scene manager to resolve registered node, or
 * @c nullptr to deactivate name output.
 * @param fileName Name of file to store the data in.
 */
MINSGAPI void treeToFile(Node * rootNode,
				const SceneManagement::SceneManager * sceneManager,
				const Util::FileName & fileName);

}
}

#endif /* MINSG_HELPER_GRAPHVIZOUTPUT_H */
