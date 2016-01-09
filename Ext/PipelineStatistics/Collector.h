/*
	This file is part of the MinSG library extension Pipeline Statistics.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PIPELINESTATISTICS

#ifndef MINSG_PIPELINESTATISTICS_COLLECTOR_H_
#define MINSG_PIPELINESTATISTICS_COLLECTOR_H_

#include "../../Core/States/State.h"
#include <Util/TypeNameMacro.h>
#include <memory>

namespace MinSG {
class FrameContext;
class Node;
class RenderParam;
namespace PipelineStatistics {

/**
 * State for executing pipeline statistics queries and reporting their results to the Statistics framework.
 *
 * @author Benjamin Eikel
 * @date 2016-01-08
 */
class Collector : public State {
		PROVIDES_TYPE_NAME(PipelineStatistics::Collector)
	public:
		Collector();
		Collector(const Collector & other);

		Collector * clone() const override;
	protected:
		//! Start queries.
		stateResult_t doEnableState(FrameContext & context, Node * node, const RenderParam & rp) override;

		//! Stop queries. Pass results to statistics.
		void doDisableState(FrameContext & context, Node * node, const RenderParam & rp) override;
	private:
		// Use Pimpl idiom
		struct Implementation;
		std::unique_ptr<Implementation> impl;
};

}
}

#endif /* MINSG_PIPELINESTATISTICS_COLLECTOR_H_ */

#endif /* MINSG_EXT_PIPELINESTATISTICS */
