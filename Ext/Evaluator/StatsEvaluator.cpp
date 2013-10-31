/*
	This file is part of the MinSG library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef MINSG_EXT_EVALUATORS

#include "StatsEvaluator.h"
#include "../../Core/Nodes/Node.h"
#include "../../Core/Statistics.h"
#include "../../Core/FrameContext.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

namespace MinSG {
namespace Evaluators {

//! [ctor]
StatsEvaluator::StatsEvaluator(DirectionMode _mode /* = SINGLE_VALUE */)
		: Evaluator(_mode), value(0.0), renderingFlags(FRUSTUM_CULLING), iterations(1), statIndex(0), callGlFinish(false) {
	setMaxValue_f(0.0f);
}

//! ---|> Evaluator
void StatsEvaluator::beginMeasure() {
	value = 0.0;
	values->clear();
}

//! ---|> Evaluator
void StatsEvaluator::measure(FrameContext & context, Node & node, const Geometry::Rect & /*r*/) {
	const unsigned int numIterations = getNumberOfIterations();

	std::vector<double> currentValues;
	currentValues.reserve(numIterations);

	Statistics & statistics = context.getStatistics();

	for(unsigned int i = 0; i < numIterations; ++i) {
		context.getRenderingContext().clearScreen(Util::Color4f(0.9f, 0.9f, 0.9f, 1.0f));

		context.getRenderingContext().finish();
		context.beginFrame();
		node.display(context,renderingFlags);
		context.endFrame(callGlFinish);

		currentValues.push_back(statistics.getValueAsDouble(statIndex));
	}
	double currentValue = 0.0;
	switch(numIterations) {
		case 0:
			break;
		case 1:
			currentValue = currentValues[0];
			break;
		case 2:
			currentValue = currentValues[1];
			break;
		default:
			const size_t medianIndex = currentValues.size() / 2;
			auto medianIt = std::next(currentValues.begin(), medianIndex);
			std::nth_element(currentValues.begin(),
							medianIt,
							currentValues.end());
			currentValue = currentValues[medianIndex];
			break;
	}

	if(mode == SINGLE_VALUE) {
		if(currentValue > value) {
			value = currentValue;
		}
	} else {
		values->push_back(new Util::_NumberAttribute<double>(currentValue));
	}
}

//! ---|> Evaluator
void StatsEvaluator::endMeasure(FrameContext & /*context*/) {
	if (mode == SINGLE_VALUE) {
		values->push_back(new Util::_NumberAttribute<double>(value));
	}
}

}
}

#endif /* MINSG_EXT_EVALUATORS */
