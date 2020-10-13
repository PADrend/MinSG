/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef MINSG_OCCLUSIONCULLINGSTATISTICS_H
#define MINSG_OCCLUSIONCULLINGSTATISTICS_H

#include <cstdint>

namespace MinSG {
class Statistics;

//! Singleton holder object for occlusion culling related counters.
class OcclusionCullingStatistics {
	private:
		MINSGAPI explicit OcclusionCullingStatistics(Statistics & statistics);
		OcclusionCullingStatistics(OcclusionCullingStatistics &&) = delete;
		OcclusionCullingStatistics(const OcclusionCullingStatistics &) = delete;
		OcclusionCullingStatistics & operator=(OcclusionCullingStatistics &&) = delete;
		OcclusionCullingStatistics & operator=(const OcclusionCullingStatistics &) = delete;

		//! Key of occlusion test counter
		uint32_t occTestCounter;
		//! Key of visible occlusion test counter
		uint32_t occTestVisibleCounter;
		//! Key of invisible occlusion test counter
		uint32_t occTestInvisibleCounter;
		//! Key of culled geometry node counter
		uint32_t culledGeometryNodeCounter;
	public:
		//! Return singleton instance.
		MINSGAPI static OcclusionCullingStatistics & instance(Statistics & statistics);

		uint32_t getOccTestCounter() const {
			return occTestCounter;
		}
		uint32_t getOccTestVisibleCounter() const {
			return occTestVisibleCounter;
		}
		uint32_t getOccTestInvisibleCounter() const {
			return occTestInvisibleCounter;
		}
		uint32_t getCulledGeometryNodeCounter() const {
			return culledGeometryNodeCounter;
		}
};

}

#endif /* MINSG_OCCLUSIONCULLINGSTATISTICS_H */
