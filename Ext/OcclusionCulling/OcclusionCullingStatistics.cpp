/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "OcclusionCullingStatistics.h"
#include "../../Core/Statistics.h"

namespace MinSG {

OcclusionCullingStatistics::OcclusionCullingStatistics(Statistics & statistics) {
	occTestCounter = statistics.addCounter("occ. tests started", "1");
	occTestVisibleCounter = statistics.addCounter("occ. tests result visible", "1");
	occTestInvisibleCounter = statistics.addCounter("occ. tests result invisible", "1");
	culledGeometryNodeCounter = statistics.addCounter("geometry nodes culled", "1");
}

OcclusionCullingStatistics & OcclusionCullingStatistics::instance(Statistics & statistics) {
	static OcclusionCullingStatistics singleton(statistics);
	return singleton;
}

}
