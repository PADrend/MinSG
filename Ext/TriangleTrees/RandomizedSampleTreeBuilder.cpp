/*
	This file is part of the MinSG library extension TriangleTrees.
	Copyright (C) 2009-2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_TRIANGLETREES

#include "RandomizedSampleTreeBuilder.h"
#include "RandomizedSampleTree.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <iostream>

namespace MinSG {
namespace TriangleTrees {
class TriangleTree;

TriangleTree * RandomizedSampleTreeBuilder::buildTriangleTree(Rendering::Mesh * mesh) {
#ifdef MINSG_PROFILING
	std::cout << "Profiling: Creating RandomizedSampleTree()" << std::endl;
	Util::Utils::outputProcessMemory();
	Util::Timer timer;
	timer.reset();
#endif

	auto rst = new RandomizedSampleTree(mesh);

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << "Profiling: Construction of root node: " << timer.getMilliseconds() << " ms" << std::endl;
	Util::Utils::outputProcessMemory();
	timer.reset();
#endif

	if (rst->shouldSplit()) {
		rst->split();
	}

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << "Profiling: Construction of tree structure: " << timer.getMilliseconds() << " ms" << std::endl;
	Util::Utils::outputProcessMemory();
	timer.reset();
#endif

	rst->createSample();

#ifdef MINSG_PROFILING
	timer.stop();
	std::cout << "Profiling: Random sampling: " << timer.getMilliseconds() << " ms" << std::endl;
	Util::Utils::outputProcessMemory();
#endif

	return rst;
}

}
}

#endif /* MINSG_EXT_TRIANGLETREES */
