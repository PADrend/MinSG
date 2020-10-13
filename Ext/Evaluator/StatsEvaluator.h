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

#ifndef STATSEVALUATOR_H_
#define STATSEVALUATOR_H_

#include "Evaluator.h"
#include <Util/TypeNameMacro.h>
#include <cstdint>

namespace MinSG {
class FrameContext;
class Node;
namespace Evaluators {

/**
 * StatsEvaluator ---|> Evaluator
 */
class StatsEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(StatsEvaluator)
	public:
		MINSGAPI StatsEvaluator(DirectionMode mode = SINGLE_VALUE);
		virtual ~StatsEvaluator() {
		}

		unsigned int getNumberOfIterations() const {
			return iterations;
		}
		void setNumberOfIterations(unsigned int _iterations) {
			iterations = _iterations;
		}

		uint8_t getStatIndex() const {
			return statIndex;
		}
		void setStatIndex(uint8_t _statIndex) {
			statIndex = _statIndex;
		}

		bool getCallGlFinish() const {
			return callGlFinish;
		}
		void setCallGlFinish(bool _callGlFinish) {
			callGlFinish = _callGlFinish;
		}

		// ---|> Evaluator
		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & context, Node & node, const Geometry::Rect & r) override;
		MINSGAPI virtual void endMeasure(FrameContext & context) override;

	private:
		double value;
		unsigned int renderingFlags; //TODO
		unsigned int iterations;
		uint8_t statIndex;

		//! If @c true, @c finish is called on the rendering context at the end of each measurement.
		bool callGlFinish;
};

}
}

#endif /* STATSEVALUATOR_H_ */
#endif /* MINSG_EXT_EVALUATORS */
