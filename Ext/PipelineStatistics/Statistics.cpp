/*
	This file is part of the MinSG library extension Pipeline Statistics.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_PIPELINESTATISTICS

#include "Statistics.h"
#include "../../Core/Statistics.h"

namespace MinSG {
namespace PipelineStatistics {

Statistics::Statistics(MinSG::Statistics & statistics) {
	verticesSubmittedCounter = statistics.addCounter("transferred vertices", "1");
	primitivesSubmittedCounter = statistics.addCounter("transferred primitives", "1");
	vertexShaderInvocationsCounter = statistics.addCounter("vertex shader invocations", "1");
	tessControlShaderPatchesCounter = statistics.addCounter("patches processed by tessellation control shader stage", "1");
	tessEvaluationShaderInvocationsCounter = statistics.addCounter("tessellation evaluation shader invocations", "1");
	geometryShaderInvocationsCounter = statistics.addCounter("geometry shader invocations", "1");
	geometryShaderPrimitivesEmittedCounter = statistics.addCounter("primitives emitted by geometry shader", "1");
	fragmentShaderInvocationsCounter = statistics.addCounter("fragment shader invocations", "1");
	computeShaderInvocationsCounter = statistics.addCounter("compute shader invocations", "1");
	clippingInputPrimitivesCounter = statistics.addCounter("clipping input primitives", "1");
	clippingOutputPrimitivesCounter = statistics.addCounter("clipping output primitives", "1");
}

Statistics & Statistics::instance(MinSG::Statistics & statistics) {
	static Statistics singleton(statistics);
	return singleton;
}

}
}

#endif /* MINSG_EXT_PIPELINESTATISTICS */
