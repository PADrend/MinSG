/*
	This file is part of the MinSG library extension Pipeline Statistics.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PIPELINESTATISTICS

#ifndef MINSG_PIPELINESTATISTICS_STATISTICS_H_
#define MINSG_PIPELINESTATISTICS_STATISTICS_H_

#include <cstdint>

namespace MinSG {
class Statistics;
//! @ingroup ext
namespace PipelineStatistics {

/**
 * Singleton holder object for Pipeline Statistics related counters.
 *
 * @author Benjamin Eikel
 * @date 2016-01-08
 */
class Statistics {
	private:
		MINSGAPI explicit Statistics(MinSG::Statistics & statistics);
		Statistics(Statistics &&) = delete;
		Statistics(const Statistics &) = delete;
		Statistics & operator=(Statistics &&) = delete;
		Statistics & operator=(const Statistics &) = delete;

		uint32_t verticesSubmittedCounter;
		uint32_t primitivesSubmittedCounter;
		uint32_t vertexShaderInvocationsCounter;
		uint32_t tessControlShaderPatchesCounter;
		uint32_t tessEvaluationShaderInvocationsCounter;
		uint32_t geometryShaderInvocationsCounter;
		uint32_t geometryShaderPrimitivesEmittedCounter;
		uint32_t fragmentShaderInvocationsCounter;
		uint32_t computeShaderInvocationsCounter;
		uint32_t clippingInputPrimitivesCounter;
		uint32_t clippingOutputPrimitivesCounter;
	public:
		//! Return singleton instance.
		MINSGAPI static Statistics & instance(MinSG::Statistics & statistics);

		uint32_t getVerticesSubmittedCounter() const {
			return verticesSubmittedCounter;
		}
		uint32_t getPrimitivesSubmittedCounter() const {
			return primitivesSubmittedCounter;
		}
		uint32_t getVertexShaderInvocationsCounter() const {
			return vertexShaderInvocationsCounter;
		}
		uint32_t getTessControlShaderPatchesCounter() const {
			return tessControlShaderPatchesCounter;
		}
		uint32_t getTessEvaluationShaderInvocationsCounter() const {
			return tessEvaluationShaderInvocationsCounter;
		}
		uint32_t getGeometryShaderInvocationsCounter() const {
			return geometryShaderInvocationsCounter;
		}
		uint32_t getGeometryShaderPrimitivesEmittedCounter() const {
			return geometryShaderPrimitivesEmittedCounter;
		}
		uint32_t getFragmentShaderInvocationsCounter() const {
			return fragmentShaderInvocationsCounter;
		}
		uint32_t getComputeShaderInvocationsCounter() const {
			return computeShaderInvocationsCounter;
		}
		uint32_t getClippingInputPrimitivesCounter() const {
			return clippingInputPrimitivesCounter;
		}
		uint32_t getClippingOutputPrimitivesCounter() const {
			return clippingOutputPrimitivesCounter;
		}
};

}
}

#endif /* MINSG_PIPELINESTATISTICS_STATISTICS_H_ */

#endif /* MINSG_EXT_PIPELINESTATISTICS */
