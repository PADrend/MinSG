/*
	This file is part of the MinSG library extension SVS.
	Copyright (C) 2012 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_SVS

#include "Statistics.h"
#include "../../Core/Statistics.h"

namespace MinSG {
namespace SVS {

Statistics::Statistics(MinSG::Statistics & statistics) {
	visitedSpheresCounter = statistics.addCounter("number of spheres visited", "1");
	enteredSpheresCounter = statistics.addCounter("number of spheres entered", "1");
}

Statistics & Statistics::instance(MinSG::Statistics & statistics) {
	static Statistics singleton(statistics);
	return singleton;
}

}
}

#endif /* MINSG_EXT_SVS */
