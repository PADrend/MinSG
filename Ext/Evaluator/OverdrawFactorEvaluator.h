/*
	This file is part of the MinSG library extension Evaluator.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef MINSG_EXT_EVALUATORS

#ifndef MINSG_EVALUATOR_OVERDRAWFACTOREVALUATOR_H
#define MINSG_EVALUATOR_OVERDRAWFACTOREVALUATOR_H

#include "Evaluator.h"

namespace MinSG {
namespace Evaluators {

/**
 * Evaluator to test how many times a pixel is overwritten during rendering.
 * The test disregards the depth test. The measured value can be an indication
 * of the depth complexity.
 *
 * @author Benjamin Eikel
 * @date 2013-01-21
 */
class OverdrawFactorEvaluator : public Evaluator {
		PROVIDES_TYPE_NAME(OverdrawFactorEvaluator)
	public:
		MINSGAPI OverdrawFactorEvaluator(DirectionMode mode);
		MINSGAPI virtual ~OverdrawFactorEvaluator();

		MINSGAPI virtual void beginMeasure() override;
		MINSGAPI virtual void measure(FrameContext & frameContext, Node & node, const Geometry::Rect & rect) override;
		MINSGAPI virtual void endMeasure(FrameContext &) override;

		/**
		 * Return the quantile that is used to calculate a single value from
		 * all the values in the image. A value of 0.0 corresponds to the
		 * minimum, 0.5 to the median, and 1.0 to the maximum.
		 * 
		 * @return Value in [0, 1]
		 */
		double getResultQuantile() const {
			return resultQuantile;
		}
		void setResultQuantile(double quantile) {
			resultQuantile = quantile;
		}

		/**
		 * @return @c true if values equal to zero are removed before
		 * the calculation of the quantile, @c false otherwise.
		 */
		bool areZeroValuesIgnored() const {
			return resultRemoveZeroValues;
		}
		//! Remove the zero values before calculating the quantile.
		void ignoreZeroValues() {
			resultRemoveZeroValues = true;
		}
		//! Keep the zero values before calculating the quantile.
		void keepZeroValues() {
			resultRemoveZeroValues = false;
		}

	private:
		double resultQuantile;
		bool resultRemoveZeroValues;
};

}
}

#endif /* MINSG_EVALUATOR_OVERDRAWFACTOREVALUATOR_H */

#endif /* MINSG_EXT_EVALUATORS */
