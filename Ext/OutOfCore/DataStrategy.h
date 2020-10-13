/*
	This file is part of the MinSG library extension OutOfCore.
	Copyright (C) 2011-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2011-2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_OUTOFCORE

#ifndef OUTOFCORE_DATASTRATEGY_H_
#define OUTOFCORE_DATASTRATEGY_H_

#include <Rendering/Mesh/MeshDataStrategy.h>

namespace Rendering {
class Mesh;
class RenderingContext;
}

namespace MinSG {
namespace OutOfCore {
class CacheManager;

/**
 * Data strategy for meshes that are handled inside a cache system.
 * It collects usage data for the meshes and handles the cache movement between CPU and GPU memory.
 *
 * @author Benjamin Eikel
 * @date 2011-02-18
 */
class DataStrategy : public Rendering::MeshDataStrategy {
	public:
		//! Mode for drawing a cache object that is currently not in memory.
		enum missing_mode_t {
			//! Do not wait until loading has been finished, and do nothing else.
			NO_WAIT_DO_NOTHING,
			//! Do not wait until loading has been finished, and display a colored bounding box.
			NO_WAIT_DISPLAY_COLORED_BOX,
			//! Do not wait until loading has been finished, and write a bounding box to the depth buffer.
			NO_WAIT_DISPLAY_DEPTH_BOX,
			//! Wait until loading has been finished, and display the cache object afterwards.
			WAIT_DISPLAY
		};

	private:
		CacheManager & cacheManager;

		//! Mode for drawing cache objects that are currently not in memory.
		missing_mode_t missingMode;

	public:
		DataStrategy(CacheManager & outOfCoreCacheManager) :
			Rendering::MeshDataStrategy(), cacheManager(outOfCoreCacheManager), missingMode(NO_WAIT_DISPLAY_COLORED_BOX) {
		}
		virtual ~DataStrategy() {
		}

		MINSGAPI void assureLocalVertexData(Rendering::Mesh * m) override;

		MINSGAPI void assureLocalIndexData(Rendering::Mesh * m) override;

		void prepare(Rendering::Mesh * /*mesh*/) override {
		}

		MINSGAPI void displayMesh(Rendering::RenderingContext & context, Rendering::Mesh * m, uint32_t startIndex, uint32_t indexCount) override;

		missing_mode_t getMissingMode() const {
			return missingMode;
		}

		void setMissingMode(missing_mode_t newMode) {
			missingMode = newMode;
		}
};

}
}

#endif /* OUTOFCORE_DATASTRATEGY_H_ */

#endif /* MINSG_EXT_OUTOFCORE */
