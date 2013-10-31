/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "ABTreeBuilder.h"
#include "ABTree.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <iostream>

namespace MinSG {
namespace TriangleTrees {
class TriangleTree;

TriangleTree * ABTreeBuilder::buildTriangleTree(Rendering::Mesh * mesh) {
#ifdef MINSG_PROFILING
	std::cout << "Profiling: Creating ABTree(triangles=" << trianglesPerNode
				<< ", enlargement=" << allowedBBEnlargement << ")" << std::endl;
	Util::Utils::outputProcessMemory();
	Util::Timer timer;
	timer.reset();
#endif

	auto k = new ABTree(mesh, trianglesPerNode, allowedBBEnlargement);

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << "Profiling: Construction of root node: " << timer.getMilliseconds() << " ms" << std::endl;
	Util::Utils::outputProcessMemory();
	timer.reset();
#endif

	if (k->shouldSplit()) {
		k->split();
	}

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << "Profiling: Construction of tree structure: " << timer.getMilliseconds() << " ms" << std::endl;
	Util::Utils::outputProcessMemory();
#endif

	return k;
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
