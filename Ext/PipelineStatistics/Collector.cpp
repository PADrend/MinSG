/*
	This file is part of the MinSG library extension Pipeline Statistics.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PIPELINESTATISTICS

#include "Collector.h"
#include "Statistics.h"
#include "../../Core/FrameContext.h"
#include "../../Core/Statistics.h"
#include <Rendering/StatisticsQuery.h>

namespace MinSG {
namespace PipelineStatistics {

struct Collector::Implementation {
	Rendering::StatisticsQuery verticesSubmittedQuery;
	Rendering::StatisticsQuery primitivesSubmittedQuery;
	Rendering::StatisticsQuery vertexShaderInvocationsQuery;
	Rendering::StatisticsQuery tessControlShaderPatchesQuery;
	Rendering::StatisticsQuery tessEvaluationShaderInvocationsQuery;
	Rendering::StatisticsQuery geometryShaderInvocationsQuery;
	Rendering::StatisticsQuery geometryShaderPrimitivesEmittedQuery;
	Rendering::StatisticsQuery fragmentShaderInvocationsQuery;
	Rendering::StatisticsQuery computeShaderInvocationsQuery;
	Rendering::StatisticsQuery clippingInputPrimitivesQuery;
	Rendering::StatisticsQuery clippingOutputPrimitivesQuery;

	Implementation() : 
		verticesSubmittedQuery(Rendering::StatisticsQuery::createVerticesSubmittedQuery()),
		primitivesSubmittedQuery(Rendering::StatisticsQuery::createPrimitivesSubmittedQuery()),
		vertexShaderInvocationsQuery(Rendering::StatisticsQuery::createVertexShaderInvocationsQuery()),
		tessControlShaderPatchesQuery(Rendering::StatisticsQuery::createTessControlShaderPatchesQuery()),
		tessEvaluationShaderInvocationsQuery(Rendering::StatisticsQuery::createTessEvaluationShaderInvocationsQuery()),
		geometryShaderInvocationsQuery(Rendering::StatisticsQuery::createGeometryShaderInvocationsQuery()),
		geometryShaderPrimitivesEmittedQuery(Rendering::StatisticsQuery::createGeometryShaderPrimitivesEmittedQuery()),
		fragmentShaderInvocationsQuery(Rendering::StatisticsQuery::createFragmentShaderInvocationsQuery()),
		computeShaderInvocationsQuery(Rendering::StatisticsQuery::createComputeShaderInvocationsQuery()),
		clippingInputPrimitivesQuery(Rendering::StatisticsQuery::createClippingInputPrimitivesQuery()),
		clippingOutputPrimitivesQuery(Rendering::StatisticsQuery::createClippingOutputPrimitivesQuery()) {
	}
};

Collector::Collector() : State(), impl(new Implementation) {
}

Collector::Collector(const Collector & other) : State(other), impl(new Implementation) {
}

State::stateResult_t Collector::doEnableState(FrameContext & context, Node *, const RenderParam &) {
	auto& rc = context.getRenderingContext();
	impl->verticesSubmittedQuery.begin(rc);
	impl->primitivesSubmittedQuery.begin(rc);
	impl->vertexShaderInvocationsQuery.begin(rc);
	impl->tessControlShaderPatchesQuery.begin(rc);
	impl->tessEvaluationShaderInvocationsQuery.begin(rc);
	impl->geometryShaderInvocationsQuery.begin(rc);
	impl->geometryShaderPrimitivesEmittedQuery.begin(rc);
	impl->fragmentShaderInvocationsQuery.begin(rc);
	impl->computeShaderInvocationsQuery.begin(rc);
	impl->clippingInputPrimitivesQuery.begin(rc);
	impl->clippingOutputPrimitivesQuery.begin(rc);
	return State::STATE_OK;
}
void Collector::doDisableState(FrameContext & context, Node *, const RenderParam &) {
	auto& rc = context.getRenderingContext();
	impl->verticesSubmittedQuery.end(rc);
	impl->primitivesSubmittedQuery.end(rc);
	impl->vertexShaderInvocationsQuery.end(rc);
	impl->tessControlShaderPatchesQuery.end(rc);
	impl->tessEvaluationShaderInvocationsQuery.end(rc);
	impl->geometryShaderInvocationsQuery.end(rc);
	impl->geometryShaderPrimitivesEmittedQuery.end(rc);
	impl->fragmentShaderInvocationsQuery.end(rc);
	impl->computeShaderInvocationsQuery.end(rc);
	impl->clippingInputPrimitivesQuery.end(rc);
	impl->clippingOutputPrimitivesQuery.end(rc);

	MinSG::Statistics & statistics = context.getStatistics();
	statistics.addValue(Statistics::instance(statistics).getVerticesSubmittedCounter(), impl->verticesSubmittedQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getPrimitivesSubmittedCounter(), impl->primitivesSubmittedQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getVertexShaderInvocationsCounter(), impl->vertexShaderInvocationsQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getTessControlShaderPatchesCounter(), impl->tessControlShaderPatchesQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getTessEvaluationShaderInvocationsCounter(), impl->tessEvaluationShaderInvocationsQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getGeometryShaderInvocationsCounter(), impl->geometryShaderInvocationsQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getGeometryShaderPrimitivesEmittedCounter(), impl->geometryShaderPrimitivesEmittedQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getFragmentShaderInvocationsCounter(), impl->fragmentShaderInvocationsQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getComputeShaderInvocationsCounter(), impl->computeShaderInvocationsQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getClippingInputPrimitivesCounter(), impl->clippingInputPrimitivesQuery.getResult());
	statistics.addValue(Statistics::instance(statistics).getClippingOutputPrimitivesCounter(), impl->clippingOutputPrimitivesQuery.getResult());
}

Collector * Collector::clone() const {
	return new Collector(*this);
}

}
}

#endif /* MINSG_EXT_PIPELINESTATISTICS */
